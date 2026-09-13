// opentype_gpos_context_match.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_gpos_context_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_sequence_lookup.h"
#include "shaped_glyph_buffer.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposContextMatchResult
    // ====================================================================

    enum class OpenTypeGposContextMatchResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGposContextMatch
    //
    // positions:
    //   Physical ShapedGlyphBuffer positions corresponding to logical
    //   context input positions 0..N-1.
    //
    // lookups:
    //   SequenceLookup records in stored/design order.
    //
    // Unlike GSUB contextual execution, GPOS does not change glyph-buffer
    // topology, so these physical positions remain stable during execution.
    // ====================================================================

    struct OpenTypeGposContextMatch
    {
        std::vector<size_t> positions{};
        std::vector<OpenTypeSequenceLookup> lookups{};

        void clear() noexcept
        {
            positions.clear();
            lookups.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return positions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return positions.size(); }
    };


    // ====================================================================
    // openTypeGposContextNext
    //
    // The current/start glyph is never filtered. Filtering applies only
    // while walking away from that position.
    // ====================================================================

    static inline OpenTypeGposContextMatchResult openTypeGposContextNext(
        const OpenTypeLookupGlyphFilter& filter, const ShapedGlyphBuffer& buffer,
        size_t currentIndex, size_t& nextIndex) noexcept
    {
        nextIndex = 0;

        const OpenTypeLookupGlyphSearchResult result =
            filter.next(buffer, currentIndex, nextIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGposContextMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGposContextMatchResult::NoMatch;

        default:
            return OpenTypeGposContextMatchResult::Invalid;
        }
    }


    // ====================================================================
    // Format 1
    //
    // Coverage chooses the SequenceRuleSet.
    //
    // Each SequenceRule contains exact glyph IDs for positions 1..N-1.
    // ====================================================================

    static inline OpenTypeGposContextMatchResult matchOpenTypeGposContextFormat1(
        const OpenTypeGposContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || pos.format() != 1 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGposContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Current glyph is matched directly against Coverage.
        // ------------------------------------------------------------

        const OpenTypeCoverageView coverage = pos.coverage();

        if (!coverage)
            return OpenTypeGposContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposContextMatchResult::NoMatch;

        if (coverageIndex >= pos.ruleSetCount())
            return OpenTypeGposContextMatchResult::NoMatch;


        // ------------------------------------------------------------
        // NULL RuleSet is legal.
        // ------------------------------------------------------------

        uint16_t ruleSetOffset = 0;

        if (!pos.ruleSetOffset(coverageIndex, ruleSetOffset))
            return OpenTypeGposContextMatchResult::Invalid;

        if (ruleSetOffset == 0)
            return OpenTypeGposContextMatchResult::NoMatch;

        const OpenTypeGposContextRuleSetView set =
            pos.ruleSet(coverageIndex);

        if (!set)
            return OpenTypeGposContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // First complete rule in stored order wins.
        // ------------------------------------------------------------

        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGposContextRuleView rule =
                set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGposContextMatchResult::Invalid;

            const uint16_t glyphCount = rule.glyphCount();

            if (glyphCount == 0)
                return OpenTypeGposContextMatchResult::Invalid;

            OpenTypeGposContextMatch candidate;
            candidate.positions.reserve(glyphCount);
            candidate.positions.push_back(glyphIndex);

            size_t position = glyphIndex;
            bool matched = true;

            for (uint16_t sequenceIndex = 1; sequenceIndex < glyphCount; ++sequenceIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.inputGlyphId(sequenceIndex - 1, expectedGlyphId))
                    return OpenTypeGposContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGposContextMatchResult nextResult =
                    openTypeGposContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGposContextMatchResult::Invalid)
                    return OpenTypeGposContextMatchResult::Invalid;

                if (nextResult == OpenTypeGposContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId =
                    buffer[nextPosition].shaping.glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGposContextMatchResult::Invalid;

                if (actualGlyphId != expectedGlyphId)
                {
                    matched = false;
                    break;
                }

                candidate.positions.push_back(nextPosition);
                position = nextPosition;
            }

            if (!matched)
                continue;


            // --------------------------------------------------------
            // Entire context matched. Collect actions afterward.
            // --------------------------------------------------------

            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGposContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGposContextMatchResult::Match;
        }

        return OpenTypeGposContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 2
    //
    // Coverage controls whether the glyph may start the context.
    // ClassDef selects the ClassSet and matches positions 1..N-1.
    // ====================================================================

    static inline OpenTypeGposContextMatchResult matchOpenTypeGposContextFormat2(
        const OpenTypeGposContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || pos.format() != 2 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGposContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposContextMatchResult::Invalid;


        const OpenTypeCoverageView coverage = pos.coverage();

        if (!coverage)
            return OpenTypeGposContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposContextMatchResult::NoMatch;


        const OpenTypeClassDefView classDef = pos.classDef();

        if (!classDef)
            return OpenTypeGposContextMatchResult::Invalid;

        uint16_t firstClass = 0;

        if (!classDef.classValue(firstGlyphId, firstClass))
            return OpenTypeGposContextMatchResult::Invalid;

        if (firstClass >= pos.classSetCount())
            return OpenTypeGposContextMatchResult::NoMatch;


        uint16_t classSetOffset = 0;

        if (!pos.classSetOffset(firstClass, classSetOffset))
            return OpenTypeGposContextMatchResult::Invalid;

        if (classSetOffset == 0)
            return OpenTypeGposContextMatchResult::NoMatch;

        const OpenTypeGposContextClassSetView set =
            pos.classSet(firstClass);

        if (!set)
            return OpenTypeGposContextMatchResult::Invalid;


        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGposContextClassRuleView rule =
                set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGposContextMatchResult::Invalid;

            const uint16_t glyphCount = rule.glyphCount();

            if (glyphCount == 0)
                return OpenTypeGposContextMatchResult::Invalid;

            OpenTypeGposContextMatch candidate;
            candidate.positions.reserve(glyphCount);
            candidate.positions.push_back(glyphIndex);

            size_t position = glyphIndex;
            bool matched = true;

            for (uint16_t sequenceIndex = 1; sequenceIndex < glyphCount; ++sequenceIndex)
            {
                uint16_t expectedClass = 0;

                if (!rule.inputClass(sequenceIndex - 1, expectedClass))
                    return OpenTypeGposContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGposContextMatchResult nextResult =
                    openTypeGposContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGposContextMatchResult::Invalid)
                    return OpenTypeGposContextMatchResult::Invalid;

                if (nextResult == OpenTypeGposContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId =
                    buffer[nextPosition].shaping.glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGposContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!classDef.classValue(actualGlyphId, actualClass))
                    return OpenTypeGposContextMatchResult::Invalid;

                if (actualClass != expectedClass)
                {
                    matched = false;
                    break;
                }

                candidate.positions.push_back(nextPosition);
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
                    return OpenTypeGposContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGposContextMatchResult::Match;
        }

        return OpenTypeGposContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 3
    //
    // One Coverage table describes each logical input position.
    // ====================================================================

    static inline OpenTypeGposContextMatchResult matchOpenTypeGposContextFormat3(
        const OpenTypeGposContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || pos.format() != 3 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGposContextMatchResult::Invalid;

        const uint16_t glyphCount = pos.glyphCount();

        if (glyphCount == 0)
            return OpenTypeGposContextMatchResult::Invalid;

        const uint32_t firstGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposContextMatchResult::Invalid;

        OpenTypeGposContextMatch candidate;
        candidate.positions.reserve(glyphCount);


        // ------------------------------------------------------------
        // Position zero is never filtered.
        // ------------------------------------------------------------

        const OpenTypeCoverageView firstCoverage =
            pos.inputCoverage(0);

        if (!firstCoverage)
            return OpenTypeGposContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!firstCoverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposContextMatchResult::NoMatch;

        candidate.positions.push_back(glyphIndex);


        // ------------------------------------------------------------
        // Remaining positions use filtered traversal.
        // ------------------------------------------------------------

        size_t position = glyphIndex;

        for (uint16_t sequenceIndex = 1; sequenceIndex < glyphCount; ++sequenceIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGposContextMatchResult nextResult =
                openTypeGposContextNext(filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGposContextMatchResult::Match)
                return nextResult;

            const uint32_t glyphId =
                buffer[nextPosition].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage =
                pos.inputCoverage(sequenceIndex);

            if (!coverage)
                return OpenTypeGposContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGposContextMatchResult::NoMatch;

            candidate.positions.push_back(nextPosition);
            position = nextPosition;
        }


        // ------------------------------------------------------------
        // Collect actions only after the full context matches.
        // ------------------------------------------------------------

        const uint16_t lookupCount = pos.sequenceLookupCount();
        candidate.lookups.reserve(lookupCount);

        for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
        {
            OpenTypeSequenceLookup lookup{};

            if (!pos.sequenceLookup(lookupIndex, lookup))
                return OpenTypeGposContextMatchResult::Invalid;

            candidate.lookups.push_back(lookup);
        }

        match = std::move(candidate);
        return OpenTypeGposContextMatchResult::Match;
    }


    // ====================================================================
    // matchOpenTypeGposContextPos
    //
    // Match one Type 7 subtable at one physical buffer position.
    //
    // No placement is modified here.
    // ====================================================================

    static inline OpenTypeGposContextMatchResult matchOpenTypeGposContextPos(
        const OpenTypeGposContextPosView& pos, const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposContextMatch& match) noexcept
    {
        match.clear();

        if (!pos || !filter)
            return OpenTypeGposContextMatchResult::Invalid;

        switch (pos.format())
        {
        case 1:
            return matchOpenTypeGposContextFormat1(
                pos, filter, buffer, glyphIndex, match);

        case 2:
            return matchOpenTypeGposContextFormat2(
                pos, filter, buffer, glyphIndex, match);

        case 3:
            return matchOpenTypeGposContextFormat3(
                pos, filter, buffer, glyphIndex, match);

        default:
            return OpenTypeGposContextMatchResult::Invalid;
        }
    }

} // namespace waavs