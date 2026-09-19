// opentype_gpos_ir_executor.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "shaped_glyph_buffer.h"
#include "opentype_gpos_attachment_state.h"
#include "opentype_gpos_apply_state.h"
#include "opentype_shaping_ir.h"

namespace waavs
{
    enum class OpenTypeGposIRResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    enum class OpenTypeGposIRGlyphSearchResult : uint8_t
    {
        Invalid = 0,
        End,
        Found
    };


    // ========================================================================
    // Shared semantic IR helpers
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGposIRGlyphSetValid(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id) noexcept
    {
        if (id >= ir.glyphSets.size())
            return false;

        const OpenTypeShapingIRGlyphSet& set = ir.glyphSets[id];

        return set.rangeOffset <= ir.glyphRanges.size() &&
            set.rangeCount <= ir.glyphRanges.size() - set.rangeOffset;
    }


    [[nodiscard]] static inline bool openTypeGposIRGlyphSetContains(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id,
        uint16_t glyphId, bool& result) noexcept
    {
        result = false;

        if (!openTypeGposIRGlyphSetValid(ir, id))
            return false;

        const OpenTypeShapingIRGlyphSet& set = ir.glyphSets[id];

        size_t first = set.rangeOffset;
        size_t last = first + set.rangeCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGlyphRange& range = ir.glyphRanges[middle];

            if (glyphId < range.first)
                last = middle;
            else if (glyphId > range.last)
                first = middle + 1;
            else
            {
                result = true;
                return true;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRGlyphClassMapValid(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphClassMapId id) noexcept
    {
        if (id >= ir.glyphClassMaps.size())
            return false;

        const OpenTypeShapingIRGlyphClassMap& map = ir.glyphClassMaps[id];

        return map.rangeOffset <= ir.glyphClassRanges.size() &&
            map.rangeCount <= ir.glyphClassRanges.size() - map.rangeOffset;
    }


    [[nodiscard]] static inline bool openTypeGposIRGlyphClassValue(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphClassMapId id,
        uint16_t glyphId, uint16_t& result) noexcept
    {
        result = 0;

        if (!openTypeGposIRGlyphClassMapValid(ir, id))
            return false;

        const OpenTypeShapingIRGlyphClassMap& map = ir.glyphClassMaps[id];

        size_t first = map.rangeOffset;
        size_t last = first + map.rangeCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGlyphClassRange& range = ir.glyphClassRanges[middle];

            if (glyphId < range.first)
                last = middle;
            else if (glyphId > range.last)
                first = middle + 1;
            else
            {
                result = range.value;
                return true;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRLookupFilterValid(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter) noexcept
    {
        static constexpr uint8_t knownFlags =
            OpenTypeShapingIRRightToLeft |
            OpenTypeShapingIRIgnoreBaseGlyphs |
            OpenTypeShapingIRIgnoreLigatures |
            OpenTypeShapingIRIgnoreMarks;

        if ((filter.flags & ~knownFlags) != 0)
            return false;

        const bool ignoreBase =
            (filter.flags & OpenTypeShapingIRIgnoreBaseGlyphs) != 0;

        const bool ignoreLigatures =
            (filter.flags & OpenTypeShapingIRIgnoreLigatures) != 0;

        const bool ignoreMarks =
            (filter.flags & OpenTypeShapingIRIgnoreMarks) != 0;

        const bool useMarkFilteringSet =
            filter.markFilteringSet != kOpenTypeShapingIRInvalid;

        const bool needsGlyphClass =
            ignoreBase ||
            ignoreLigatures ||
            ignoreMarks ||
            useMarkFilteringSet ||
            filter.markAttachmentType != 0;

        if (needsGlyphClass &&
            ir.gdefGlyphClasses.size() != kOpenTypeShapingIRGlyphDomainSize)
        {
            return false;
        }

        if (useMarkFilteringSet &&
            !openTypeGposIRGlyphSetValid(ir, filter.markFilteringSet))
        {
            return false;
        }

        if (!ignoreMarks &&
            !useMarkFilteringSet &&
            filter.markAttachmentType != 0 &&
            ir.gdefMarkAttachClasses.size() != kOpenTypeShapingIRGlyphDomainSize)
        {
            return false;
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRLookupShouldSkip(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        uint32_t glyphId, bool& result) noexcept
    {
        result = false;

        if (!openTypeGposIRLookupFilterValid(ir, filter) || glyphId > 0xFFFFu)
            return false;

        const bool ignoreBase =
            (filter.flags & OpenTypeShapingIRIgnoreBaseGlyphs) != 0;

        const bool ignoreLigatures =
            (filter.flags & OpenTypeShapingIRIgnoreLigatures) != 0;

        const bool ignoreMarks =
            (filter.flags & OpenTypeShapingIRIgnoreMarks) != 0;

        const bool useMarkFilteringSet =
            filter.markFilteringSet != kOpenTypeShapingIRInvalid;

        const bool needsGlyphClass =
            ignoreBase ||
            ignoreLigatures ||
            ignoreMarks ||
            useMarkFilteringSet ||
            filter.markAttachmentType != 0;

        if (!needsGlyphClass)
            return true;

        const uint16_t glyphClass = ir.gdefGlyphClasses[glyphId];

        if (glyphClass == 1)
        {
            result = ignoreBase;
            return true;
        }

        if (glyphClass == 2)
        {
            result = ignoreLigatures;
            return true;
        }

        if (glyphClass != 3)
            return true;

        if (ignoreMarks)
        {
            result = true;
            return true;
        }

        if (useMarkFilteringSet)
        {
            bool member = false;

            if (!openTypeGposIRGlyphSetContains(
                ir, filter.markFilteringSet,
                static_cast<uint16_t>(glyphId), member))
            {
                return false;
            }

            result = !member;
            return true;
        }

        if (filter.markAttachmentType != 0)
        {
            result =
                ir.gdefMarkAttachClasses[glyphId] != filter.markAttachmentType;

            return true;
        }

        return true;
    }


    static inline OpenTypeGposIRGlyphSearchResult openTypeGposIRLookupNext(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t currentIndex, size_t& result) noexcept
    {
        result = buffer.size();

        if (!openTypeGposIRLookupFilterValid(ir, filter) ||
            currentIndex >= buffer.size())
        {
            return OpenTypeGposIRGlyphSearchResult::Invalid;
        }

        for (size_t index = currentIndex + 1; index < buffer.size(); ++index)
        {
            bool skip = false;

            if (!openTypeGposIRLookupShouldSkip(
                ir, filter, buffer[index].shaping.glyphId, skip))
            {
                return OpenTypeGposIRGlyphSearchResult::Invalid;
            }

            if (!skip)
            {
                result = index;
                return OpenTypeGposIRGlyphSearchResult::Found;
            }
        }

        return OpenTypeGposIRGlyphSearchResult::End;
    }


    static inline OpenTypeGposIRGlyphSearchResult openTypeGposIRLookupPrevious(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t currentIndex, size_t& result) noexcept
    {
        result = 0;

        if (!openTypeGposIRLookupFilterValid(ir, filter) ||
            currentIndex >= buffer.size())
        {
            return OpenTypeGposIRGlyphSearchResult::Invalid;
        }

        for (size_t index = currentIndex; index != 0;)
        {
            --index;

            bool skip = false;

            if (!openTypeGposIRLookupShouldSkip(
                ir, filter, buffer[index].shaping.glyphId, skip))
            {
                return OpenTypeGposIRGlyphSearchResult::Invalid;
            }

            if (!skip)
            {
                result = index;
                return OpenTypeGposIRGlyphSearchResult::Found;
            }
        }

        return OpenTypeGposIRGlyphSearchResult::End;
    }


    static inline void applyOpenTypeGposIRPositionAdjustment(
        const OpenTypeShapingIRPositionAdjustment& adjustment,
        GlyphPlacement& placement) noexcept
    {
        placement.offsetX += adjustment.offsetX;
        placement.offsetY += adjustment.offsetY;
        placement.advanceX += adjustment.advanceX;
        placement.advanceY += adjustment.advanceY;
    }


    // ========================================================================
    // GPOS Type 1 - Single
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGposIRSingleSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposSingleSubtable& subtable) noexcept
    {
        return subtable.pairOffset <= ir.gposSinglePairs.size() &&
            subtable.pairCount <= ir.gposSinglePairs.size() - subtable.pairOffset;
    }


    [[nodiscard]] static inline bool openTypeGposIRSingleLookupValid(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposSingle ||
            lookup.payloadCount == 0 ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter) ||
            lookup.payloadOffset > ir.gposSingleSubtables.size() ||
            lookup.payloadCount >
            ir.gposSingleSubtables.size() - lookup.payloadOffset)
        {
            return false;
        }

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            if (!openTypeGposIRSingleSubtableValid(
                ir, ir.gposSingleSubtables[lookup.payloadOffset + i]))
            {
                return false;
            }
        }

        return true;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRSingleSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposSingleSubtable& subtable,
        uint16_t glyphId, OpenTypeShapingIRPositionAdjustment& adjustment) noexcept
    {
        adjustment = {};

        if (!openTypeGposIRSingleSubtableValid(ir, subtable))
            return OpenTypeGposIRResult::Invalid;

        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGposSinglePair& pair = ir.gposSinglePairs[middle];

            if (glyphId < pair.glyph)
                last = middle;
            else if (glyphId > pair.glyph)
                first = middle + 1;
            else
            {
                adjustment = pair.adjustment;
                return OpenTypeGposIRResult::Match;
            }
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRSingleLookup(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, OpenTypeShapingIRPositionAdjustment& adjustment) noexcept
    {
        adjustment = {};

        if (!openTypeGposIRSingleLookupValid(ir, lookup))
            return OpenTypeGposIRResult::Invalid;

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGposSingleSubtable& subtable =
                ir.gposSingleSubtables[lookup.payloadOffset + i];

            const OpenTypeGposIRResult result =
                resolveOpenTypeGposIRSingleSubtable(
                    ir, subtable, glyphId, adjustment);

            if (result != OpenTypeGposIRResult::NoMatch)
                return result;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRSingleLookupAt(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer, size_t glyphIndex) noexcept
    {
        if (!openTypeGposIRSingleLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size() ||
            buffer[glyphIndex].shaping.glyphId > 0xFFFFu)
        {
            return OpenTypeGposIRResult::Invalid;
        }

        OpenTypeShapingIRPositionAdjustment adjustment{};

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRSingleLookup(
                ir, lookup,
                static_cast<uint16_t>(buffer[glyphIndex].shaping.glyphId),
                adjustment);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        applyOpenTypeGposIRPositionAdjustment(
            adjustment, buffer[glyphIndex].placement);

        return OpenTypeGposIRResult::Match;
    }


    static inline bool applyOpenTypeGposIRSingleLookup(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer)
    {
        if (!openTypeGposIRSingleLookupValid(ir, lookup))
            return false;

        ShapedGlyphBuffer working = buffer;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRSingleLookupAt(
                    ir, lookup, working, glyphIndex);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 2 - Pair
    // ========================================================================

    struct OpenTypeGposIRPairMatch
    {
        size_t firstIndex{ 0 };
        size_t secondIndex{ 0 };

        OpenTypeShapingIRPositionAdjustment firstAdjustment{};
        OpenTypeShapingIRPositionAdjustment secondAdjustment{};

        bool secondParticipates{ false };

        void clear() noexcept
        {
            firstIndex = 0;
            secondIndex = 0;
            firstAdjustment = {};
            secondAdjustment = {};
            secondParticipates = false;
        }
    };


    [[nodiscard]] static inline bool openTypeGposIRPairExplicitSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposPairExplicitSubtable& subtable) noexcept
    {
        return subtable.pairOffset <= ir.gposPairExplicitPairs.size() &&
            subtable.pairCount <=
            ir.gposPairExplicitPairs.size() - subtable.pairOffset;
    }


    [[nodiscard]] static inline bool openTypeGposIRPairClassSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposPairClassSubtable& subtable) noexcept
    {
        if (!openTypeGposIRGlyphSetValid(ir, subtable.firstCoverage) ||
            !openTypeGposIRGlyphClassMapValid(ir, subtable.firstClassMap) ||
            !openTypeGposIRGlyphClassMapValid(ir, subtable.secondClassMap) ||
            subtable.firstClassCount == 0 ||
            subtable.secondClassCount == 0)
        {
            return false;
        }

        const uint64_t valueCount =
            uint64_t(subtable.firstClassCount) *
            uint64_t(subtable.secondClassCount);

        return subtable.valueOffset <= ir.gposPairClassValues.size() &&
            valueCount <=
            uint64_t(ir.gposPairClassValues.size() - subtable.valueOffset);
    }


    [[nodiscard]] static inline bool openTypeGposIRPairSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposPairSubtable& subtable) noexcept
    {
        if (subtable.secondParticipates > 1)
            return false;

        switch (subtable.kind)
        {
        case OpenTypeShapingIRGposPairKind::Explicit:
            return subtable.payloadIndex < ir.gposPairExplicitSubtables.size() &&
                openTypeGposIRPairExplicitSubtableValid(
                    ir, ir.gposPairExplicitSubtables[subtable.payloadIndex]);

        case OpenTypeShapingIRGposPairKind::Class:
            return subtable.payloadIndex < ir.gposPairClassSubtables.size() &&
                openTypeGposIRPairClassSubtableValid(
                    ir, ir.gposPairClassSubtables[subtable.payloadIndex]);

        default:
            return false;
        }
    }


    [[nodiscard]] static inline bool openTypeGposIRPairLookupValid(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposPair ||
            lookup.payloadCount == 0 ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter) ||
            lookup.payloadOffset > ir.gposPairSubtables.size() ||
            lookup.payloadCount >
            ir.gposPairSubtables.size() - lookup.payloadOffset)
        {
            return false;
        }

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            if (!openTypeGposIRPairSubtableValid(
                ir, ir.gposPairSubtables[lookup.payloadOffset + i]))
            {
                return false;
            }
        }

        return true;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRPairExplicitSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposPairExplicitSubtable& subtable,
        uint16_t firstGlyph, uint16_t secondGlyph,
        OpenTypeShapingIRPositionAdjustment& firstAdjustment,
        OpenTypeShapingIRPositionAdjustment& secondAdjustment) noexcept
    {
        firstAdjustment = {};
        secondAdjustment = {};

        if (!openTypeGposIRPairExplicitSubtableValid(ir, subtable))
            return OpenTypeGposIRResult::Invalid;

        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGposPairExplicit& pair =
                ir.gposPairExplicitPairs[middle];

            if (firstGlyph < pair.first ||
                (firstGlyph == pair.first && secondGlyph < pair.second))
            {
                last = middle;
            }
            else if (firstGlyph > pair.first ||
                (firstGlyph == pair.first && secondGlyph > pair.second))
            {
                first = middle + 1;
            }
            else
            {
                firstAdjustment = pair.firstAdjustment;
                secondAdjustment = pair.secondAdjustment;
                return OpenTypeGposIRResult::Match;
            }
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRPairClassSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposPairClassSubtable& subtable,
        uint16_t firstGlyph, uint16_t secondGlyph,
        OpenTypeShapingIRPositionAdjustment& firstAdjustment,
        OpenTypeShapingIRPositionAdjustment& secondAdjustment) noexcept
    {
        firstAdjustment = {};
        secondAdjustment = {};

        if (!openTypeGposIRPairClassSubtableValid(ir, subtable))
            return OpenTypeGposIRResult::Invalid;

        bool covered = false;

        if (!openTypeGposIRGlyphSetContains(
            ir, subtable.firstCoverage, firstGlyph, covered))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        if (!covered)
            return OpenTypeGposIRResult::NoMatch;

        uint16_t firstClass = 0;
        uint16_t secondClass = 0;

        if (!openTypeGposIRGlyphClassValue(
            ir, subtable.firstClassMap, firstGlyph, firstClass) ||
            !openTypeGposIRGlyphClassValue(
                ir, subtable.secondClassMap, secondGlyph, secondClass))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        if (firstClass >= subtable.firstClassCount ||
            secondClass >= subtable.secondClassCount)
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint64_t localIndex =
            uint64_t(firstClass) * uint64_t(subtable.secondClassCount) +
            uint64_t(secondClass);

        const uint64_t valueIndex =
            uint64_t(subtable.valueOffset) + localIndex;

        if (valueIndex >= ir.gposPairClassValues.size())
            return OpenTypeGposIRResult::Invalid;

        const OpenTypeShapingIRGposPairClassValue& value =
            ir.gposPairClassValues[static_cast<size_t>(valueIndex)];

        firstAdjustment = value.firstAdjustment;
        secondAdjustment = value.secondAdjustment;

        return OpenTypeGposIRResult::Match;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRPairSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposPairSubtable& subtable,
        uint16_t firstGlyph, uint16_t secondGlyph,
        OpenTypeShapingIRPositionAdjustment& firstAdjustment,
        OpenTypeShapingIRPositionAdjustment& secondAdjustment) noexcept
    {
        firstAdjustment = {};
        secondAdjustment = {};

        if (!openTypeGposIRPairSubtableValid(ir, subtable))
            return OpenTypeGposIRResult::Invalid;

        switch (subtable.kind)
        {
        case OpenTypeShapingIRGposPairKind::Explicit:
            return resolveOpenTypeGposIRPairExplicitSubtable(
                ir, ir.gposPairExplicitSubtables[subtable.payloadIndex],
                firstGlyph, secondGlyph, firstAdjustment, secondAdjustment);

        case OpenTypeShapingIRGposPairKind::Class:
            return resolveOpenTypeGposIRPairClassSubtable(
                ir, ir.gposPairClassSubtables[subtable.payloadIndex],
                firstGlyph, secondGlyph, firstAdjustment, secondAdjustment);

        default:
            return OpenTypeGposIRResult::Invalid;
        }
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRPairLookup(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposIRPairMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRPairLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint32_t firstGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        size_t secondIndex = 0;

        const OpenTypeGposIRGlyphSearchResult search =
            openTypeGposIRLookupNext(
                ir, lookup.filter, buffer, glyphIndex, secondIndex);

        if (search == OpenTypeGposIRGlyphSearchResult::Invalid)
            return OpenTypeGposIRResult::Invalid;

        if (search == OpenTypeGposIRGlyphSearchResult::End)
            return OpenTypeGposIRResult::NoMatch;

        if (secondIndex <= glyphIndex || secondIndex >= buffer.size())
            return OpenTypeGposIRResult::Invalid;

        const uint32_t secondGlyphId =
            buffer[secondIndex].shaping.glyphId;

        if (secondGlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGposPairSubtable& subtable =
                ir.gposPairSubtables[lookup.payloadOffset + i];

            OpenTypeShapingIRPositionAdjustment firstAdjustment{};
            OpenTypeShapingIRPositionAdjustment secondAdjustment{};

            const OpenTypeGposIRResult result =
                resolveOpenTypeGposIRPairSubtable(
                    ir, subtable,
                    static_cast<uint16_t>(firstGlyphId),
                    static_cast<uint16_t>(secondGlyphId),
                    firstAdjustment, secondAdjustment);

            if (result == OpenTypeGposIRResult::Invalid)
                return result;

            if (result == OpenTypeGposIRResult::NoMatch)
                continue;

            match.firstIndex = glyphIndex;
            match.secondIndex = secondIndex;
            match.firstAdjustment = firstAdjustment;
            match.secondAdjustment = secondAdjustment;
            match.secondParticipates = subtable.secondParticipates != 0;

            return OpenTypeGposIRResult::Match;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRPairLookupAt(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        size_t* resumeIndex = nullptr) noexcept
    {
        if (resumeIndex)
            *resumeIndex = glyphIndex;

        OpenTypeGposIRPairMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRPairLookup(
                ir, lookup, buffer, glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        applyOpenTypeGposIRPositionAdjustment(
            match.firstAdjustment,
            buffer[match.firstIndex].placement);

        applyOpenTypeGposIRPositionAdjustment(
            match.secondAdjustment,
            buffer[match.secondIndex].placement);

        if (resumeIndex)
        {
            *resumeIndex =
                match.secondParticipates
                ? match.secondIndex + 1
                : match.secondIndex;
        }

        return OpenTypeGposIRResult::Match;
    }


    static inline bool applyOpenTypeGposIRPairLookup(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer)
    {
        if (!openTypeGposIRPairLookupValid(ir, lookup))
            return false;

        ShapedGlyphBuffer working = buffer;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            size_t resumeIndex = glyphIndex;

            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRPairLookupAt(
                    ir, lookup, working, glyphIndex, &resumeIndex);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;

            if (result == OpenTypeGposIRResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            if (resumeIndex <= glyphIndex ||
                resumeIndex > working.size())
            {
                return false;
            }

            glyphIndex = resumeIndex;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 3 - Cursive
    // ========================================================================

    struct OpenTypeGposIRCursiveMatch
    {
        size_t firstIndex{ 0 };
        size_t secondIndex{ 0 };

        OpenTypeShapingIRAnchor exit{};
        OpenTypeShapingIRAnchor entry{};

        void clear() noexcept
        {
            firstIndex = 0;
            secondIndex = 0;
            exit = {};
            entry = {};
        }
    };


    [[nodiscard]] static inline bool openTypeGposIRCursiveSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposCursiveSubtable& subtable) noexcept
    {
        if (subtable.recordOffset > ir.gposCursiveRecords.size() ||
            subtable.recordCount >
            ir.gposCursiveRecords.size() - subtable.recordOffset)
        {
            return false;
        }

        bool havePrevious = false;
        uint16_t previous = 0;

        for (uint32_t i = 0; i < subtable.recordCount; ++i)
        {
            const OpenTypeShapingIRGposCursiveRecord& record =
                ir.gposCursiveRecords[subtable.recordOffset + i];

            if (record.hasEntry > 1 || record.hasExit > 1)
                return false;

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRCursiveLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposCursive ||
            lookup.payloadCount == 0 ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter) ||
            lookup.payloadOffset > ir.gposCursiveSubtables.size() ||
            lookup.payloadCount >
            ir.gposCursiveSubtables.size() - lookup.payloadOffset)
        {
            return false;
        }

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            if (!openTypeGposIRCursiveSubtableValid(
                ir, ir.gposCursiveSubtables[lookup.payloadOffset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline const OpenTypeShapingIRGposCursiveRecord*
        openTypeGposIRCursiveRecord(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRGposCursiveSubtable& subtable,
            uint16_t glyphId) noexcept
    {
        if (!openTypeGposIRCursiveSubtableValid(ir, subtable))
            return nullptr;

        size_t first = subtable.recordOffset;
        size_t last = first + subtable.recordCount;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGposCursiveRecord& record =
                ir.gposCursiveRecords[middle];

            if (glyphId < record.glyph)
                last = middle;
            else if (glyphId > record.glyph)
                first = middle + 1;
            else
                return &record;
        }

        return nullptr;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRCursiveLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposIRCursiveMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRCursiveLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint32_t firstGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        size_t secondIndex = 0;

        const OpenTypeGposIRGlyphSearchResult search =
            openTypeGposIRLookupNext(
                ir, lookup.filter, buffer,
                glyphIndex, secondIndex);

        if (search == OpenTypeGposIRGlyphSearchResult::Invalid)
            return OpenTypeGposIRResult::Invalid;

        if (search == OpenTypeGposIRGlyphSearchResult::End)
            return OpenTypeGposIRResult::NoMatch;

        if (secondIndex <= glyphIndex ||
            secondIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint32_t secondGlyphId =
            buffer[secondIndex].shaping.glyphId;

        if (secondGlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGposCursiveSubtable& subtable =
                ir.gposCursiveSubtables[lookup.payloadOffset + i];

            const OpenTypeShapingIRGposCursiveRecord* first =
                openTypeGposIRCursiveRecord(
                    ir, subtable,
                    static_cast<uint16_t>(firstGlyphId));

            if (!first || !first->hasExit)
                continue;

            const OpenTypeShapingIRGposCursiveRecord* second =
                openTypeGposIRCursiveRecord(
                    ir, subtable,
                    static_cast<uint16_t>(secondGlyphId));

            if (!second || !second->hasEntry)
                continue;

            match.firstIndex = glyphIndex;
            match.secondIndex = secondIndex;
            match.exit = first->exit;
            match.entry = second->entry;

            return OpenTypeGposIRResult::Match;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline bool applyOpenTypeGposIRCursiveMatch(
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposIRCursiveMatch& match,
        bool runRightToLeft)
    {
        if (!attachments.matches(buffer.size()) ||
            match.firstIndex >= buffer.size() ||
            match.secondIndex >= buffer.size() ||
            match.secondIndex <= match.firstIndex)
        {
            return false;
        }

        ShapedGlyph& first =
            buffer[match.firstIndex];

        ShapedGlyph& second =
            buffer[match.secondIndex];

        // Main-line attachment.
        //
        // Buffer order remains logical. advanceX is stored as a positive
        // logical magnitude, so run direction determines the sign of the
        // origin delta. LookupFlag RightToLeft is unrelated to this equation.

        int64_t advance = 0;

        if (!runRightToLeft)
        {
            advance =
                int64_t(first.placement.offsetX) +
                int64_t(match.exit.x) -
                int64_t(second.placement.offsetX) -
                int64_t(match.entry.x);
        }
        else
        {
            advance =
                int64_t(second.placement.offsetX) +
                int64_t(match.entry.x) -
                int64_t(first.placement.offsetX) -
                int64_t(match.exit.x);
        }

        if (advance < std::numeric_limits<int32_t>::min() ||
            advance > std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        first.placement.advanceX =
            static_cast<int32_t>(advance);

        // Cross-stream attachment.
        //
        // LookupFlag RightToLeft controls which glyph becomes child/parent.
        // It does not reverse traversal.

        const bool lookupRightToLeft =
            (lookup.filter.flags &
                OpenTypeShapingIRRightToLeft) != 0;

        size_t child = 0;
        size_t parent = 0;
        int64_t minorOffset = 0;

        if (!lookupRightToLeft)
        {
            child = match.secondIndex;
            parent = match.firstIndex;

            minorOffset =
                int64_t(match.exit.y) -
                int64_t(match.entry.y);
        }
        else
        {
            child = match.firstIndex;
            parent = match.secondIndex;

            minorOffset =
                int64_t(match.entry.y) -
                int64_t(match.exit.y);
        }

        if (minorOffset < std::numeric_limits<int32_t>::min() ||
            minorOffset > std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposCursive(
            attachments, buffer,
            child, parent,
            static_cast<int32_t>(minorOffset));
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRCursiveLookupAt(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments) noexcept
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposIRCursiveMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRCursiveLookup(
                ir, lookup, buffer, glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposIRCursiveMatch(
            lookup, working, workingAttachments,
            match, runRightToLeft))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposIRResult::Match;
    }


    static inline bool applyOpenTypeGposIRCursiveLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposIRCursiveLookupValid(ir, lookup) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposIRCursiveMatch match;

            const OpenTypeGposIRResult result =
                resolveOpenTypeGposIRCursiveLookup(
                    ir, lookup, working,
                    glyphIndex, match);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;

            if (result == OpenTypeGposIRResult::NoMatch)
                continue;

            if (!applyOpenTypeGposIRCursiveMatch(
                lookup, working, workingAttachments,
                match, runRightToLeft))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    static inline bool applyOpenTypeGposIRCursiveLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposIRCursiveLookup(
            ir, lookup, working,
            runRightToLeft, attachments))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 4 - MarkBase
    // ========================================================================

    struct OpenTypeGposIRMarkBaseMatch
    {
        size_t baseIndex{ 0 };
        size_t markIndex{ 0 };

        OpenTypeShapingIRAnchor base{};
        OpenTypeShapingIRAnchor mark{};

        void clear() noexcept
        {
            baseIndex = 0;
            markIndex = 0;
            base = {};
            mark = {};
        }
    };


    [[nodiscard]] static inline bool openTypeGposIRMarkBaseSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposMarkBaseSubtable& subtable) noexcept
    {
        if (subtable.markClassCount == 0 ||
            subtable.markOffset > ir.gposMarkRecords.size() ||
            subtable.markCount >
            ir.gposMarkRecords.size() - subtable.markOffset ||
            subtable.baseOffset > ir.gposMarkBaseRecords.size() ||
            subtable.baseCount >
            ir.gposMarkBaseRecords.size() - subtable.baseOffset ||
            subtable.baseAnchorOffset > ir.gposMarkBaseAnchorRefs.size())
        {
            return false;
        }

        const uint64_t anchorCount =
            uint64_t(subtable.baseCount) *
            uint64_t(subtable.markClassCount);

        if (anchorCount >
            ir.gposMarkBaseAnchorRefs.size() -
            subtable.baseAnchorOffset)
        {
            return false;
        }

        bool havePrevious = false;
        uint16_t previous = 0;

        for (uint32_t i = 0; i < subtable.markCount; ++i)
        {
            const OpenTypeShapingIRGposMarkRecord& record =
                ir.gposMarkRecords[subtable.markOffset + i];

            if (record.markClass >= subtable.markClassCount ||
                !ir.anchor(record.anchor))
            {
                return false;
            }

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;
        }

        havePrevious = false;
        previous = 0;

        for (uint32_t i = 0; i < subtable.baseCount; ++i)
        {
            const OpenTypeShapingIRGposBaseRecord& record =
                ir.gposMarkBaseRecords[subtable.baseOffset + i];

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;

            const size_t row =
                subtable.baseAnchorOffset +
                size_t(i) * subtable.markClassCount;

            for (uint16_t markClass = 0;
                markClass < subtable.markClassCount;
                ++markClass)
            {
                const OpenTypeShapingIRAnchorId anchorId =
                    ir.gposMarkBaseAnchorRefs[row + markClass];

                if (anchorId != kOpenTypeShapingIRInvalid &&
                    !ir.anchor(anchorId))
                {
                    return false;
                }
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRMarkBaseLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposMarkBase ||
            lookup.payloadCount == 0 ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter) ||
            lookup.payloadOffset > ir.gposMarkBaseSubtables.size() ||
            lookup.payloadCount >
            ir.gposMarkBaseSubtables.size() - lookup.payloadOffset)
        {
            return false;
        }

        if (!ir.gdefGlyphClasses.empty() &&
            ir.gdefGlyphClasses.size() != kOpenTypeShapingIRGlyphDomainSize)
        {
            return false;
        }

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            if (!openTypeGposIRMarkBaseSubtableValid(
                ir, ir.gposMarkBaseSubtables[lookup.payloadOffset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline const OpenTypeShapingIRGposMarkRecord*
        openTypeGposIRMarkRecord(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRGposMarkBaseSubtable& subtable,
            uint16_t glyphId) noexcept
    {
        size_t first = subtable.markOffset;
        size_t last = first + subtable.markCount;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGposMarkRecord& record =
                ir.gposMarkRecords[middle];

            if (glyphId < record.glyph)
                last = middle;
            else if (glyphId > record.glyph)
                first = middle + 1;
            else
                return &record;
        }

        return nullptr;
    }


    [[nodiscard]] static inline bool openTypeGposIRMarkBaseRecordIndex(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposMarkBaseSubtable& subtable,
        uint16_t glyphId, uint32_t& result) noexcept
    {
        result = 0;

        size_t first = subtable.baseOffset;
        size_t last = first + subtable.baseCount;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGposBaseRecord& record =
                ir.gposMarkBaseRecords[middle];

            if (glyphId < record.glyph)
                last = middle;
            else if (glyphId > record.glyph)
                first = middle + 1;
            else
            {
                result =
                    static_cast<uint32_t>(
                        middle - subtable.baseOffset);

                return true;
            }
        }

        return false;
    }


    static inline OpenTypeGposIRResult findOpenTypeGposIRPreviousNonMark(
        const OpenTypeShapingIR& ir,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex, size_t& result) noexcept
    {
        result = 0;

        if (glyphIndex >= buffer.size())
            return OpenTypeGposIRResult::Invalid;

        const bool haveGdefClasses =
            !ir.gdefGlyphClasses.empty();

        if (haveGdefClasses &&
            ir.gdefGlyphClasses.size() !=
            kOpenTypeShapingIRGlyphDomainSize)
        {
            return OpenTypeGposIRResult::Invalid;
        }

        for (size_t i = glyphIndex; i != 0; --i)
        {
            const size_t candidate = i - 1;
            const uint32_t glyphId =
                buffer[candidate].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposIRResult::Invalid;

            const uint16_t glyphClass =
                haveGdefClasses
                ? ir.gdefGlyphClasses[glyphId]
                : 0;

            if (glyphClass == 3)
                continue;

            result = candidate;
            return OpenTypeGposIRResult::Match;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRMarkBaseLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRMarkBaseMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRMarkBaseLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint32_t markGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (markGlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        bool haveBase = false;
        size_t baseIndex = 0;
        uint32_t baseGlyphId = 0;

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGposMarkBaseSubtable& subtable =
                ir.gposMarkBaseSubtables[
                    lookup.payloadOffset + i];

            const OpenTypeShapingIRGposMarkRecord* markRecord =
                openTypeGposIRMarkRecord(
                    ir, subtable,
                    static_cast<uint16_t>(markGlyphId));

            if (!markRecord)
                continue;

            if (!haveBase)
            {
                const OpenTypeGposIRResult baseResult =
                    findOpenTypeGposIRPreviousNonMark(
                        ir, buffer,
                        glyphIndex, baseIndex);

                if (baseResult != OpenTypeGposIRResult::Match)
                    return baseResult;

                baseGlyphId =
                    buffer[baseIndex].shaping.glyphId;

                if (baseGlyphId > 0xFFFFu)
                    return OpenTypeGposIRResult::Invalid;

                haveBase = true;
            }

            uint32_t localBaseIndex = 0;

            if (!openTypeGposIRMarkBaseRecordIndex(
                ir, subtable,
                static_cast<uint16_t>(baseGlyphId),
                localBaseIndex))
            {
                continue;
            }

            if (markRecord->markClass >=
                subtable.markClassCount)
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const size_t anchorRefIndex =
                subtable.baseAnchorOffset +
                size_t(localBaseIndex) *
                subtable.markClassCount +
                markRecord->markClass;

            if (anchorRefIndex >=
                ir.gposMarkBaseAnchorRefs.size())
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const OpenTypeShapingIRAnchorId baseAnchorId =
                ir.gposMarkBaseAnchorRefs[
                    anchorRefIndex];

            if (baseAnchorId ==
                kOpenTypeShapingIRInvalid)
            {
                continue;
            }

            const OpenTypeShapingIRAnchor* markAnchor =
                ir.anchor(markRecord->anchor);

            const OpenTypeShapingIRAnchor* baseAnchor =
                ir.anchor(baseAnchorId);

            if (!markAnchor || !baseAnchor)
                return OpenTypeGposIRResult::Invalid;

            match.baseIndex = baseIndex;
            match.markIndex = glyphIndex;
            match.base = *baseAnchor;
            match.mark = *markAnchor;

            return OpenTypeGposIRResult::Match;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline bool applyOpenTypeGposIRMarkBaseMatch(
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposIRMarkBaseMatch& match)
    {
        if (!attachments.matches(buffer.size()) ||
            match.baseIndex >= buffer.size() ||
            match.markIndex >= buffer.size() ||
            match.baseIndex >= match.markIndex)
        {
            return false;
        }

        const int64_t localX =
            int64_t(match.base.x) -
            int64_t(match.mark.x);

        const int64_t localY =
            int64_t(match.base.y) -
            int64_t(match.mark.y);

        if (localX <
            std::numeric_limits<int32_t>::min() ||
            localX >
            std::numeric_limits<int32_t>::max() ||
            localY <
            std::numeric_limits<int32_t>::min() ||
            localY >
            std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposMark(
            attachments, buffer,
            match.markIndex, match.baseIndex,
            static_cast<int32_t>(localX),
            static_cast<int32_t>(localY));
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRMarkBaseLookupAt(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments) noexcept
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposIRMarkBaseMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRMarkBaseLookup(
                ir, lookup, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposIRMarkBaseMatch(
            working, workingAttachments,
            match))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposIRResult::Match;
    }


    static inline bool applyOpenTypeGposIRMarkBaseLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposIRMarkBaseLookupValid(ir, lookup) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposIRMarkBaseMatch match;

            const OpenTypeGposIRResult result =
                resolveOpenTypeGposIRMarkBaseLookup(
                    ir, lookup, working,
                    glyphIndex, match);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;

            if (result == OpenTypeGposIRResult::NoMatch)
                continue;

            if (!applyOpenTypeGposIRMarkBaseMatch(
                working, workingAttachments,
                match))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    static inline bool applyOpenTypeGposIRMarkBaseLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposIRMarkBaseLookup(
            ir, lookup, working,
            attachments))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 5 - MarkLigature
    // ========================================================================

    struct OpenTypeGposIRMarkLigatureMatch
    {
        size_t ligatureIndex{ 0 };
        size_t markIndex{ 0 };

        uint16_t componentIndex{ 0 };

        OpenTypeShapingIRAnchor ligature{};
        OpenTypeShapingIRAnchor mark{};

        void clear() noexcept
        {
            ligatureIndex = 0;
            markIndex = 0;
            componentIndex = 0;
            ligature = {};
            mark = {};
        }
    };


    [[nodiscard]] static inline bool openTypeGposIRMarkLigatureSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposMarkLigatureSubtable& subtable) noexcept
    {
        if (subtable.markClassCount == 0 ||
            subtable.markOffset > ir.gposMarkRecords.size() ||
            subtable.markCount >
            ir.gposMarkRecords.size() - subtable.markOffset ||
            subtable.ligatureOffset > ir.gposMarkLigatureRecords.size() ||
            subtable.ligatureCount >
            ir.gposMarkLigatureRecords.size() - subtable.ligatureOffset)
        {
            return false;
        }

        bool havePrevious = false;
        uint16_t previous = 0;

        for (uint32_t i = 0; i < subtable.markCount; ++i)
        {
            const OpenTypeShapingIRGposMarkRecord& record =
                ir.gposMarkRecords[subtable.markOffset + i];

            if (record.markClass >= subtable.markClassCount ||
                !ir.anchor(record.anchor))
            {
                return false;
            }

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;
        }

        havePrevious = false;
        previous = 0;

        for (uint32_t i = 0; i < subtable.ligatureCount; ++i)
        {
            const OpenTypeShapingIRGposLigatureRecord& record =
                ir.gposMarkLigatureRecords[subtable.ligatureOffset + i];

            if (record.componentCount == 0)
                return false;

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;

            if (record.componentAnchorOffset >
                ir.gposMarkLigatureAnchorRefs.size())
            {
                return false;
            }

            const uint64_t anchorCount =
                uint64_t(record.componentCount) *
                uint64_t(subtable.markClassCount);

            if (anchorCount >
                ir.gposMarkLigatureAnchorRefs.size() -
                record.componentAnchorOffset)
            {
                return false;
            }

            const size_t first =
                record.componentAnchorOffset;

            const size_t last =
                first + static_cast<size_t>(anchorCount);

            for (size_t index = first; index < last; ++index)
            {
                const OpenTypeShapingIRAnchorId anchorId =
                    ir.gposMarkLigatureAnchorRefs[index];

                if (anchorId != kOpenTypeShapingIRInvalid &&
                    !ir.anchor(anchorId))
                {
                    return false;
                }
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRMarkLigatureLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposMarkLigature ||
            lookup.payloadCount == 0 ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter) ||
            lookup.payloadOffset > ir.gposMarkLigatureSubtables.size() ||
            lookup.payloadCount >
            ir.gposMarkLigatureSubtables.size() - lookup.payloadOffset)
        {
            return false;
        }

        if (!ir.gdefGlyphClasses.empty() &&
            ir.gdefGlyphClasses.size() != kOpenTypeShapingIRGlyphDomainSize)
        {
            return false;
        }

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            if (!openTypeGposIRMarkLigatureSubtableValid(
                ir, ir.gposMarkLigatureSubtables[lookup.payloadOffset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline const OpenTypeShapingIRGposLigatureRecord*
        openTypeGposIRMarkLigatureRecord(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRGposMarkLigatureSubtable& subtable,
            uint16_t glyphId) noexcept
    {
        size_t first = subtable.ligatureOffset;
        size_t last = first + subtable.ligatureCount;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGposLigatureRecord& record =
                ir.gposMarkLigatureRecords[middle];

            if (glyphId < record.glyph)
                last = middle;
            else if (glyphId > record.glyph)
                first = middle + 1;
            else
                return &record;
        }

        return nullptr;
    }


    [[nodiscard]] static inline bool openTypeGposIRMarkLigatureComponent(
        const OpenTypeShapingGlyph& ligature,
        const OpenTypeShapingGlyph& mark,
        uint16_t componentCount,
        uint16_t& componentIndex) noexcept
    {
        componentIndex = 0;

        if (componentCount == 0)
            return false;

        componentIndex =
            static_cast<uint16_t>(
                componentCount - 1);

        const uint32_t ligatureId =
            ligature.ligature.id;

        const uint32_t markId =
            mark.ligature.id;

        const uint16_t markComponent =
            mark.ligature.component;

        if (ligatureId != 0 &&
            ligatureId == markId &&
            markComponent != 0)
        {
            uint16_t component =
                markComponent;

            if (component > componentCount)
                component = componentCount;

            componentIndex =
                static_cast<uint16_t>(
                    component - 1);
        }

        return true;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRMarkLigatureLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRMarkLigatureMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRMarkLigatureLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint32_t markGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (markGlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        bool haveLigature = false;
        size_t ligatureIndex = 0;
        uint32_t ligatureGlyphId = 0;

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGposMarkLigatureSubtable& subtable =
                ir.gposMarkLigatureSubtables[
                    lookup.payloadOffset + i];

            // Mark records use the same sorted semantic pool as Type 4.
            size_t markFirst = subtable.markOffset;
            size_t markLast = markFirst + subtable.markCount;
            const OpenTypeShapingIRGposMarkRecord* markRecord = nullptr;

            while (markFirst < markLast)
            {
                const size_t middle =
                    markFirst + (markLast - markFirst) / 2;

                const OpenTypeShapingIRGposMarkRecord& record =
                    ir.gposMarkRecords[middle];

                if (markGlyphId < record.glyph)
                    markLast = middle;
                else if (markGlyphId > record.glyph)
                    markFirst = middle + 1;
                else
                {
                    markRecord = &record;
                    break;
                }
            }

            if (!markRecord)
                continue;

            if (!haveLigature)
            {
                const OpenTypeGposIRResult parentResult =
                    findOpenTypeGposIRPreviousNonMark(
                        ir, buffer,
                        glyphIndex, ligatureIndex);

                if (parentResult != OpenTypeGposIRResult::Match)
                    return parentResult;

                ligatureGlyphId =
                    buffer[ligatureIndex].shaping.glyphId;

                if (ligatureGlyphId > 0xFFFFu)
                    return OpenTypeGposIRResult::Invalid;

                haveLigature = true;
            }

            const OpenTypeShapingIRGposLigatureRecord* ligatureRecord =
                openTypeGposIRMarkLigatureRecord(
                    ir, subtable,
                    static_cast<uint16_t>(ligatureGlyphId));

            if (!ligatureRecord)
                continue;

            if (markRecord->markClass >=
                subtable.markClassCount)
            {
                return OpenTypeGposIRResult::Invalid;
            }

            uint16_t componentIndex = 0;

            if (!openTypeGposIRMarkLigatureComponent(
                buffer[ligatureIndex].shaping,
                buffer[glyphIndex].shaping,
                ligatureRecord->componentCount,
                componentIndex))
            {
                return OpenTypeGposIRResult::Invalid;
            }

            if (componentIndex >=
                ligatureRecord->componentCount)
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const size_t anchorRefIndex =
                ligatureRecord->componentAnchorOffset +
                size_t(componentIndex) *
                subtable.markClassCount +
                markRecord->markClass;

            if (anchorRefIndex >=
                ir.gposMarkLigatureAnchorRefs.size())
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const OpenTypeShapingIRAnchorId ligatureAnchorId =
                ir.gposMarkLigatureAnchorRefs[
                    anchorRefIndex];

            if (ligatureAnchorId ==
                kOpenTypeShapingIRInvalid)
            {
                continue;
            }

            const OpenTypeShapingIRAnchor* markAnchor =
                ir.anchor(markRecord->anchor);

            const OpenTypeShapingIRAnchor* ligatureAnchor =
                ir.anchor(ligatureAnchorId);

            if (!markAnchor || !ligatureAnchor)
                return OpenTypeGposIRResult::Invalid;

            match.ligatureIndex = ligatureIndex;
            match.markIndex = glyphIndex;
            match.componentIndex = componentIndex;
            match.ligature = *ligatureAnchor;
            match.mark = *markAnchor;

            return OpenTypeGposIRResult::Match;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline bool applyOpenTypeGposIRMarkLigatureMatch(
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposIRMarkLigatureMatch& match)
    {
        if (!attachments.matches(buffer.size()) ||
            match.ligatureIndex >= buffer.size() ||
            match.markIndex >= buffer.size() ||
            match.ligatureIndex >= match.markIndex)
        {
            return false;
        }

        const int64_t localX =
            int64_t(match.ligature.x) -
            int64_t(match.mark.x);

        const int64_t localY =
            int64_t(match.ligature.y) -
            int64_t(match.mark.y);

        if (localX <
            std::numeric_limits<int32_t>::min() ||
            localX >
            std::numeric_limits<int32_t>::max() ||
            localY <
            std::numeric_limits<int32_t>::min() ||
            localY >
            std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposMark(
            attachments, buffer,
            match.markIndex,
            match.ligatureIndex,
            static_cast<int32_t>(localX),
            static_cast<int32_t>(localY));
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRMarkLigatureLookupAt(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments) noexcept
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposIRMarkLigatureMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRMarkLigatureLookup(
                ir, lookup, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposIRMarkLigatureMatch(
            working, workingAttachments,
            match))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposIRResult::Match;
    }


    static inline bool applyOpenTypeGposIRMarkLigatureLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposIRMarkLigatureLookupValid(ir, lookup) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposIRMarkLigatureMatch match;

            const OpenTypeGposIRResult result =
                resolveOpenTypeGposIRMarkLigatureLookup(
                    ir, lookup, working,
                    glyphIndex, match);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;

            if (result == OpenTypeGposIRResult::NoMatch)
                continue;

            if (!applyOpenTypeGposIRMarkLigatureMatch(
                working, workingAttachments,
                match))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    static inline bool applyOpenTypeGposIRMarkLigatureLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposIRMarkLigatureLookup(
            ir, lookup, working,
            attachments))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 6 - MarkMark
    // ========================================================================

    struct OpenTypeGposIRMarkMarkMatch
    {
        size_t mark1Index{ 0 };
        size_t mark2Index{ 0 };

        OpenTypeShapingIRAnchor mark1{};
        OpenTypeShapingIRAnchor mark2{};

        void clear() noexcept
        {
            mark1Index = 0;
            mark2Index = 0;
            mark1 = {};
            mark2 = {};
        }
    };


    [[nodiscard]] static inline bool openTypeGposIRMarkMarkSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposMarkMarkSubtable& subtable) noexcept
    {
        if (subtable.markClassCount == 0 ||
            subtable.mark1Offset > ir.gposMarkRecords.size() ||
            subtable.mark1Count >
            ir.gposMarkRecords.size() - subtable.mark1Offset ||
            subtable.mark2Offset > ir.gposMark2Records.size() ||
            subtable.mark2Count >
            ir.gposMark2Records.size() - subtable.mark2Offset ||
            subtable.mark2AnchorOffset > ir.gposMark2AnchorRefs.size())
        {
            return false;
        }

        const uint64_t anchorCount =
            uint64_t(subtable.mark2Count) *
            uint64_t(subtable.markClassCount);

        if (anchorCount >
            ir.gposMark2AnchorRefs.size() -
            subtable.mark2AnchorOffset)
        {
            return false;
        }

        bool havePrevious = false;
        uint16_t previous = 0;

        for (uint32_t i = 0; i < subtable.mark1Count; ++i)
        {
            const OpenTypeShapingIRGposMarkRecord& record =
                ir.gposMarkRecords[subtable.mark1Offset + i];

            if (record.markClass >= subtable.markClassCount ||
                !ir.anchor(record.anchor))
            {
                return false;
            }

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;
        }

        havePrevious = false;
        previous = 0;

        for (uint32_t i = 0; i < subtable.mark2Count; ++i)
        {
            const OpenTypeShapingIRGposMark2Record& record =
                ir.gposMark2Records[subtable.mark2Offset + i];

            if (havePrevious && record.glyph <= previous)
                return false;

            previous = record.glyph;
            havePrevious = true;

            const size_t row =
                subtable.mark2AnchorOffset +
                size_t(i) * subtable.markClassCount;

            for (uint16_t markClass = 0;
                markClass < subtable.markClassCount;
                ++markClass)
            {
                const OpenTypeShapingIRAnchorId anchorId =
                    ir.gposMark2AnchorRefs[row + markClass];

                if (anchorId != kOpenTypeShapingIRInvalid &&
                    !ir.anchor(anchorId))
                {
                    return false;
                }
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRMarkMarkLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposMarkMark ||
            lookup.payloadCount == 0 ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter) ||
            lookup.payloadOffset > ir.gposMarkMarkSubtables.size() ||
            lookup.payloadCount >
            ir.gposMarkMarkSubtables.size() - lookup.payloadOffset)
        {
            return false;
        }

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            if (!openTypeGposIRMarkMarkSubtableValid(
                ir, ir.gposMarkMarkSubtables[lookup.payloadOffset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline const OpenTypeShapingIRGposMarkRecord*
        openTypeGposIRMark1Record(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRGposMarkMarkSubtable& subtable,
            uint16_t glyphId) noexcept
    {
        size_t first = subtable.mark1Offset;
        size_t last = first + subtable.mark1Count;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGposMarkRecord& record =
                ir.gposMarkRecords[middle];

            if (glyphId < record.glyph)
                last = middle;
            else if (glyphId > record.glyph)
                first = middle + 1;
            else
                return &record;
        }

        return nullptr;
    }


    [[nodiscard]] static inline bool openTypeGposIRMark2RecordIndex(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposMarkMarkSubtable& subtable,
        uint16_t glyphId, uint32_t& result) noexcept
    {
        result = 0;

        size_t first = subtable.mark2Offset;
        size_t last = first + subtable.mark2Count;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGposMark2Record& record =
                ir.gposMark2Records[middle];

            if (glyphId < record.glyph)
                last = middle;
            else if (glyphId > record.glyph)
                first = middle + 1;
            else
            {
                result =
                    static_cast<uint32_t>(
                        middle - subtable.mark2Offset);

                return true;
            }
        }

        return false;
    }


    [[nodiscard]] static inline bool openTypeGposIRMarkMarkCompatible(
        const OpenTypeShapingGlyph& mark1,
        const OpenTypeShapingGlyph& mark2) noexcept
    {
        const uint32_t id1 =
            mark1.ligature.id;

        const uint32_t id2 =
            mark2.ligature.id;

        const uint16_t component1 =
            mark1.ligature.component;

        const uint16_t component2 =
            mark2.ligature.component;

        if (id1 == id2)
        {
            if (id1 == 0)
                return true;

            return component1 == component2;
        }

        if ((id1 != 0 && component1 == 0) ||
            (id2 != 0 && component2 == 0))
        {
            return true;
        }

        return false;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRMarkMarkLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRMarkMarkMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRMarkMarkLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const uint32_t mark1GlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (mark1GlyphId > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        bool haveMark2 = false;
        size_t mark2Index = 0;
        uint32_t mark2GlyphId = 0;

        for (uint32_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGposMarkMarkSubtable& subtable =
                ir.gposMarkMarkSubtables[
                    lookup.payloadOffset + i];

            const OpenTypeShapingIRGposMarkRecord* mark1Record =
                openTypeGposIRMark1Record(
                    ir, subtable,
                    static_cast<uint16_t>(mark1GlyphId));

            if (!mark1Record)
                continue;

            if (!haveMark2)
            {
                const OpenTypeGposIRGlyphSearchResult previousResult =
                    openTypeGposIRLookupPrevious(
                        ir, lookup.filter, buffer,
                        glyphIndex, mark2Index);

                if (previousResult ==
                    OpenTypeGposIRGlyphSearchResult::End)
                {
                    return OpenTypeGposIRResult::NoMatch;
                }

                if (previousResult !=
                    OpenTypeGposIRGlyphSearchResult::Found)
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                if (mark2Index >= glyphIndex ||
                    mark2Index >= buffer.size())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                mark2GlyphId =
                    buffer[mark2Index].shaping.glyphId;

                if (mark2GlyphId > 0xFFFFu)
                    return OpenTypeGposIRResult::Invalid;

                haveMark2 = true;

                if (!openTypeGposIRMarkMarkCompatible(
                    buffer[glyphIndex].shaping,
                    buffer[mark2Index].shaping))
                {
                    return OpenTypeGposIRResult::NoMatch;
                }
            }

            uint32_t localMark2Index = 0;

            if (!openTypeGposIRMark2RecordIndex(
                ir, subtable,
                static_cast<uint16_t>(mark2GlyphId),
                localMark2Index))
            {
                continue;
            }

            if (mark1Record->markClass >=
                subtable.markClassCount)
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const size_t anchorRefIndex =
                subtable.mark2AnchorOffset +
                size_t(localMark2Index) *
                subtable.markClassCount +
                mark1Record->markClass;

            if (anchorRefIndex >=
                ir.gposMark2AnchorRefs.size())
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const OpenTypeShapingIRAnchorId mark2AnchorId =
                ir.gposMark2AnchorRefs[
                    anchorRefIndex];

            if (mark2AnchorId ==
                kOpenTypeShapingIRInvalid)
            {
                continue;
            }

            const OpenTypeShapingIRAnchor* mark1Anchor =
                ir.anchor(mark1Record->anchor);

            const OpenTypeShapingIRAnchor* mark2Anchor =
                ir.anchor(mark2AnchorId);

            if (!mark1Anchor || !mark2Anchor)
                return OpenTypeGposIRResult::Invalid;

            match.mark1Index = glyphIndex;
            match.mark2Index = mark2Index;
            match.mark1 = *mark1Anchor;
            match.mark2 = *mark2Anchor;

            return OpenTypeGposIRResult::Match;
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline bool applyOpenTypeGposIRMarkMarkMatch(
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposIRMarkMarkMatch& match)
    {
        if (!attachments.matches(buffer.size()) ||
            match.mark1Index >= buffer.size() ||
            match.mark2Index >= buffer.size() ||
            match.mark2Index >= match.mark1Index)
        {
            return false;
        }

        const int64_t localX =
            int64_t(match.mark2.x) -
            int64_t(match.mark1.x);

        const int64_t localY =
            int64_t(match.mark2.y) -
            int64_t(match.mark1.y);

        if (localX <
            std::numeric_limits<int32_t>::min() ||
            localX >
            std::numeric_limits<int32_t>::max() ||
            localY <
            std::numeric_limits<int32_t>::min() ||
            localY >
            std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposMark(
            attachments, buffer,
            match.mark1Index,
            match.mark2Index,
            static_cast<int32_t>(localX),
            static_cast<int32_t>(localY));
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRMarkMarkLookupAt(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments) noexcept
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposIRMarkMarkMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRMarkMarkLookup(
                ir, lookup, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposIRMarkMarkMatch(
            working, workingAttachments,
            match))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposIRResult::Match;
    }


    static inline bool applyOpenTypeGposIRMarkMarkLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposIRMarkMarkLookupValid(ir, lookup) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposIRMarkMarkMatch match;

            const OpenTypeGposIRResult result =
                resolveOpenTypeGposIRMarkMarkLookup(
                    ir, lookup, working,
                    glyphIndex, match);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;

            if (result == OpenTypeGposIRResult::NoMatch)
                continue;

            if (!applyOpenTypeGposIRMarkMarkMatch(
                working, workingAttachments,
                match))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    static inline bool applyOpenTypeGposIRMarkMarkLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposIRMarkMarkLookup(
            ir, lookup, working,
            attachments))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 7 - Context
    // ========================================================================

    struct OpenTypeGposIRContextMatch
    {
        std::vector<size_t> positions{};
        uint32_t lookupOffset{ 0 };
        uint32_t lookupCount{ 0 };

        void clear() noexcept
        {
            positions.clear();
            lookupOffset = 0;
            lookupCount = 0;
        }

        [[nodiscard]] bool empty() const noexcept { return positions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return positions.size(); }
    };


    [[nodiscard]] static inline bool openTypeGposIRContextRuleValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposContextRule& rule) noexcept
    {
        const size_t inputOffset = rule.inputSetOffset;
        const size_t inputCount = rule.inputCount;
        const size_t lookupOffset = rule.lookupOffset;
        const size_t lookupCount = rule.lookupCount;

        if (inputCount == 0)
            return false;

        if (inputOffset > ir.gposContextInputSets.size() ||
            inputCount > ir.gposContextInputSets.size() - inputOffset)
        {
            return false;
        }

        if (lookupOffset > ir.gposContextLookups.size() ||
            lookupCount > ir.gposContextLookups.size() - lookupOffset)
        {
            return false;
        }

        for (size_t i = 0; i < inputCount; ++i)
        {
            if (!openTypeGposIRGlyphSetValid(
                ir, ir.gposContextInputSets[inputOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gposContextLookups[lookupOffset + i];

            if (action.lookup == kOpenTypeShapingIRInvalid ||
                !ir.lookup(action.lookup))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRContextSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposContextSubtable& subtable) noexcept
    {
        const size_t offset = subtable.ruleOffset;
        const size_t count = subtable.ruleCount;

        if (offset > ir.gposContextRules.size() ||
            count > ir.gposContextRules.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGposIRContextRuleValid(
                ir, ir.gposContextRules[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRContextLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposContext ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter))
        {
            return false;
        }

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gposContextSubtables.size() ||
            count == 0 ||
            count > ir.gposContextSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGposIRContextSubtableValid(
                ir, ir.gposContextSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    static inline OpenTypeGposIRResult matchOpenTypeGposIRContextRuleUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeShapingIRGposContextRule& rule,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRContextMatch& match) noexcept
    {
        match.clear();

        if (glyphIndex >= buffer.size() || rule.inputCount == 0)
            return OpenTypeGposIRResult::Invalid;

        const uint32_t firstGlyph =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyph > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        bool member = false;

        if (!openTypeGposIRGlyphSetContains(
            ir,
            ir.gposContextInputSets[rule.inputSetOffset],
            static_cast<uint16_t>(firstGlyph),
            member))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        if (!member)
            return OpenTypeGposIRResult::NoMatch;

        match.positions.reserve(rule.inputCount);
        match.positions.push_back(glyphIndex);

        size_t position = glyphIndex;

        for (uint32_t sequenceIndex = 1;
            sequenceIndex < rule.inputCount;
            ++sequenceIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGposIRGlyphSearchResult nextResult =
                openTypeGposIRLookupNext(
                    ir, filter, buffer,
                    position, nextPosition);

            if (nextResult == OpenTypeGposIRGlyphSearchResult::End)
                return OpenTypeGposIRResult::NoMatch;

            if (nextResult != OpenTypeGposIRGlyphSearchResult::Found)
                return OpenTypeGposIRResult::Invalid;

            if (nextPosition >= buffer.size())
                return OpenTypeGposIRResult::Invalid;

            const uint32_t glyphId =
                buffer[nextPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposIRResult::Invalid;

            if (!openTypeGposIRGlyphSetContains(
                ir,
                ir.gposContextInputSets[
                    rule.inputSetOffset + sequenceIndex],
                    static_cast<uint16_t>(glyphId),
                    member))
            {
                return OpenTypeGposIRResult::Invalid;
            }

            if (!member)
                return OpenTypeGposIRResult::NoMatch;

            match.positions.push_back(nextPosition);
            position = nextPosition;
        }

        match.lookupOffset = rule.lookupOffset;
        match.lookupCount = rule.lookupCount;

        return OpenTypeGposIRResult::Match;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRContextMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRContextLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        for (uint32_t subtableIndex = 0;
            subtableIndex < lookup.payloadCount;
            ++subtableIndex)
        {
            const OpenTypeShapingIRGposContextSubtable& subtable =
                ir.gposContextSubtables[
                    lookup.payloadOffset + subtableIndex];

            for (uint32_t ruleIndex = 0;
                ruleIndex < subtable.ruleCount;
                ++ruleIndex)
            {
                const OpenTypeShapingIRGposContextRule& rule =
                    ir.gposContextRules[
                        subtable.ruleOffset + ruleIndex];

                const OpenTypeGposIRResult result =
                    matchOpenTypeGposIRContextRuleUnchecked(
                        ir, lookup.filter, rule,
                        buffer, glyphIndex, match);

                if (result == OpenTypeGposIRResult::Invalid)
                    return result;

                if (result == OpenTypeGposIRResult::Match)
                    return result;
            }
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    // ========================================================================
    // Contextual physical-range workspace
    //
    // Nested positioning is restricted to the physical span occupied by the
    // matched input sequence. Glyphs ignored by the outer lookup remain inside
    // this span so nested lookups can apply their own LookupFlag semantics.
    // ========================================================================

    static inline bool copyOpenTypeGposIRRange(
        const ShapedGlyphBuffer& source,
        const OpenTypeGposAttachmentState& sourceAttachments,
        const OpenTypeGposApplyRange& range,
        ShapedGlyphBuffer& target,
        OpenTypeGposAttachmentState& targetAttachments)
    {
        if (!range.validFor(source.size()) ||
            !sourceAttachments.matches(source.size()))
        {
            return false;
        }

        target.clear();

        for (size_t i = range.begin; i < range.end; ++i)
            target.pushBack(source[i]);

        targetAttachments.reset(target.size());

        for (size_t i = range.begin; i < range.end; ++i)
        {
            const size_t localIndex = i - range.begin;
            const OpenTypeGposAttachment& attachment =
                sourceAttachments[i];

            if (attachment.type == OpenTypeGposAttachmentType::None)
                continue;

            if (attachment.parent < range.begin ||
                attachment.parent >= range.end)
            {
                continue;
            }

            targetAttachments[localIndex] = attachment;
            targetAttachments[localIndex].parent =
                attachment.parent - range.begin;
        }

        return true;
    }


    static inline bool mergeOpenTypeGposIRRange(
        ShapedGlyphBuffer& target,
        OpenTypeGposAttachmentState& targetAttachments,
        const OpenTypeGposApplyRange& range,
        const ShapedGlyphBuffer& source,
        const OpenTypeGposAttachmentState& sourceAttachments)
    {
        if (!range.validFor(target.size()) ||
            source.size() != range.size() ||
            !targetAttachments.matches(target.size()) ||
            !sourceAttachments.matches(source.size()))
        {
            return false;
        }

        for (size_t i = 0; i < source.size(); ++i)
        {
            const size_t globalIndex = range.begin + i;

            if (target[globalIndex].shaping.glyphId !=
                source[i].shaping.glyphId ||
                target[globalIndex].shaping.scalarOffset !=
                source[i].shaping.scalarOffset ||
                target[globalIndex].shaping.scalarCount !=
                source[i].shaping.scalarCount ||
                target[globalIndex].shaping.ligature.id !=
                source[i].shaping.ligature.id ||
                target[globalIndex].shaping.ligature.component !=
                source[i].shaping.ligature.component ||
                target[globalIndex].shaping.ligature.componentCount !=
                source[i].shaping.ligature.componentCount)
            {
                return false;
            }

            target[globalIndex].placement =
                source[i].placement;

            const OpenTypeGposAttachment& attachment =
                sourceAttachments[i];

            if (attachment.type == OpenTypeGposAttachmentType::None)
                continue;

            if (attachment.parent >= source.size())
                return false;

            targetAttachments[globalIndex] = attachment;
            targetAttachments[globalIndex].parent =
                range.begin + attachment.parent;
        }

        return true;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRContextLookupAtInternal(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state) noexcept;


    static inline OpenTypeGposIRResult applyOpenTypeGposIRChainContextLookupAtInternal(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state) noexcept;


    static inline OpenTypeGposIRResult applyOpenTypeGposIRNestedLookupAt(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId lookupId,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state,
        const OpenTypeGposApplyRange& range) noexcept
    {
        if (!attachments.matches(buffer.size()) ||
            !range.validFor(buffer.size()) ||
            !range.contains(glyphIndex))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const OpenTypeShapingIRLookup* lookup =
            ir.lookup(lookupId);

        if (!lookup)
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposApplyScope scope(state);

        if (!scope || !state.consumeOperation())
            return OpenTypeGposIRResult::Invalid;

        ShapedGlyphBuffer localBuffer;
        OpenTypeGposAttachmentState localAttachments;

        if (!copyOpenTypeGposIRRange(
            buffer, attachments, range,
            localBuffer, localAttachments))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const size_t localIndex =
            glyphIndex - range.begin;

        OpenTypeGposIRResult result =
            OpenTypeGposIRResult::Invalid;

        if (lookup->op == OpenTypeShapingIROp::GposContext)
        {
            result =
                applyOpenTypeGposIRContextLookupAtInternal(
                    ir, *lookup, localBuffer,
                    localIndex, runRightToLeft,
                    localAttachments, state);
        }
        else if (lookup->op == OpenTypeShapingIROp::GposChainContext)
        {
            result =
                applyOpenTypeGposIRChainContextLookupAtInternal(
                    ir, *lookup, localBuffer,
                    localIndex, runRightToLeft,
                    localAttachments, state);
        }
        else
        {
            switch (lookup->op)
            {
            case OpenTypeShapingIROp::GposSingle:
                result = applyOpenTypeGposIRSingleLookupAt(
                    ir, *lookup, localBuffer, localIndex);
                break;

            case OpenTypeShapingIROp::GposPair:
                result = applyOpenTypeGposIRPairLookupAt(
                    ir, *lookup, localBuffer,
                    localIndex, nullptr);
                break;

            case OpenTypeShapingIROp::GposCursive:
                result = applyOpenTypeGposIRCursiveLookupAt(
                    ir, *lookup, localBuffer,
                    localIndex, runRightToLeft,
                    localAttachments);
                break;

            case OpenTypeShapingIROp::GposMarkBase:
                result = applyOpenTypeGposIRMarkBaseLookupAt(
                    ir, *lookup, localBuffer,
                    localIndex, localAttachments);
                break;

            case OpenTypeShapingIROp::GposMarkLigature:
                result = applyOpenTypeGposIRMarkLigatureLookupAt(
                    ir, *lookup, localBuffer,
                    localIndex, localAttachments);
                break;

            case OpenTypeShapingIROp::GposMarkMark:
                result = applyOpenTypeGposIRMarkMarkLookupAt(
                    ir, *lookup, localBuffer,
                    localIndex, localAttachments);
                break;

            default:
                return OpenTypeGposIRResult::Invalid;
            }
        }

        if (result != OpenTypeGposIRResult::Match)
            return result;

        if (!mergeOpenTypeGposIRRange(
            buffer, attachments, range,
            localBuffer, localAttachments))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        return OpenTypeGposIRResult::Match;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRContextLookupAtInternal(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state) noexcept
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposIRContextMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRContextLookup(
                ir, lookup, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        if (match.positions.empty() ||
            match.positions[0] != glyphIndex)
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const size_t lastPosition =
            match.positions.back();

        if (lastPosition == std::numeric_limits<size_t>::max())
            return OpenTypeGposIRResult::Invalid;

        const size_t rangeEnd =
            lastPosition + 1;

        if (rangeEnd <= lastPosition ||
            rangeEnd > buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const OpenTypeGposApplyRange range{
            match.positions.front(),
            rangeEnd
        };

        for (uint32_t i = 0; i < match.lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gposContextLookups[
                    match.lookupOffset + i];

            // OpenType permits oversized sequenceIndex values. They simply
            // produce no positioning action.
            if (action.sequenceIndex >= match.positions.size())
                continue;

            const size_t targetPosition =
                match.positions[action.sequenceIndex];

            const OpenTypeGposIRResult nestedResult =
                applyOpenTypeGposIRNestedLookupAt(
                    ir, action.lookup,
                    buffer, targetPosition,
                    runRightToLeft, attachments,
                    state, range);

            if (nestedResult == OpenTypeGposIRResult::Invalid)
                return OpenTypeGposIRResult::Invalid;

            // Nested NoMatch is legal. Later actions still execute.
        }

        // A contextual rule with zero effective positioning actions still
        // counts as a successful match. Later rules/subtables are not tried.
        return OpenTypeGposIRResult::Match;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRContextLookupAt(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state) noexcept
    {
        OpenTypeGposApplyScope scope(state);

        if (!scope || !state.consumeOperation())
            return OpenTypeGposIRResult::Invalid;

        return applyOpenTypeGposIRContextLookupAtInternal(
            ir, lookup, buffer, glyphIndex,
            runRightToLeft, attachments, state);
    }


    static inline bool applyOpenTypeGposIRContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state)
    {
        if (!openTypeGposIRContextLookupValid(ir, lookup) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRContextLookupAt(
                    ir, lookup, working,
                    glyphIndex, runRightToLeft,
                    workingAttachments, state);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);
        return true;
    }


    static inline bool applyOpenTypeGposIRContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments)
    {
        OpenTypeGposApplyState state;

        return applyOpenTypeGposIRContextLookup(
            ir, lookup, buffer,
            runRightToLeft, attachments, state);
    }


    static inline bool applyOpenTypeGposIRContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        OpenTypeGposApplyState state;

        if (!applyOpenTypeGposIRContextLookup(
            ir, lookup, working,
            runRightToLeft, attachments, state))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GPOS Type 8 - ChainContext
    // ========================================================================

    struct OpenTypeGposIRChainContextMatch
    {
        std::vector<size_t> inputPositions{};
        uint32_t lookupOffset{ 0 };
        uint32_t lookupCount{ 0 };

        void clear() noexcept
        {
            inputPositions.clear();
            lookupOffset = 0;
            lookupCount = 0;
        }

        [[nodiscard]] bool empty() const noexcept { return inputPositions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return inputPositions.size(); }
    };


    [[nodiscard]] static inline bool openTypeGposIRChainContextRuleValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposChainContextRule& rule) noexcept
    {
        const size_t backtrackOffset = rule.backtrackSetOffset;
        const size_t backtrackCount = rule.backtrackCount;
        const size_t inputOffset = rule.inputSetOffset;
        const size_t inputCount = rule.inputCount;
        const size_t lookaheadOffset = rule.lookaheadSetOffset;
        const size_t lookaheadCount = rule.lookaheadCount;
        const size_t lookupOffset = rule.lookupOffset;
        const size_t lookupCount = rule.lookupCount;

        if (inputCount == 0)
            return false;

        if (backtrackOffset > ir.gposChainContextBacktrackSets.size() ||
            backtrackCount > ir.gposChainContextBacktrackSets.size() - backtrackOffset ||
            inputOffset > ir.gposChainContextInputSets.size() ||
            inputCount > ir.gposChainContextInputSets.size() - inputOffset ||
            lookaheadOffset > ir.gposChainContextLookaheadSets.size() ||
            lookaheadCount > ir.gposChainContextLookaheadSets.size() - lookaheadOffset ||
            lookupOffset > ir.gposChainContextLookups.size() ||
            lookupCount > ir.gposChainContextLookups.size() - lookupOffset)
        {
            return false;
        }

        for (size_t i = 0; i < backtrackCount; ++i)
        {
            if (!openTypeGposIRGlyphSetValid(
                ir, ir.gposChainContextBacktrackSets[backtrackOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < inputCount; ++i)
        {
            if (!openTypeGposIRGlyphSetValid(
                ir, ir.gposChainContextInputSets[inputOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookaheadCount; ++i)
        {
            if (!openTypeGposIRGlyphSetValid(
                ir, ir.gposChainContextLookaheadSets[lookaheadOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gposChainContextLookups[lookupOffset + i];

            if (action.lookup == kOpenTypeShapingIRInvalid ||
                !ir.lookup(action.lookup))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRChainContextSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGposChainContextSubtable& subtable) noexcept
    {
        const size_t offset = subtable.ruleOffset;
        const size_t count = subtable.ruleCount;

        if (offset > ir.gposChainContextRules.size() ||
            count > ir.gposChainContextRules.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGposIRChainContextRuleValid(
                ir, ir.gposChainContextRules[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGposIRChainContextLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GposChainContext ||
            !openTypeGposIRLookupFilterValid(ir, lookup.filter))
        {
            return false;
        }

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gposChainContextSubtables.size() ||
            count == 0 ||
            count > ir.gposChainContextSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGposIRChainContextSubtableValid(
                ir, ir.gposChainContextSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    static inline OpenTypeGposIRResult matchOpenTypeGposIRChainContextRuleUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeShapingIRGposChainContextRule& rule,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRChainContextMatch& match) noexcept
    {
        match.clear();

        if (glyphIndex >= buffer.size() || rule.inputCount == 0)
            return OpenTypeGposIRResult::Invalid;

        const uint32_t firstGlyph =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyph > 0xFFFFu)
            return OpenTypeGposIRResult::Invalid;

        bool member = false;

        if (!openTypeGposIRGlyphSetContains(
            ir,
            ir.gposChainContextInputSets[rule.inputSetOffset],
            static_cast<uint16_t>(firstGlyph),
            member))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        if (!member)
            return OpenTypeGposIRResult::NoMatch;

        match.inputPositions.reserve(rule.inputCount);
        match.inputPositions.push_back(glyphIndex);

        // Match the rest of the input sequence using the parent LookupFlag.
        size_t position = glyphIndex;

        for (uint32_t i = 1; i < rule.inputCount; ++i)
        {
            size_t nextPosition = 0;

            const OpenTypeGposIRGlyphSearchResult nextResult =
                openTypeGposIRLookupNext(
                    ir, filter, buffer,
                    position, nextPosition);

            if (nextResult == OpenTypeGposIRGlyphSearchResult::End)
                return OpenTypeGposIRResult::NoMatch;

            if (nextResult != OpenTypeGposIRGlyphSearchResult::Found ||
                nextPosition >= buffer.size())
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const uint32_t glyphId =
                buffer[nextPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposIRResult::Invalid;

            if (!openTypeGposIRGlyphSetContains(
                ir,
                ir.gposChainContextInputSets[rule.inputSetOffset + i],
                static_cast<uint16_t>(glyphId),
                member))
            {
                return OpenTypeGposIRResult::Invalid;
            }

            if (!member)
                return OpenTypeGposIRResult::NoMatch;

            match.inputPositions.push_back(nextPosition);
            position = nextPosition;
        }

        // Backtrack sets are nearest-first relative to the current glyph.
        position = glyphIndex;

        for (uint32_t i = 0; i < rule.backtrackCount; ++i)
        {
            size_t previousPosition = 0;

            const OpenTypeGposIRGlyphSearchResult previousResult =
                openTypeGposIRLookupPrevious(
                    ir, filter, buffer,
                    position, previousPosition);

            if (previousResult == OpenTypeGposIRGlyphSearchResult::End)
                return OpenTypeGposIRResult::NoMatch;

            if (previousResult != OpenTypeGposIRGlyphSearchResult::Found ||
                previousPosition >= buffer.size())
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const uint32_t glyphId =
                buffer[previousPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposIRResult::Invalid;

            if (!openTypeGposIRGlyphSetContains(
                ir,
                ir.gposChainContextBacktrackSets[
                    rule.backtrackSetOffset + i],
                    static_cast<uint16_t>(glyphId),
                    member))
            {
                return OpenTypeGposIRResult::Invalid;
            }

            if (!member)
                return OpenTypeGposIRResult::NoMatch;

            position = previousPosition;
        }

        // Lookahead starts after the last matched input glyph and is also
        // stored nearest-first.
        position = match.inputPositions.back();

        for (uint32_t i = 0; i < rule.lookaheadCount; ++i)
        {
            size_t nextPosition = 0;

            const OpenTypeGposIRGlyphSearchResult nextResult =
                openTypeGposIRLookupNext(
                    ir, filter, buffer,
                    position, nextPosition);

            if (nextResult == OpenTypeGposIRGlyphSearchResult::End)
                return OpenTypeGposIRResult::NoMatch;

            if (nextResult != OpenTypeGposIRGlyphSearchResult::Found ||
                nextPosition >= buffer.size())
            {
                return OpenTypeGposIRResult::Invalid;
            }

            const uint32_t glyphId =
                buffer[nextPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposIRResult::Invalid;

            if (!openTypeGposIRGlyphSetContains(
                ir,
                ir.gposChainContextLookaheadSets[
                    rule.lookaheadSetOffset + i],
                    static_cast<uint16_t>(glyphId),
                    member))
            {
                return OpenTypeGposIRResult::Invalid;
            }

            if (!member)
                return OpenTypeGposIRResult::NoMatch;

            position = nextPosition;
        }

        match.lookupOffset = rule.lookupOffset;
        match.lookupCount = rule.lookupCount;
        return OpenTypeGposIRResult::Match;
    }


    static inline OpenTypeGposIRResult resolveOpenTypeGposIRChainContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposIRChainContextMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposIRChainContextLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        for (uint32_t subtableIndex = 0;
            subtableIndex < lookup.payloadCount;
            ++subtableIndex)
        {
            const OpenTypeShapingIRGposChainContextSubtable& subtable =
                ir.gposChainContextSubtables[
                    lookup.payloadOffset + subtableIndex];

            for (uint32_t ruleIndex = 0;
                ruleIndex < subtable.ruleCount;
                ++ruleIndex)
            {
                const OpenTypeShapingIRGposChainContextRule& rule =
                    ir.gposChainContextRules[
                        subtable.ruleOffset + ruleIndex];

                const OpenTypeGposIRResult result =
                    matchOpenTypeGposIRChainContextRuleUnchecked(
                        ir, lookup.filter, rule,
                        buffer, glyphIndex, match);

                if (result == OpenTypeGposIRResult::Invalid)
                    return result;

                if (result == OpenTypeGposIRResult::Match)
                    return result;
            }
        }

        return OpenTypeGposIRResult::NoMatch;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRChainContextLookupAtInternal(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state) noexcept
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposIRResult::Invalid;

        OpenTypeGposIRChainContextMatch match;

        const OpenTypeGposIRResult result =
            resolveOpenTypeGposIRChainContextLookup(
                ir, lookup, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposIRResult::Match)
            return result;

        if (match.inputPositions.empty() ||
            match.inputPositions[0] != glyphIndex)
        {
            return OpenTypeGposIRResult::Invalid;
        }

        const size_t lastPosition =
            match.inputPositions.back();

        if (lastPosition == std::numeric_limits<size_t>::max())
            return OpenTypeGposIRResult::Invalid;

        const size_t rangeEnd =
            lastPosition + 1;

        if (rangeEnd <= lastPosition ||
            rangeEnd > buffer.size())
        {
            return OpenTypeGposIRResult::Invalid;
        }

        // Only the matched input sequence is the nested positioning workspace.
        // Backtrack and lookahead constraints remain outside this range.
        const OpenTypeGposApplyRange range{
            match.inputPositions.front(),
            rangeEnd
        };

        for (uint32_t i = 0; i < match.lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gposChainContextLookups[
                    match.lookupOffset + i];

            if (action.sequenceIndex >= match.inputPositions.size())
                continue;

            const size_t targetPosition =
                match.inputPositions[action.sequenceIndex];

            const OpenTypeGposIRResult nestedResult =
                applyOpenTypeGposIRNestedLookupAt(
                    ir, action.lookup,
                    buffer, targetPosition,
                    runRightToLeft, attachments,
                    state, range);

            if (nestedResult == OpenTypeGposIRResult::Invalid)
                return OpenTypeGposIRResult::Invalid;

            // Nested NoMatch is legal. Continue with later actions.
        }

        return OpenTypeGposIRResult::Match;
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRChainContextLookupAt(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state) noexcept
    {
        OpenTypeGposApplyScope scope(state);

        if (!scope || !state.consumeOperation())
            return OpenTypeGposIRResult::Invalid;

        return applyOpenTypeGposIRChainContextLookupAtInternal(
            ir, lookup, buffer, glyphIndex,
            runRightToLeft, attachments, state);
    }


    static inline bool applyOpenTypeGposIRChainContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state)
    {
        if (!openTypeGposIRChainContextLookupValid(ir, lookup) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRChainContextLookupAt(
                    ir, lookup, working,
                    glyphIndex, runRightToLeft,
                    workingAttachments, state);

            if (result == OpenTypeGposIRResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);
        return true;
    }


    static inline bool applyOpenTypeGposIRChainContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments)
    {
        OpenTypeGposApplyState state;

        return applyOpenTypeGposIRChainContextLookup(
            ir, lookup, buffer,
            runRightToLeft, attachments, state);
    }


    static inline bool applyOpenTypeGposIRChainContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        OpenTypeGposApplyState state;

        if (!applyOpenTypeGposIRChainContextLookup(
            ir, lookup, working,
            runRightToLeft, attachments, state))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // Generic GPOS IR dispatch
    // ========================================================================

    static inline OpenTypeGposIRResult applyOpenTypeGposIRLookupAt(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        bool runRightToLeft, OpenTypeGposAttachmentState& attachments,
        size_t* resumeIndex = nullptr) noexcept
    {
        const OpenTypeShapingIRLookup* lookup =
            ir.lookup(lookupId);

        if (!lookup ||
            !attachments.matches(buffer.size()))
        {
            return OpenTypeGposIRResult::Invalid;
        }

        if (resumeIndex)
            *resumeIndex = glyphIndex;

        switch (lookup->op)
        {
        case OpenTypeShapingIROp::GposSingle:
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRSingleLookupAt(
                    ir, *lookup, buffer, glyphIndex);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposPair:
            return applyOpenTypeGposIRPairLookupAt(
                ir, *lookup, buffer,
                glyphIndex, resumeIndex);

        case OpenTypeShapingIROp::GposCursive:
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRCursiveLookupAt(
                    ir, *lookup, buffer,
                    glyphIndex, runRightToLeft,
                    attachments);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposMarkBase:
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRMarkBaseLookupAt(
                    ir, *lookup, buffer,
                    glyphIndex, attachments);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposMarkLigature:
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRMarkLigatureLookupAt(
                    ir, *lookup, buffer,
                    glyphIndex, attachments);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposMarkMark:
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRMarkMarkLookupAt(
                    ir, *lookup, buffer,
                    glyphIndex, attachments);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposContext:
        {
            OpenTypeGposApplyState state;

            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRContextLookupAt(
                    ir, *lookup, buffer,
                    glyphIndex, runRightToLeft,
                    attachments, state);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposChainContext:
        {
            OpenTypeGposApplyState state;

            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRChainContextLookupAt(
                    ir, *lookup, buffer,
                    glyphIndex, runRightToLeft,
                    attachments, state);

            if (result == OpenTypeGposIRResult::Match &&
                resumeIndex)
            {
                if (glyphIndex ==
                    std::numeric_limits<size_t>::max())
                {
                    return OpenTypeGposIRResult::Invalid;
                }

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        default:
            return OpenTypeGposIRResult::Invalid;
        }
    }


    static inline OpenTypeGposIRResult applyOpenTypeGposIRLookupAt(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        size_t* resumeIndex = nullptr) noexcept
    {
        const OpenTypeShapingIRLookup* lookup = ir.lookup(lookupId);

        if (!lookup)
            return OpenTypeGposIRResult::Invalid;

        if (resumeIndex)
            *resumeIndex = glyphIndex;

        switch (lookup->op)
        {
        case OpenTypeShapingIROp::GposSingle:
        {
            const OpenTypeGposIRResult result =
                applyOpenTypeGposIRSingleLookupAt(
                    ir, *lookup, buffer, glyphIndex);

            if (result == OpenTypeGposIRResult::Match && resumeIndex)
            {
                if (glyphIndex == std::numeric_limits<size_t>::max())
                    return OpenTypeGposIRResult::Invalid;

                *resumeIndex = glyphIndex + 1;
            }

            return result;
        }

        case OpenTypeShapingIROp::GposPair:
            return applyOpenTypeGposIRPairLookupAt(
                ir, *lookup, buffer, glyphIndex, resumeIndex);

        default:
            return OpenTypeGposIRResult::Invalid;
        }
    }


    static inline bool applyOpenTypeGposIRLookup(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        ShapedGlyphBuffer& buffer, bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments)
    {
        const OpenTypeShapingIRLookup* lookup =
            ir.lookup(lookupId);

        if (!lookup ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        switch (lookup->op)
        {
        case OpenTypeShapingIROp::GposSingle:
            return applyOpenTypeGposIRSingleLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GposPair:
            return applyOpenTypeGposIRPairLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GposCursive:
            return applyOpenTypeGposIRCursiveLookup(
                ir, *lookup, buffer,
                runRightToLeft, attachments);

        case OpenTypeShapingIROp::GposMarkBase:
            return applyOpenTypeGposIRMarkBaseLookup(
                ir, *lookup, buffer,
                attachments);

        case OpenTypeShapingIROp::GposMarkLigature:
            return applyOpenTypeGposIRMarkLigatureLookup(
                ir, *lookup, buffer,
                attachments);

        case OpenTypeShapingIROp::GposMarkMark:
            return applyOpenTypeGposIRMarkMarkLookup(
                ir, *lookup, buffer,
                attachments);

        case OpenTypeShapingIROp::GposContext:
            return applyOpenTypeGposIRContextLookup(
                ir, *lookup, buffer,
                runRightToLeft, attachments);

        case OpenTypeShapingIROp::GposChainContext:
            return applyOpenTypeGposIRChainContextLookup(
                ir, *lookup, buffer,
                runRightToLeft, attachments);

        default:
            return false;
        }
    }


    static inline bool applyOpenTypeGposIRLookup(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        ShapedGlyphBuffer& buffer)
    {
        const OpenTypeShapingIRLookup* lookup = ir.lookup(lookupId);

        if (!lookup)
            return false;

        switch (lookup->op)
        {
        case OpenTypeShapingIROp::GposSingle:
            return applyOpenTypeGposIRSingleLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GposPair:
            return applyOpenTypeGposIRPairLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GposCursive:
            return applyOpenTypeGposIRCursiveLookup(
                ir, *lookup, buffer, false);

        case OpenTypeShapingIROp::GposMarkBase:
            return applyOpenTypeGposIRMarkBaseLookup(
                ir, *lookup, buffer, false);

        case OpenTypeShapingIROp::GposMarkLigature:
            return applyOpenTypeGposIRMarkLigatureLookup(
                ir, *lookup, buffer, false);

        case OpenTypeShapingIROp::GposMarkMark:
            return applyOpenTypeGposIRMarkMarkLookup(
                ir, *lookup, buffer, false);

        case OpenTypeShapingIROp::GposContext:
            return applyOpenTypeGposIRContextLookup(
                ir, *lookup, buffer, false);

        case OpenTypeShapingIROp::GposChainContext:
            return applyOpenTypeGposIRChainContextLookup(
                ir, *lookup, buffer, false);

        default:
            return false;
        }
    }

} // namespace waavs
