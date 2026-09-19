// opentype_gpos_ir_compiler.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gpos_extension_view.h"
#include "opentype_gpos_cursive_view.h"
#include "opentype_gpos_context_view.h"
#include "opentype_gpos_chain_context_view.h"
#include "opentype_gpos_mark_base_view.h"
#include "opentype_gpos_mark_ligature_view.h"
#include "opentype_gpos_mark_mark_view.h"
#include "opentype_gpos_pair_view.h"
#include "opentype_gpos_single_view.h"
#include "opentype_gpos_value_record.h"
#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_shaping_ir.h"

namespace waavs
{
    // ========================================================================
    // Effective GPOS lookup type / subtable
    //
    // GPOS Type 9 ExtensionPos is a source-format detail. It is erased during
    // compilation and never appears in semantic IR.
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGposIREffectiveType(
        const OpenTypeLayoutLookupView& lookup, uint16_t& result) noexcept
    {
        result = 0;

        if (!lookup)
            return false;

        const uint16_t lookupType = lookup.lookupType();

        if (lookupType != 9)
        {
            if (lookupType < 1 || lookupType > 8)
                return false;

            result = lookupType;
            return true;
        }

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        uint16_t effectiveType = 0;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data = lookup.subtable(i);

            if (!data)
                return false;

            const OpenTypeGposExtensionPosView extension(data);

            if (!extension)
                return false;

            const uint16_t type = extension.extensionLookupType();

            if (type < 1 || type > 8)
                return false;

            if (i == 0)
                effectiveType = type;
            else if (type != effectiveType)
                return false;
        }

        result = effectiveType;
        return true;
    }


    [[nodiscard]] static inline ByteSpan openTypeGposIREffectiveSubtable(
        const OpenTypeLayoutLookupView& lookup, uint16_t effectiveType,
        uint16_t subtableIndex) noexcept
    {
        if (!lookup || subtableIndex >= lookup.subtableCount())
            return {};

        const ByteSpan data = lookup.subtable(subtableIndex);

        if (!data)
            return {};

        if (lookup.lookupType() != 9)
            return lookup.lookupType() == effectiveType ? data : ByteSpan{};

        const OpenTypeGposExtensionPosView extension(data);

        if (!extension || extension.extensionLookupType() != effectiveType)
            return {};

        return extension.extensionSubtable();
    }


    // ========================================================================
    // Generic normalized glyph set helper
    // ========================================================================

    static inline bool appendOpenTypeGposIRGlyphSetRanges(
        const std::vector<OpenTypeShapingIRGlyphRange>& ranges,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        const uint64_t max32 = std::numeric_limits<uint32_t>::max();

        if (ir.glyphRanges.size() > max32 ||
            ir.glyphSets.size() > max32 ||
            ranges.size() > max32)
        {
            return false;
        }

        const uint32_t rangeOffset =
            static_cast<uint32_t>(ir.glyphRanges.size());

        const uint32_t rangeCount =
            static_cast<uint32_t>(ranges.size());

        if (uint64_t(rangeOffset) + rangeCount > max32)
            return false;

        result =
            static_cast<OpenTypeShapingIRGlyphSetId>(
                ir.glyphSets.size());

        ir.glyphRanges.insert(
            ir.glyphRanges.end(),
            ranges.begin(),
            ranges.end());

        ir.glyphSets.push_back({
            rangeOffset,
            rangeCount
            });

        return true;
    }


    static inline bool compileOpenTypeGposIRCoverageGlyphSet(
        const OpenTypeCoverageView& coverage,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!coverage)
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;
            const bool member = coverage.find(glyphId, coverageIndex);

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        return appendOpenTypeGposIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeGposIRSingletonGlyphSet(
        uint16_t glyphId, OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId& result)
    {
        const std::vector<OpenTypeShapingIRGlyphRange> ranges{
            { glyphId, glyphId }
        };

        return appendOpenTypeGposIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeGposIRClassGlyphSet(
        const OpenTypeClassDefView& classDef, uint16_t classValue,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!classDef)
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t actualClass = 0;

            if (!classDef.classValue(glyphId, actualClass))
                return false;

            const bool member = actualClass == classValue;

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        return appendOpenTypeGposIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeGposIRCoverageClassGlyphSet(
        const OpenTypeCoverageView& coverage,
        const OpenTypeClassDefView& classDef, uint16_t classValue,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!coverage || !classDef)
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;
            uint16_t actualClass = 0;

            if (!classDef.classValue(glyphId, actualClass))
                return false;

            const bool member =
                actualClass == classValue &&
                coverage.find(glyphId, coverageIndex);

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        return appendOpenTypeGposIRGlyphSetRanges(
            ranges, ir, result);
    }


    [[nodiscard]] static inline bool openTypeGposIRGlyphSetEmpty(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId id) noexcept
    {
        const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(id);
        return !set || set->rangeCount == 0;
    }


    // ========================================================================
    // Normalized glyph class maps
    // ========================================================================

    static inline bool compileOpenTypeGposIRGlyphClassMap(
        const OpenTypeClassDefView& classDef,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphClassMapId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!classDef)
            return false;

        std::vector<OpenTypeShapingIRGlyphClassRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;
        uint16_t rangeValue = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t value = 0;

            if (!classDef.classValue(glyphId, value))
                return false;

            if (value == 0)
            {
                if (inRange)
                {
                    ranges.push_back({
                        rangeFirst,
                        rangeLast,
                        rangeValue,
                        0
                        });

                    inRange = false;
                }

                continue;
            }

            const uint16_t glyph =
                static_cast<uint16_t>(glyphId);

            if (!inRange)
            {
                rangeFirst = glyph;
                rangeLast = glyph;
                rangeValue = value;
                inRange = true;
            }
            else if (value == rangeValue &&
                glyphId == uint32_t(rangeLast) + 1u)
            {
                rangeLast = glyph;
            }
            else
            {
                ranges.push_back({
                    rangeFirst,
                    rangeLast,
                    rangeValue,
                    0
                    });

                rangeFirst = glyph;
                rangeLast = glyph;
                rangeValue = value;
            }
        }

        if (inRange)
        {
            ranges.push_back({
                rangeFirst,
                rangeLast,
                rangeValue,
                0
                });
        }

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.glyphClassRanges.size() > max32 ||
            ir.glyphClassMaps.size() > max32 ||
            ranges.size() > max32)
        {
            return false;
        }

        const uint32_t rangeOffset =
            static_cast<uint32_t>(
                ir.glyphClassRanges.size());

        const uint32_t rangeCount =
            static_cast<uint32_t>(
                ranges.size());

        if (uint64_t(rangeOffset) + rangeCount > max32)
            return false;

        result =
            static_cast<OpenTypeShapingIRGlyphClassMapId>(
                ir.glyphClassMaps.size());

        ir.glyphClassRanges.insert(
            ir.glyphClassRanges.end(),
            ranges.begin(),
            ranges.end());

        ir.glyphClassMaps.push_back({
            rangeOffset,
            rangeCount
            });

        return true;
    }


    // ========================================================================
    // GDEF / LookupFlag normalization
    //
    // These helpers intentionally mirror the already-proven GSUB compiler
    // behavior while keeping the GPOS compiler independently includable.
    // ========================================================================

    static inline bool compileOpenTypeGposIRGdefGlyphClasses(
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        bool requireClassDef)
    {
        if (ir.gdefGlyphClasses.size() ==
            kOpenTypeShapingIRGlyphDomainSize)
        {
            return true;
        }

        if (!ir.gdefGlyphClasses.empty())
            return false;

        if (!gdef)
            return !requireClassDef;

        const OpenTypeClassDefView classes =
            gdef.glyphClassDef();

        if (requireClassDef && !classes)
            return false;

        std::vector<uint16_t> values(
            kOpenTypeShapingIRGlyphDomainSize, 0);

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t value = 0;

            if (!gdef.glyphClass(glyphId, value))
                return false;

            values[glyphId] = value;
        }

        ir.gdefGlyphClasses = std::move(values);
        return true;
    }


    static inline bool compileOpenTypeGposIRGdefMarkAttachClasses(
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir)
    {
        if (ir.gdefMarkAttachClasses.size() ==
            kOpenTypeShapingIRGlyphDomainSize)
        {
            return true;
        }

        if (!ir.gdefMarkAttachClasses.empty() || !gdef)
            return false;

        const OpenTypeClassDefView classes =
            gdef.markAttachClassDef();

        if (!classes)
            return false;

        std::vector<uint16_t> values(
            kOpenTypeShapingIRGlyphDomainSize, 0);

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t value = 0;

            if (!gdef.markAttachClass(glyphId, value))
                return false;

            values[glyphId] = value;
        }

        ir.gdefMarkAttachClasses = std::move(values);
        return true;
    }


    static inline bool compileOpenTypeGposIRMarkFilteringSet(
        const OpenTypeGdefView& gdef, uint16_t setIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!gdef)
            return false;

        const OpenTypeMarkGlyphSetsView sets =
            gdef.markGlyphSetsDef();

        if (!sets || setIndex >= sets.size())
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            bool member = false;

            if (!sets.contains(
                setIndex, glyphId, member))
            {
                return false;
            }

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst =
                        static_cast<uint16_t>(glyphId);

                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast =
                        static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({
                    rangeFirst,
                    rangeLast
                    });

                inRange = false;
            }
        }

        if (inRange)
        {
            ranges.push_back({
                rangeFirst,
                rangeLast
                });
        }

        return appendOpenTypeGposIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeGposIRLookupFilter(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupFilter& result)
    {
        result = {};

        if (!lookup)
            return false;

        const uint16_t lookupFlag =
            lookup.lookupFlag();

        if ((lookupFlag &
            kOpenTypeLookupFlagReservedMask) != 0)
        {
            return false;
        }

        if ((lookupFlag &
            kOpenTypeLookupFlagRightToLeft) != 0)
        {
            result.flags |=
                OpenTypeShapingIRRightToLeft;
        }

        if ((lookupFlag &
            kOpenTypeLookupFlagIgnoreBaseGlyphs) != 0)
        {
            result.flags |=
                OpenTypeShapingIRIgnoreBaseGlyphs;
        }

        if ((lookupFlag &
            kOpenTypeLookupFlagIgnoreLigatures) != 0)
        {
            result.flags |=
                OpenTypeShapingIRIgnoreLigatures;
        }

        if ((lookupFlag &
            kOpenTypeLookupFlagIgnoreMarks) != 0)
        {
            result.flags |=
                OpenTypeShapingIRIgnoreMarks;
        }

        const bool ignoreBase =
            (lookupFlag &
                kOpenTypeLookupFlagIgnoreBaseGlyphs) != 0;

        const bool ignoreLigatures =
            (lookupFlag &
                kOpenTypeLookupFlagIgnoreLigatures) != 0;

        const bool ignoreMarks =
            (lookupFlag &
                kOpenTypeLookupFlagIgnoreMarks) != 0;

        const bool useMarkFilteringSet =
            (lookupFlag &
                kOpenTypeLookupFlagUseMarkFilteringSet) != 0;

        const uint16_t markAttachmentType =
            static_cast<uint16_t>(
                (lookupFlag &
                    kOpenTypeLookupFlagMarkAttachmentTypeMask) >>
                8);

        result.markAttachmentType =
            static_cast<uint8_t>(
                markAttachmentType);

        const bool needsGlyphClass =
            ignoreBase ||
            ignoreLigatures ||
            ignoreMarks ||
            useMarkFilteringSet ||
            markAttachmentType != 0;

        if (needsGlyphClass)
        {
            if (!gdef ||
                !compileOpenTypeGposIRGdefGlyphClasses(
                    gdef, ir, true))
            {
                return false;
            }
        }

        if (useMarkFilteringSet)
        {
            uint16_t setIndex = 0;

            if (!lookup.markFilteringSet(setIndex))
                return false;

            if (!compileOpenTypeGposIRMarkFilteringSet(
                gdef, setIndex, ir,
                result.markFilteringSet))
            {
                return false;
            }
        }

        // IgnoreMarks supersedes both of these.
        // MarkFilteringSet supersedes MarkAttachmentType.
        if (!ignoreMarks &&
            !useMarkFilteringSet &&
            markAttachmentType != 0)
        {
            if (!compileOpenTypeGposIRGdefMarkAttachClasses(
                gdef, ir))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // ValueRecord -> semantic positioning adjustment
    //
    // Device / VariationIndex offsets are intentionally not carried into the
    // initial IR because the current raw executor does not apply them either.
    // ========================================================================

    [[nodiscard]] static inline OpenTypeShapingIRPositionAdjustment
        compileOpenTypeGposIRPositionAdjustment(
            const OpenTypeGposValueRecord& value) noexcept
    {
        OpenTypeShapingIRPositionAdjustment result{};

        result.offsetX =
            static_cast<int32_t>(value.xPlacement);

        result.offsetY =
            static_cast<int32_t>(value.yPlacement);

        result.advanceX =
            static_cast<int32_t>(value.xAdvance);

        result.advanceY =
            static_cast<int32_t>(value.yAdvance);

        return result;
    }


    // ========================================================================
    // GPOS Type 1 - SinglePos
    // ========================================================================

    static inline bool compileOpenTypeGposSingleSubtable(
        const ByteSpan& data,
        std::vector<OpenTypeShapingIRGposSinglePair>& pairs)
    {
        pairs.clear();

        const OpenTypeGposSinglePosView single(data);

        if (!single)
            return false;

        const uint16_t format =
            single.format();

        if (format != 1 && format != 2)
            return false;

        const OpenTypeCoverageView coverage =
            single.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            OpenTypeGposValueRecord value{};

            if (!single.valueRecordForCoverageIndex(
                coverageIndex, value))
            {
                return false;
            }

            pairs.push_back({
                static_cast<uint16_t>(glyphId),
                0,
                compileOpenTypeGposIRPositionAdjustment(
                    value)
                });
        }

        return true;
    }


    static inline bool appendOpenTypeGposSingleSubtable(
        const std::vector<OpenTypeShapingIRGposSinglePair>& pairs,
        OpenTypeShapingIR& ir)
    {
        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gposSinglePairs.size() > max32 ||
            ir.gposSingleSubtables.size() > max32 ||
            pairs.size() > max32)
        {
            return false;
        }

        const uint32_t pairOffset =
            static_cast<uint32_t>(
                ir.gposSinglePairs.size());

        const uint32_t pairCount =
            static_cast<uint32_t>(
                pairs.size());

        if (uint64_t(pairOffset) + pairCount > max32)
            return false;

        ir.gposSinglePairs.insert(
            ir.gposSinglePairs.end(),
            pairs.begin(),
            pairs.end());

        ir.gposSingleSubtables.push_back({
            pairOffset,
            pairCount
            });

        return true;
    }


    static inline bool compileOpenTypeGposSingleLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 1)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldPairCount =
            ir.gposSinglePairs.size();

        const size_t oldSubtableCount =
            ir.gposSingleSubtables.size();

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldGdefGlyphClassCount =
            ir.gdefGlyphClasses.size();

        const size_t oldGdefMarkAttachClassCount =
            ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(
                    oldLookupCount);

                ir.gposSinglePairs.resize(
                    oldPairCount);

                ir.gposSingleSubtables.resize(
                    oldSubtableCount);

                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.gdefGlyphClasses.resize(
                    oldGdefGlyphClassCount);

                ir.gdefMarkAttachClasses.resize(
                    oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldSubtableCount);

        std::vector<OpenTypeShapingIRGposSinglePair>
            pairs;

        for (uint16_t i = 0;
            i < subtableCount;
            ++i)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, i);

            if (!data ||
                !compileOpenTypeGposSingleSubtable(
                    data, pairs) ||
                !appendOpenTypeGposSingleSubtable(
                    pairs, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GposSingle;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter =
            filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGposSingleLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposSingleLookup(
            lookup, gdef, ir, result);
    }


    // ========================================================================
    // GPOS Type 2 - PairPos Format 1
    // ========================================================================

    static inline bool compileOpenTypeGposPairExplicitSubtable(
        const OpenTypeGposPairPosView& pair,
        std::vector<OpenTypeShapingIRGposPairExplicit>& pairs)
    {
        pairs.clear();

        if (!pair || pair.format() != 1)
            return false;

        const OpenTypeCoverageView coverage =
            pair.coverage();

        if (!coverage)
            return false;

        for (uint32_t firstGlyph = 0;
            firstGlyph <= 0xFFFFu;
            ++firstGlyph)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(
                firstGlyph, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >=
                pair.pairSetCount())
            {
                return false;
            }

            const OpenTypeGposPairSetView set =
                pair.pairSet(coverageIndex);

            if (!set)
                return false;

            for (uint16_t i = 0;
                i < set.size();
                ++i)
            {
                uint16_t secondGlyph = 0;
                OpenTypeGposValueRecord firstValue{};
                OpenTypeGposValueRecord secondValue{};

                if (!set.secondGlyph(
                    i, secondGlyph) ||
                    !set.valueRecords(
                        i,
                        firstValue,
                        secondValue))
                {
                    return false;
                }

                pairs.push_back({
                    static_cast<uint16_t>(
                        firstGlyph),
                    secondGlyph,
                    compileOpenTypeGposIRPositionAdjustment(
                        firstValue),
                    compileOpenTypeGposIRPositionAdjustment(
                        secondValue)
                    });
            }
        }

        return true;
    }


    static inline bool appendOpenTypeGposPairExplicitSubtable(
        const std::vector<OpenTypeShapingIRGposPairExplicit>& pairs,
        OpenTypeShapingIR& ir, uint32_t& result)
    {
        result = kOpenTypeShapingIRInvalid;

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gposPairExplicitPairs.size() > max32 ||
            ir.gposPairExplicitSubtables.size() > max32 ||
            pairs.size() > max32)
        {
            return false;
        }

        const uint32_t pairOffset =
            static_cast<uint32_t>(
                ir.gposPairExplicitPairs.size());

        const uint32_t pairCount =
            static_cast<uint32_t>(
                pairs.size());

        if (uint64_t(pairOffset) + pairCount > max32)
            return false;

        result =
            static_cast<uint32_t>(
                ir.gposPairExplicitSubtables.size());

        ir.gposPairExplicitPairs.insert(
            ir.gposPairExplicitPairs.end(),
            pairs.begin(),
            pairs.end());

        ir.gposPairExplicitSubtables.push_back({
            pairOffset,
            pairCount
            });

        return true;
    }


    // ========================================================================
    // GPOS Type 2 - PairPos Format 2
    // ========================================================================

    static inline bool compileOpenTypeGposPairClassSubtable(
        const OpenTypeGposPairPosView& pair,
        OpenTypeShapingIR& ir, uint32_t& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!pair || pair.format() != 2)
            return false;

        const OpenTypeCoverageView coverage =
            pair.coverage();

        const OpenTypeClassDefView firstClassDef =
            pair.classDef1();

        const OpenTypeClassDefView secondClassDef =
            pair.classDef2();

        if (!coverage ||
            !firstClassDef ||
            !secondClassDef)
        {
            return false;
        }

        const uint16_t firstClassCount =
            pair.class1Count();

        const uint16_t secondClassCount =
            pair.class2Count();

        if (firstClassCount == 0 ||
            secondClassCount == 0)
        {
            return false;
        }

        const uint64_t valueCount64 =
            uint64_t(firstClassCount) *
            uint64_t(secondClassCount);

        if (valueCount64 >
            std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldClassRangeCount =
            ir.glyphClassRanges.size();

        const size_t oldClassMapCount =
            ir.glyphClassMaps.size();

        const size_t oldValueCount =
            ir.gposPairClassValues.size();

        const size_t oldSubtableCount =
            ir.gposPairClassSubtables.size();

        auto rollback = [&]() noexcept
            {
                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.glyphClassRanges.resize(
                    oldClassRangeCount);

                ir.glyphClassMaps.resize(
                    oldClassMapCount);

                ir.gposPairClassValues.resize(
                    oldValueCount);

                ir.gposPairClassSubtables.resize(
                    oldSubtableCount);
            };

        OpenTypeShapingIRGlyphSetId firstCoverage =
            kOpenTypeShapingIRInvalid;

        OpenTypeShapingIRGlyphClassMapId firstClassMap =
            kOpenTypeShapingIRInvalid;

        OpenTypeShapingIRGlyphClassMapId secondClassMap =
            kOpenTypeShapingIRInvalid;

        if (!compileOpenTypeGposIRCoverageGlyphSet(
            coverage, ir, firstCoverage) ||
            !compileOpenTypeGposIRGlyphClassMap(
                firstClassDef, ir, firstClassMap) ||
            !compileOpenTypeGposIRGlyphClassMap(
                secondClassDef, ir, secondClassMap))
        {
            rollback();
            return false;
        }

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gposPairClassValues.size() > max32 ||
            ir.gposPairClassSubtables.size() > max32)
        {
            rollback();
            return false;
        }

        const uint32_t valueOffset =
            static_cast<uint32_t>(
                ir.gposPairClassValues.size());

        if (uint64_t(valueOffset) +
            valueCount64 > max32)
        {
            rollback();
            return false;
        }

        ir.gposPairClassValues.reserve(
            ir.gposPairClassValues.size() +
            static_cast<size_t>(valueCount64));

        for (uint16_t class1 = 0;
            class1 < firstClassCount;
            ++class1)
        {
            for (uint16_t class2 = 0;
                class2 < secondClassCount;
                ++class2)
            {
                OpenTypeGposValueRecord firstValue{};
                OpenTypeGposValueRecord secondValue{};

                if (!pair.classValueRecords(
                    class1,
                    class2,
                    firstValue,
                    secondValue))
                {
                    rollback();
                    return false;
                }

                ir.gposPairClassValues.push_back({
                    compileOpenTypeGposIRPositionAdjustment(
                        firstValue),
                    compileOpenTypeGposIRPositionAdjustment(
                        secondValue)
                    });
            }
        }

        result =
            static_cast<uint32_t>(
                ir.gposPairClassSubtables.size());

        ir.gposPairClassSubtables.push_back({
            firstCoverage,
            firstClassMap,
            secondClassMap,
            valueOffset,
            firstClassCount,
            secondClassCount
            });

        return true;
    }


    // ========================================================================
    // GPOS Type 2 - complete PairPos lookup
    // ========================================================================

    static inline bool compileOpenTypeGposPairLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 2)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldClassRangeCount =
            ir.glyphClassRanges.size();

        const size_t oldClassMapCount =
            ir.glyphClassMaps.size();

        const size_t oldGdefGlyphClassCount =
            ir.gdefGlyphClasses.size();

        const size_t oldGdefMarkAttachClassCount =
            ir.gdefMarkAttachClasses.size();

        const size_t oldExplicitPairCount =
            ir.gposPairExplicitPairs.size();

        const size_t oldExplicitSubtableCount =
            ir.gposPairExplicitSubtables.size();

        const size_t oldClassValueCount =
            ir.gposPairClassValues.size();

        const size_t oldClassSubtableCount =
            ir.gposPairClassSubtables.size();

        const size_t oldPairSubtableCount =
            ir.gposPairSubtables.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(
                    oldLookupCount);

                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.glyphClassRanges.resize(
                    oldClassRangeCount);

                ir.glyphClassMaps.resize(
                    oldClassMapCount);

                ir.gdefGlyphClasses.resize(
                    oldGdefGlyphClassCount);

                ir.gdefMarkAttachClasses.resize(
                    oldGdefMarkAttachClassCount);

                ir.gposPairExplicitPairs.resize(
                    oldExplicitPairCount);

                ir.gposPairExplicitSubtables.resize(
                    oldExplicitSubtableCount);

                ir.gposPairClassValues.resize(
                    oldClassValueCount);

                ir.gposPairClassSubtables.resize(
                    oldClassSubtableCount);

                ir.gposPairSubtables.resize(
                    oldPairSubtableCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldPairSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldPairSubtableCount);

        std::vector<OpenTypeShapingIRGposPairExplicit>
            explicitPairs;

        for (uint16_t i = 0;
            i < subtableCount;
            ++i)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, i);

            if (!data)
            {
                rollback();
                return false;
            }

            const OpenTypeGposPairPosView pair(data);

            if (!pair)
            {
                rollback();
                return false;
            }

            OpenTypeShapingIRGposPairSubtable compiled{};

            compiled.secondParticipates =
                pair.valueFormat2() != 0 ? 1u : 0u;

            if (pair.format() == 1)
            {
                if (!compileOpenTypeGposPairExplicitSubtable(
                    pair, explicitPairs))
                {
                    rollback();
                    return false;
                }

                uint32_t payloadIndex =
                    kOpenTypeShapingIRInvalid;

                if (!appendOpenTypeGposPairExplicitSubtable(
                    explicitPairs, ir, payloadIndex))
                {
                    rollback();
                    return false;
                }

                compiled.kind =
                    OpenTypeShapingIRGposPairKind::Explicit;

                compiled.payloadIndex =
                    payloadIndex;
            }
            else if (pair.format() == 2)
            {
                uint32_t payloadIndex =
                    kOpenTypeShapingIRInvalid;

                if (!compileOpenTypeGposPairClassSubtable(
                    pair, ir, payloadIndex))
                {
                    rollback();
                    return false;
                }

                compiled.kind =
                    OpenTypeShapingIRGposPairKind::Class;

                compiled.payloadIndex =
                    payloadIndex;
            }
            else
            {
                rollback();
                return false;
            }

            ir.gposPairSubtables.push_back(
                compiled);
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GposPair;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter =
            filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGposPairLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposPairLookup(
            lookup, gdef, ir, result);
    }


    // ========================================================================
    // GPOS Type 3 - CursivePos
    //
    // Compile Coverage-indexed EntryExitRecords into sorted semantic records:
    //
    //     glyph -> optional entry anchor + optional exit anchor
    //
    // OpenType Coverage indexing, anchor offsets and Anchor table formats do
    // not survive compilation. Device / VariationIndex adjustments carried by
    // Anchor Format 3 are deliberately not represented yet, matching the
    // current raw execution path.
    // ========================================================================

    static inline bool compileOpenTypeGposCursiveSubtable(
        const ByteSpan& data,
        std::vector<OpenTypeShapingIRGposCursiveRecord>& records)
    {
        records.clear();

        const OpenTypeGposCursivePosView cursive(data);

        if (!cursive || cursive.format() != 1)
            return false;

        const OpenTypeCoverageView coverage =
            cursive.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= cursive.entryExitCount())
                return false;

            OpenTypeShapingIRGposCursiveRecord record{};
            record.glyph = static_cast<uint16_t>(glyphId);

            if (cursive.hasEntryAnchor(coverageIndex))
            {
                const OpenTypeGposAnchorView anchor =
                    cursive.entryAnchor(coverageIndex);

                if (!anchor)
                    return false;

                record.hasEntry = 1;
                record.entry.x =
                    static_cast<int32_t>(anchor.xCoordinate());
                record.entry.y =
                    static_cast<int32_t>(anchor.yCoordinate());
            }

            if (cursive.hasExitAnchor(coverageIndex))
            {
                const OpenTypeGposAnchorView anchor =
                    cursive.exitAnchor(coverageIndex);

                if (!anchor)
                    return false;

                record.hasExit = 1;
                record.exit.x =
                    static_cast<int32_t>(anchor.xCoordinate());
                record.exit.y =
                    static_cast<int32_t>(anchor.yCoordinate());
            }

            records.push_back(record);
        }

        return true;
    }


    static inline bool appendOpenTypeGposCursiveSubtable(
        const std::vector<OpenTypeShapingIRGposCursiveRecord>& records,
        OpenTypeShapingIR& ir)
    {
        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gposCursiveRecords.size() > max32 ||
            ir.gposCursiveSubtables.size() > max32 ||
            records.size() > max32)
        {
            return false;
        }

        const uint32_t recordOffset =
            static_cast<uint32_t>(
                ir.gposCursiveRecords.size());

        const uint32_t recordCount =
            static_cast<uint32_t>(
                records.size());

        if (uint64_t(recordOffset) + recordCount > max32)
            return false;

        ir.gposCursiveRecords.insert(
            ir.gposCursiveRecords.end(),
            records.begin(),
            records.end());

        ir.gposCursiveSubtables.push_back({
            recordOffset,
            recordCount
            });

        return true;
    }


    static inline bool compileOpenTypeGposCursiveLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 3)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldRecordCount =
            ir.gposCursiveRecords.size();

        const size_t oldSubtableCount =
            ir.gposCursiveSubtables.size();

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldGdefGlyphClassCount =
            ir.gdefGlyphClasses.size();

        const size_t oldGdefMarkAttachClassCount =
            ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(
                    oldLookupCount);

                ir.gposCursiveRecords.resize(
                    oldRecordCount);

                ir.gposCursiveSubtables.resize(
                    oldSubtableCount);

                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.gdefGlyphClasses.resize(
                    oldGdefGlyphClassCount);

                ir.gdefMarkAttachClasses.resize(
                    oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldSubtableCount);

        std::vector<OpenTypeShapingIRGposCursiveRecord>
            records;

        for (uint16_t i = 0;
            i < subtableCount;
            ++i)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, i);

            if (!data ||
                !compileOpenTypeGposCursiveSubtable(
                    data, records) ||
                !appendOpenTypeGposCursiveSubtable(
                    records, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GposCursive;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter =
            filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGposCursiveLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposCursiveLookup(
            lookup, gdef, ir, result);
    }


    // ========================================================================
    // Shared semantic anchor compilation
    //
    // Anchor table serialization does not survive compilation. The current
    // semantic IR stores only design-unit X/Y coordinates. Anchor Format 2
    // contour-point metadata and Anchor Format 3 Device/VariationIndex data
    // are deliberately not represented yet, matching the current raw GPOS
    // execution path.
    // ========================================================================

    static inline bool compileOpenTypeGposIRAnchor(
        const OpenTypeGposAnchorView& anchor,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRAnchorId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!anchor ||
            ir.anchors.size() >=
            uint64_t(kOpenTypeShapingIRInvalid))
        {
            return false;
        }

        result =
            static_cast<OpenTypeShapingIRAnchorId>(
                ir.anchors.size());

        ir.anchors.push_back({
            static_cast<int32_t>(anchor.xCoordinate()),
            static_cast<int32_t>(anchor.yCoordinate())
            });

        return true;
    }


    // ========================================================================
    // GPOS Type 4 - MarkBasePos
    //
    // Compile:
    //
    //     MarkCoverage + MarkArray
    //
    // into:
    //
    //     sorted mark glyph -> class + semantic anchor
    //
    // and:
    //
    //     BaseCoverage + BaseArray
    //
    // into:
    //
    //     sorted base glyphs
    //     row-major base/class AnchorId matrix
    //
    // A NULL BaseAnchor is represented by kOpenTypeShapingIRInvalid.
    // ========================================================================

    static inline bool compileOpenTypeGposMarkBaseSubtable(
        const ByteSpan& data,
        OpenTypeShapingIR& ir)
    {
        const OpenTypeGposMarkBasePosView markBase(data);

        if (!markBase || markBase.format() != 1)
            return false;

        const uint16_t markClassCount =
            markBase.markClassCount();

        if (markClassCount == 0)
            return false;

        const OpenTypeCoverageView markCoverage =
            markBase.markCoverage();

        const OpenTypeCoverageView baseCoverage =
            markBase.baseCoverage();

        const OpenTypeGposMarkArrayView marks =
            markBase.markArray();

        const OpenTypeGposBaseArrayView bases =
            markBase.baseArray();

        if (!markCoverage || !baseCoverage ||
            !marks || !bases)
        {
            return false;
        }

        const size_t oldAnchorCount =
            ir.anchors.size();

        const size_t oldMarkCount =
            ir.gposMarkRecords.size();

        const size_t oldBaseCount =
            ir.gposMarkBaseRecords.size();

        const size_t oldBaseAnchorRefCount =
            ir.gposMarkBaseAnchorRefs.size();

        const size_t oldSubtableCount =
            ir.gposMarkBaseSubtables.size();

        auto rollback = [&]() noexcept
            {
                ir.anchors.resize(
                    oldAnchorCount);

                ir.gposMarkRecords.resize(
                    oldMarkCount);

                ir.gposMarkBaseRecords.resize(
                    oldBaseCount);

                ir.gposMarkBaseAnchorRefs.resize(
                    oldBaseAnchorRefCount);

                ir.gposMarkBaseSubtables.resize(
                    oldSubtableCount);
            };

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (oldMarkCount > max32 ||
            oldBaseCount > max32 ||
            oldBaseAnchorRefCount > max32 ||
            oldSubtableCount > max32)
        {
            rollback();
            return false;
        }

        const uint32_t markOffset =
            static_cast<uint32_t>(oldMarkCount);

        uint32_t markCount = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!markCoverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >= marks.markCount())
            {
                rollback();
                return false;
            }

            uint16_t markClass = 0;

            if (!marks.markClass(
                coverageIndex, markClass) ||
                markClass >= markClassCount)
            {
                rollback();
                return false;
            }

            const OpenTypeGposAnchorView markAnchor =
                marks.markAnchor(coverageIndex);

            OpenTypeShapingIRAnchorId anchorId =
                kOpenTypeShapingIRInvalid;

            if (!markAnchor ||
                !compileOpenTypeGposIRAnchor(
                    markAnchor, ir, anchorId))
            {
                rollback();
                return false;
            }

            if (ir.gposMarkRecords.size() >= max32)
            {
                rollback();
                return false;
            }

            ir.gposMarkRecords.push_back({
                static_cast<uint16_t>(glyphId),
                markClass,
                anchorId
                });

            ++markCount;
        }

        const uint32_t baseOffset =
            static_cast<uint32_t>(
                ir.gposMarkBaseRecords.size());

        const uint32_t baseAnchorOffset =
            static_cast<uint32_t>(
                ir.gposMarkBaseAnchorRefs.size());

        uint32_t baseCount = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!baseCoverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >= bases.baseCount())
            {
                rollback();
                return false;
            }

            if (ir.gposMarkBaseRecords.size() >= max32)
            {
                rollback();
                return false;
            }

            ir.gposMarkBaseRecords.push_back({
                static_cast<uint16_t>(glyphId),
                0
                });

            ++baseCount;

            for (uint16_t markClass = 0;
                markClass < markClassCount;
                ++markClass)
            {
                if (ir.gposMarkBaseAnchorRefs.size() >= max32)
                {
                    rollback();
                    return false;
                }

                if (!bases.hasBaseAnchor(
                    coverageIndex, markClass))
                {
                    ir.gposMarkBaseAnchorRefs.push_back(
                        kOpenTypeShapingIRInvalid);

                    continue;
                }

                const OpenTypeGposAnchorView baseAnchor =
                    bases.baseAnchor(
                        coverageIndex, markClass);

                OpenTypeShapingIRAnchorId anchorId =
                    kOpenTypeShapingIRInvalid;

                if (!baseAnchor ||
                    !compileOpenTypeGposIRAnchor(
                        baseAnchor, ir, anchorId))
                {
                    rollback();
                    return false;
                }

                ir.gposMarkBaseAnchorRefs.push_back(
                    anchorId);
            }
        }

        const uint64_t matrixCount =
            uint64_t(baseCount) *
            uint64_t(markClassCount);

        if (matrixCount !=
            ir.gposMarkBaseAnchorRefs.size() -
            oldBaseAnchorRefCount)
        {
            rollback();
            return false;
        }

        ir.gposMarkBaseSubtables.push_back({
            markOffset,
            markCount,
            baseOffset,
            baseCount,
            baseAnchorOffset,
            markClassCount,
            0
            });

        return true;
    }


    static inline bool compileOpenTypeGposMarkBaseLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 4)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldAnchorCount =
            ir.anchors.size();

        const size_t oldMarkCount =
            ir.gposMarkRecords.size();

        const size_t oldBaseCount =
            ir.gposMarkBaseRecords.size();

        const size_t oldBaseAnchorRefCount =
            ir.gposMarkBaseAnchorRefs.size();

        const size_t oldSubtableCount =
            ir.gposMarkBaseSubtables.size();

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldGdefGlyphClassCount =
            ir.gdefGlyphClasses.size();

        const size_t oldGdefMarkAttachClassCount =
            ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(
                    oldLookupCount);

                ir.anchors.resize(
                    oldAnchorCount);

                ir.gposMarkRecords.resize(
                    oldMarkCount);

                ir.gposMarkBaseRecords.resize(
                    oldBaseCount);

                ir.gposMarkBaseAnchorRefs.resize(
                    oldBaseAnchorRefCount);

                ir.gposMarkBaseSubtables.resize(
                    oldSubtableCount);

                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.gdefGlyphClasses.resize(
                    oldGdefGlyphClassCount);

                ir.gdefMarkAttachClasses.resize(
                    oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        // MarkBase has intrinsic GDEF traversal semantics independent of
        // LookupFlag: preceding glyphs classified as marks must be skipped.
        // If GDEF is absent, an empty table semantically means every glyph is
        // class zero / non-mark.
        if (!compileOpenTypeGposIRGdefGlyphClasses(
            gdef, ir, false))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldSubtableCount);

        for (uint16_t i = 0;
            i < subtableCount;
            ++i)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, i);

            if (!data ||
                !compileOpenTypeGposMarkBaseSubtable(
                    data, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GposMarkBase;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter =
            filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGposMarkBaseLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposMarkBaseLookup(
            lookup, gdef, ir, result);
    }


    // ========================================================================
    // GPOS Type 5 - MarkLigaturePos
    //
    // Compile:
    //
    //     MarkCoverage + MarkArray
    //
    // into:
    //
    //     sorted mark glyph -> class + semantic anchor
    //
    // and:
    //
    //     LigatureCoverage + LigatureArray + LigatureAttach
    //
    // into:
    //
    //     sorted ligature glyph -> component count + component/class matrix
    //
    // The component/class matrix is row-major:
    //
    //     componentIndex * markClassCount + markClass
    //
    // A NULL component anchor is represented by
    // kOpenTypeShapingIRInvalid.
    // ========================================================================

    static inline bool compileOpenTypeGposMarkLigatureSubtable(
        const ByteSpan& data,
        OpenTypeShapingIR& ir)
    {
        const OpenTypeGposMarkLigaturePosView markLigature(data);

        if (!markLigature ||
            markLigature.format() != 1)
        {
            return false;
        }

        const uint16_t markClassCount =
            markLigature.markClassCount();

        if (markClassCount == 0)
            return false;

        const OpenTypeCoverageView markCoverage =
            markLigature.markCoverage();

        const OpenTypeCoverageView ligatureCoverage =
            markLigature.ligatureCoverage();

        const OpenTypeGposMarkArrayView marks =
            markLigature.markArray();

        const OpenTypeGposLigatureArrayView ligatures =
            markLigature.ligatureArray();

        if (!markCoverage || !ligatureCoverage ||
            !marks || !ligatures)
        {
            return false;
        }

        const size_t oldAnchorCount =
            ir.anchors.size();

        const size_t oldMarkCount =
            ir.gposMarkRecords.size();

        const size_t oldLigatureCount =
            ir.gposMarkLigatureRecords.size();

        const size_t oldAnchorRefCount =
            ir.gposMarkLigatureAnchorRefs.size();

        const size_t oldSubtableCount =
            ir.gposMarkLigatureSubtables.size();

        auto rollback = [&]() noexcept
            {
                ir.anchors.resize(
                    oldAnchorCount);

                ir.gposMarkRecords.resize(
                    oldMarkCount);

                ir.gposMarkLigatureRecords.resize(
                    oldLigatureCount);

                ir.gposMarkLigatureAnchorRefs.resize(
                    oldAnchorRefCount);

                ir.gposMarkLigatureSubtables.resize(
                    oldSubtableCount);
            };

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (oldMarkCount > max32 ||
            oldLigatureCount > max32 ||
            oldAnchorRefCount > max32 ||
            oldSubtableCount > max32)
        {
            rollback();
            return false;
        }

        const uint32_t markOffset =
            static_cast<uint32_t>(
                oldMarkCount);

        uint32_t markCount = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!markCoverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >=
                marks.markCount())
            {
                rollback();
                return false;
            }

            uint16_t markClass = 0;

            if (!marks.markClass(
                coverageIndex, markClass) ||
                markClass >= markClassCount)
            {
                rollback();
                return false;
            }

            const OpenTypeGposAnchorView markAnchor =
                marks.markAnchor(
                    coverageIndex);

            OpenTypeShapingIRAnchorId anchorId =
                kOpenTypeShapingIRInvalid;

            if (!markAnchor ||
                !compileOpenTypeGposIRAnchor(
                    markAnchor, ir, anchorId))
            {
                rollback();
                return false;
            }

            if (ir.gposMarkRecords.size() >= max32)
            {
                rollback();
                return false;
            }

            ir.gposMarkRecords.push_back({
                static_cast<uint16_t>(glyphId),
                markClass,
                anchorId
                });

            ++markCount;
        }

        const uint32_t ligatureOffset =
            static_cast<uint32_t>(
                ir.gposMarkLigatureRecords.size());

        uint32_t ligatureCount = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!ligatureCoverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >=
                ligatures.ligatureCount())
            {
                rollback();
                return false;
            }

            const OpenTypeGposLigatureAttachView attach =
                ligatures.ligatureAttach(
                    coverageIndex);

            if (!attach)
            {
                rollback();
                return false;
            }

            const uint16_t componentCount =
                attach.componentCount();

            if (componentCount == 0)
            {
                rollback();
                return false;
            }

            if (ir.gposMarkLigatureAnchorRefs.size() > max32)
            {
                rollback();
                return false;
            }

            const uint32_t componentAnchorOffset =
                static_cast<uint32_t>(
                    ir.gposMarkLigatureAnchorRefs.size());

            const uint64_t anchorCellCount =
                uint64_t(componentCount) *
                uint64_t(markClassCount);

            if (anchorCellCount >
                max32 -
                ir.gposMarkLigatureAnchorRefs.size())
            {
                rollback();
                return false;
            }

            for (uint16_t componentIndex = 0;
                componentIndex < componentCount;
                ++componentIndex)
            {
                for (uint16_t markClass = 0;
                    markClass < markClassCount;
                    ++markClass)
                {
                    if (!attach.hasAnchor(
                        componentIndex, markClass))
                    {
                        ir.gposMarkLigatureAnchorRefs.push_back(
                            kOpenTypeShapingIRInvalid);

                        continue;
                    }

                    const OpenTypeGposAnchorView anchor =
                        attach.anchor(
                            componentIndex, markClass);

                    OpenTypeShapingIRAnchorId anchorId =
                        kOpenTypeShapingIRInvalid;

                    if (!anchor ||
                        !compileOpenTypeGposIRAnchor(
                            anchor, ir, anchorId))
                    {
                        rollback();
                        return false;
                    }

                    ir.gposMarkLigatureAnchorRefs.push_back(
                        anchorId);
                }
            }

            if (ir.gposMarkLigatureRecords.size() >= max32)
            {
                rollback();
                return false;
            }

            ir.gposMarkLigatureRecords.push_back({
                static_cast<uint16_t>(glyphId),
                componentCount,
                componentAnchorOffset
                });

            ++ligatureCount;
        }

        ir.gposMarkLigatureSubtables.push_back({
            markOffset,
            markCount,
            ligatureOffset,
            ligatureCount,
            markClassCount,
            0
            });

        return true;
    }


    static inline bool compileOpenTypeGposMarkLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 5)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldAnchorCount =
            ir.anchors.size();

        const size_t oldMarkCount =
            ir.gposMarkRecords.size();

        const size_t oldLigatureCount =
            ir.gposMarkLigatureRecords.size();

        const size_t oldAnchorRefCount =
            ir.gposMarkLigatureAnchorRefs.size();

        const size_t oldSubtableCount =
            ir.gposMarkLigatureSubtables.size();

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldGdefGlyphClassCount =
            ir.gdefGlyphClasses.size();

        const size_t oldGdefMarkAttachClassCount =
            ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(
                    oldLookupCount);

                ir.anchors.resize(
                    oldAnchorCount);

                ir.gposMarkRecords.resize(
                    oldMarkCount);

                ir.gposMarkLigatureRecords.resize(
                    oldLigatureCount);

                ir.gposMarkLigatureAnchorRefs.resize(
                    oldAnchorRefCount);

                ir.gposMarkLigatureSubtables.resize(
                    oldSubtableCount);

                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.gdefGlyphClasses.resize(
                    oldGdefGlyphClassCount);

                ir.gdefMarkAttachClasses.resize(
                    oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        // MarkLigature has intrinsic GDEF traversal semantics independent of
        // LookupFlag: preceding glyphs classified as marks must be skipped.
        // If GDEF is absent, an empty table semantically means every glyph is
        // class zero / non-mark.
        if (!compileOpenTypeGposIRGdefGlyphClasses(
            gdef, ir, false))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldSubtableCount);

        for (uint16_t i = 0;
            i < subtableCount;
            ++i)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, i);

            if (!data ||
                !compileOpenTypeGposMarkLigatureSubtable(
                    data, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GposMarkLigature;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter =
            filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGposMarkLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposMarkLigatureLookup(
            lookup, gdef, ir, result);
    }


    // ========================================================================
    // GPOS Type 6 - MarkMarkPos
    //
    // Compile:
    //
    //     Mark1Coverage + Mark1Array
    //
    // into:
    //
    //     sorted Mark1 glyph -> class + semantic anchor
    //
    // and:
    //
    //     Mark2Coverage + Mark2Array
    //
    // into:
    //
    //     sorted Mark2 glyphs
    //     row-major Mark2/class AnchorId matrix
    //
    // The Mark2 anchor matrix is:
    //
    //     localMark2Index * markClassCount + markClass
    //
    // A NULL Mark2 anchor is represented by
    // kOpenTypeShapingIRInvalid.
    // ========================================================================

    static inline bool compileOpenTypeGposMarkMarkSubtable(
        const ByteSpan& data,
        OpenTypeShapingIR& ir)
    {
        const OpenTypeGposMarkMarkPosView markMark(data);

        if (!markMark ||
            markMark.format() != 1)
        {
            return false;
        }

        const uint16_t markClassCount =
            markMark.markClassCount();

        if (markClassCount == 0)
            return false;

        const OpenTypeCoverageView mark1Coverage =
            markMark.mark1Coverage();

        const OpenTypeCoverageView mark2Coverage =
            markMark.mark2Coverage();

        const OpenTypeGposMarkArrayView mark1Array =
            markMark.mark1Array();

        const OpenTypeGposMark2ArrayView mark2Array =
            markMark.mark2Array();

        if (!mark1Coverage || !mark2Coverage ||
            !mark1Array || !mark2Array)
        {
            return false;
        }

        const size_t oldAnchorCount =
            ir.anchors.size();

        const size_t oldMark1Count =
            ir.gposMarkRecords.size();

        const size_t oldMark2Count =
            ir.gposMark2Records.size();

        const size_t oldMark2AnchorRefCount =
            ir.gposMark2AnchorRefs.size();

        const size_t oldSubtableCount =
            ir.gposMarkMarkSubtables.size();

        auto rollback = [&]() noexcept
            {
                ir.anchors.resize(
                    oldAnchorCount);

                ir.gposMarkRecords.resize(
                    oldMark1Count);

                ir.gposMark2Records.resize(
                    oldMark2Count);

                ir.gposMark2AnchorRefs.resize(
                    oldMark2AnchorRefCount);

                ir.gposMarkMarkSubtables.resize(
                    oldSubtableCount);
            };

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (oldMark1Count > max32 ||
            oldMark2Count > max32 ||
            oldMark2AnchorRefCount > max32 ||
            oldSubtableCount > max32)
        {
            rollback();
            return false;
        }

        const uint32_t mark1Offset =
            static_cast<uint32_t>(
                oldMark1Count);

        uint32_t mark1Count = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!mark1Coverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >=
                mark1Array.markCount())
            {
                rollback();
                return false;
            }

            uint16_t markClass = 0;

            if (!mark1Array.markClass(
                coverageIndex, markClass) ||
                markClass >= markClassCount)
            {
                rollback();
                return false;
            }

            const OpenTypeGposAnchorView mark1Anchor =
                mark1Array.markAnchor(
                    coverageIndex);

            OpenTypeShapingIRAnchorId anchorId =
                kOpenTypeShapingIRInvalid;

            if (!mark1Anchor ||
                !compileOpenTypeGposIRAnchor(
                    mark1Anchor, ir, anchorId))
            {
                rollback();
                return false;
            }

            if (ir.gposMarkRecords.size() >= max32)
            {
                rollback();
                return false;
            }

            ir.gposMarkRecords.push_back({
                static_cast<uint16_t>(glyphId),
                markClass,
                anchorId
                });

            ++mark1Count;
        }

        const uint32_t mark2Offset =
            static_cast<uint32_t>(
                ir.gposMark2Records.size());

        const uint32_t mark2AnchorOffset =
            static_cast<uint32_t>(
                ir.gposMark2AnchorRefs.size());

        uint32_t mark2Count = 0;

        for (uint32_t glyphId = 0;
            glyphId <= 0xFFFFu;
            ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!mark2Coverage.find(
                glyphId, coverageIndex))
            {
                continue;
            }

            if (coverageIndex >=
                mark2Array.mark2Count())
            {
                rollback();
                return false;
            }

            if (ir.gposMark2Records.size() >= max32)
            {
                rollback();
                return false;
            }

            ir.gposMark2Records.push_back({
                static_cast<uint16_t>(glyphId),
                0
                });

            ++mark2Count;

            for (uint16_t markClass = 0;
                markClass < markClassCount;
                ++markClass)
            {
                if (ir.gposMark2AnchorRefs.size() >= max32)
                {
                    rollback();
                    return false;
                }

                if (!mark2Array.hasAnchor(
                    coverageIndex, markClass))
                {
                    ir.gposMark2AnchorRefs.push_back(
                        kOpenTypeShapingIRInvalid);

                    continue;
                }

                const OpenTypeGposAnchorView mark2Anchor =
                    mark2Array.anchor(
                        coverageIndex, markClass);

                OpenTypeShapingIRAnchorId anchorId =
                    kOpenTypeShapingIRInvalid;

                if (!mark2Anchor ||
                    !compileOpenTypeGposIRAnchor(
                        mark2Anchor, ir, anchorId))
                {
                    rollback();
                    return false;
                }

                ir.gposMark2AnchorRefs.push_back(
                    anchorId);
            }
        }

        const uint64_t matrixCount =
            uint64_t(mark2Count) *
            uint64_t(markClassCount);

        if (matrixCount !=
            ir.gposMark2AnchorRefs.size() -
            oldMark2AnchorRefCount)
        {
            rollback();
            return false;
        }

        ir.gposMarkMarkSubtables.push_back({
            mark1Offset,
            mark1Count,
            mark2Offset,
            mark2Count,
            mark2AnchorOffset,
            markClassCount,
            0
            });

        return true;
    }


    static inline bool compileOpenTypeGposMarkMarkLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 6)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldAnchorCount =
            ir.anchors.size();

        const size_t oldMark1Count =
            ir.gposMarkRecords.size();

        const size_t oldMark2Count =
            ir.gposMark2Records.size();

        const size_t oldMark2AnchorRefCount =
            ir.gposMark2AnchorRefs.size();

        const size_t oldSubtableCount =
            ir.gposMarkMarkSubtables.size();

        const size_t oldGlyphRangeCount =
            ir.glyphRanges.size();

        const size_t oldGlyphSetCount =
            ir.glyphSets.size();

        const size_t oldGdefGlyphClassCount =
            ir.gdefGlyphClasses.size();

        const size_t oldGdefMarkAttachClassCount =
            ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(
                    oldLookupCount);

                ir.anchors.resize(
                    oldAnchorCount);

                ir.gposMarkRecords.resize(
                    oldMark1Count);

                ir.gposMark2Records.resize(
                    oldMark2Count);

                ir.gposMark2AnchorRefs.resize(
                    oldMark2AnchorRefCount);

                ir.gposMarkMarkSubtables.resize(
                    oldSubtableCount);

                ir.glyphRanges.resize(
                    oldGlyphRangeCount);

                ir.glyphSets.resize(
                    oldGlyphSetCount);

                ir.gdefGlyphClasses.resize(
                    oldGdefGlyphClassCount);

                ir.gdefMarkAttachClasses.resize(
                    oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldSubtableCount);

        for (uint16_t i = 0;
            i < subtableCount;
            ++i)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, i);

            if (!data ||
                !compileOpenTypeGposMarkMarkSubtable(
                    data, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GposMarkMark;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter =
            filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGposMarkMarkLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposMarkMarkLookup(
            lookup, gdef, ir, result);
    }


    // ========================================================================
    // GPOS Type 7 - ContextPos
    //
    // Formats 1, 2, and 3 compile to:
    //
    //     ordered rules
    //         ordered input glyph sets
    //         ordered semantic lookup actions
    //
    // Raw LookupList indices do not survive compilation.
    // ========================================================================

    struct OpenTypeGposIRCompileCheckpoint
    {
        size_t glyphRangeCount{ 0 };
        size_t glyphSetCount{ 0 };
        size_t glyphClassRangeCount{ 0 };
        size_t glyphClassMapCount{ 0 };
        size_t gdefGlyphClassCount{ 0 };
        size_t gdefMarkAttachClassCount{ 0 };

        size_t lookupCount{ 0 };

        size_t singlePairCount{ 0 };
        size_t singleSubtableCount{ 0 };

        size_t pairExplicitPairCount{ 0 };
        size_t pairExplicitSubtableCount{ 0 };
        size_t pairClassValueCount{ 0 };
        size_t pairClassSubtableCount{ 0 };
        size_t pairSubtableCount{ 0 };

        size_t cursiveRecordCount{ 0 };
        size_t cursiveSubtableCount{ 0 };

        size_t anchorCount{ 0 };
        size_t markRecordCount{ 0 };

        size_t markBaseRecordCount{ 0 };
        size_t markBaseAnchorRefCount{ 0 };
        size_t markBaseSubtableCount{ 0 };

        size_t markLigatureRecordCount{ 0 };
        size_t markLigatureAnchorRefCount{ 0 };
        size_t markLigatureSubtableCount{ 0 };

        size_t mark2RecordCount{ 0 };
        size_t mark2AnchorRefCount{ 0 };
        size_t markMarkSubtableCount{ 0 };

        size_t contextInputSetCount{ 0 };
        size_t contextLookupCount{ 0 };
        size_t contextRuleCount{ 0 };
        size_t contextSubtableCount{ 0 };

        size_t chainContextBacktrackSetCount{ 0 };
        size_t chainContextInputSetCount{ 0 };
        size_t chainContextLookaheadSetCount{ 0 };
        size_t chainContextLookupCount{ 0 };
        size_t chainContextRuleCount{ 0 };
        size_t chainContextSubtableCount{ 0 };

        explicit OpenTypeGposIRCompileCheckpoint(
            const OpenTypeShapingIR& ir) noexcept
            : glyphRangeCount(ir.glyphRanges.size()),
            glyphSetCount(ir.glyphSets.size()),
            glyphClassRangeCount(ir.glyphClassRanges.size()),
            glyphClassMapCount(ir.glyphClassMaps.size()),
            gdefGlyphClassCount(ir.gdefGlyphClasses.size()),
            gdefMarkAttachClassCount(ir.gdefMarkAttachClasses.size()),
            lookupCount(ir.lookups.size()),
            singlePairCount(ir.gposSinglePairs.size()),
            singleSubtableCount(ir.gposSingleSubtables.size()),
            pairExplicitPairCount(ir.gposPairExplicitPairs.size()),
            pairExplicitSubtableCount(ir.gposPairExplicitSubtables.size()),
            pairClassValueCount(ir.gposPairClassValues.size()),
            pairClassSubtableCount(ir.gposPairClassSubtables.size()),
            pairSubtableCount(ir.gposPairSubtables.size()),
            cursiveRecordCount(ir.gposCursiveRecords.size()),
            cursiveSubtableCount(ir.gposCursiveSubtables.size()),
            anchorCount(ir.anchors.size()),
            markRecordCount(ir.gposMarkRecords.size()),
            markBaseRecordCount(ir.gposMarkBaseRecords.size()),
            markBaseAnchorRefCount(ir.gposMarkBaseAnchorRefs.size()),
            markBaseSubtableCount(ir.gposMarkBaseSubtables.size()),
            markLigatureRecordCount(ir.gposMarkLigatureRecords.size()),
            markLigatureAnchorRefCount(ir.gposMarkLigatureAnchorRefs.size()),
            markLigatureSubtableCount(ir.gposMarkLigatureSubtables.size()),
            mark2RecordCount(ir.gposMark2Records.size()),
            mark2AnchorRefCount(ir.gposMark2AnchorRefs.size()),
            markMarkSubtableCount(ir.gposMarkMarkSubtables.size()),
            contextInputSetCount(ir.gposContextInputSets.size()),
            contextLookupCount(ir.gposContextLookups.size()),
            contextRuleCount(ir.gposContextRules.size()),
            contextSubtableCount(ir.gposContextSubtables.size()),
            chainContextBacktrackSetCount(ir.gposChainContextBacktrackSets.size()),
            chainContextInputSetCount(ir.gposChainContextInputSets.size()),
            chainContextLookaheadSetCount(ir.gposChainContextLookaheadSets.size()),
            chainContextLookupCount(ir.gposChainContextLookups.size()),
            chainContextRuleCount(ir.gposChainContextRules.size()),
            chainContextSubtableCount(ir.gposChainContextSubtables.size())
        {}

        void rollback(OpenTypeShapingIR& ir) const noexcept
        {
            ir.glyphRanges.resize(glyphRangeCount);
            ir.glyphSets.resize(glyphSetCount);
            ir.glyphClassRanges.resize(glyphClassRangeCount);
            ir.glyphClassMaps.resize(glyphClassMapCount);
            ir.gdefGlyphClasses.resize(gdefGlyphClassCount);
            ir.gdefMarkAttachClasses.resize(gdefMarkAttachClassCount);

            ir.lookups.resize(lookupCount);

            ir.gposSinglePairs.resize(singlePairCount);
            ir.gposSingleSubtables.resize(singleSubtableCount);

            ir.gposPairExplicitPairs.resize(pairExplicitPairCount);
            ir.gposPairExplicitSubtables.resize(pairExplicitSubtableCount);
            ir.gposPairClassValues.resize(pairClassValueCount);
            ir.gposPairClassSubtables.resize(pairClassSubtableCount);
            ir.gposPairSubtables.resize(pairSubtableCount);

            ir.gposCursiveRecords.resize(cursiveRecordCount);
            ir.gposCursiveSubtables.resize(cursiveSubtableCount);

            ir.anchors.resize(anchorCount);
            ir.gposMarkRecords.resize(markRecordCount);

            ir.gposMarkBaseRecords.resize(markBaseRecordCount);
            ir.gposMarkBaseAnchorRefs.resize(markBaseAnchorRefCount);
            ir.gposMarkBaseSubtables.resize(markBaseSubtableCount);

            ir.gposMarkLigatureRecords.resize(markLigatureRecordCount);
            ir.gposMarkLigatureAnchorRefs.resize(markLigatureAnchorRefCount);
            ir.gposMarkLigatureSubtables.resize(markLigatureSubtableCount);

            ir.gposMark2Records.resize(mark2RecordCount);
            ir.gposMark2AnchorRefs.resize(mark2AnchorRefCount);
            ir.gposMarkMarkSubtables.resize(markMarkSubtableCount);

            ir.gposContextInputSets.resize(contextInputSetCount);
            ir.gposContextLookups.resize(contextLookupCount);
            ir.gposContextRules.resize(contextRuleCount);
            ir.gposContextSubtables.resize(contextSubtableCount);

            ir.gposChainContextBacktrackSets.resize(chainContextBacktrackSetCount);
            ir.gposChainContextInputSets.resize(chainContextInputSetCount);
            ir.gposChainContextLookaheadSets.resize(chainContextLookaheadSetCount);
            ir.gposChainContextLookups.resize(chainContextLookupCount);
            ir.gposChainContextRules.resize(chainContextRuleCount);
            ir.gposChainContextSubtables.resize(chainContextSubtableCount);
        }
    };


    struct OpenTypeGposIRContextRuleTemp
    {
        std::vector<OpenTypeShapingIRGlyphSetId> inputSets{};
        std::vector<OpenTypeSequenceLookup> sourceLookups{};
        std::vector<OpenTypeShapingIRSequenceLookup> lookups{};
    };


    struct OpenTypeGposIRContextSubtableTemp
    {
        std::vector<OpenTypeGposIRContextRuleTemp> rules{};
    };


    enum class OpenTypeGposIRLookupCompileState : uint8_t
    {
        Unseen = 0,
        Compiling,
        Done
    };


    struct OpenTypeGposIRCompilerContext
    {
        std::vector<OpenTypeGposIRLookupCompileState> states{};
        std::vector<OpenTypeShapingIRLookupId> lookupIds{};

        bool reset(size_t lookupCount)
        {
            states.assign(
                lookupCount,
                OpenTypeGposIRLookupCompileState::Unseen);

            lookupIds.assign(
                lookupCount,
                kOpenTypeShapingIRInvalid);

            return true;
        }
    };


    static inline bool compileOpenTypeGposIRLookupByIndex(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result);


    static inline bool compileOpenTypeGposContextFormat1(
        const OpenTypeGposContextPosView& pos,
        OpenTypeShapingIR& ir,
        OpenTypeGposIRContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!pos || pos.format() != 1)
            return false;

        const OpenTypeCoverageView coverage = pos.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= pos.ruleSetCount())
                continue;

            uint16_t ruleSetOffset = 0;

            if (!pos.ruleSetOffset(coverageIndex, ruleSetOffset))
                return false;

            if (ruleSetOffset == 0)
                continue;

            const OpenTypeGposContextRuleSetView set =
                pos.ruleSet(coverageIndex);

            if (!set)
                return false;

            OpenTypeShapingIRGlyphSetId firstSet =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRSingletonGlyphSet(
                static_cast<uint16_t>(glyphId), ir, firstSet))
            {
                return false;
            }

            for (uint16_t ruleIndex = 0;
                ruleIndex < set.size();
                ++ruleIndex)
            {
                const OpenTypeGposContextRuleView rule =
                    set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t glyphCount = rule.glyphCount();

                if (glyphCount == 0)
                    return false;

                OpenTypeGposIRContextRuleTemp compiled;
                compiled.inputSets.reserve(glyphCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t sequenceIndex = 1;
                    sequenceIndex < glyphCount;
                    ++sequenceIndex)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.inputGlyphId(
                        sequenceIndex - 1, expectedGlyph))
                    {
                        return false;
                    }

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGposIRSingletonGlyphSet(
                        expectedGlyph, ir, setId))
                    {
                        return false;
                    }

                    compiled.inputSets.push_back(setId);
                }

                const uint16_t lookupCount =
                    rule.sequenceLookupCount();

                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t actionIndex = 0;
                    actionIndex < lookupCount;
                    ++actionIndex)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(
                        actionIndex, action))
                    {
                        return false;
                    }

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    static inline bool compileOpenTypeGposContextFormat2(
        const OpenTypeGposContextPosView& pos,
        OpenTypeShapingIR& ir,
        OpenTypeGposIRContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!pos || pos.format() != 2)
            return false;

        const OpenTypeCoverageView coverage = pos.coverage();
        const OpenTypeClassDefView classDef = pos.classDef();

        if (!coverage || !classDef)
            return false;

        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>
            classSetCache;

        auto classGlyphSet =
            [&](uint16_t classValue,
                OpenTypeShapingIRGlyphSetId& setId) -> bool
            {
                for (const auto& entry : classSetCache)
                {
                    if (entry.first == classValue)
                    {
                        setId = entry.second;
                        return true;
                    }
                }

                if (!compileOpenTypeGposIRClassGlyphSet(
                    classDef, classValue, ir, setId))
                {
                    return false;
                }

                classSetCache.push_back({
                    classValue,
                    setId
                    });

                return true;
            };

        const uint16_t classSetCount = pos.classSetCount();

        for (uint16_t firstClass = 0;
            firstClass < classSetCount;
            ++firstClass)
        {
            uint16_t classSetOffset = 0;

            if (!pos.classSetOffset(
                firstClass, classSetOffset))
            {
                return false;
            }

            if (classSetOffset == 0)
                continue;

            OpenTypeShapingIRGlyphSetId firstSet =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRCoverageClassGlyphSet(
                coverage, classDef, firstClass, ir, firstSet))
            {
                return false;
            }

            if (openTypeGposIRGlyphSetEmpty(ir, firstSet))
                continue;

            const OpenTypeGposContextClassSetView set =
                pos.classSet(firstClass);

            if (!set)
                return false;

            for (uint16_t ruleIndex = 0;
                ruleIndex < set.size();
                ++ruleIndex)
            {
                const OpenTypeGposContextClassRuleView rule =
                    set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t glyphCount = rule.glyphCount();

                if (glyphCount == 0)
                    return false;

                OpenTypeGposIRContextRuleTemp compiled;
                compiled.inputSets.reserve(glyphCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t sequenceIndex = 1;
                    sequenceIndex < glyphCount;
                    ++sequenceIndex)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.inputClass(
                        sequenceIndex - 1,
                        expectedClass))
                    {
                        return false;
                    }

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(
                        expectedClass, setId))
                    {
                        return false;
                    }

                    compiled.inputSets.push_back(setId);
                }

                const uint16_t lookupCount =
                    rule.sequenceLookupCount();

                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t actionIndex = 0;
                    actionIndex < lookupCount;
                    ++actionIndex)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(
                        actionIndex, action))
                    {
                        return false;
                    }

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    static inline bool compileOpenTypeGposContextFormat3(
        const OpenTypeGposContextPosView& pos,
        OpenTypeShapingIR& ir,
        OpenTypeGposIRContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!pos || pos.format() != 3)
            return false;

        const uint16_t glyphCount = pos.glyphCount();

        if (glyphCount == 0)
            return false;

        OpenTypeGposIRContextRuleTemp compiled;
        compiled.inputSets.reserve(glyphCount);

        for (uint16_t sequenceIndex = 0;
            sequenceIndex < glyphCount;
            ++sequenceIndex)
        {
            const OpenTypeCoverageView coverage =
                pos.inputCoverage(sequenceIndex);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRCoverageGlyphSet(
                coverage, ir, setId))
            {
                return false;
            }

            compiled.inputSets.push_back(setId);
        }

        const uint16_t lookupCount =
            pos.sequenceLookupCount();

        compiled.sourceLookups.reserve(lookupCount);

        for (uint16_t actionIndex = 0;
            actionIndex < lookupCount;
            ++actionIndex)
        {
            OpenTypeSequenceLookup action{};

            if (!pos.sequenceLookup(
                actionIndex, action))
            {
                return false;
            }

            compiled.sourceLookups.push_back(action);
        }

        result.rules.push_back(std::move(compiled));
        return true;
    }


    static inline bool compileOpenTypeGposContextSubtable(
        const ByteSpan& data, OpenTypeShapingIR& ir,
        OpenTypeGposIRContextSubtableTemp& result)
    {
        result.rules.clear();

        const OpenTypeGposContextPosView pos(data);

        if (!pos)
            return false;

        switch (pos.format())
        {
        case 1:
            return compileOpenTypeGposContextFormat1(
                pos, ir, result);

        case 2:
            return compileOpenTypeGposContextFormat2(
                pos, ir, result);

        case 3:
            return compileOpenTypeGposContextFormat3(
                pos, ir, result);

        default:
            return false;
        }
    }


    static inline bool resolveOpenTypeGposIRContextActions(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        std::vector<OpenTypeGposIRContextSubtableTemp>& subtables)
    {
        for (OpenTypeGposIRContextSubtableTemp& subtable : subtables)
        {
            for (OpenTypeGposIRContextRuleTemp& rule : subtable.rules)
            {
                rule.lookups.clear();
                rule.lookups.reserve(rule.sourceLookups.size());

                for (const OpenTypeSequenceLookup& action :
                    rule.sourceLookups)
                {
                    OpenTypeShapingIRLookupId lookupId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGposIRLookupByIndex(
                        lookups,
                        action.lookupListIndex,
                        gdef,
                        ir,
                        context,
                        lookupId))
                    {
                        return false;
                    }

                    rule.lookups.push_back({
                        action.sequenceIndex,
                        lookupId
                        });
                }
            }
        }

        return true;
    }


    static inline bool appendOpenTypeGposContextSubtables(
        const std::vector<OpenTypeGposIRContextSubtableTemp>& subtables,
        OpenTypeShapingIR& ir, uint32_t& payloadOffset)
    {
        payloadOffset = 0;

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gposContextSubtables.size() > max32 ||
            ir.gposContextRules.size() > max32 ||
            ir.gposContextInputSets.size() > max32 ||
            ir.gposContextLookups.size() > max32 ||
            subtables.size() > max32)
        {
            return false;
        }

        payloadOffset =
            static_cast<uint32_t>(
                ir.gposContextSubtables.size());

        if (uint64_t(payloadOffset) + subtables.size() > max32)
            return false;

        for (const OpenTypeGposIRContextSubtableTemp& subtable :
            subtables)
        {
            if (ir.gposContextRules.size() > max32 ||
                subtable.rules.size() > max32)
            {
                return false;
            }

            const uint32_t ruleOffset =
                static_cast<uint32_t>(
                    ir.gposContextRules.size());

            if (uint64_t(ruleOffset) + subtable.rules.size() > max32)
                return false;

            for (const OpenTypeGposIRContextRuleTemp& rule :
                subtable.rules)
            {
                if (rule.inputSets.empty())
                    return false;

                if (ir.gposContextInputSets.size() > max32 ||
                    ir.gposContextLookups.size() > max32 ||
                    rule.inputSets.size() > max32 ||
                    rule.lookups.size() > max32)
                {
                    return false;
                }

                const uint32_t inputSetOffset =
                    static_cast<uint32_t>(
                        ir.gposContextInputSets.size());

                const uint32_t inputCount =
                    static_cast<uint32_t>(
                        rule.inputSets.size());

                const uint32_t lookupOffset =
                    static_cast<uint32_t>(
                        ir.gposContextLookups.size());

                const uint32_t lookupCount =
                    static_cast<uint32_t>(
                        rule.lookups.size());

                if (uint64_t(inputSetOffset) + inputCount > max32 ||
                    uint64_t(lookupOffset) + lookupCount > max32)
                {
                    return false;
                }

                for (OpenTypeShapingIRGlyphSetId setId :
                rule.inputSets)
                {
                    if (!ir.glyphSet(setId))
                        return false;
                }

                for (const OpenTypeShapingIRSequenceLookup& action :
                    rule.lookups)
                {
                    if (action.lookup == kOpenTypeShapingIRInvalid ||
                        action.lookup >= ir.lookups.size())
                    {
                        return false;
                    }
                }

                ir.gposContextInputSets.insert(
                    ir.gposContextInputSets.end(),
                    rule.inputSets.begin(),
                    rule.inputSets.end());

                ir.gposContextLookups.insert(
                    ir.gposContextLookups.end(),
                    rule.lookups.begin(),
                    rule.lookups.end());

                ir.gposContextRules.push_back({
                    inputSetOffset,
                    inputCount,
                    lookupOffset,
                    lookupCount
                    });
            }

            ir.gposContextSubtables.push_back({
                ruleOffset,
                static_cast<uint32_t>(
                    subtable.rules.size())
                });
        }

        return true;
    }


    static inline bool compileOpenTypeGposContextLookupInternal(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || !lookup ||
            lookupIndex >= lookups.size())
        {
            return false;
        }

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 7)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const OpenTypeGposIRCompileCheckpoint checkpoint(ir);

        auto rollback = [&]() noexcept
            {
                checkpoint.rollback(ir);
                result = kOpenTypeShapingIRInvalid;

                if (lookupIndex < context.lookupIds.size())
                    context.lookupIds[lookupIndex] =
                    kOpenTypeShapingIRInvalid;
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        std::vector<OpenTypeGposIRContextSubtableTemp>
            subtables;

        subtables.resize(subtableCount);

        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, subtableIndex);

            if (!data ||
                !compileOpenTypeGposContextSubtable(
                    data, ir, subtables[subtableIndex]))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >=
            uint64_t(kOpenTypeShapingIRInvalid))
        {
            rollback();
            return false;
        }

        // Reserve this lookup ID before compiling referenced actions. This
        // gives self-recursive and mutually recursive Type 7 lookups a stable
        // semantic lookup ID.
        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back({});

        if (lookupIndex >= context.lookupIds.size())
        {
            rollback();
            return false;
        }

        context.lookupIds[lookupIndex] = result;

        if (!resolveOpenTypeGposIRContextActions(
            lookups, gdef, ir, context, subtables))
        {
            rollback();
            return false;
        }

        uint32_t payloadOffset = 0;

        if (!appendOpenTypeGposContextSubtables(
            subtables, ir, payloadOffset))
        {
            rollback();
            return false;
        }

        if (result >= ir.lookups.size())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};
        compiled.op = OpenTypeShapingIROp::GposContext;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = subtableCount;
        compiled.filter = filter;

        ir.lookups[result] = compiled;
        return true;
    }

    // forward declaration
    static inline bool compileOpenTypeGposChainContextLookupInternal(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result);

    static inline bool compileOpenTypeGposIRLookupByIndex(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size() ||
            context.states.size() != lookups.size() ||
            context.lookupIds.size() != lookups.size())
        {
            return false;
        }

        const OpenTypeGposIRLookupCompileState state =
            context.states[lookupIndex];

        if (state == OpenTypeGposIRLookupCompileState::Done)
        {
            result = context.lookupIds[lookupIndex];
            return ir.lookup(result) != nullptr;
        }

        if (state == OpenTypeGposIRLookupCompileState::Compiling)
        {
            result = context.lookupIds[lookupIndex];

            return result != kOpenTypeShapingIRInvalid &&
                result < ir.lookups.size();
        }

        const OpenTypeLayoutLookupView lookup =
            lookups.lookup(lookupIndex);

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType))
        {
            return false;
        }

        context.states[lookupIndex] =
            OpenTypeGposIRLookupCompileState::Compiling;

        bool success = false;

        switch (effectiveType)
        {
        case 1:
            success = compileOpenTypeGposSingleLookup(
                lookup, gdef, ir, result);
            break;

        case 2:
            success = compileOpenTypeGposPairLookup(
                lookup, gdef, ir, result);
            break;

        case 3:
            success = compileOpenTypeGposCursiveLookup(
                lookup, gdef, ir, result);
            break;

        case 4:
            success = compileOpenTypeGposMarkBaseLookup(
                lookup, gdef, ir, result);
            break;

        case 5:
            success = compileOpenTypeGposMarkLigatureLookup(
                lookup, gdef, ir, result);
            break;

        case 6:
            success = compileOpenTypeGposMarkMarkLookup(
                lookup, gdef, ir, result);
            break;

        case 7:
            success = compileOpenTypeGposContextLookupInternal(
                lookups, lookupIndex, lookup, gdef,
                ir, context, result);
            break;

        case 8:
            success = compileOpenTypeGposChainContextLookupInternal(
                lookups, lookupIndex, lookup, gdef,
                ir, context, result);
            break;

        default:
            success = false;
            break;
        }

        if (!success)
        {
            context.states[lookupIndex] =
                OpenTypeGposIRLookupCompileState::Unseen;

            context.lookupIds[lookupIndex] =
                kOpenTypeShapingIRInvalid;

            result = kOpenTypeShapingIRInvalid;
            return false;
        }

        if (result == kOpenTypeShapingIRInvalid ||
            result >= ir.lookups.size())
        {
            context.states[lookupIndex] =
                OpenTypeGposIRLookupCompileState::Unseen;

            context.lookupIds[lookupIndex] =
                kOpenTypeShapingIRInvalid;

            result = kOpenTypeShapingIRInvalid;
            return false;
        }

        context.lookupIds[lookupIndex] = result;
        context.states[lookupIndex] =
            OpenTypeGposIRLookupCompileState::Done;

        return true;
    }


    static inline bool compileOpenTypeGposContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size())
            return false;

        OpenTypeGposIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        return compileOpenTypeGposIRLookupByIndex(
            lookups, lookupIndex, gdef,
            ir, context, result);
    }


    static inline bool compileOpenTypeGposContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposContextLookup(
            lookups, lookupIndex, gdef, ir, result);
    }


    static inline bool compileOpenTypeGposChainContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size())
            return false;

        OpenTypeGposIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        return compileOpenTypeGposIRLookupByIndex(
            lookups, lookupIndex, gdef,
            ir, context, result);
    }


    static inline bool compileOpenTypeGposChainContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposChainContextLookup(
            lookups, lookupIndex, gdef, ir, result);
    }


    static inline bool compileOpenTypeGposLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size())
            return false;

        OpenTypeGposIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        return compileOpenTypeGposIRLookupByIndex(
            lookups, lookupIndex, gdef,
            ir, context, result);
    }


    static inline bool compileOpenTypeGposLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposLookup(
            lookups, lookupIndex, gdef, ir, result);
    }


    // ========================================================================
    // GPOS Type 8 - ChainContextPos
    //
    // Formats 1, 2, and 3 compile to:
    //
    //     ordered rules
    //         backtrack glyph sets, nearest-first
    //         input glyph sets, current-first
    //         lookahead glyph sets, nearest-first
    //         ordered semantic lookup actions
    //
    // Backtrack and lookahead are match-only constraints. SequenceLookup
    // actions address only the input sequence.
    // ========================================================================

    struct OpenTypeGposIRChainContextRuleTemp
    {
        std::vector<OpenTypeShapingIRGlyphSetId> backtrackSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> inputSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> lookaheadSets{};
        std::vector<OpenTypeSequenceLookup> sourceLookups{};
        std::vector<OpenTypeShapingIRSequenceLookup> lookups{};
    };


    struct OpenTypeGposIRChainContextSubtableTemp
    {
        std::vector<OpenTypeGposIRChainContextRuleTemp> rules{};
    };


    static inline bool compileOpenTypeGposChainContextFormat1(
        const OpenTypeGposChainContextPosView& pos,
        OpenTypeShapingIR& ir,
        OpenTypeGposIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!pos || pos.format() != 1)
            return false;

        const OpenTypeCoverageView coverage = pos.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= pos.ruleSetCount())
                continue;

            uint16_t ruleSetOffset = 0;

            if (!pos.ruleSetOffset(coverageIndex, ruleSetOffset))
                return false;

            if (ruleSetOffset == 0)
                continue;

            const OpenTypeGposChainContextRuleSetView set =
                pos.ruleSet(coverageIndex);

            if (!set)
                return false;

            OpenTypeShapingIRGlyphSetId firstSet =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRSingletonGlyphSet(
                static_cast<uint16_t>(glyphId), ir, firstSet))
            {
                return false;
            }

            for (uint16_t ruleIndex = 0;
                ruleIndex < set.size();
                ++ruleIndex)
            {
                const OpenTypeGposChainContextRuleView rule =
                    set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t backtrackCount =
                    rule.backtrackGlyphCount();

                const uint16_t inputCount =
                    rule.inputGlyphCount();

                const uint16_t lookaheadCount =
                    rule.lookaheadGlyphCount();

                if (inputCount == 0)
                    return false;

                OpenTypeGposIRChainContextRuleTemp compiled;
                compiled.backtrackSets.reserve(backtrackCount);
                compiled.inputSets.reserve(inputCount);
                compiled.lookaheadSets.reserve(lookaheadCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t i = 0; i < backtrackCount; ++i)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.backtrackGlyphId(i, expectedGlyph))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGposIRSingletonGlyphSet(
                        expectedGlyph, ir, setId))
                    {
                        return false;
                    }

                    compiled.backtrackSets.push_back(setId);
                }

                for (uint16_t i = 1; i < inputCount; ++i)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.inputGlyphId(i - 1, expectedGlyph))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGposIRSingletonGlyphSet(
                        expectedGlyph, ir, setId))
                    {
                        return false;
                    }

                    compiled.inputSets.push_back(setId);
                }

                for (uint16_t i = 0; i < lookaheadCount; ++i)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.lookaheadGlyphId(i, expectedGlyph))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGposIRSingletonGlyphSet(
                        expectedGlyph, ir, setId))
                    {
                        return false;
                    }

                    compiled.lookaheadSets.push_back(setId);
                }

                const uint16_t lookupCount =
                    rule.sequenceLookupCount();

                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t i = 0; i < lookupCount; ++i)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(i, action))
                        return false;

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    static inline bool compileOpenTypeGposChainContextFormat2(
        const OpenTypeGposChainContextPosView& pos,
        OpenTypeShapingIR& ir,
        OpenTypeGposIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!pos || pos.format() != 2)
            return false;

        const OpenTypeCoverageView coverage =
            pos.coverage();

        const OpenTypeClassDefView backtrackClassDef =
            pos.backtrackClassDef();

        const OpenTypeClassDefView inputClassDef =
            pos.inputClassDef();

        const OpenTypeClassDefView lookaheadClassDef =
            pos.lookaheadClassDef();

        if (!coverage ||
            !backtrackClassDef ||
            !inputClassDef ||
            !lookaheadClassDef)
        {
            return false;
        }

        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>
            backtrackCache;

        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>
            inputCache;

        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>
            lookaheadCache;

        auto classGlyphSet =
            [&](const OpenTypeClassDefView& classDef,
                std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>& cache,
                uint16_t classValue,
                OpenTypeShapingIRGlyphSetId& setId) -> bool
            {
                for (const auto& entry : cache)
                {
                    if (entry.first == classValue)
                    {
                        setId = entry.second;
                        return true;
                    }
                }

                if (!compileOpenTypeGposIRClassGlyphSet(
                    classDef, classValue, ir, setId))
                {
                    return false;
                }

                cache.push_back({
                    classValue,
                    setId
                    });

                return true;
            };

        const uint16_t classSetCount =
            pos.classSetCount();

        for (uint16_t firstClass = 0;
            firstClass < classSetCount;
            ++firstClass)
        {
            uint16_t classSetOffset = 0;

            if (!pos.classSetOffset(
                firstClass, classSetOffset))
            {
                return false;
            }

            if (classSetOffset == 0)
                continue;

            OpenTypeShapingIRGlyphSetId firstSet =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRCoverageClassGlyphSet(
                coverage, inputClassDef,
                firstClass, ir, firstSet))
            {
                return false;
            }

            if (openTypeGposIRGlyphSetEmpty(ir, firstSet))
                continue;

            const OpenTypeGposChainContextClassSetView set =
                pos.classSet(firstClass);

            if (!set)
                return false;

            for (uint16_t ruleIndex = 0;
                ruleIndex < set.size();
                ++ruleIndex)
            {
                const OpenTypeGposChainContextClassRuleView rule =
                    set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t backtrackCount =
                    rule.backtrackGlyphCount();

                const uint16_t inputCount =
                    rule.inputGlyphCount();

                const uint16_t lookaheadCount =
                    rule.lookaheadGlyphCount();

                if (inputCount == 0)
                    return false;

                OpenTypeGposIRChainContextRuleTemp compiled;
                compiled.backtrackSets.reserve(backtrackCount);
                compiled.inputSets.reserve(inputCount);
                compiled.lookaheadSets.reserve(lookaheadCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t i = 0; i < backtrackCount; ++i)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.backtrackClass(i, expectedClass))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(
                        backtrackClassDef, backtrackCache,
                        expectedClass, setId))
                    {
                        return false;
                    }

                    compiled.backtrackSets.push_back(setId);
                }

                for (uint16_t i = 1; i < inputCount; ++i)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.inputClass(
                        i - 1, expectedClass))
                    {
                        return false;
                    }

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(
                        inputClassDef, inputCache,
                        expectedClass, setId))
                    {
                        return false;
                    }

                    compiled.inputSets.push_back(setId);
                }

                for (uint16_t i = 0; i < lookaheadCount; ++i)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.lookaheadClass(i, expectedClass))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(
                        lookaheadClassDef, lookaheadCache,
                        expectedClass, setId))
                    {
                        return false;
                    }

                    compiled.lookaheadSets.push_back(setId);
                }

                const uint16_t lookupCount =
                    rule.sequenceLookupCount();

                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t i = 0; i < lookupCount; ++i)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(i, action))
                        return false;

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    static inline bool compileOpenTypeGposChainContextFormat3(
        const OpenTypeGposChainContextPosView& pos,
        OpenTypeShapingIR& ir,
        OpenTypeGposIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!pos || pos.format() != 3)
            return false;

        const uint16_t backtrackCount =
            pos.backtrackGlyphCount();

        const uint16_t inputCount =
            pos.inputGlyphCount();

        const uint16_t lookaheadCount =
            pos.lookaheadGlyphCount();

        if (inputCount == 0)
            return false;

        OpenTypeGposIRChainContextRuleTemp compiled;
        compiled.backtrackSets.reserve(backtrackCount);
        compiled.inputSets.reserve(inputCount);
        compiled.lookaheadSets.reserve(lookaheadCount);

        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            const OpenTypeCoverageView coverage =
                pos.backtrackCoverage(i);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRCoverageGlyphSet(
                coverage, ir, setId))
            {
                return false;
            }

            compiled.backtrackSets.push_back(setId);
        }

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            const OpenTypeCoverageView coverage =
                pos.inputCoverage(i);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRCoverageGlyphSet(
                coverage, ir, setId))
            {
                return false;
            }

            compiled.inputSets.push_back(setId);
        }

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            const OpenTypeCoverageView coverage =
                pos.lookaheadCoverage(i);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRCoverageGlyphSet(
                coverage, ir, setId))
            {
                return false;
            }

            compiled.lookaheadSets.push_back(setId);
        }

        const uint16_t lookupCount =
            pos.sequenceLookupCount();

        compiled.sourceLookups.reserve(lookupCount);

        for (uint16_t i = 0; i < lookupCount; ++i)
        {
            OpenTypeSequenceLookup action{};

            if (!pos.sequenceLookup(i, action))
                return false;

            compiled.sourceLookups.push_back(action);
        }

        result.rules.push_back(std::move(compiled));
        return true;
    }


    static inline bool compileOpenTypeGposChainContextSubtable(
        const ByteSpan& data, OpenTypeShapingIR& ir,
        OpenTypeGposIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        const OpenTypeGposChainContextPosView pos(data);

        if (!pos)
            return false;

        switch (pos.format())
        {
        case 1:
            return compileOpenTypeGposChainContextFormat1(
                pos, ir, result);

        case 2:
            return compileOpenTypeGposChainContextFormat2(
                pos, ir, result);

        case 3:
            return compileOpenTypeGposChainContextFormat3(
                pos, ir, result);

        default:
            return false;
        }
    }


    static inline bool resolveOpenTypeGposIRChainContextActions(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        std::vector<OpenTypeGposIRChainContextSubtableTemp>& subtables)
    {
        for (OpenTypeGposIRChainContextSubtableTemp& subtable :
            subtables)
        {
            for (OpenTypeGposIRChainContextRuleTemp& rule :
                subtable.rules)
            {
                rule.lookups.clear();
                rule.lookups.reserve(rule.sourceLookups.size());

                for (const OpenTypeSequenceLookup& action :
                    rule.sourceLookups)
                {
                    OpenTypeShapingIRLookupId lookupId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGposIRLookupByIndex(
                        lookups,
                        action.lookupListIndex,
                        gdef,
                        ir,
                        context,
                        lookupId))
                    {
                        return false;
                    }

                    rule.lookups.push_back({
                        action.sequenceIndex,
                        lookupId
                        });
                }
            }
        }

        return true;
    }


    static inline bool appendOpenTypeGposChainContextSubtables(
        const std::vector<OpenTypeGposIRChainContextSubtableTemp>& subtables,
        OpenTypeShapingIR& ir, uint32_t& payloadOffset)
    {
        payloadOffset = 0;

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gposChainContextSubtables.size() > max32 ||
            ir.gposChainContextRules.size() > max32 ||
            ir.gposChainContextBacktrackSets.size() > max32 ||
            ir.gposChainContextInputSets.size() > max32 ||
            ir.gposChainContextLookaheadSets.size() > max32 ||
            ir.gposChainContextLookups.size() > max32 ||
            subtables.size() > max32)
        {
            return false;
        }

        payloadOffset =
            static_cast<uint32_t>(
                ir.gposChainContextSubtables.size());

        if (uint64_t(payloadOffset) + subtables.size() > max32)
            return false;

        for (const OpenTypeGposIRChainContextSubtableTemp& subtable :
            subtables)
        {
            if (ir.gposChainContextRules.size() > max32 ||
                subtable.rules.size() > max32)
            {
                return false;
            }

            const uint32_t ruleOffset =
                static_cast<uint32_t>(
                    ir.gposChainContextRules.size());

            if (uint64_t(ruleOffset) + subtable.rules.size() > max32)
                return false;

            for (const OpenTypeGposIRChainContextRuleTemp& rule :
                subtable.rules)
            {
                if (rule.inputSets.empty())
                    return false;

                for (OpenTypeShapingIRGlyphSetId setId :
                rule.backtrackSets)
                {
                    if (!ir.glyphSet(setId))
                        return false;
                }

                for (OpenTypeShapingIRGlyphSetId setId :
                rule.inputSets)
                {
                    if (!ir.glyphSet(setId))
                        return false;
                }

                for (OpenTypeShapingIRGlyphSetId setId :
                rule.lookaheadSets)
                {
                    if (!ir.glyphSet(setId))
                        return false;
                }

                if (ir.gposChainContextBacktrackSets.size() > max32 ||
                    ir.gposChainContextInputSets.size() > max32 ||
                    ir.gposChainContextLookaheadSets.size() > max32 ||
                    ir.gposChainContextLookups.size() > max32 ||
                    rule.backtrackSets.size() > max32 ||
                    rule.inputSets.size() > max32 ||
                    rule.lookaheadSets.size() > max32 ||
                    rule.lookups.size() > max32)
                {
                    return false;
                }

                const uint32_t backtrackSetOffset =
                    static_cast<uint32_t>(
                        ir.gposChainContextBacktrackSets.size());

                const uint32_t backtrackCount =
                    static_cast<uint32_t>(
                        rule.backtrackSets.size());

                const uint32_t inputSetOffset =
                    static_cast<uint32_t>(
                        ir.gposChainContextInputSets.size());

                const uint32_t inputCount =
                    static_cast<uint32_t>(
                        rule.inputSets.size());

                const uint32_t lookaheadSetOffset =
                    static_cast<uint32_t>(
                        ir.gposChainContextLookaheadSets.size());

                const uint32_t lookaheadCount =
                    static_cast<uint32_t>(
                        rule.lookaheadSets.size());

                const uint32_t lookupOffset =
                    static_cast<uint32_t>(
                        ir.gposChainContextLookups.size());

                const uint32_t lookupCount =
                    static_cast<uint32_t>(
                        rule.lookups.size());

                if (uint64_t(backtrackSetOffset) + backtrackCount > max32 ||
                    uint64_t(inputSetOffset) + inputCount > max32 ||
                    uint64_t(lookaheadSetOffset) + lookaheadCount > max32 ||
                    uint64_t(lookupOffset) + lookupCount > max32)
                {
                    return false;
                }

                for (const OpenTypeShapingIRSequenceLookup& action :
                    rule.lookups)
                {
                    if (action.lookup == kOpenTypeShapingIRInvalid ||
                        action.lookup >= ir.lookups.size())
                    {
                        return false;
                    }
                }

                ir.gposChainContextBacktrackSets.insert(
                    ir.gposChainContextBacktrackSets.end(),
                    rule.backtrackSets.begin(),
                    rule.backtrackSets.end());

                ir.gposChainContextInputSets.insert(
                    ir.gposChainContextInputSets.end(),
                    rule.inputSets.begin(),
                    rule.inputSets.end());

                ir.gposChainContextLookaheadSets.insert(
                    ir.gposChainContextLookaheadSets.end(),
                    rule.lookaheadSets.begin(),
                    rule.lookaheadSets.end());

                ir.gposChainContextLookups.insert(
                    ir.gposChainContextLookups.end(),
                    rule.lookups.begin(),
                    rule.lookups.end());

                ir.gposChainContextRules.push_back({
                    backtrackSetOffset,
                    backtrackCount,
                    inputSetOffset,
                    inputCount,
                    lookaheadSetOffset,
                    lookaheadCount,
                    lookupOffset,
                    lookupCount
                    });
            }

            ir.gposChainContextSubtables.push_back({
                ruleOffset,
                static_cast<uint32_t>(
                    subtable.rules.size())
                });
        }

        return true;
    }


    static inline bool compileOpenTypeGposChainContextLookupInternal(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGposIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || !lookup ||
            lookupIndex >= lookups.size())
        {
            return false;
        }

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 8)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const OpenTypeGposIRCompileCheckpoint checkpoint(ir);

        auto rollback = [&]() noexcept
            {
                checkpoint.rollback(ir);
                result = kOpenTypeShapingIRInvalid;

                if (lookupIndex < context.lookupIds.size())
                    context.lookupIds[lookupIndex] =
                    kOpenTypeShapingIRInvalid;
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeGposIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        std::vector<OpenTypeGposIRChainContextSubtableTemp>
            subtables(subtableCount);

        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGposIREffectiveSubtable(
                    lookup, effectiveType, subtableIndex);

            if (!data ||
                !compileOpenTypeGposChainContextSubtable(
                    data, ir, subtables[subtableIndex]))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >=
            uint64_t(kOpenTypeShapingIRInvalid))
        {
            rollback();
            return false;
        }

        // Reserve this lookup ID before resolving nested actions so recursive
        // Type 7/8 references have a stable semantic ID.
        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back({});

        if (lookupIndex >= context.lookupIds.size())
        {
            rollback();
            return false;
        }

        context.lookupIds[lookupIndex] = result;

        if (!resolveOpenTypeGposIRChainContextActions(
            lookups, gdef, ir, context, subtables))
        {
            rollback();
            return false;
        }

        uint32_t payloadOffset = 0;

        if (!appendOpenTypeGposChainContextSubtables(
            subtables, ir, payloadOffset))
        {
            rollback();
            return false;
        }

        if (result >= ir.lookups.size())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};
        compiled.op = OpenTypeShapingIROp::GposChainContext;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = subtableCount;
        compiled.filter = filter;

        ir.lookups[result] = compiled;
        return true;
    }


    // ========================================================================
    // Generic initial GPOS compiler dispatch
    //
    // View-only dispatch handles Types 1..6.
    // Types 7 and 8 require the parent LookupList because SequenceLookupRecords
    // reference other LookupList entries. Use the LookupList overload above.
    // ExtensionPos Type 9 is resolved before dispatch.
    // ========================================================================

    static inline bool compileOpenTypeGposLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        uint16_t effectiveType = 0;

        if (!openTypeGposIREffectiveType(
            lookup, effectiveType))
        {
            return false;
        }

        switch (effectiveType)
        {
        case 1:
            return compileOpenTypeGposSingleLookup(
                lookup, gdef, ir, result);

        case 2:
            return compileOpenTypeGposPairLookup(
                lookup, gdef, ir, result);

        case 3:
            return compileOpenTypeGposCursiveLookup(
                lookup, gdef, ir, result);

        case 4:
            return compileOpenTypeGposMarkBaseLookup(
                lookup, gdef, ir, result);

        case 5:
            return compileOpenTypeGposMarkLigatureLookup(
                lookup, gdef, ir, result);

        case 6:
            return compileOpenTypeGposMarkMarkLookup(
                lookup, gdef, ir, result);

        default:
            return false;
        }
    }


    static inline bool compileOpenTypeGposLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGposLookup(
            lookup, gdef, ir, result);
    }

} // namespace waavs
