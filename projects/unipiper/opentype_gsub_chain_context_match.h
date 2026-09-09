// opentype_gsub_chain_context_match.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_gsub_chain_context_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_sequence_lookup.h"
#include "opentype_shaping_buffer.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGsubChainContextMatchResult
    // ====================================================================

    enum class OpenTypeGsubChainContextMatchResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGsubChainContextMatch
    //
    // inputPositions:
    //   Physical shaping-buffer positions corresponding to the logical input
    //   sequence positions 0..N-1.
    //
    //   Backtrack and lookahead positions are match-only constraints and are
    //   deliberately not retained.
    //
    // lookups:
    //   SequenceLookup records in stored/design order.
    //
    // Example with IgnoreMarks:
    //
    //   buffer:
    //
    //       B0 mark B1 mark I0 mark I1 mark I2 mark L0
    //
    //   inputPositions:
    //
    //       { physical(I0), physical(I1), physical(I2) }
    // ====================================================================

    struct OpenTypeGsubChainContextMatch
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
    // Filtered traversal helpers.
    //
    // The current lookup glyph is never filtered. These helpers are used only
    // when walking away from that current position.
    // ====================================================================

    static inline OpenTypeGsubChainContextMatchResult openTypeGsubChainContextNext(
        const OpenTypeLookupGlyphFilter& filter, const OpenTypeShapingBuffer& buffer,
        size_t currentIndex, size_t& nextIndex) noexcept
    {
        nextIndex = 0;

        const OpenTypeLookupGlyphSearchResult result =
            filter.next(buffer, currentIndex, nextIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGsubChainContextMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGsubChainContextMatchResult::NoMatch;

        default:
            return OpenTypeGsubChainContextMatchResult::Invalid;
        }
    }


    static inline OpenTypeGsubChainContextMatchResult openTypeGsubChainContextPrevious(
        const OpenTypeLookupGlyphFilter& filter, const OpenTypeShapingBuffer& buffer,
        size_t currentIndex, size_t& previousIndex) noexcept
    {
        previousIndex = 0;

        const OpenTypeLookupGlyphSearchResult result =
            filter.previous(buffer, currentIndex, previousIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGsubChainContextMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGsubChainContextMatchResult::NoMatch;

        default:
            return OpenTypeGsubChainContextMatchResult::Invalid;
        }
    }


    // ====================================================================
    // Format 1
    //
    // Coverage chooses the ChainedSequenceRuleSet.
    //
    // Each rule contains:
    //
    //   backtrack glyph IDs, nearest-first
    //   input glyph IDs for positions 1..N-1
    //   lookahead glyph IDs
    //   SequenceLookup records
    //
    // Only input positions are retained in the result.
    // ====================================================================

    static inline OpenTypeGsubChainContextMatchResult matchOpenTypeGsubChainContextFormat1(
        const OpenTypeGsubChainContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubChainContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 1 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubChainContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubChainContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Current glyph is matched directly against Coverage.
        // ------------------------------------------------------------

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGsubChainContextMatchResult::NoMatch;


        // Coverage and RuleSet counts are permitted to differ.

        if (coverageIndex >= subst.ruleSetCount())
            return OpenTypeGsubChainContextMatchResult::NoMatch;


        // Distinguish a legal NULL RuleSet from malformed data.

        uint16_t ruleSetOffset = 0;

        if (!subst.ruleSetOffset(coverageIndex, ruleSetOffset))
            return OpenTypeGsubChainContextMatchResult::Invalid;

        if (ruleSetOffset == 0)
            return OpenTypeGsubChainContextMatchResult::NoMatch;

        const OpenTypeGsubChainContextRuleSetView set = subst.ruleSet(coverageIndex);

        if (!set)
            return OpenTypeGsubChainContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Rule order is significant. First complete match wins.
        // ------------------------------------------------------------

        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGsubChainContextRuleView rule = set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const uint16_t backtrackCount = rule.backtrackGlyphCount();
            const uint16_t inputCount = rule.inputGlyphCount();
            const uint16_t lookaheadCount = rule.lookaheadGlyphCount();

            if (inputCount == 0)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            OpenTypeGsubChainContextMatch candidate;
            candidate.inputPositions.reserve(inputCount);
            candidate.inputPositions.push_back(glyphIndex);

            bool matched = true;


            // --------------------------------------------------------
            // Backtrack.
            //
            // OpenType stores backtrackSequence[] nearest-first.
            //
            // Logical:
            //
            //     A B C D | X
            //
            // Encoded:
            //
            //     D C B A
            //
            // Therefore walk previous() while reading indices upward.
            // --------------------------------------------------------

            size_t position = glyphIndex;

            for (uint16_t backtrackIndex = 0; backtrackIndex < backtrackCount; ++backtrackIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.backtrackGlyphId(backtrackIndex, expectedGlyphId))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                size_t previousPosition = 0;

                const OpenTypeGsubChainContextMatchResult previousResult =
                    openTypeGsubChainContextPrevious(filter, buffer, position, previousPosition);

                if (previousResult == OpenTypeGsubChainContextMatchResult::Invalid)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (previousResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[previousPosition].glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

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
            //
            // Input position 0 is the current glyph from Coverage.
            // inputGlyphId(0) describes logical input position 1.
            // --------------------------------------------------------

            position = glyphIndex;

            for (uint16_t inputIndex = 1; inputIndex < inputCount; ++inputIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.inputGlyphId(inputIndex - 1, expectedGlyphId))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGsubChainContextMatchResult nextResult =
                    openTypeGsubChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGsubChainContextMatchResult::Invalid)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[nextPosition].glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

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
            //
            // Lookahead begins after the last matched input glyph.
            // Lookahead positions are not retained.
            // --------------------------------------------------------

            position = candidate.inputPositions.back();

            for (uint16_t lookaheadIndex = 0; lookaheadIndex < lookaheadCount; ++lookaheadIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.lookaheadGlyphId(lookaheadIndex, expectedGlyphId))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGsubChainContextMatchResult nextResult =
                    openTypeGsubChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGsubChainContextMatchResult::Invalid)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[nextPosition].glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

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
            // Entire chain matched.
            //
            // Copy SequenceLookup records only now. sequenceIndex is
            // deliberately not validated against the original inputCount.
            // --------------------------------------------------------

            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGsubChainContextMatchResult::Match;
        }

        return OpenTypeGsubChainContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 2
    //
    // Coverage determines whether the current glyph may begin the chain.
    //
    // Three independent ClassDef tables are used:
    //
    //   backtrackClassDef
    //   inputClassDef
    //   lookaheadClassDef
    //
    // The current glyph's input class chooses the ClassSet.
    // ====================================================================

    static inline OpenTypeGsubChainContextMatchResult matchOpenTypeGsubChainContextFormat2(
        const OpenTypeGsubChainContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubChainContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 2 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubChainContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubChainContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Coverage restricts valid chain starts.
        // ------------------------------------------------------------

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGsubChainContextMatchResult::NoMatch;


        // ------------------------------------------------------------
        // Three independent ClassDefs.
        // ------------------------------------------------------------

        const OpenTypeClassDefView backtrackClassDef = subst.backtrackClassDef();
        const OpenTypeClassDefView inputClassDef = subst.inputClassDef();
        const OpenTypeClassDefView lookaheadClassDef = subst.lookaheadClassDef();

        if (!backtrackClassDef || !inputClassDef || !lookaheadClassDef)
            return OpenTypeGsubChainContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Current glyph's input class selects the ClassSet.
        // ------------------------------------------------------------

        uint16_t firstClass = 0;

        if (!inputClassDef.classValue(firstGlyphId, firstClass))
            return OpenTypeGsubChainContextMatchResult::Invalid;

        if (firstClass >= subst.classSetCount())
            return OpenTypeGsubChainContextMatchResult::NoMatch;


        uint16_t classSetOffset = 0;

        if (!subst.classSetOffset(firstClass, classSetOffset))
            return OpenTypeGsubChainContextMatchResult::Invalid;

        if (classSetOffset == 0)
            return OpenTypeGsubChainContextMatchResult::NoMatch;

        const OpenTypeGsubChainContextClassSetView set = subst.classSet(firstClass);

        if (!set)
            return OpenTypeGsubChainContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Try rules in stored order.
        // ------------------------------------------------------------

        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGsubChainContextClassRuleView rule = set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const uint16_t backtrackCount = rule.backtrackGlyphCount();
            const uint16_t inputCount = rule.inputGlyphCount();
            const uint16_t lookaheadCount = rule.lookaheadGlyphCount();

            if (inputCount == 0)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            OpenTypeGsubChainContextMatch candidate;
            candidate.inputPositions.reserve(inputCount);
            candidate.inputPositions.push_back(glyphIndex);

            bool matched = true;


            // --------------------------------------------------------
            // Backtrack classes, nearest-first.
            // --------------------------------------------------------

            size_t position = glyphIndex;

            for (uint16_t backtrackIndex = 0; backtrackIndex < backtrackCount; ++backtrackIndex)
            {
                uint16_t expectedClass = 0;

                if (!rule.backtrackClass(backtrackIndex, expectedClass))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                size_t previousPosition = 0;

                const OpenTypeGsubChainContextMatchResult previousResult =
                    openTypeGsubChainContextPrevious(filter, buffer, position, previousPosition);

                if (previousResult == OpenTypeGsubChainContextMatchResult::Invalid)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (previousResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t glyphId = buffer[previousPosition].glyphId;

                if (glyphId > 0xFFFFu)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!backtrackClassDef.classValue(glyphId, actualClass))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

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
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGsubChainContextMatchResult nextResult =
                    openTypeGsubChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGsubChainContextMatchResult::Invalid)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t glyphId = buffer[nextPosition].glyphId;

                if (glyphId > 0xFFFFu)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!inputClassDef.classValue(glyphId, actualClass))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

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
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGsubChainContextMatchResult nextResult =
                    openTypeGsubChainContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGsubChainContextMatchResult::Invalid)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (nextResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t glyphId = buffer[nextPosition].glyphId;

                if (glyphId > 0xFFFFu)
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!lookaheadClassDef.classValue(glyphId, actualClass))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                if (actualClass != expectedClass)
                {
                    matched = false;
                    break;
                }

                position = nextPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Complete match. Copy actions in stored/design order.
            // --------------------------------------------------------

            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGsubChainContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGsubChainContextMatchResult::Match;
        }

        return OpenTypeGsubChainContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 3
    //
    // Coverage arrays directly describe:
    //
    //   backtrack sequence
    //   input sequence
    //   lookahead sequence
    //
    // backtrackCoverage(0) is nearest to the current glyph.
    // inputCoverage(0) matches the current glyph directly.
    // ====================================================================

    static inline OpenTypeGsubChainContextMatchResult matchOpenTypeGsubChainContextFormat3(
        const OpenTypeGsubChainContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubChainContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 3 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubChainContextMatchResult::Invalid;

        const uint16_t backtrackCount = subst.backtrackGlyphCount();
        const uint16_t inputCount = subst.inputGlyphCount();
        const uint16_t lookaheadCount = subst.lookaheadGlyphCount();

        if (inputCount == 0)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        OpenTypeGsubChainContextMatch candidate;
        candidate.inputPositions.reserve(inputCount);


        // ------------------------------------------------------------
        // Input position 0 is the current glyph and is never filtered.
        // ------------------------------------------------------------

        const OpenTypeCoverageView firstCoverage = subst.inputCoverage(0);

        if (!firstCoverage)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!firstCoverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGsubChainContextMatchResult::NoMatch;

        candidate.inputPositions.push_back(glyphIndex);


        // ------------------------------------------------------------
        // Backtrack Coverages.
        //
        // Index 0 is nearest previous glyph.
        // ------------------------------------------------------------

        size_t position = glyphIndex;

        for (uint16_t backtrackIndex = 0; backtrackIndex < backtrackCount; ++backtrackIndex)
        {
            size_t previousPosition = 0;

            const OpenTypeGsubChainContextMatchResult previousResult =
                openTypeGsubChainContextPrevious(filter, buffer, position, previousPosition);

            if (previousResult != OpenTypeGsubChainContextMatchResult::Match)
                return previousResult;

            const uint32_t glyphId = buffer[previousPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = subst.backtrackCoverage(backtrackIndex);

            if (!coverage)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGsubChainContextMatchResult::NoMatch;

            position = previousPosition;
        }


        // ------------------------------------------------------------
        // Remaining input Coverages.
        // ------------------------------------------------------------

        position = glyphIndex;

        for (uint16_t inputIndex = 1; inputIndex < inputCount; ++inputIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGsubChainContextMatchResult nextResult =
                openTypeGsubChainContextNext(filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGsubChainContextMatchResult::Match)
                return nextResult;

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = subst.inputCoverage(inputIndex);

            if (!coverage)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGsubChainContextMatchResult::NoMatch;

            candidate.inputPositions.push_back(nextPosition);
            position = nextPosition;
        }


        // ------------------------------------------------------------
        // Lookahead Coverages begin after the final input position.
        // ------------------------------------------------------------

        position = candidate.inputPositions.back();

        for (uint16_t lookaheadIndex = 0; lookaheadIndex < lookaheadCount; ++lookaheadIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGsubChainContextMatchResult nextResult =
                openTypeGsubChainContextNext(filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGsubChainContextMatchResult::Match)
                return nextResult;

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = subst.lookaheadCoverage(lookaheadIndex);

            if (!coverage)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGsubChainContextMatchResult::NoMatch;

            position = nextPosition;
        }


        // ------------------------------------------------------------
        // Entire chain matched. Only now collect actions.
        //
        // sequenceIndex is intentionally preserved exactly as encoded.
        // It is interpreted later against the mutable input sequence.
        // ------------------------------------------------------------

        const uint16_t lookupCount = subst.sequenceLookupCount();
        candidate.lookups.reserve(lookupCount);

        for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
        {
            OpenTypeSequenceLookup lookup{};

            if (!subst.sequenceLookup(lookupIndex, lookup))
                return OpenTypeGsubChainContextMatchResult::Invalid;

            candidate.lookups.push_back(lookup);
        }

        match = std::move(candidate);
        return OpenTypeGsubChainContextMatchResult::Match;
    }


    // ====================================================================
    // matchOpenTypeGsubChainContextSubst
    //
    // Match one GSUB LookupType 6 subtable at one physical buffer position.
    //
    // No substitution occurs here. The shaping buffer remains unchanged.
    //
    // LookupFlag filtering applies while traversing backtrack, later input,
    // and lookahead glyphs. The current glyph is matched directly.
    // ====================================================================

    static inline OpenTypeGsubChainContextMatchResult matchOpenTypeGsubChainContextSubst(
        const OpenTypeGsubChainContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubChainContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || !filter)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        switch (subst.format())
        {
        case 1:
            return matchOpenTypeGsubChainContextFormat1(subst, filter, buffer, glyphIndex, match);

        case 2:
            return matchOpenTypeGsubChainContextFormat2(subst, filter, buffer, glyphIndex, match);

        case 3:
            return matchOpenTypeGsubChainContextFormat3(subst, filter, buffer, glyphIndex, match);

        default:
            return OpenTypeGsubChainContextMatchResult::Invalid;
        }
    }

} // namespace waavs