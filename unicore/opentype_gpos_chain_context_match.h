// opentype_gpos_chain_context_match.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_gpos_chain_context_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_sequence_lookup.h"
#include "shaped_glyph_buffer.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposChainContextMatchResult
    // ====================================================================

    enum class OpenTypeGposChainContextMatchResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGposChainContextMatch
    //
    // inputPositions:
    //   Physical ShapedGlyphBuffer positions corresponding to logical input
    //   sequence positions 0..N-1.
    //
    // Backtrack and lookahead positions are match-only constraints.
    //
    // lookups:
    //   SequenceLookup records in stored/design order.
    //
    // GPOS never changes glyph-buffer topology, so inputPositions remain
    // stable during later Type 8 execution.
    // ====================================================================

    struct OpenTypeGposChainContextMatch
    {
        std::vector<size_t> inputPositions{};
        std::vector<OpenTypeSequenceLookup> lookups{};

        void clear() noexcept
        {
            inputPositions.clear();
            lookups.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return inputPositions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return inputPositions.size(); }
    };


    // ====================================================================
    // Filtered traversal.
    //
    // The current/start glyph is never filtered.
    // ====================================================================

    static inline OpenTypeGposChainContextMatchResult openTypeGposChainContextNext(
        const OpenTypeLookupGlyphFilter& filter, const ShapedGlyphBuffer& buffer,
        size_t currentIndex, size_t& nextIndex) noexcept
    {
        nextIndex = 0;

        const OpenTypeLookupGlyphSearchResult result = filter.next(buffer, currentIndex, nextIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGposChainContextMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGposChainContextMatchResult::NoMatch;

        default:
            return OpenTypeGposChainContextMatchResult::Invalid;
        }
    }


    static inline OpenTypeGposChainContextMatchResult openTypeGposChainContextPrevious(
        const OpenTypeLookupGlyphFilter& filter, const ShapedGlyphBuffer& buffer,
        size_t currentIndex, size_t& previousIndex) noexcept
    {
        previousIndex = 0;

        const OpenTypeLookupGlyphSearchResult result = filter.previous(buffer, currentIndex, previousIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGposChainContextMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGposChainContextMatchResult::NoMatch;

        default:
            return OpenTypeGposChainContextMatchResult::Invalid;
        }
    }


    // ====================================================================
    // Format 1.
    // ====================================================================

    static inline OpenTypeGposChainContextMatchResult matchOpenTypeGposChainContextFormat1(
        const OpenTypeGposChainContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposChainContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || pos.format() != 1 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGposChainContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposChainContextMatchResult::Invalid;

        const OpenTypeCoverageView coverage = pos.coverage();

        if (!coverage)
            return OpenTypeGposChainContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposChainContextMatchResult::NoMatch;

        if (coverageIndex >= pos.ruleSetCount())
            return OpenTypeGposChainContextMatchResult::NoMatch;

        uint16_t ruleSetOffset = 0;

        if (!pos.ruleSetOffset(coverageIndex, ruleSetOffset))
            return OpenTypeGposChainContextMatchResult::Invalid;

        if (ruleSetOffset == 0)
            return OpenTypeGposChainContextMatchResult::NoMatch;

        const OpenTypeGposChainContextRuleSetView set = pos.ruleSet(coverageIndex);

        if (!set)
            return OpenTypeGposChainContextMatchResult::Invalid;


        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGposChainContextRuleView rule = set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGposChainContextMatchResult::Invalid;

            const uint16_t backtrackCount = rule.backtrackGlyphCount();
            const uint16_t inputCount = rule.inputGlyphCount();
            const uint16_t lookaheadCount = rule.lookaheadGlyphCount();

            if (inputCount == 0)
                return OpenTypeGposChainContextMatchResult::Invalid;

            OpenTypeGposChainContextMatch candidate;
            candidate.inputPositions.reserve(inputCount);
            candidate.inputPositions.push_back(glyphIndex);

            bool matched = true;


            // --------------------------------------------------------
            // Backtrack.
            //
            // Stored nearest-first.
            // --------------------------------------------------------

            size_t position = glyphIndex;

            for (uint16_t backtrackIndex = 0; backtrackIndex < backtrackCount; ++backtrackIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.backtrackGlyphId(backtrackIndex, expectedGlyphId))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                size_t previousPosition = 0;

                const OpenTypeGposChainContextMatchResult previousResult =
                    openTypeGposChainContextPrevious(filter, buffer, position, previousPosition);

                if (previousResult == OpenTypeGposChainContextMatchResult::Invalid)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (previousResult == OpenTypeGposChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[previousPosition].shaping.glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (actualGlyphId != expectedGlyphId)
                {
                    matched = false;
                    break;
                }

                position = previousPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Input.
            // --------------------------------------------------------

            position = glyphIndex;

            for (uint16_t inputIndex = 1; inputIndex < inputCount; ++inputIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.inputGlyphId(inputIndex - 1, expectedGlyphId))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGposChainContextMatchResult nextResult =
                    openTypeGposChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGposChainContextMatchResult::Invalid)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGposChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[nextPosition].shaping.glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (actualGlyphId != expectedGlyphId)
                {
                    matched = false;
                    break;
                }

                candidate.inputPositions.push_back(nextPosition);
                position = nextPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Lookahead.
            // --------------------------------------------------------

            position = candidate.inputPositions.back();

            for (uint16_t lookaheadIndex = 0; lookaheadIndex < lookaheadCount; ++lookaheadIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.lookaheadGlyphId(lookaheadIndex, expectedGlyphId))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGposChainContextMatchResult nextResult =
                    openTypeGposChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGposChainContextMatchResult::Invalid)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGposChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[nextPosition].shaping.glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (actualGlyphId != expectedGlyphId)
                {
                    matched = false;
                    break;
                }

                position = nextPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Complete match.
            // --------------------------------------------------------

            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGposChainContextMatchResult::Match;
        }

        return OpenTypeGposChainContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 2.
    // ====================================================================

    static inline OpenTypeGposChainContextMatchResult matchOpenTypeGposChainContextFormat2(
        const OpenTypeGposChainContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposChainContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || pos.format() != 2 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGposChainContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposChainContextMatchResult::Invalid;

        const OpenTypeCoverageView coverage = pos.coverage();

        if (!coverage)
            return OpenTypeGposChainContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposChainContextMatchResult::NoMatch;

        const OpenTypeClassDefView backtrackClassDef = pos.backtrackClassDef();
        const OpenTypeClassDefView inputClassDef = pos.inputClassDef();
        const OpenTypeClassDefView lookaheadClassDef = pos.lookaheadClassDef();

        if (!backtrackClassDef || !inputClassDef || !lookaheadClassDef)
            return OpenTypeGposChainContextMatchResult::Invalid;

        uint16_t firstClass = 0;

        if (!inputClassDef.classValue(firstGlyphId, firstClass))
            return OpenTypeGposChainContextMatchResult::Invalid;

        if (firstClass >= pos.classSetCount())
            return OpenTypeGposChainContextMatchResult::NoMatch;

        uint16_t classSetOffset = 0;

        if (!pos.classSetOffset(firstClass, classSetOffset))
            return OpenTypeGposChainContextMatchResult::Invalid;

        if (classSetOffset == 0)
            return OpenTypeGposChainContextMatchResult::NoMatch;

        const OpenTypeGposChainContextClassSetView set = pos.classSet(firstClass);

        if (!set)
            return OpenTypeGposChainContextMatchResult::Invalid;


        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGposChainContextClassRuleView rule = set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGposChainContextMatchResult::Invalid;

            const uint16_t backtrackCount = rule.backtrackGlyphCount();
            const uint16_t inputCount = rule.inputGlyphCount();
            const uint16_t lookaheadCount = rule.lookaheadGlyphCount();

            if (inputCount == 0)
                return OpenTypeGposChainContextMatchResult::Invalid;

            OpenTypeGposChainContextMatch candidate;
            candidate.inputPositions.reserve(inputCount);
            candidate.inputPositions.push_back(glyphIndex);

            bool matched = true;


            // --------------------------------------------------------
            // Backtrack classes.
            // --------------------------------------------------------

            size_t position = glyphIndex;

            for (uint16_t backtrackIndex = 0; backtrackIndex < backtrackCount; ++backtrackIndex)
            {
                uint16_t expectedClass = 0;

                if (!rule.backtrackClass(backtrackIndex, expectedClass))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                size_t previousPosition = 0;

                const OpenTypeGposChainContextMatchResult previousResult =
                    openTypeGposChainContextPrevious(filter, buffer, position, previousPosition);

                if (previousResult == OpenTypeGposChainContextMatchResult::Invalid)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (previousResult == OpenTypeGposChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t glyphId = buffer[previousPosition].shaping.glyphId;

                if (glyphId > 0xFFFFu)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!backtrackClassDef.classValue(glyphId, actualClass))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (actualClass != expectedClass)
                {
                    matched = false;
                    break;
                }

                position = previousPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Input classes.
            // --------------------------------------------------------

            position = glyphIndex;

            for (uint16_t inputIndex = 1; inputIndex < inputCount; ++inputIndex)
            {
                uint16_t expectedClass = 0;

                if (!rule.inputClass(inputIndex - 1, expectedClass))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGposChainContextMatchResult nextResult =
                    openTypeGposChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGposChainContextMatchResult::Invalid)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGposChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t glyphId = buffer[nextPosition].shaping.glyphId;

                if (glyphId > 0xFFFFu)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!inputClassDef.classValue(glyphId, actualClass))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (actualClass != expectedClass)
                {
                    matched = false;
                    break;
                }

                candidate.inputPositions.push_back(nextPosition);
                position = nextPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Lookahead classes.
            // --------------------------------------------------------

            position = candidate.inputPositions.back();

            for (uint16_t lookaheadIndex = 0; lookaheadIndex < lookaheadCount; ++lookaheadIndex)
            {
                uint16_t expectedClass = 0;

                if (!rule.lookaheadClass(lookaheadIndex, expectedClass))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGposChainContextMatchResult nextResult =
                    openTypeGposChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGposChainContextMatchResult::Invalid)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGposChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t glyphId = buffer[nextPosition].shaping.glyphId;

                if (glyphId > 0xFFFFu)
                    return OpenTypeGposChainContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!lookaheadClassDef.classValue(glyphId, actualClass))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                if (actualClass != expectedClass)
                {
                    matched = false;
                    break;
                }

                position = nextPosition;
            }

            if (!matched)
                continue;


            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGposChainContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGposChainContextMatchResult::Match;
        }

        return OpenTypeGposChainContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 3.
    // ====================================================================

    static inline OpenTypeGposChainContextMatchResult matchOpenTypeGposChainContextFormat3(
        const OpenTypeGposChainContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposChainContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || pos.format() != 3 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGposChainContextMatchResult::Invalid;

        const uint16_t backtrackCount = pos.backtrackGlyphCount();
        const uint16_t inputCount = pos.inputGlyphCount();
        const uint16_t lookaheadCount = pos.lookaheadGlyphCount();

        if (inputCount == 0)
            return OpenTypeGposChainContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposChainContextMatchResult::Invalid;

        OpenTypeGposChainContextMatch candidate;
        candidate.inputPositions.reserve(inputCount);

        const OpenTypeCoverageView firstCoverage = pos.inputCoverage(0);

        if (!firstCoverage)
            return OpenTypeGposChainContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!firstCoverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposChainContextMatchResult::NoMatch;

        candidate.inputPositions.push_back(glyphIndex);


        // ------------------------------------------------------------
        // Backtrack Coverages.
        // ------------------------------------------------------------

        size_t position = glyphIndex;

        for (uint16_t backtrackIndex = 0; backtrackIndex < backtrackCount; ++backtrackIndex)
        {
            size_t previousPosition = 0;

            const OpenTypeGposChainContextMatchResult previousResult =
                openTypeGposChainContextPrevious(filter, buffer, position, previousPosition);

            if (previousResult != OpenTypeGposChainContextMatchResult::Match)
                return previousResult;

            const uint32_t glyphId = buffer[previousPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposChainContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = pos.backtrackCoverage(backtrackIndex);

            if (!coverage)
                return OpenTypeGposChainContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGposChainContextMatchResult::NoMatch;

            position = previousPosition;
        }


        // ------------------------------------------------------------
        // Remaining input Coverages.
        // ------------------------------------------------------------

        position = glyphIndex;

        for (uint16_t inputIndex = 1; inputIndex < inputCount; ++inputIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGposChainContextMatchResult nextResult =
                openTypeGposChainContextNext(filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGposChainContextMatchResult::Match)
                return nextResult;

            const uint32_t glyphId = buffer[nextPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposChainContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = pos.inputCoverage(inputIndex);

            if (!coverage)
                return OpenTypeGposChainContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGposChainContextMatchResult::NoMatch;

            candidate.inputPositions.push_back(nextPosition);
            position = nextPosition;
        }


        // ------------------------------------------------------------
        // Lookahead Coverages.
        // ------------------------------------------------------------

        position = candidate.inputPositions.back();

        for (uint16_t lookaheadIndex = 0; lookaheadIndex < lookaheadCount; ++lookaheadIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGposChainContextMatchResult nextResult =
                openTypeGposChainContextNext(filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGposChainContextMatchResult::Match)
                return nextResult;

            const uint32_t glyphId = buffer[nextPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposChainContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = pos.lookaheadCoverage(lookaheadIndex);

            if (!coverage)
                return OpenTypeGposChainContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGposChainContextMatchResult::NoMatch;

            position = nextPosition;
        }


        // ------------------------------------------------------------
        // Entire chain matched.
        //
        // Preserve sequenceIndex exactly as encoded. GPOS execution later
        // interprets it against the stable inputPositions array.
        // ------------------------------------------------------------

        const uint16_t lookupCount = pos.sequenceLookupCount();
        candidate.lookups.reserve(lookupCount);

        for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
        {
            OpenTypeSequenceLookup lookup{};

            if (!pos.sequenceLookup(lookupIndex, lookup))
                return OpenTypeGposChainContextMatchResult::Invalid;

            candidate.lookups.push_back(lookup);
        }

        match = std::move(candidate);
        return OpenTypeGposChainContextMatchResult::Match;
    }


    // ====================================================================
    // matchOpenTypeGposChainContextPos
    // ====================================================================

    static inline OpenTypeGposChainContextMatchResult matchOpenTypeGposChainContextPos(
        const OpenTypeGposChainContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposChainContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || !filter)
            return OpenTypeGposChainContextMatchResult::Invalid;

        switch (pos.format())
        {
        case 1:
            return matchOpenTypeGposChainContextFormat1(pos, filter, buffer, glyphIndex, match);

        case 2:
            return matchOpenTypeGposChainContextFormat2(pos, filter, buffer, glyphIndex, match);

        case 3:
            return matchOpenTypeGposChainContextFormat3(pos, filter, buffer, glyphIndex, match);

        default:
            return OpenTypeGposChainContextMatchResult::Invalid;
        }
    }

} // namespace waavs