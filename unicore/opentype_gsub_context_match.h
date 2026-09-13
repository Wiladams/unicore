// opentype_gsub_context_match.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_gsub_context_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_sequence_lookup.h"
#include "opentype_shaping_buffer.h"


namespace waavs
{
    // ====================================================================
    // OpenTypeGsubContextMatchResult
    // ====================================================================

    enum class OpenTypeGsubContextMatchResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGsubContextMatch
    //
    // positions:
    //   Physical shaping-buffer positions corresponding to logical input
    //   sequence positions 0..N-1.
    //
    // lookups:
    //   SequenceLookup records in stored/design order.
    //
    // Example with IgnoreMarks:
    //
    //   buffer:       A mark B mark C
    //   physical:     0  1   2  3   4
    //
    //   positions = { 0, 2, 4 }
    // ====================================================================

    struct OpenTypeGsubContextMatch
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
    // openTypeGsubContextNext
    //
    // Find the next eligible physical glyph position.
    //
    // The current/start glyph is never passed through shouldSkip(). The
    // caller supplies it directly as positions[0].
    // ====================================================================

    static inline OpenTypeGsubContextMatchResult openTypeGsubContextNext(
        const OpenTypeLookupGlyphFilter& filter, const OpenTypeShapingBuffer& buffer,
        size_t currentIndex, size_t& nextIndex) noexcept
    {
        nextIndex = 0;

        const OpenTypeLookupGlyphSearchResult result =
            filter.next(buffer, currentIndex, nextIndex);

        switch (result)
        {
        case OpenTypeLookupGlyphSearchResult::Found:
            return OpenTypeGsubContextMatchResult::Match;

        case OpenTypeLookupGlyphSearchResult::End:
            return OpenTypeGsubContextMatchResult::NoMatch;

        default:
            return OpenTypeGsubContextMatchResult::Invalid;
        }
    }


    // ====================================================================
    // Format 1
    //
    // Coverage chooses the RuleSet.
    // Each rule contains exact glyph IDs for input positions 1..N-1.
    // ====================================================================

    static inline OpenTypeGsubContextMatchResult matchOpenTypeGsubContextFormat1(
        const OpenTypeGsubContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 1 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // The current glyph is matched directly against Coverage.
        //
        // Lookup filtering does not apply to the initial lookup
        // position.
        // ------------------------------------------------------------

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return OpenTypeGsubContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGsubContextMatchResult::NoMatch;


        // The spec permits Coverage and RuleSet counts to differ.
        // Extra Coverage entries simply have no corresponding rule.

        if (coverageIndex >= subst.ruleSetCount())
            return OpenTypeGsubContextMatchResult::NoMatch;


        // Distinguish a legal NULL RuleSet from a malformed child.

        uint16_t ruleSetOffset = 0;

        if (!subst.ruleSetOffset(coverageIndex, ruleSetOffset))
            return OpenTypeGsubContextMatchResult::Invalid;

        if (ruleSetOffset == 0)
            return OpenTypeGsubContextMatchResult::NoMatch;

        const OpenTypeGsubContextRuleSetView set = subst.ruleSet(coverageIndex);

        if (!set)
            return OpenTypeGsubContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Rule order is significant.
        //
        // The first complete matching rule wins.
        // ------------------------------------------------------------

        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGsubContextRuleView rule = set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGsubContextMatchResult::Invalid;

            const uint16_t glyphCount = rule.glyphCount();

            if (glyphCount == 0)
                return OpenTypeGsubContextMatchResult::Invalid;

            OpenTypeGsubContextMatch candidate;
            candidate.positions.reserve(glyphCount);
            candidate.positions.push_back(glyphIndex);

            size_t position = glyphIndex;
            bool matched = true;


            // inputGlyphId(0) describes logical sequence position 1.

            for (uint16_t sequenceIndex = 1; sequenceIndex < glyphCount; ++sequenceIndex)
            {
                uint16_t expectedGlyphId = 0;

                if (!rule.inputGlyphId(sequenceIndex - 1, expectedGlyphId))
                    return OpenTypeGsubContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGsubContextMatchResult nextResult =
                    openTypeGsubContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGsubContextMatchResult::Invalid)
                    return OpenTypeGsubContextMatchResult::Invalid;

                if (nextResult == OpenTypeGsubContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[nextPosition].glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGsubContextMatchResult::Invalid;

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


            // ------------------------------------------------------------
            // Context is completely matched.
            //
            // Only now collect the SequenceLookup actions.
            // ------------------------------------------------------------

            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGsubContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGsubContextMatchResult::Match;
        }

        return OpenTypeGsubContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 2
    //
    // Coverage determines whether this glyph may begin a context.
    //
    // ClassDef maps the first glyph to the corresponding ClassSet.
    // Each ClassRule contains class values for positions 1..N-1.
    // ====================================================================

    static inline OpenTypeGsubContextMatchResult matchOpenTypeGsubContextFormat2(
        const OpenTypeGsubContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 2 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Coverage restricts which glyphs may begin a class sequence.
        // ------------------------------------------------------------

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return OpenTypeGsubContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGsubContextMatchResult::NoMatch;


        // ------------------------------------------------------------
        // Determine the first glyph's class.
        // ------------------------------------------------------------

        const OpenTypeClassDefView classDef = subst.classDef();

        if (!classDef)
            return OpenTypeGsubContextMatchResult::Invalid;

        uint16_t firstClass = 0;

        if (!classDef.classValue(firstGlyphId, firstClass))
            return OpenTypeGsubContextMatchResult::Invalid;


        // No ClassSet exists for this class.

        if (firstClass >= subst.classSetCount())
            return OpenTypeGsubContextMatchResult::NoMatch;


        // Distinguish legal NULL ClassSet from malformed data.

        uint16_t classSetOffset = 0;

        if (!subst.classSetOffset(firstClass, classSetOffset))
            return OpenTypeGsubContextMatchResult::Invalid;

        if (classSetOffset == 0)
            return OpenTypeGsubContextMatchResult::NoMatch;

        const OpenTypeGsubContextClassSetView set = subst.classSet(firstClass);

        if (!set)
            return OpenTypeGsubContextMatchResult::Invalid;


        // ------------------------------------------------------------
        // Try class rules in stored order.
        // ------------------------------------------------------------

        for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
        {
            const OpenTypeGsubContextClassRuleView rule = set.rule(ruleIndex);

            if (!rule)
                return OpenTypeGsubContextMatchResult::Invalid;

            const uint16_t glyphCount = rule.glyphCount();

            if (glyphCount == 0)
                return OpenTypeGsubContextMatchResult::Invalid;

            OpenTypeGsubContextMatch candidate;
            candidate.positions.reserve(glyphCount);
            candidate.positions.push_back(glyphIndex);

            size_t position = glyphIndex;
            bool matched = true;


            // inputClass(0) describes logical sequence position 1.

            for (uint16_t sequenceIndex = 1; sequenceIndex < glyphCount; ++sequenceIndex)
            {
                uint16_t expectedClass = 0;

                if (!rule.inputClass(sequenceIndex - 1, expectedClass))
                    return OpenTypeGsubContextMatchResult::Invalid;

                size_t nextPosition = 0;

                const OpenTypeGsubContextMatchResult nextResult =
                    openTypeGsubContextNext(filter, buffer, position, nextPosition);

                if (nextResult == OpenTypeGsubContextMatchResult::Invalid)
                    return OpenTypeGsubContextMatchResult::Invalid;

                if (nextResult == OpenTypeGsubContextMatchResult::NoMatch)
                {
                    matched = false;
                    break;
                }

                const uint32_t actualGlyphId = buffer[nextPosition].glyphId;

                if (actualGlyphId > 0xFFFFu)
                    return OpenTypeGsubContextMatchResult::Invalid;

                uint16_t actualClass = 0;

                if (!classDef.classValue(actualGlyphId, actualClass))
                    return OpenTypeGsubContextMatchResult::Invalid;

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


            // ------------------------------------------------------------
            // Complete match. Copy actions in stored/design order.
            // ------------------------------------------------------------

            const uint16_t lookupCount = rule.sequenceLookupCount();
            candidate.lookups.reserve(lookupCount);

            for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
            {
                OpenTypeSequenceLookup lookup{};

                if (!rule.sequenceLookup(lookupIndex, lookup))
                    return OpenTypeGsubContextMatchResult::Invalid;

                candidate.lookups.push_back(lookup);
            }

            match = std::move(candidate);
            return OpenTypeGsubContextMatchResult::Match;
        }

        return OpenTypeGsubContextMatchResult::NoMatch;
    }


    // ====================================================================
    // Format 3
    //
    // One Coverage table describes each logical input position.
    //
    // There is exactly one context rule in this subtable.
    // ====================================================================

    static inline OpenTypeGsubContextMatchResult matchOpenTypeGsubContextFormat3(
        const OpenTypeGsubContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || subst.format() != 3 || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubContextMatchResult::Invalid;

        const uint16_t glyphCount = subst.glyphCount();

        if (glyphCount == 0)
            return OpenTypeGsubContextMatchResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubContextMatchResult::Invalid;


        OpenTypeGsubContextMatch candidate;
        candidate.positions.reserve(glyphCount);


        // ------------------------------------------------------------
        // Position 0 is the current glyph and is not filtered.
        // ------------------------------------------------------------

        const OpenTypeCoverageView firstCoverage = subst.inputCoverage(0);

        if (!firstCoverage)
            return OpenTypeGsubContextMatchResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!firstCoverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGsubContextMatchResult::NoMatch;

        candidate.positions.push_back(glyphIndex);


        // ------------------------------------------------------------
        // Positions 1..N-1 use filtered traversal.
        // ------------------------------------------------------------

        size_t position = glyphIndex;

        for (uint16_t sequenceIndex = 1; sequenceIndex < glyphCount; ++sequenceIndex)
        {
            size_t nextPosition = 0;

            const OpenTypeGsubContextMatchResult nextResult =
                openTypeGsubContextNext(filter, buffer, position, nextPosition);

            if (nextResult != OpenTypeGsubContextMatchResult::Match)
                return nextResult;

            const uint32_t glyphId = buffer[nextPosition].glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGsubContextMatchResult::Invalid;

            const OpenTypeCoverageView coverage = subst.inputCoverage(sequenceIndex);

            if (!coverage)
                return OpenTypeGsubContextMatchResult::Invalid;

            if (!coverage.find(glyphId, coverageIndex))
                return OpenTypeGsubContextMatchResult::NoMatch;

            candidate.positions.push_back(nextPosition);
            position = nextPosition;
        }


        // ------------------------------------------------------------
        // The entire context matched. Copy actions afterward.
        // ------------------------------------------------------------

        const uint16_t lookupCount = subst.sequenceLookupCount();
        candidate.lookups.reserve(lookupCount);

        for (uint16_t lookupIndex = 0; lookupIndex < lookupCount; ++lookupIndex)
        {
            OpenTypeSequenceLookup lookup{};

            if (!subst.sequenceLookup(lookupIndex, lookup))
                return OpenTypeGsubContextMatchResult::Invalid;

            candidate.lookups.push_back(lookup);
        }

        match = std::move(candidate);
        return OpenTypeGsubContextMatchResult::Match;
    }


    // ====================================================================
    // matchOpenTypeGsubContextSubst
    //
    // Match one Type 5 subtable at one physical buffer position.
    //
    // This performs no substitutions. The buffer remains unchanged.
    // ====================================================================

    static inline OpenTypeGsubContextMatchResult matchOpenTypeGsubContextSubst(
        const OpenTypeGsubContextSubstView& subst, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubContextMatch& match) noexcept
    {
        match.clear();

        if (!subst || !filter)
            return OpenTypeGsubContextMatchResult::Invalid;

        switch (subst.format())
        {
        case 1:
            return matchOpenTypeGsubContextFormat1(subst, filter, buffer, glyphIndex, match);

        case 2:
            return matchOpenTypeGsubContextFormat2(subst, filter, buffer, glyphIndex, match);

        case 3:
            return matchOpenTypeGsubContextFormat3(subst, filter, buffer, glyphIndex, match);

        default:
            return OpenTypeGsubContextMatchResult::Invalid;
        }
    }

} // namespace waavs