// opentype_gsub_reverse_chain_single_match.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_gsub_reverse_chain_single_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_shaping_buffer.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGsubReverseChainSingleMatchResult
    // ====================================================================

    enum class OpenTypeGsubReverseChainSingleMatchResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGsubReverseChainSingleMatch
    //
    // Type 8 has exactly one actionable input glyph. Backtrack and
    // lookahead positions are match-only and need not be retained.
    // ====================================================================

    struct OpenTypeGsubReverseChainSingleMatch
    {
        uint16_t substituteGlyph{ 0 };

        void clear() noexcept { substituteGlyph = 0; }
    };


    // ====================================================================
    // Filtered traversal helpers.
    //
    // The current input glyph is never filtered. LookupFlag filtering is
    // used only while traversing backtrack and lookahead context.
    // ====================================================================

    static inline OpenTypeGsubReverseChainSingleMatchResult
        openTypeGsubReverseChainSinglePrevious(
            const OpenTypeLookupGlyphFilter& filter,
            const OpenTypeShapingBuffer& buffer,
            size_t currentIndex, size_t& previousIndex) noexcept
    {
        previousIndex = 0;

        const OpenTypeLookupGlyphSearchResult result =
            filter.previous(buffer, currentIndex, previousIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGsubReverseChainSingleMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGsubReverseChainSingleMatchResult::NoMatch;

        default:
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;
        }
    }


    static inline OpenTypeGsubReverseChainSingleMatchResult
        openTypeGsubReverseChainSingleNext(
            const OpenTypeLookupGlyphFilter& filter,
            const OpenTypeShapingBuffer& buffer,
            size_t currentIndex, size_t& nextIndex) noexcept
    {
        nextIndex = 0;

        const OpenTypeLookupGlyphSearchResult result =
            filter.next(buffer, currentIndex, nextIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGsubReverseChainSingleMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGsubReverseChainSingleMatchResult::NoMatch;

        default:
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;
        }
    }


    // ====================================================================
    // matchOpenTypeGsubReverseChainSingleSubst
    //
    // Match one Type 8 Format 1 subtable at one physical buffer position.
    //
    // Logical context:
    //
    //     Bn ... B1 B0 | I | L0 L1 ... Ln
    //
    // Encoded Coverage arrays:
    //
    //     backtrack[0] -> B0, nearest previous
    //     backtrack[1] -> B1
    //
    //     lookahead[0] -> L0, nearest following
    //     lookahead[1] -> L1
    //
    // The input Coverage index selects substituteGlyphIDs[index].
    //
    // No mutation occurs here. Reverse whole-buffer traversal belongs to
    // the Type 8 executor, not this exact-position matcher.
    // ====================================================================

    static inline OpenTypeGsubReverseChainSingleMatchResult
        matchOpenTypeGsubReverseChainSingleSubst(
            const OpenTypeGsubReverseChainSingleSubstView& subst,
            const OpenTypeLookupGlyphFilter& filter,
            const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
            OpenTypeGsubReverseChainSingleMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 1 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

        const uint32_t inputGlyphId = buffer[glyphIndex].glyphId;

        if (inputGlyphId > 0xFFFFu)
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;


        // ------------------------------------------------------------
        // Input.
        //
        // The current glyph is matched directly and is never filtered.
        // ------------------------------------------------------------

        const OpenTypeCoverageView inputCoverage = subst.coverage();

        if (!inputCoverage)
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

        uint16_t inputCoverageIndex = 0;

        if (!inputCoverage.find(inputGlyphId, inputCoverageIndex))
            return OpenTypeGsubReverseChainSingleMatchResult::NoMatch;


        // The specification requires one substitute per input Coverage
        // entry. Detect the malformed case that affects this candidate.

        if (inputCoverageIndex >= subst.glyphCount())
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;


        // ------------------------------------------------------------
        // Backtrack.
        //
        // Index zero is the nearest logical previous glyph.
        // ------------------------------------------------------------

        size_t position = glyphIndex;

        for (uint16_t backtrackIndex = 0;
            backtrackIndex < subst.backtrackGlyphCount();
            ++backtrackIndex)
        {
            size_t previousPosition = 0;

            const OpenTypeGsubReverseChainSingleMatchResult previousResult =
                openTypeGsubReverseChainSinglePrevious(
                    filter, buffer, position, previousPosition);

            if (previousResult != OpenTypeGsubReverseChainSingleMatchResult::Match)
                return previousResult;

            const uint32_t glyphId = buffer[previousPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

            const OpenTypeCoverageView coverage =
                subst.backtrackCoverage(backtrackIndex);

            if (!coverage)
                return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGsubReverseChainSingleMatchResult::NoMatch;

            position = previousPosition;
        }


        // ------------------------------------------------------------
        // Lookahead.
        //
        // Index zero is the nearest logical following glyph.
        // ------------------------------------------------------------

        position = glyphIndex;

        for (uint16_t lookaheadIndex = 0;
            lookaheadIndex < subst.lookaheadGlyphCount();
            ++lookaheadIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGsubReverseChainSingleMatchResult nextResult =
                openTypeGsubReverseChainSingleNext(
                    filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGsubReverseChainSingleMatchResult::Match)
                return nextResult;

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

            const OpenTypeCoverageView coverage =
                subst.lookaheadCoverage(lookaheadIndex);

            if (!coverage)
                return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGsubReverseChainSingleMatchResult::NoMatch;

            position = nextPosition;
        }


        // ------------------------------------------------------------
        // Complete match.
        //
        // Resolve the replacement only after the entire context matched.
        // ------------------------------------------------------------

        uint16_t substituteGlyph = 0;

        if (!subst.substituteGlyphId(inputCoverageIndex, substituteGlyph))
            return OpenTypeGsubReverseChainSingleMatchResult::Invalid;

        match.substituteGlyph = substituteGlyph;
        return OpenTypeGsubReverseChainSingleMatchResult::Match;
    }

} // namespace waavs