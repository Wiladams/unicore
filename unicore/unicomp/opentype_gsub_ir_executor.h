// opentype_gsub_ir_executor.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "opentype_shaping_buffer.h"
#include "opentype_shaping_ir.h"
#include "opentype_gsub_edit.h"
#include "opentype_gsub_sequence_state.h"
#include "opentype_gsub_apply_state.h"

namespace waavs
{
    enum class OpenTypeShapingIRResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ========================================================================
    // Common lookup helpers
    // ========================================================================



    [[nodiscard]] static inline bool openTypeGsubIRBufferGlyphIdsValid(
        const OpenTypeShapingBuffer& buffer) noexcept
    {
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            if (buffer[i].glyphId > 0xFFFFu)
                return false;
        }

        return true;
    }

    enum class OpenTypeShapingIRGlyphSearchResult : uint8_t
    {
        Invalid = 0,
        End,
        Found
    };


    [[nodiscard]] static inline bool openTypeShapingIRGlyphSetValid(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId id) noexcept
    {
        const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(id);

        if (!set)
            return false;

        const size_t offset = set->rangeOffset;
        const size_t count = set->rangeCount;

        if (offset > ir.glyphRanges.size() ||
            count > ir.glyphRanges.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            const OpenTypeShapingIRGlyphRange& range =
                ir.glyphRanges[offset + i];

            if (range.first > range.last)
                return false;

            if (i != 0)
            {
                const OpenTypeShapingIRGlyphRange& previous =
                    ir.glyphRanges[offset + i - 1];

                if (previous.last >= range.first)
                    return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeShapingIRGlyphSetContains(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId id,
        uint16_t glyphId, bool& result) noexcept
    {
        result = false;

        if (!openTypeShapingIRGlyphSetValid(ir, id))
            return false;

        const OpenTypeShapingIRGlyphSet& set =
            ir.glyphSets[id];

        size_t first = set.rangeOffset;
        size_t last = first + set.rangeCount;

        while (first < last)
        {
            const size_t middle =
                first + (last - first) / 2;

            const OpenTypeShapingIRGlyphRange& range =
                ir.glyphRanges[middle];

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


    [[nodiscard]] static inline bool openTypeShapingIRLookupFilterValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookupFilter& filter) noexcept
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
            !openTypeShapingIRGlyphSetValid(
                ir, filter.markFilteringSet))
        {
            return false;
        }

        if (!ignoreMarks &&
            !useMarkFilteringSet &&
            filter.markAttachmentType != 0 &&
            ir.gdefMarkAttachClasses.size() !=
            kOpenTypeShapingIRGlyphDomainSize)
        {
            return false;
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeShapingIRLookupShouldSkip(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookupFilter& filter,
        uint32_t glyphId, bool& result) noexcept
    {
        result = false;

        if (!openTypeShapingIRLookupFilterValid(ir, filter) ||
            glyphId > 0xFFFFu)
        {
            return false;
        }

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

        const uint16_t glyphClass =
            ir.gdefGlyphClasses[glyphId];

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

            if (!openTypeShapingIRGlyphSetContains(
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
            const uint16_t markClass =
                ir.gdefMarkAttachClasses[glyphId];

            result =
                markClass != filter.markAttachmentType;

            return true;
        }

        return true;
    }


    static inline OpenTypeShapingIRGlyphSearchResult
        openTypeShapingIRLookupNext(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingBuffer& buffer,
            size_t currentIndex, size_t& result) noexcept
    {
        result = buffer.size();

        if (!openTypeShapingIRLookupFilterValid(ir, filter) ||
            currentIndex >= buffer.size())
        {
            return OpenTypeShapingIRGlyphSearchResult::Invalid;
        }

        for (size_t index = currentIndex + 1;
            index < buffer.size();
            ++index)
        {
            bool skip = false;

            if (!openTypeShapingIRLookupShouldSkip(
                ir, filter, buffer[index].glyphId, skip))
            {
                return OpenTypeShapingIRGlyphSearchResult::Invalid;
            }

            if (!skip)
            {
                result = index;
                return OpenTypeShapingIRGlyphSearchResult::Found;
            }
        }

        return OpenTypeShapingIRGlyphSearchResult::End;
    }


    static inline OpenTypeShapingIRGlyphSearchResult
        openTypeShapingIRLookupPrevious(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingBuffer& buffer,
            size_t currentIndex, size_t& result) noexcept
    {
        result = buffer.size();

        if (!openTypeShapingIRLookupFilterValid(ir, filter) ||
            currentIndex >= buffer.size())
        {
            return OpenTypeShapingIRGlyphSearchResult::Invalid;
        }

        size_t index = currentIndex;

        while (index != 0)
        {
            --index;

            bool skip = false;

            if (!openTypeShapingIRLookupShouldSkip(
                ir, filter, buffer[index].glyphId, skip))
            {
                return OpenTypeShapingIRGlyphSearchResult::Invalid;
            }

            if (!skip)
            {
                result = index;
                return OpenTypeShapingIRGlyphSearchResult::Found;
            }
        }

        return OpenTypeShapingIRGlyphSearchResult::End;
    }

    // ========================================================================
    // GSUB Single
    // ========================================================================

    // ========================================================================
    // openTypeGsubIRSingleSubtableValid
    //
    // Validate one compiled SingleSubst subtable slice.
    //
    // Binary search requires pairs to be strictly ordered by input glyph.
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRSingleSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubSingleSubtable& subtable) noexcept
    {
        const size_t offset = subtable.pairOffset;
        const size_t count = subtable.pairCount;

        if (offset > ir.gsubSinglePairs.size() ||
            count > ir.gsubSinglePairs.size() - offset)
        {
            return false;
        }

        for (size_t i = 1; i < count; ++i)
        {
            if (ir.gsubSinglePairs[offset + i - 1].input >=
                ir.gsubSinglePairs[offset + i].input)
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRSingleSubtableUnchecked
    //
    // Caller has already validated the subtable.
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRSingleSubtableUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubSingleSubtable& subtable,
        uint16_t glyphId, uint16_t& replacement) noexcept
    {
        replacement = 0;

        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGsubSinglePair& pair = ir.gsubSinglePairs[middle];

            if (glyphId < pair.input)
                last = middle;
            else if (glyphId > pair.input)
                first = middle + 1;
            else
            {
                replacement = pair.output;
                return OpenTypeShapingIRResult::Match;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRSingleSubtable
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRSingleSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubSingleSubtable& subtable,
        uint16_t glyphId, uint16_t& replacement) noexcept
    {
        replacement = 0;

        if (!openTypeGsubIRSingleSubtableValid(ir, subtable))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRSingleSubtableUnchecked(
            ir, subtable, glyphId, replacement);
    }


    // ========================================================================
    // openTypeGsubIRSingleLookupValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRSingleLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubSingle)
            return false;

        if (!openTypeShapingIRLookupFilterValid(ir, lookup.filter))
            return false;

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubSingleSubtables.size() ||
            count == 0 ||
            count > ir.gsubSingleSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRSingleSubtableValid(
                ir, ir.gsubSingleSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRSingleLookupUnchecked
    //
    // Subtables are tried in compiled order. First match wins.
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRSingleLookupUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, uint16_t& replacement) noexcept
    {
        replacement = 0;

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRSingleSubtableUnchecked(
                    ir,
                    ir.gsubSingleSubtables[firstSubtable + i],
                    glyphId,
                    replacement);

            if (result == OpenTypeShapingIRResult::Match)
                return result;
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRSingleLookup
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRSingleLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, uint16_t& replacement) noexcept
    {
        replacement = 0;

        if (!openTypeGsubIRSingleLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRSingleLookupUnchecked(
            ir, lookup, glyphId, replacement);
    }


    // ========================================================================
    // applyOpenTypeGsubIRSingleLookup
    //
    // SingleSubst:
    //
    //     1 -> 1
    //
    // Only glyphId changes.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRSingleLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer) noexcept
    {
        if (!openTypeGsubIRSingleLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            uint16_t replacement = 0;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRSingleLookupUnchecked(
                    ir, lookup,
                    static_cast<uint16_t>(buffer[i].glyphId),
                    replacement);

            if (result == OpenTypeShapingIRResult::Match)
                buffer[i].glyphId = replacement;
        }

        return true;
    }


    // ========================================================================
    // GSUB Multiple
    // ========================================================================

    // ========================================================================
    // openTypeGsubIRGlyphSequenceValid
    //
    // Validate one semantic replacement sequence.
    //
    // Empty MultipleSubst sequences are not permitted.
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRGlyphSequenceValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGlyphSequence& sequence) noexcept
    {
        const size_t offset = sequence.glyphOffset;
        const size_t count = sequence.glyphCount;

        if (count == 0)
            return false;

        if (offset > ir.gsubMultipleGlyphs.size() ||
            count > ir.gsubMultipleGlyphs.size() - offset)
        {
            return false;
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRMultipleSubtableValid
    //
    // Validate pair storage and every referenced sequence.
    //
    // Pairs must be strictly ordered by input glyph for binary search.
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRMultipleSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubMultipleSubtable& subtable) noexcept
    {
        const size_t offset = subtable.pairOffset;
        const size_t count = subtable.pairCount;

        if (offset > ir.gsubMultiplePairs.size() ||
            count > ir.gsubMultiplePairs.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            const OpenTypeShapingIRGsubMultiplePair& pair =
                ir.gsubMultiplePairs[offset + i];

            if (pair.sequenceIndex >= ir.gsubMultipleSequences.size())
                return false;

            if (!openTypeGsubIRGlyphSequenceValid(
                ir, ir.gsubMultipleSequences[pair.sequenceIndex]))
            {
                return false;
            }

            if (i != 0 &&
                ir.gsubMultiplePairs[offset + i - 1].input >= pair.input)
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRMultipleSubtableUnchecked
    //
    // Resolve one input glyph to a semantic sequence index.
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRMultipleSubtableUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubMultipleSubtable& subtable,
        uint16_t glyphId, uint32_t& sequenceIndex) noexcept
    {
        sequenceIndex = kOpenTypeShapingIRInvalid;

        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGsubMultiplePair& pair =
                ir.gsubMultiplePairs[middle];

            if (glyphId < pair.input)
                last = middle;
            else if (glyphId > pair.input)
                first = middle + 1;
            else
            {
                sequenceIndex = pair.sequenceIndex;
                return OpenTypeShapingIRResult::Match;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRMultipleSubtable
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRMultipleSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubMultipleSubtable& subtable,
        uint16_t glyphId, uint32_t& sequenceIndex) noexcept
    {
        sequenceIndex = kOpenTypeShapingIRInvalid;

        if (!openTypeGsubIRMultipleSubtableValid(ir, subtable))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRMultipleSubtableUnchecked(
            ir, subtable, glyphId, sequenceIndex);
    }


    // ========================================================================
    // openTypeGsubIRMultipleLookupValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRMultipleLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubMultiple)
            return false;

        if (!openTypeShapingIRLookupFilterValid(ir, lookup.filter))
            return false;

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubMultipleSubtables.size() ||
            count == 0 ||
            count > ir.gsubMultipleSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRMultipleSubtableValid(
                ir, ir.gsubMultipleSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRMultipleLookupUnchecked
    //
    // Subtables are tried in stored order. First match wins.
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRMultipleLookupUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, uint32_t& sequenceIndex) noexcept
    {
        sequenceIndex = kOpenTypeShapingIRInvalid;

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRMultipleSubtableUnchecked(
                    ir,
                    ir.gsubMultipleSubtables[firstSubtable + i],
                    glyphId,
                    sequenceIndex);

            if (result == OpenTypeShapingIRResult::Match)
                return result;
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRMultipleLookup
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRMultipleLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, uint32_t& sequenceIndex) noexcept
    {
        sequenceIndex = kOpenTypeShapingIRInvalid;

        if (!openTypeGsubIRMultipleLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRMultipleLookupUnchecked(
            ir, lookup, glyphId, sequenceIndex);
    }


    // ========================================================================
    // applyOpenTypeGsubIRMultipleSequence
    //
    // Apply:
    //
    //     1 glyph -> N glyphs
    //
    // Every emitted glyph inherits the complete source shaping record:
    //
    //     scalarOffset
    //     scalarCount
    //     ligature provenance
    //
    // Only glyphId differs between the outputs.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRMultipleSequence(
        const OpenTypeShapingIR& ir, uint32_t sequenceIndex,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex)
    {
        if (sequenceIndex >= ir.gsubMultipleSequences.size() ||
            glyphIndex >= buffer.size())
        {
            return false;
        }

        const OpenTypeShapingIRGlyphSequence& sequence =
            ir.gsubMultipleSequences[sequenceIndex];

        if (!openTypeGsubIRGlyphSequenceValid(ir, sequence))
            return false;

        const size_t replacementCount = sequence.glyphCount;
        const size_t glyphOffset = sequence.glyphOffset;

        const OpenTypeShapingGlyph source = buffer[glyphIndex];

        if (replacementCount > 1)
        {
            std::vector<OpenTypeShapingGlyph>& glyphs = buffer.glyphs();

            glyphs.insert(
                glyphs.begin() + glyphIndex + 1,
                replacementCount - 1,
                source);
        }

        for (size_t i = 0; i < replacementCount; ++i)
        {
            OpenTypeShapingGlyph& glyph = buffer[glyphIndex + i];

            glyph = source;
            glyph.glyphId = ir.gsubMultipleGlyphs[glyphOffset + i];
        }

        return true;
    }


    // ========================================================================
    // applyOpenTypeGsubIRMultipleLookup
    //
    // Execute one complete compiled MultipleSubst lookup.
    //
    // The original input stream is preflighted before mutation.
    //
    // Newly emitted glyphs are skipped for this lookup:
    //
    //     A -> B C
    //
    // execution resumes after C, not at B.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRMultipleLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubIRMultipleLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Preflight every original input glyph before changing topology.
        // ------------------------------------------------------------

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            uint32_t sequenceIndex = kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRMultipleLookupUnchecked(
                    ir, lookup,
                    static_cast<uint16_t>(buffer[i].glyphId),
                    sequenceIndex);

            if (result == OpenTypeShapingIRResult::Match)
            {
                if (sequenceIndex >= ir.gsubMultipleSequences.size() ||
                    !openTypeGsubIRGlyphSequenceValid(
                        ir, ir.gsubMultipleSequences[sequenceIndex]))
                {
                    return false;
                }
            }
        }


        // ------------------------------------------------------------
        // Execute.
        // ------------------------------------------------------------

        size_t glyphIndex = 0;

        while (glyphIndex < buffer.size())
        {
            uint32_t sequenceIndex = kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRMultipleLookupUnchecked(
                    ir, lookup,
                    static_cast<uint16_t>(buffer[glyphIndex].glyphId),
                    sequenceIndex);

            if (result == OpenTypeShapingIRResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            if (result != OpenTypeShapingIRResult::Match ||
                sequenceIndex >= ir.gsubMultipleSequences.size())
            {
                return false;
            }

            const size_t replacementCount =
                ir.gsubMultipleSequences[sequenceIndex].glyphCount;

            if (!applyOpenTypeGsubIRMultipleSequence(
                ir, sequenceIndex, buffer, glyphIndex))
            {
                return false;
            }

            glyphIndex += replacementCount;
        }

        return true;
    }



    // ========================================================================
    // GSUB Alternate
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRAlternateSetValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubAlternateSet& set) noexcept
    {
        const size_t offset = set.glyphOffset;
        const size_t count = set.glyphCount;

        return count != 0 &&
            offset <= ir.gsubAlternateGlyphs.size() &&
            count <= ir.gsubAlternateGlyphs.size() - offset;
    }


    [[nodiscard]] static inline bool openTypeGsubIRAlternateSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubAlternateSubtable& subtable) noexcept
    {
        const size_t offset = subtable.pairOffset;
        const size_t count = subtable.pairCount;

        if (offset > ir.gsubAlternatePairs.size() ||
            count > ir.gsubAlternatePairs.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            const OpenTypeShapingIRGsubAlternatePair& pair =
                ir.gsubAlternatePairs[offset + i];

            if (pair.reserved != 0 ||
                pair.alternateSetIndex >= ir.gsubAlternateSets.size() ||
                !openTypeGsubIRAlternateSetValid(
                    ir, ir.gsubAlternateSets[pair.alternateSetIndex]))
            {
                return false;
            }

            if (i != 0 &&
                ir.gsubAlternatePairs[offset + i - 1].input >= pair.input)
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGsubIRAlternateLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubAlternate ||
            !openTypeShapingIRLookupFilterValid(ir, lookup.filter))
        {
            return false;
        }

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubAlternateSubtables.size() ||
            count == 0 ||
            count > ir.gsubAlternateSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRAlternateSubtableValid(
                ir, ir.gsubAlternateSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRAlternateSubtableUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubAlternateSubtable& subtable,
        uint16_t glyphId, uint32_t& alternateSetIndex) noexcept
    {
        alternateSetIndex = kOpenTypeShapingIRInvalid;

        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGsubAlternatePair& pair =
                ir.gsubAlternatePairs[middle];

            if (glyphId < pair.input)
                last = middle;
            else if (glyphId > pair.input)
                first = middle + 1;
            else
            {
                alternateSetIndex = pair.alternateSetIndex;
                return OpenTypeShapingIRResult::Match;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRAlternateLookupUnchecked(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, uint16_t& replacement) noexcept
    {
        replacement = 0;
        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            uint32_t setIndex = kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRAlternateSubtableUnchecked(
                    ir, ir.gsubAlternateSubtables[firstSubtable + i],
                    glyphId, setIndex);

            if (result == OpenTypeShapingIRResult::NoMatch)
                continue;

            if (result != OpenTypeShapingIRResult::Match ||
                setIndex >= ir.gsubAlternateSets.size())
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            const OpenTypeShapingIRGsubAlternateSet& set =
                ir.gsubAlternateSets[setIndex];

            if (!openTypeGsubIRAlternateSetValid(ir, set))
                return OpenTypeShapingIRResult::Invalid;

            // Default alternate-selection policy: alternate zero.
            replacement = ir.gsubAlternateGlyphs[set.glyphOffset];
            return OpenTypeShapingIRResult::Match;
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRAlternateLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        uint16_t glyphId, uint16_t& replacement) noexcept
    {
        replacement = 0;

        if (!openTypeGsubIRAlternateLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRAlternateLookupUnchecked(
            ir, lookup, glyphId, replacement);
    }


    static inline bool applyOpenTypeGsubIRAlternateLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubIRAlternateLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }

        OpenTypeShapingBuffer working = buffer;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            uint16_t replacement = 0;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRAlternateLookupUnchecked(
                    ir, lookup,
                    static_cast<uint16_t>(working[glyphIndex].glyphId),
                    replacement);

            if (result == OpenTypeShapingIRResult::Invalid)
                return false;

            if (result == OpenTypeShapingIRResult::Match)
                working[glyphIndex].glyphId = replacement;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GSUB Ligature
    // ========================================================================

    struct OpenTypeGsubIRLigatureMatch
    {
        uint16_t ligatureGlyph{ 0 };
        std::vector<size_t> positions{};

        void clear() noexcept
        {
            ligatureGlyph = 0;
            positions.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return positions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return positions.size(); }
    };


    // ========================================================================
    // openTypeGsubIRLigatureRecordValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRLigatureRecordValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubLigature& ligature) noexcept
    {
        if (ligature.componentCount < 2)
            return false;

        const size_t offset = ligature.componentOffset;
        const size_t count = size_t(ligature.componentCount) - 1u;

        if (offset > ir.gsubLigatureComponents.size() ||
            count > ir.gsubLigatureComponents.size() - offset)
        {
            return false;
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRLigatureSubtableValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRLigatureSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubLigatureSubtable& subtable) noexcept
    {
        const size_t offset = subtable.pairOffset;
        const size_t count = subtable.pairCount;

        if (offset > ir.gsubLigaturePairs.size() ||
            count > ir.gsubLigaturePairs.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            const OpenTypeShapingIRGsubLigaturePair& pair =
                ir.gsubLigaturePairs[offset + i];

            if (pair.reserved != 0)
                return false;

            if (pair.ligatureOffset > ir.gsubLigatures.size() ||
                pair.ligatureCount >
                ir.gsubLigatures.size() - pair.ligatureOffset)
            {
                return false;
            }

            for (size_t j = 0; j < pair.ligatureCount; ++j)
            {
                if (!openTypeGsubIRLigatureRecordValid(
                    ir, ir.gsubLigatures[pair.ligatureOffset + j]))
                {
                    return false;
                }
            }

            if (i != 0 &&
                ir.gsubLigaturePairs[offset + i - 1].input >= pair.input)
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRLigatureLookupValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRLigatureLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubLigature)
            return false;

        if (!openTypeShapingIRLookupFilterValid(ir, lookup.filter))
            return false;

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubLigatureSubtables.size() ||
            count == 0 ||
            count > ir.gsubLigatureSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRLigatureSubtableValid(
                ir, ir.gsubLigatureSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRFindLigaturePairUnchecked
    //
    // Find the candidate set associated with the first input glyph.
    // ========================================================================

    [[nodiscard]] static inline const OpenTypeShapingIRGsubLigaturePair*
        openTypeGsubIRFindLigaturePairUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRGsubLigatureSubtable& subtable,
            uint16_t glyphId) noexcept
    {
        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;

            const OpenTypeShapingIRGsubLigaturePair& pair =
                ir.gsubLigaturePairs[middle];

            if (glyphId < pair.input)
                last = middle;
            else if (glyphId > pair.input)
                first = middle + 1;
            else
                return &pair;
        }

        return nullptr;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRLigatureSubtableUnchecked
    //
    // The first input glyph is tested directly.
    //
    // LookupFlag filtering is used only while walking to subsequent
    // components. Ignored glyphs remain physically present in the buffer.
    //
    // match.positions records the exact physical participating positions.
    // ========================================================================

    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRLigatureSubtableUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingIRGsubLigatureSubtable& subtable,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRLigatureMatch& match) noexcept
    {
        match.clear();

        if (glyphIndex >= buffer.size())
            return OpenTypeShapingIRResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeShapingIRResult::Invalid;

        const OpenTypeShapingIRGsubLigaturePair* pair =
            openTypeGsubIRFindLigaturePairUnchecked(
                ir, subtable,
                static_cast<uint16_t>(firstGlyphId));

        if (!pair)
            return OpenTypeShapingIRResult::NoMatch;

        // Candidate order is significant. First matching candidate wins.

        for (size_t ligatureIndex = 0;
            ligatureIndex < pair->ligatureCount;
            ++ligatureIndex)
        {
            const OpenTypeShapingIRGsubLigature& ligature =
                ir.gsubLigatures[
                    pair->ligatureOffset + ligatureIndex];

            match.clear();
            match.positions.reserve(ligature.componentCount);
            match.positions.push_back(glyphIndex);

            bool matches = true;
            size_t position = glyphIndex;

            for (uint16_t componentIndex = 1;
                componentIndex < ligature.componentCount;
                ++componentIndex)
            {
                const uint16_t expectedGlyph =
                    ir.gsubLigatureComponents[
                        ligature.componentOffset +
                            componentIndex - 1];

                size_t nextPosition = 0;

                const OpenTypeShapingIRGlyphSearchResult search =
                    openTypeShapingIRLookupNext(
                        ir, filter, buffer,
                        position, nextPosition);

                if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                    return OpenTypeShapingIRResult::Invalid;

                if (search == OpenTypeShapingIRGlyphSearchResult::End)
                {
                    matches = false;
                    break;
                }

                const uint32_t actualGlyph =
                    buffer[nextPosition].glyphId;

                if (actualGlyph > 0xFFFFu)
                    return OpenTypeShapingIRResult::Invalid;

                if (actualGlyph != expectedGlyph)
                {
                    matches = false;
                    break;
                }

                match.positions.push_back(nextPosition);
                position = nextPosition;
            }

            if (!matches)
                continue;

            if (match.positions.size() != ligature.componentCount)
                return OpenTypeShapingIRResult::Invalid;

            match.ligatureGlyph = ligature.output;
            return OpenTypeShapingIRResult::Match;
        }

        match.clear();
        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRLigatureLookupUnchecked
    //
    // Subtables are tried in compiled source order.
    // ========================================================================

    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRLigatureLookupUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRLigatureMatch& match) noexcept
    {
        match.clear();

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRLigatureSubtableUnchecked(
                    ir,
                    lookup.filter,
                    ir.gsubLigatureSubtables[firstSubtable + i],
                    buffer,
                    glyphIndex,
                    match);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
            {
                return result;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRLigatureLookup
    // ========================================================================

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRLigatureLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGsubIRLigatureMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGsubIRLigatureLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRLigatureLookupUnchecked(
            ir, lookup, buffer, glyphIndex, match);
    }


    // ========================================================================
    // openTypeGsubIRNextLigatureId
    // ========================================================================

    static inline uint32_t openTypeGsubIRNextLigatureId(
        const OpenTypeShapingBuffer& buffer) noexcept
    {
        uint32_t maximum = 0;

        for (const OpenTypeShapingGlyph& glyph : buffer)
        {
            if (glyph.ligature.id > maximum)
                maximum = glyph.ligature.id;
        }

        if (maximum == std::numeric_limits<uint32_t>::max())
            return 0;

        return maximum + 1;
    }


    static inline uint16_t openTypeGsubIRLigatureComponentCount(
        const OpenTypeShapingGlyph& glyph) noexcept
    {
        return glyph.ligature.effectiveComponentCount();
    }


    // ========================================================================
    // openTypeGsubIRGlyphIsMark
    //
    // Empty compiled GDEF classification follows the existing no-GDEF
    // compatibility path: no glyph can be positively identified as a mark.
    // ========================================================================

    static inline bool openTypeGsubIRGlyphIsMark(
        const OpenTypeShapingIR& ir,
        uint32_t glyphId, bool& result) noexcept
    {
        result = false;

        if (glyphId > 0xFFFFu)
            return false;

        if (ir.gdefGlyphClasses.empty())
            return true;

        if (ir.gdefGlyphClasses.size() !=
            kOpenTypeShapingIRGlyphDomainSize)
        {
            return false;
        }

        result = ir.gdefGlyphClasses[glyphId] == 3;
        return true;
    }


    // ========================================================================
    // applyOpenTypeGsubIRLigatureMatch
    //
    // Produces the ligature and preserves/remaps component provenance.
    //
    // Participating component positions may be non-contiguous. Filtered glyphs
    // between them survive physically in the shaping buffer.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRLigatureMatch(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingBuffer& buffer,
        const OpenTypeGsubIRLigatureMatch& match)
    {
        if (match.size() < 2 ||
            match.positions[0] >= buffer.size())
        {
            return false;
        }

        for (size_t i = 0; i < match.positions.size(); ++i)
        {
            if (match.positions[i] >= buffer.size())
                return false;

            if (i != 0 &&
                match.positions[i] <= match.positions[i - 1])
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Preserve scalar provenance.
        // ------------------------------------------------------------

        const OpenTypeShapingGlyph& first =
            buffer[match.positions[0]];

        uint32_t scalarBegin = first.scalarOffset;

        uint64_t scalarEnd =
            uint64_t(first.scalarOffset) +
            uint64_t(first.scalarCount);

        for (size_t i = 1; i < match.positions.size(); ++i)
        {
            const OpenTypeShapingGlyph& component =
                buffer[match.positions[i]];

            if (component.scalarOffset < scalarBegin)
                scalarBegin = component.scalarOffset;

            const uint64_t componentEnd =
                uint64_t(component.scalarOffset) +
                uint64_t(component.scalarCount);

            if (componentEnd > scalarEnd)
                scalarEnd = componentEnd;
        }

        if (scalarEnd < scalarBegin ||
            scalarEnd - scalarBegin > 0xFFFFFFFFull)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Determine whether all participating glyphs are GDEF marks.
        //
        // A mark-only ligature retains an existing parent-ligature
        // association rather than becoming a new base ligature.
        // ------------------------------------------------------------

        bool allMarks =
            ir.gdefGlyphClasses.size() ==
            kOpenTypeShapingIRGlyphDomainSize;

        if (allMarks)
        {
            for (size_t position : match.positions)
            {
                bool isMark = false;

                if (!openTypeGsubIRGlyphIsMark(
                    ir, buffer[position].glyphId, isMark))
                {
                    return false;
                }

                if (!isMark)
                {
                    allMarks = false;
                    break;
                }
            }
        }

        OpenTypeLigatureProvenance outputLigature{};


        // ------------------------------------------------------------
        // Mark-only ligature.
        // ------------------------------------------------------------

        if (allMarks)
        {
            outputLigature =
                buffer[match.positions[0]].ligature;

            outputLigature.componentCount = 0;

            for (size_t i = 1; i < match.positions.size(); ++i)
            {
                const OpenTypeLigatureProvenance& candidate =
                    buffer[match.positions[i]].ligature;

                if (candidate.id != outputLigature.id ||
                    candidate.component != outputLigature.component)
                {
                    outputLigature.clear();
                    break;
                }
            }
        }
        else
        {
            // ------------------------------------------------------------
            // Normal base ligature.
            // ------------------------------------------------------------

            const uint32_t ligatureId =
                openTypeGsubIRNextLigatureId(buffer);

            if (ligatureId == 0)
                return false;

            uint32_t totalComponentCount = 0;

            for (size_t position : match.positions)
            {
                totalComponentCount +=
                    openTypeGsubIRLigatureComponentCount(
                        buffer[position]);

                if (totalComponentCount >
                    std::numeric_limits<uint16_t>::max())
                {
                    return false;
                }
            }

            if (totalComponentCount < 2)
                return false;

            outputLigature.id = ligatureId;
            outputLigature.component = 0;
            outputLigature.componentCount =
                static_cast<uint16_t>(totalComponentCount);


            // ------------------------------------------------------------
            // Remap surviving marks between participating components.
            // ------------------------------------------------------------

            uint32_t componentsBefore = 0;

            for (size_t i = 0;
                i + 1 < match.positions.size();
                ++i)
            {
                const size_t componentPosition =
                    match.positions[i];

                const size_t nextComponentPosition =
                    match.positions[i + 1];

                const OpenTypeShapingGlyph componentGlyph =
                    buffer[componentPosition];

                const uint16_t componentCount =
                    openTypeGsubIRLigatureComponentCount(
                        componentGlyph);

                for (size_t position = componentPosition + 1;
                    position < nextComponentPosition;
                    ++position)
                {
                    bool isMark = false;

                    if (!openTypeGsubIRGlyphIsMark(
                        ir,
                        buffer[position].glyphId,
                        isMark))
                    {
                        return false;
                    }

                    if (!isMark)
                        continue;

                    uint16_t localComponent = componentCount;

                    const OpenTypeLigatureProvenance oldAssociation =
                        buffer[position].ligature;

                    if (componentGlyph.ligature.id != 0 &&
                        oldAssociation.id ==
                        componentGlyph.ligature.id &&
                        oldAssociation.component != 0)
                    {
                        localComponent =
                            std::min(
                                oldAssociation.component,
                                componentCount);
                    }

                    const uint32_t newComponent =
                        componentsBefore +
                        uint32_t(localComponent);

                    if (newComponent == 0 ||
                        newComponent >
                        std::numeric_limits<uint16_t>::max())
                    {
                        return false;
                    }

                    buffer[position].ligature.id = ligatureId;
                    buffer[position].ligature.component =
                        static_cast<uint16_t>(newComponent);
                    buffer[position].ligature.componentCount = 0;
                }

                componentsBefore += componentCount;
            }


            // ------------------------------------------------------------
            // Remap associations following a final nested ligature.
            // ------------------------------------------------------------

            const size_t lastPosition =
                match.positions.back();

            const OpenTypeShapingGlyph lastComponent =
                buffer[lastPosition];

            const uint16_t lastComponentCount =
                openTypeGsubIRLigatureComponentCount(
                    lastComponent);

            if (lastComponent.ligature.id != 0)
            {
                const uint32_t componentBase =
                    totalComponentCount -
                    lastComponentCount;

                for (size_t position = lastPosition + 1;
                    position < buffer.size();
                    ++position)
                {
                    OpenTypeLigatureProvenance& association =
                        buffer[position].ligature;

                    if (association.id !=
                        lastComponent.ligature.id ||
                        association.component == 0)
                    {
                        break;
                    }

                    const uint16_t localComponent =
                        std::min(
                            association.component,
                            lastComponentCount);

                    const uint32_t newComponent =
                        componentBase +
                        uint32_t(localComponent);

                    if (newComponent == 0 ||
                        newComponent >
                        std::numeric_limits<uint16_t>::max())
                    {
                        return false;
                    }

                    association.id = ligatureId;
                    association.component =
                        static_cast<uint16_t>(newComponent);
                    association.componentCount = 0;
                }
            }
        }


        // ------------------------------------------------------------
        // Produce the ligature glyph at the first participating position.
        // ------------------------------------------------------------

        OpenTypeShapingGlyph& output =
            buffer[match.positions[0]];

        output.glyphId = match.ligatureGlyph;
        output.scalarOffset = scalarBegin;
        output.scalarCount =
            static_cast<uint32_t>(scalarEnd - scalarBegin);
        output.ligature = outputLigature;


        // ------------------------------------------------------------
        // Remove only participating trailing components.
        //
        // Erasing backwards keeps the recorded physical indices valid.
        // Ignored glyphs between components remain in the buffer.
        // ------------------------------------------------------------

        std::vector<OpenTypeShapingGlyph>& glyphs =
            buffer.glyphs();

        for (size_t i = match.positions.size(); i > 1; --i)
        {
            glyphs.erase(
                glyphs.begin() +
                match.positions[i - 1]);
        }

        return true;
    }


    // ========================================================================
    // applyOpenTypeGsubIRLigatureLookup
    // ========================================================================

    static inline bool applyOpenTypeGsubIRLigatureLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubIRLigatureLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }

        OpenTypeShapingBuffer working = buffer;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGsubIRLigatureMatch match;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRLigatureLookupUnchecked(
                    ir, lookup, working,
                    glyphIndex, match);

            if (result == OpenTypeShapingIRResult::Invalid)
                return false;

            if (result == OpenTypeShapingIRResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            if (match.positions.empty() ||
                match.positions[0] != glyphIndex)
            {
                return false;
            }

            if (!applyOpenTypeGsubIRLigatureMatch(
                ir, working, match))
            {
                return false;
            }

            // The new ligature remains at glyphIndex. Do not apply this same
            // lookup to it again.

            ++glyphIndex;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GSUB Reverse Chain Single
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRReverseChainSingleSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubReverseChainSingleSubtable& subtable) noexcept
    {
        const size_t backtrackOffset = subtable.backtrackSetOffset;
        const size_t backtrackCount = subtable.backtrackCount;
        const size_t lookaheadOffset = subtable.lookaheadSetOffset;
        const size_t lookaheadCount = subtable.lookaheadCount;
        const size_t pairOffset = subtable.pairOffset;
        const size_t pairCount = subtable.pairCount;

        if (backtrackOffset > ir.gsubReverseChainSingleSets.size() ||
            backtrackCount > ir.gsubReverseChainSingleSets.size() - backtrackOffset ||
            lookaheadOffset > ir.gsubReverseChainSingleSets.size() ||
            lookaheadCount > ir.gsubReverseChainSingleSets.size() - lookaheadOffset ||
            pairOffset > ir.gsubReverseChainSinglePairs.size() ||
            pairCount > ir.gsubReverseChainSinglePairs.size() - pairOffset)
        {
            return false;
        }

        for (size_t i = 0; i < backtrackCount; ++i)
        {
            if (!openTypeShapingIRGlyphSetValid(
                ir, ir.gsubReverseChainSingleSets[backtrackOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookaheadCount; ++i)
        {
            if (!openTypeShapingIRGlyphSetValid(
                ir, ir.gsubReverseChainSingleSets[lookaheadOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 1; i < pairCount; ++i)
        {
            if (ir.gsubReverseChainSinglePairs[pairOffset + i - 1].input >=
                ir.gsubReverseChainSinglePairs[pairOffset + i].input)
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline bool openTypeGsubIRReverseChainSingleLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubReverseChainSingle ||
            !openTypeShapingIRLookupFilterValid(ir, lookup.filter))
        {
            return false;
        }

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubReverseChainSingleSubtables.size() ||
            count == 0 ||
            count > ir.gsubReverseChainSingleSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRReverseChainSingleSubtableValid(
                ir, ir.gsubReverseChainSingleSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    [[nodiscard]] static inline const OpenTypeShapingIRGsubReverseChainSinglePair*
        openTypeGsubIRFindReverseChainPairUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRGsubReverseChainSingleSubtable& subtable,
            uint16_t glyphId) noexcept
    {
        size_t first = subtable.pairOffset;
        size_t last = first + subtable.pairCount;

        while (first < last)
        {
            const size_t middle = first + (last - first) / 2;
            const OpenTypeShapingIRGsubReverseChainSinglePair& pair =
                ir.gsubReverseChainSinglePairs[middle];

            if (glyphId < pair.input)
                last = middle;
            else if (glyphId > pair.input)
                first = middle + 1;
            else
                return &pair;
        }

        return nullptr;
    }


    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRReverseChainSingleSubtableUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingIRGsubReverseChainSingleSubtable& subtable,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex, uint16_t& replacement) noexcept
    {
        replacement = 0;

        if (glyphIndex >= buffer.size() || buffer[glyphIndex].glyphId > 0xFFFFu)
            return OpenTypeShapingIRResult::Invalid;

        const OpenTypeShapingIRGsubReverseChainSinglePair* pair =
            openTypeGsubIRFindReverseChainPairUnchecked(
                ir, subtable, static_cast<uint16_t>(buffer[glyphIndex].glyphId));

        if (!pair)
            return OpenTypeShapingIRResult::NoMatch;

        bool member = false;
        size_t position = glyphIndex;

        // Backtrack index zero is nearest to the current glyph.
        for (uint32_t i = 0; i < subtable.backtrackCount; ++i)
        {
            size_t previousPosition = 0;

            const OpenTypeShapingIRGlyphSearchResult search =
                openTypeShapingIRLookupPrevious(
                    ir, filter, buffer, position, previousPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
                return OpenTypeShapingIRResult::NoMatch;

            const uint32_t glyphId = buffer[previousPosition].glyphId;

            if (glyphId > 0xFFFFu ||
                !openTypeShapingIRGlyphSetContains(
                    ir,
                    ir.gsubReverseChainSingleSets[subtable.backtrackSetOffset + i],
                    static_cast<uint16_t>(glyphId), member))
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            if (!member)
                return OpenTypeShapingIRResult::NoMatch;

            position = previousPosition;
        }

        position = glyphIndex;

        for (uint32_t i = 0; i < subtable.lookaheadCount; ++i)
        {
            size_t nextPosition = 0;

            const OpenTypeShapingIRGlyphSearchResult search =
                openTypeShapingIRLookupNext(
                    ir, filter, buffer, position, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
                return OpenTypeShapingIRResult::NoMatch;

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu ||
                !openTypeShapingIRGlyphSetContains(
                    ir,
                    ir.gsubReverseChainSingleSets[subtable.lookaheadSetOffset + i],
                    static_cast<uint16_t>(glyphId), member))
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            if (!member)
                return OpenTypeShapingIRResult::NoMatch;

            position = nextPosition;
        }

        replacement = pair->output;
        return OpenTypeShapingIRResult::Match;
    }


    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRReverseChainSingleLookupUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex, uint16_t& replacement) noexcept
    {
        replacement = 0;
        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRReverseChainSingleSubtableUnchecked(
                    ir, lookup.filter,
                    ir.gsubReverseChainSingleSubtables[firstSubtable + i],
                    buffer, glyphIndex, replacement);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
            {
                return result;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRReverseChainSingleLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, uint16_t& replacement) noexcept
    {
        replacement = 0;

        if (!openTypeGsubIRReverseChainSingleLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRReverseChainSingleLookupUnchecked(
            ir, lookup, buffer, glyphIndex, replacement);
    }


    static inline bool applyOpenTypeGsubIRReverseChainSingleLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubIRReverseChainSingleLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }

        OpenTypeShapingBuffer working = buffer;

        // Type 8 is defined to scan the candidate stream in reverse.
        for (size_t position = working.size(); position > 0; --position)
        {
            const size_t glyphIndex = position - 1;
            uint16_t replacement = 0;

            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRReverseChainSingleLookupUnchecked(
                    ir, lookup, working, glyphIndex, replacement);

            if (result == OpenTypeShapingIRResult::Invalid)
                return false;

            if (result == OpenTypeShapingIRResult::Match)
                working[glyphIndex].glyphId = replacement;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // GSUB Context
    // ========================================================================

    struct OpenTypeGsubIRContextMatch
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


    // ========================================================================
    // openTypeGsubIRContextRuleValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRContextRuleValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubContextRule& rule) noexcept
    {
        const size_t inputOffset = rule.inputSetOffset;
        const size_t inputCount = rule.inputCount;
        const size_t lookupOffset = rule.lookupOffset;
        const size_t lookupCount = rule.lookupCount;

        if (inputCount == 0)
            return false;

        if (inputOffset > ir.gsubContextInputSets.size() ||
            inputCount > ir.gsubContextInputSets.size() - inputOffset)
        {
            return false;
        }

        if (lookupOffset > ir.gsubContextLookups.size() ||
            lookupCount > ir.gsubContextLookups.size() - lookupOffset)
        {
            return false;
        }

        for (size_t i = 0; i < inputCount; ++i)
        {
            if (!openTypeShapingIRGlyphSetValid(
                ir, ir.gsubContextInputSets[inputOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gsubContextLookups[lookupOffset + i];

            if (action.lookup == kOpenTypeShapingIRInvalid ||
                !ir.lookup(action.lookup))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRContextSubtableValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRContextSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubContextSubtable& subtable) noexcept
    {
        const size_t offset = subtable.ruleOffset;
        const size_t count = subtable.ruleCount;

        if (offset > ir.gsubContextRules.size() ||
            count > ir.gsubContextRules.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRContextRuleValid(
                ir, ir.gsubContextRules[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRContextLookupValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRContextLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubContext)
            return false;

        if (!openTypeShapingIRLookupFilterValid(ir, lookup.filter))
            return false;

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubContextSubtables.size() ||
            count == 0 ||
            count > ir.gsubContextSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRContextSubtableValid(
                ir, ir.gsubContextSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // matchOpenTypeGsubIRContextRuleUnchecked
    //
    // Position 0 is the current glyph and is matched directly. Positions
    // 1..N-1 are reached through filtered forward traversal.
    // ========================================================================

    static inline OpenTypeShapingIRResult
        matchOpenTypeGsubIRContextRuleUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingIRGsubContextRule& rule,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRContextMatch& match) noexcept
    {
        match.clear();

        if (glyphIndex >= buffer.size() || rule.inputCount == 0)
            return OpenTypeShapingIRResult::Invalid;

        const uint32_t firstGlyph = buffer[glyphIndex].glyphId;

        if (firstGlyph > 0xFFFFu)
            return OpenTypeShapingIRResult::Invalid;

        bool member = false;

        if (!openTypeShapingIRGlyphSetContains(
            ir,
            ir.gsubContextInputSets[rule.inputSetOffset],
            static_cast<uint16_t>(firstGlyph),
            member))
        {
            return OpenTypeShapingIRResult::Invalid;
        }

        if (!member)
            return OpenTypeShapingIRResult::NoMatch;

        match.positions.reserve(rule.inputCount);
        match.positions.push_back(glyphIndex);

        size_t position = glyphIndex;

        for (uint32_t sequenceIndex = 1;
            sequenceIndex < rule.inputCount;
            ++sequenceIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeShapingIRGlyphSearchResult search =
                openTypeShapingIRLookupNext(
                    ir, filter, buffer, position, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            const uint32_t glyphId =
                buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeShapingIRResult::Invalid;

            if (!openTypeShapingIRGlyphSetContains(
                ir,
                ir.gsubContextInputSets[
                    rule.inputSetOffset + sequenceIndex],
                    static_cast<uint16_t>(glyphId),
                    member))
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            if (!member)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            match.positions.push_back(nextPosition);
            position = nextPosition;
        }

        match.lookupOffset = rule.lookupOffset;
        match.lookupCount = rule.lookupCount;

        return OpenTypeShapingIRResult::Match;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRContextSubtableUnchecked
    //
    // Rules remain in source/design order. First complete match wins.
    // ========================================================================

    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRContextSubtableUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingIRGsubContextSubtable& subtable,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRContextMatch& match) noexcept
    {
        match.clear();

        for (size_t i = 0; i < subtable.ruleCount; ++i)
        {
            const OpenTypeShapingIRGsubContextRule& rule =
                ir.gsubContextRules[subtable.ruleOffset + i];

            const OpenTypeShapingIRResult result =
                matchOpenTypeGsubIRContextRuleUnchecked(
                    ir, filter, rule, buffer, glyphIndex, match);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
            {
                return result;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRContextLookupUnchecked
    //
    // Subtables remain in source order. First matching subtable wins.
    // ========================================================================

    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRContextLookupUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRContextMatch& match) noexcept
    {
        match.clear();

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRContextSubtableUnchecked(
                    ir,
                    lookup.filter,
                    ir.gsubContextSubtables[firstSubtable + i],
                    buffer,
                    glyphIndex,
                    match);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
            {
                return result;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRContextLookup(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRContextMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGsubIRContextLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRContextLookupUnchecked(
            ir, lookup, buffer, glyphIndex, match);
    }


    // ========================================================================
    // GSUB Chain Context
    // ========================================================================

    struct OpenTypeGsubIRChainContextMatch
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


    // ========================================================================
    // openTypeGsubIRChainContextRuleValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRChainContextRuleValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubChainContextRule& rule) noexcept
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

        if (backtrackOffset > ir.gsubChainContextSets.size() ||
            backtrackCount > ir.gsubChainContextSets.size() - backtrackOffset)
        {
            return false;
        }

        if (inputOffset > ir.gsubChainContextSets.size() ||
            inputCount > ir.gsubChainContextSets.size() - inputOffset)
        {
            return false;
        }

        if (lookaheadOffset > ir.gsubChainContextSets.size() ||
            lookaheadCount > ir.gsubChainContextSets.size() - lookaheadOffset)
        {
            return false;
        }

        if (lookupOffset > ir.gsubChainContextLookups.size() ||
            lookupCount > ir.gsubChainContextLookups.size() - lookupOffset)
        {
            return false;
        }

        for (size_t i = 0; i < backtrackCount; ++i)
        {
            if (!openTypeShapingIRGlyphSetValid(
                ir, ir.gsubChainContextSets[backtrackOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < inputCount; ++i)
        {
            if (!openTypeShapingIRGlyphSetValid(
                ir, ir.gsubChainContextSets[inputOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookaheadCount; ++i)
        {
            if (!openTypeShapingIRGlyphSetValid(
                ir, ir.gsubChainContextSets[lookaheadOffset + i]))
            {
                return false;
            }
        }

        for (size_t i = 0; i < lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gsubChainContextLookups[lookupOffset + i];

            if (action.lookup == kOpenTypeShapingIRInvalid ||
                !ir.lookup(action.lookup))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRChainContextSubtableValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRChainContextSubtableValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubChainContextSubtable& subtable) noexcept
    {
        const size_t offset = subtable.ruleOffset;
        const size_t count = subtable.ruleCount;

        if (offset > ir.gsubChainContextRules.size() ||
            count > ir.gsubChainContextRules.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRChainContextRuleValid(
                ir, ir.gsubChainContextRules[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRChainContextLookupValid
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIRChainContextLookupValid(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup) noexcept
    {
        if (lookup.op != OpenTypeShapingIROp::GsubChainContext)
            return false;

        if (!openTypeShapingIRLookupFilterValid(ir, lookup.filter))
            return false;

        const size_t offset = lookup.payloadOffset;
        const size_t count = lookup.payloadCount;

        if (offset > ir.gsubChainContextSubtables.size() ||
            count == 0 ||
            count > ir.gsubChainContextSubtables.size() - offset)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (!openTypeGsubIRChainContextSubtableValid(
                ir, ir.gsubChainContextSubtables[offset + i]))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // matchOpenTypeGsubIRChainContextRuleUnchecked
    //
    // Backtrack set index 0 is nearest to the current glyph. Input position 0
    // is the current glyph and is matched directly. Lookahead begins after the
    // final matched input position. LookupFlag filtering is used only while
    // traversing away from the current/input positions.
    // ========================================================================

    static inline OpenTypeShapingIRResult
        matchOpenTypeGsubIRChainContextRuleUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingIRGsubChainContextRule& rule,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRChainContextMatch& match) noexcept
    {
        match.clear();

        if (glyphIndex >= buffer.size() || rule.inputCount == 0)
            return OpenTypeShapingIRResult::Invalid;

        const uint32_t firstGlyph = buffer[glyphIndex].glyphId;

        if (firstGlyph > 0xFFFFu)
            return OpenTypeShapingIRResult::Invalid;

        bool member = false;

        if (!openTypeShapingIRGlyphSetContains(
            ir,
            ir.gsubChainContextSets[rule.inputSetOffset],
            static_cast<uint16_t>(firstGlyph), member))
        {
            return OpenTypeShapingIRResult::Invalid;
        }

        if (!member)
            return OpenTypeShapingIRResult::NoMatch;

        match.inputPositions.reserve(rule.inputCount);
        match.inputPositions.push_back(glyphIndex);

        // Backtrack, nearest first.
        size_t position = glyphIndex;

        for (uint32_t backtrackIndex = 0;
            backtrackIndex < rule.backtrackCount;
            ++backtrackIndex)
        {
            size_t previousPosition = 0;

            const OpenTypeShapingIRGlyphSearchResult search =
                openTypeShapingIRLookupPrevious(
                    ir, filter, buffer, position, previousPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            const uint32_t glyphId = buffer[previousPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeShapingIRResult::Invalid;

            if (!openTypeShapingIRGlyphSetContains(
                ir,
                ir.gsubChainContextSets[
                    rule.backtrackSetOffset + backtrackIndex],
                    static_cast<uint16_t>(glyphId), member))
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            if (!member)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            position = previousPosition;
        }

        // Remaining input positions.
        position = glyphIndex;

        for (uint32_t inputIndex = 1;
            inputIndex < rule.inputCount;
            ++inputIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeShapingIRGlyphSearchResult search =
                openTypeShapingIRLookupNext(
                    ir, filter, buffer, position, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeShapingIRResult::Invalid;

            if (!openTypeShapingIRGlyphSetContains(
                ir,
                ir.gsubChainContextSets[
                    rule.inputSetOffset + inputIndex],
                    static_cast<uint16_t>(glyphId), member))
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            if (!member)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            match.inputPositions.push_back(nextPosition);
            position = nextPosition;
        }

        // Lookahead begins after the final input position.
        position = match.inputPositions.back();

        for (uint32_t lookaheadIndex = 0;
            lookaheadIndex < rule.lookaheadCount;
            ++lookaheadIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeShapingIRGlyphSearchResult search =
                openTypeShapingIRLookupNext(
                    ir, filter, buffer, position, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeShapingIRResult::Invalid;

            if (!openTypeShapingIRGlyphSetContains(
                ir,
                ir.gsubChainContextSets[
                    rule.lookaheadSetOffset + lookaheadIndex],
                    static_cast<uint16_t>(glyphId), member))
            {
                return OpenTypeShapingIRResult::Invalid;
            }

            if (!member)
            {
                match.clear();
                return OpenTypeShapingIRResult::NoMatch;
            }

            position = nextPosition;
        }

        match.lookupOffset = rule.lookupOffset;
        match.lookupCount = rule.lookupCount;

        return OpenTypeShapingIRResult::Match;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRChainContextSubtableUnchecked
    // ========================================================================

    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRChainContextSubtableUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookupFilter& filter,
            const OpenTypeShapingIRGsubChainContextSubtable& subtable,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRChainContextMatch& match) noexcept
    {
        match.clear();

        for (size_t i = 0; i < subtable.ruleCount; ++i)
        {
            const OpenTypeShapingIRGsubChainContextRule& rule =
                ir.gsubChainContextRules[subtable.ruleOffset + i];

            const OpenTypeShapingIRResult result =
                matchOpenTypeGsubIRChainContextRuleUnchecked(
                    ir, filter, rule, buffer, glyphIndex, match);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
            {
                return result;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    // ========================================================================
    // resolveOpenTypeGsubIRChainContextLookupUnchecked
    // ========================================================================

    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRChainContextLookupUnchecked(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRChainContextMatch& match) noexcept
    {
        match.clear();

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRResult result =
                resolveOpenTypeGsubIRChainContextSubtableUnchecked(
                    ir,
                    lookup.filter,
                    ir.gsubChainContextSubtables[firstSubtable + i],
                    buffer,
                    glyphIndex,
                    match);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
            {
                return result;
            }
        }

        return OpenTypeShapingIRResult::NoMatch;
    }


    static inline OpenTypeShapingIRResult
        resolveOpenTypeGsubIRChainContextLookup(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            const OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubIRChainContextMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGsubIRChainContextLookupValid(ir, lookup))
            return OpenTypeShapingIRResult::Invalid;

        return resolveOpenTypeGsubIRChainContextLookupUnchecked(
            ir, lookup, buffer, glyphIndex, match);
    }


    // ========================================================================
    // IR nested execution
    // ========================================================================

    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRLookupAt(
            const OpenTypeShapingIR& ir,
            OpenTypeShapingIRLookupId lookupId,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubApplyState& state,
            OpenTypeGsubEditLog& edits);


    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRSingleAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubEditLog& edits)
    {
        if (!openTypeGsubIRSingleLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size() ||
            buffer[glyphIndex].glyphId > 0xFFFFu)
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        uint16_t replacement = 0;

        const OpenTypeShapingIRResult result =
            resolveOpenTypeGsubIRSingleLookupUnchecked(
                ir, lookup,
                static_cast<uint16_t>(buffer[glyphIndex].glyphId),
                replacement);

        if (result == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (result != OpenTypeShapingIRResult::Match)
            return OpenTypeGsubApplyAtResult::Invalid;

        buffer[glyphIndex].glyphId = replacement;

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = 1;
        edits.push_back(std::move(edit));

        return OpenTypeGsubApplyAtResult::Match;
    }


    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRMultipleAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubEditLog& edits)
    {
        if (!openTypeGsubIRMultipleLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size() ||
            buffer[glyphIndex].glyphId > 0xFFFFu)
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        uint32_t sequenceIndex =
            kOpenTypeShapingIRInvalid;

        const OpenTypeShapingIRResult result =
            resolveOpenTypeGsubIRMultipleLookupUnchecked(
                ir, lookup,
                static_cast<uint16_t>(buffer[glyphIndex].glyphId),
                sequenceIndex);

        if (result == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (result != OpenTypeShapingIRResult::Match ||
            sequenceIndex >= ir.gsubMultipleSequences.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        const size_t outputCount =
            ir.gsubMultipleSequences[sequenceIndex].glyphCount;

        if (outputCount == 0 ||
            !applyOpenTypeGsubIRMultipleSequence(
                ir, sequenceIndex, buffer, glyphIndex))
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = outputCount;
        edits.push_back(std::move(edit));

        return OpenTypeGsubApplyAtResult::Match;
    }


    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRLigatureAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubEditLog& edits)
    {
        if (!openTypeGsubIRLigatureLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubIRLigatureMatch match;

        const OpenTypeShapingIRResult result =
            resolveOpenTypeGsubIRLigatureLookupUnchecked(
                ir, lookup, buffer, glyphIndex, match);

        if (result == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (result != OpenTypeShapingIRResult::Match ||
            match.positions.empty() ||
            match.positions[0] != glyphIndex)
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubEdit edit;
        edit.inputPositions = match.positions;
        edit.outputCount = 1;

        if (!applyOpenTypeGsubIRLigatureMatch(
            ir, buffer, match))
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }



    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRAlternateAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubEditLog& edits)
    {
        if (!openTypeGsubIRAlternateLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size() || buffer[glyphIndex].glyphId > 0xFFFFu)
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        uint16_t replacement = 0;

        const OpenTypeShapingIRResult result =
            resolveOpenTypeGsubIRAlternateLookupUnchecked(
                ir, lookup,
                static_cast<uint16_t>(buffer[glyphIndex].glyphId),
                replacement);

        if (result == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (result != OpenTypeShapingIRResult::Match)
            return OpenTypeGsubApplyAtResult::Invalid;

        buffer[glyphIndex].glyphId = replacement;

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = 1;
        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }


    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRReverseChainSingleAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubEditLog& edits)
    {
        if (!openTypeGsubIRReverseChainSingleLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        uint16_t replacement = 0;

        const OpenTypeShapingIRResult result =
            resolveOpenTypeGsubIRReverseChainSingleLookupUnchecked(
                ir, lookup, buffer, glyphIndex, replacement);

        if (result == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (result != OpenTypeShapingIRResult::Match)
            return OpenTypeGsubApplyAtResult::Invalid;

        buffer[glyphIndex].glyphId = replacement;

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = 1;
        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }


    // ========================================================================
    // openTypeGsubIRAdjustBoundaryForEdit
    //
    // Keep the one-past matched-input boundary synchronized with atomic edits.
    // ========================================================================

    static inline bool openTypeGsubIRAdjustBoundaryForEdit(
        size_t& boundary, const OpenTypeGsubEdit& edit) noexcept
    {
        if (!edit)
            return false;

        const size_t oldBoundary = boundary;
        const size_t insertedCount = edit.outputCount - 1;
        size_t removedBeforeBoundary = 0;

        for (size_t i = 1; i < edit.inputPositions.size(); ++i)
        {
            if (edit.inputPositions[i] < oldBoundary)
                ++removedBeforeBoundary;
        }

        size_t newBoundary = oldBoundary;

        if (edit.anchor() < oldBoundary)
        {
            if (insertedCount >
                std::numeric_limits<size_t>::max() - newBoundary)
            {
                return false;
            }

            newBoundary += insertedCount;
        }

        if (removedBeforeBoundary > newBoundary)
            return false;

        newBoundary -= removedBeforeBoundary;
        boundary = newBoundary;

        return true;
    }


    // ========================================================================
    // applyOpenTypeGsubIRContextAt
    //
    // Match first, then execute contextual actions in stored order against the
    // CURRENT mutable input sequence.
    // ========================================================================

    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRContextAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubApplyState& state,
            OpenTypeGsubEditLog& edits,
            size_t* resumeIndex = nullptr)
    {
        if (!openTypeGsubIRContextLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubIRContextMatch match;

        const OpenTypeShapingIRResult matchResult =
            resolveOpenTypeGsubIRContextLookupUnchecked(
                ir, lookup, buffer, glyphIndex, match);

        if (matchResult == OpenTypeShapingIRResult::Invalid)
            return OpenTypeGsubApplyAtResult::Invalid;

        if (matchResult == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (match.positions.empty() ||
            match.positions[0] != glyphIndex ||
            match.positions.back() ==
            std::numeric_limits<size_t>::max())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubSequenceState sequence;

        if (!sequence.reset(
            match.positions.data(),
            match.positions.size()))
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        size_t boundary =
            match.positions.back() + 1;

        for (size_t i = 0; i < match.lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gsubContextLookups[
                    match.lookupOffset + i];

            size_t targetPosition = 0;

            // The source format is allowed to encode an index that only
            // becomes meaningful after preceding 1 -> N substitutions.
            if (!sequence.position(
                action.sequenceIndex,
                targetPosition))
            {
                continue;
            }

            if (targetPosition >= buffer.size())
                return OpenTypeGsubApplyAtResult::Invalid;

            const size_t firstNewEdit =
                edits.size();

            const OpenTypeGsubApplyAtResult nestedResult =
                applyOpenTypeGsubIRLookupAt(
                    ir,
                    action.lookup,
                    buffer,
                    targetPosition,
                    state,
                    edits);

            if (nestedResult ==
                OpenTypeGsubApplyAtResult::Invalid)
            {
                return OpenTypeGsubApplyAtResult::Invalid;
            }

            if (nestedResult ==
                OpenTypeGsubApplyAtResult::NoMatch)
            {
                if (edits.size() != firstNewEdit)
                    return OpenTypeGsubApplyAtResult::Invalid;

                continue;
            }

            for (size_t editIndex = firstNewEdit;
                editIndex < edits.size();
                ++editIndex)
            {
                if (!openTypeGsubIRAdjustBoundaryForEdit(
                    boundary, edits[editIndex]))
                {
                    return OpenTypeGsubApplyAtResult::Invalid;
                }

                if (!sequence.applyEdit(
                    edits[editIndex]))
                {
                    return OpenTypeGsubApplyAtResult::Invalid;
                }
            }
        }

        if (boundary > buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        if (resumeIndex)
            *resumeIndex = boundary;

        // A context with zero effective substitutions is still a match.
        return OpenTypeGsubApplyAtResult::Match;
    }


    // ========================================================================
    // applyOpenTypeGsubIRChainContextAt
    //
    // Backtrack and lookahead are match-only constraints. SequenceLookup
    // actions operate only on the mutable matched input sequence.
    // ========================================================================

    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRChainContextAt(
            const OpenTypeShapingIR& ir,
            const OpenTypeShapingIRLookup& lookup,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubApplyState& state,
            OpenTypeGsubEditLog& edits,
            size_t* resumeIndex = nullptr)
    {
        if (!openTypeGsubIRChainContextLookupValid(ir, lookup) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubIRChainContextMatch match;

        const OpenTypeShapingIRResult matchResult =
            resolveOpenTypeGsubIRChainContextLookupUnchecked(
                ir, lookup, buffer, glyphIndex, match);

        if (matchResult == OpenTypeShapingIRResult::Invalid)
            return OpenTypeGsubApplyAtResult::Invalid;

        if (matchResult == OpenTypeShapingIRResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (match.inputPositions.empty() ||
            match.inputPositions[0] != glyphIndex ||
            match.inputPositions.back() == std::numeric_limits<size_t>::max())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        OpenTypeGsubSequenceState sequence;

        if (!sequence.reset(
            match.inputPositions.data(),
            match.inputPositions.size()))
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        size_t boundary = match.inputPositions.back() + 1;

        for (size_t i = 0; i < match.lookupCount; ++i)
        {
            const OpenTypeShapingIRSequenceLookup& action =
                ir.gsubChainContextLookups[
                    match.lookupOffset + i];

            size_t targetPosition = 0;

            // sequenceIndex is interpreted against the current mutable input
            // sequence after edits from preceding nested actions.
            if (!sequence.position(action.sequenceIndex, targetPosition))
                continue;

            if (targetPosition >= buffer.size())
                return OpenTypeGsubApplyAtResult::Invalid;

            const size_t firstNewEdit = edits.size();

            const OpenTypeGsubApplyAtResult nestedResult =
                applyOpenTypeGsubIRLookupAt(
                    ir,
                    action.lookup,
                    buffer,
                    targetPosition,
                    state,
                    edits);

            if (nestedResult == OpenTypeGsubApplyAtResult::Invalid)
                return OpenTypeGsubApplyAtResult::Invalid;

            if (nestedResult == OpenTypeGsubApplyAtResult::NoMatch)
            {
                if (edits.size() != firstNewEdit)
                    return OpenTypeGsubApplyAtResult::Invalid;

                continue;
            }

            for (size_t editIndex = firstNewEdit;
                editIndex < edits.size();
                ++editIndex)
            {
                if (!openTypeGsubIRAdjustBoundaryForEdit(
                    boundary, edits[editIndex]))
                {
                    return OpenTypeGsubApplyAtResult::Invalid;
                }

                if (!sequence.applyEdit(edits[editIndex]))
                    return OpenTypeGsubApplyAtResult::Invalid;
            }
        }

        if (boundary > buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        if (resumeIndex)
            *resumeIndex = boundary;

        // A chaining rule matching with zero effective substitutions is still
        // a Match. Backtrack and lookahead remain match-only constraints.
        return OpenTypeGsubApplyAtResult::Match;
    }


    // ========================================================================
    // applyOpenTypeGsubIRLookupAt
    //
    // Apply one compiled lookup exactly at one physical position.
    // ========================================================================

    static inline OpenTypeGsubApplyAtResult
        applyOpenTypeGsubIRLookupAt(
            const OpenTypeShapingIR& ir,
            OpenTypeShapingIRLookupId lookupId,
            OpenTypeShapingBuffer& buffer,
            size_t glyphIndex,
            OpenTypeGsubApplyState& state,
            OpenTypeGsubEditLog& edits)
    {
        if (glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        const OpenTypeShapingIRLookup* lookup =
            ir.lookup(lookupId);

        if (!lookup)
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubApplyScope scope(state);

        if (!scope || !state.consumeOperation())
            return OpenTypeGsubApplyAtResult::Invalid;

        switch (lookup->op)
        {
        case OpenTypeShapingIROp::GsubSingle:
            return applyOpenTypeGsubIRSingleAt(
                ir, *lookup, buffer, glyphIndex, edits);

        case OpenTypeShapingIROp::GsubMultiple:
            return applyOpenTypeGsubIRMultipleAt(
                ir, *lookup, buffer, glyphIndex, edits);

        case OpenTypeShapingIROp::GsubAlternate:
            return applyOpenTypeGsubIRAlternateAt(
                ir, *lookup, buffer, glyphIndex, edits);

        case OpenTypeShapingIROp::GsubLigature:
            return applyOpenTypeGsubIRLigatureAt(
                ir, *lookup, buffer, glyphIndex, edits);

        case OpenTypeShapingIROp::GsubContext:
            return applyOpenTypeGsubIRContextAt(
                ir, *lookup, buffer, glyphIndex,
                state, edits);

        case OpenTypeShapingIROp::GsubChainContext:
            return applyOpenTypeGsubIRChainContextAt(
                ir, *lookup, buffer, glyphIndex,
                state, edits);

        case OpenTypeShapingIROp::GsubReverseChainSingle:
            return applyOpenTypeGsubIRReverseChainSingleAt(
                ir, *lookup, buffer, glyphIndex, edits);

        default:
            return OpenTypeGsubApplyAtResult::Invalid;
        }
    }


    // ========================================================================
    // applyOpenTypeGsubIRContextLookup
    //
    // Complete Type 5 lookup application is transactional. Nested actions
    // mutate the local working buffer directly; failure discards it.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubIRContextLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }

        OpenTypeShapingBuffer working = buffer;
        OpenTypeGsubApplyState state;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGsubApplyScope scope(state);

            if (!scope || !state.consumeOperation())
                return false;

            OpenTypeGsubEditLog edits;
            size_t resumeIndex = glyphIndex + 1;

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubIRContextAt(
                    ir, lookup, working, glyphIndex,
                    state, edits, &resumeIndex);

            if (result == OpenTypeGsubApplyAtResult::Invalid)
                return false;

            if (result == OpenTypeGsubApplyAtResult::NoMatch)
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
    // applyOpenTypeGsubIRChainContextLookup
    //
    // Complete Type 6 lookup application is transactional. After a successful
    // match, scanning resumes immediately after the mapped current input
    // sequence. Backtrack is already behind the scan point and lookahead remains
    // eligible unless a nested substitution actually consumed it.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRChainContextLookup(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubIRChainContextLookupValid(ir, lookup) ||
            !openTypeGsubIRBufferGlyphIdsValid(buffer))
        {
            return false;
        }

        OpenTypeShapingBuffer working = buffer;
        OpenTypeGsubApplyState state;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGsubApplyScope scope(state);

            if (!scope || !state.consumeOperation())
                return false;

            OpenTypeGsubEditLog edits;
            size_t resumeIndex = glyphIndex + 1;

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubIRChainContextAt(
                    ir, lookup, working, glyphIndex,
                    state, edits, &resumeIndex);

            if (result == OpenTypeGsubApplyAtResult::Invalid)
                return false;

            if (result == OpenTypeGsubApplyAtResult::NoMatch)
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
    // applyOpenTypeGsubIRLookup
    //
    // Generic semantic GSUB IR dispatch point.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRLookup(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId lookupId,
        OpenTypeShapingBuffer& buffer)
    {
        const OpenTypeShapingIRLookup* lookup =
            ir.lookup(lookupId);

        if (!lookup)
            return false;

        switch (lookup->op)
        {
        case OpenTypeShapingIROp::GsubSingle:
            return applyOpenTypeGsubIRSingleLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GsubMultiple:
            return applyOpenTypeGsubIRMultipleLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GsubAlternate:
            return applyOpenTypeGsubIRAlternateLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GsubLigature:
            return applyOpenTypeGsubIRLigatureLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GsubContext:
            return applyOpenTypeGsubIRContextLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GsubChainContext:
            return applyOpenTypeGsubIRChainContextLookup(
                ir, *lookup, buffer);

        case OpenTypeShapingIROp::GsubReverseChainSingle:
            return applyOpenTypeGsubIRReverseChainSingleLookup(
                ir, *lookup, buffer);

        default:
            return false;
        }
    }

} // namespace waavs
