// test_opentype_gsub_context_match.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_context_match.h"
#include "opentype_gdef_view.h"
#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"

namespace waavs
{
    // ====================================================================
    // Binary construction helpers.
    // ====================================================================

    static void appendGsubContextMatchU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubContextMatchU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGsubContextMatchCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, glyphId);
    }


    static void appendGsubContextMatchClassDef2(std::vector<uint8_t>& data, const uint16_t* starts,
        const uint16_t* ends, const uint16_t* classes, uint16_t count)
    {
        appendGsubContextMatchU16(data, 2);
        appendGsubContextMatchU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
        {
            appendGsubContextMatchU16(data, starts[i]);
            appendGsubContextMatchU16(data, ends[i]);
            appendGsubContextMatchU16(data, classes[i]);
        }
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    //
    // This is sufficient for IgnoreMarks tests. All other glyphs fall
    // through to GDEF glyph class 0.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchGdef()
    {
        std::vector<uint8_t> data;

        // GDEF version 1.0.

        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 0);

        // GlyphClassDef offset = 12.

        appendGsubContextMatchU16(data, 12);

        // AttachList, LigCaretList, MarkAttachClassDef.

        appendGsubContextMatchU16(data, 0);
        appendGsubContextMatchU16(data, 0);
        appendGsubContextMatchU16(data, 0);


        // GlyphClassDef Format 2:
        //
        // glyphs 100..102 -> class 3 (Mark).

        appendGsubContextMatchU16(data, 2);
        appendGsubContextMatchU16(data, 1);

        appendGsubContextMatchU16(data, 100);
        appendGsubContextMatchU16(data, 102);
        appendGsubContextMatchU16(data, 3);

        return data;
    }


    // ====================================================================
    // Wrap one Type 5 subtable in a Lookup.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchLookup(const std::vector<uint8_t>& subtable, uint16_t lookupFlag)
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 5);
        appendGsubContextMatchU16(data, lookupFlag);
        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    // ====================================================================
    // Format 1 rule.
    //
    // Input:
    //
    //   10 20 30
    //
    // The first glyph, 10, comes from Coverage.
    // ====================================================================

    static void appendGsubContextMatchFormat1Rule(std::vector<uint8_t>& data, uint16_t firstLookup)
    {
        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 2);

        appendGsubContextMatchU16(data, 20);
        appendGsubContextMatchU16(data, 30);

        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, firstLookup);

        appendGsubContextMatchU16(data, 2);
        appendGsubContextMatchU16(data, static_cast<uint16_t>(firstLookup + 1));
    }


    // ====================================================================
    // Format 1.
    //
    // Coverage:
    //
    //   glyph 10 -> RuleSet 0
    //
    // Normally one rule:
    //
    //   input 10 20 30
    //   SequenceLookup { 1, 7 }
    //   SequenceLookup { 2, 8 }
    //
    // If twoRules is true, two identical rules are emitted. Their actions
    // differ so stored rule order can be verified.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchFormat1(bool twoRules = false)
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubContextMatchU16(data, 0);

        appendGsubContextMatchU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGsubContextMatchU16(data, 0);


        // RuleSet.

        const size_t ruleSetBase = data.size();
        patchGsubContextMatchU16(data, ruleSetPatch, static_cast<uint16_t>(ruleSetBase));

        appendGsubContextMatchU16(data, twoRules ? 2 : 1);

        const size_t rule0Patch = data.size();
        appendGsubContextMatchU16(data, 0);

        size_t rule1Patch = 0;

        if (twoRules)
        {
            rule1Patch = data.size();
            appendGsubContextMatchU16(data, 0);
        }


        // Rule 0.
        //
        // Rule offset is relative to RuleSet.

        const size_t rule0Offset = data.size() - ruleSetBase;
        patchGsubContextMatchU16(data, rule0Patch, static_cast<uint16_t>(rule0Offset));

        appendGsubContextMatchFormat1Rule(data, twoRules ? 90 : 7);


        // Optional identical Rule 1.

        if (twoRules)
        {
            const size_t rule1Offset = data.size() - ruleSetBase;
            patchGsubContextMatchU16(data, rule1Patch, static_cast<uint16_t>(rule1Offset));

            appendGsubContextMatchFormat1Rule(data, 100);
        }


        // Coverage.

        const size_t coverageOffset = data.size();
        patchGsubContextMatchU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubContextMatchCoverage1(data, 10);

        return data;
    }


    // ====================================================================
    // Format 1 with legal NULL RuleSet.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchFormat1NullRuleSet()
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubContextMatchU16(data, 0);

        appendGsubContextMatchU16(data, 1);

        // RuleSet 0 = NULL.

        appendGsubContextMatchU16(data, 0);


        const size_t coverageOffset = data.size();
        patchGsubContextMatchU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubContextMatchCoverage1(data, 10);

        return data;
    }


    // ====================================================================
    // Format 2.
    //
    // Coverage:
    //
    //   glyph 10
    //
    // ClassDef:
    //
    //   10 -> class 2
    //   20 -> class 3
    //   30 -> class 4
    //
    // ClassSet 2:
    //
    //   classes 2, 3, 4
    //
    //   SequenceLookup { 1, 7 }
    //   SequenceLookup { 2, 8 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchFormat2()
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubContextMatchU16(data, 0);

        const size_t classDefPatch = data.size();
        appendGsubContextMatchU16(data, 0);

        appendGsubContextMatchU16(data, 4);

        appendGsubContextMatchU16(data, 0);
        appendGsubContextMatchU16(data, 0);

        const size_t classSet2Patch = data.size();
        appendGsubContextMatchU16(data, 0);

        appendGsubContextMatchU16(data, 0);


        // ClassSet 2.

        const size_t classSetBase = data.size();
        patchGsubContextMatchU16(data, classSet2Patch, static_cast<uint16_t>(classSetBase));

        appendGsubContextMatchU16(data, 1);

        const size_t classRulePatch = data.size();
        appendGsubContextMatchU16(data, 0);


        // ClassRule offset is relative to ClassSet.

        const size_t classRuleOffset = data.size() - classSetBase;
        patchGsubContextMatchU16(data, classRulePatch, static_cast<uint16_t>(classRuleOffset));


        // glyphCount = 3
        // seqLookupCount = 2

        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 2);


        // Input positions 1 and 2.

        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 4);


        // Actions.

        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 7);

        appendGsubContextMatchU16(data, 2);
        appendGsubContextMatchU16(data, 8);


        // Coverage.

        const size_t coverageOffset = data.size();
        patchGsubContextMatchU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubContextMatchCoverage1(data, 10);


        // ClassDef.

        const size_t classDefOffset = data.size();
        patchGsubContextMatchU16(data, classDefPatch, static_cast<uint16_t>(classDefOffset));

        {
            const uint16_t starts[] = { 10, 20, 30 };
            const uint16_t ends[] = { 10, 20, 30 };
            const uint16_t classes[] = { 2, 3, 4 };

            appendGsubContextMatchClassDef2(data, starts, ends, classes, 3);
        }

        return data;
    }


    // ====================================================================
    // Format 3.
    //
    // Input Coverages:
    //
    //   0 -> glyph 10
    //   1 -> glyph 20
    //   2 -> glyph 30
    //
    // SequenceLookup:
    //
    //   { 1, 7 }
    //   { 2, 8 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchFormat3()
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 2);

        const size_t coverage0Patch = data.size();
        appendGsubContextMatchU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGsubContextMatchU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGsubContextMatchU16(data, 0);


        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 7);

        appendGsubContextMatchU16(data, 2);
        appendGsubContextMatchU16(data, 8);


        auto appendCoverage = [&](size_t patch, uint16_t glyphId)
            {
                const size_t offset = data.size();

                patchGsubContextMatchU16(data, patch, static_cast<uint16_t>(offset));
                appendGsubContextMatchCoverage1(data, glyphId);
            };


        appendCoverage(coverage0Patch, 10);
        appendCoverage(coverage1Patch, 20);
        appendCoverage(coverage2Patch, 30);

        return data;
    }


    // ====================================================================
    // Format 3 containing one input glyph and no actions.
    //
    // A successful context match with zero SequenceLookup records is legal.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchFormat3InputOnly(uint16_t glyphId)
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 0);

        const size_t coveragePatch = data.size();
        appendGsubContextMatchU16(data, 0);


        const size_t coverageOffset = data.size();
        patchGsubContextMatchU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubContextMatchCoverage1(data, glyphId);

        return data;
    }


    // ====================================================================
    // Valid Format 3 parent with malformed Coverage child.
    //
    // The view should remain structurally valid. The matcher discovers the
    // malformed child when it attempts to use Coverage.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextMatchMalformedFormat3()
    {
        std::vector<uint8_t> data;

        appendGsubContextMatchU16(data, 3);
        appendGsubContextMatchU16(data, 1);
        appendGsubContextMatchU16(data, 0);

        appendGsubContextMatchU16(data, 8);

        // Unsupported Coverage format.

        appendGsubContextMatchU16(data, 9);

        return data;
    }


    // ====================================================================
    // Shaping-buffer helpers.
    // ====================================================================

    static void appendGsubContextMatchGlyph(OpenTypeShapingBuffer& buffer, uint32_t glyphId)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = static_cast<uint32_t>(buffer.size());
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubContextMatchFilteredBuffer()
    {
        OpenTypeShapingBuffer buffer;

        const uint32_t glyphs[] =
        {
            10, 100,
            20, 101,
            30
        };

        for (uint32_t glyphId : glyphs)
            appendGsubContextMatchGlyph(buffer, glyphId);

        return buffer;
    }


    static OpenTypeShapingBuffer makeGsubContextMatchAdjacentBuffer()
    {
        OpenTypeShapingBuffer buffer;

        appendGsubContextMatchGlyph(buffer, 10);
        appendGsubContextMatchGlyph(buffer, 20);
        appendGsubContextMatchGlyph(buffer, 30);

        return buffer;
    }


    static bool gsubContextMatchPositionsEqual(const OpenTypeGsubContextMatch& match,
        const size_t* expected, size_t count) noexcept
    {
        if (match.positions.size() != count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (match.positions[i] != expected[i])
                return false;
        }

        return true;
    }


    static bool gsubContextMatchLookupsEqual(const OpenTypeGsubContextMatch& match,
        uint16_t firstLookup, uint16_t secondLookup) noexcept
    {
        if (match.lookups.size() != 2)
            return false;

        return match.lookups[0].sequenceIndex == 1 &&
            match.lookups[0].lookupListIndex == firstLookup &&
            match.lookups[1].sequenceIndex == 2 &&
            match.lookups[1].lookupListIndex == secondLookup;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGsubContextMatch()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB ContextSubst matcher: FAIL\n  %s\n", message);
                return false;
            };


        const std::vector<uint8_t> gdefData = makeGsubContextMatchGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

        if (!gdef)
            return fail("synthetic GDEF invalid");


        // ====================================================================
        // Case 1 - Format 1 filtered input matching.
        //
        // Physical:
        //
        //   10 M 20 M 30
        //    0 1  2 3  4
        //
        // IgnoreMarks produces:
        //
        //   positions = { 0, 2, 4 }
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat1();
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubContextMatchFilteredBuffer();
            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0, 2, 4 };

            if (result != OpenTypeGsubContextMatchResult::Match)
                return fail("case 1 result");

            if (!gsubContextMatchPositionsEqual(match, expected, 3))
                return fail("case 1 positions");

            if (!gsubContextMatchLookupsEqual(match, 7, 8))
                return fail("case 1 SequenceLookups");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 2 filtered input matching.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat2();
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubContextMatchFilteredBuffer();
            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0, 2, 4 };

            if (result != OpenTypeGsubContextMatchResult::Match)
                return fail("case 2 result");

            if (!gsubContextMatchPositionsEqual(match, expected, 3))
                return fail("case 2 positions");

            if (!gsubContextMatchLookupsEqual(match, 7, 8))
                return fail("case 2 SequenceLookups");

            ++passed;
        }


        // ====================================================================
        // Case 3 - Format 3 filtered input matching.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3();
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubContextMatchFilteredBuffer();
            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0, 2, 4 };

            if (result != OpenTypeGsubContextMatchResult::Match)
                return fail("case 3 result");

            if (!gsubContextMatchPositionsEqual(match, expected, 3))
                return fail("case 3 positions");

            if (!gsubContextMatchLookupsEqual(match, 7, 8))
                return fail("case 3 SequenceLookups");

            ++passed;
        }


        // ====================================================================
        // Case 4 - Without IgnoreMarks, marks block matching.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3();
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubContextMatchFilteredBuffer();
            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            if (result != OpenTypeGsubContextMatchResult::NoMatch)
                return fail("case 4 marks did not block");

            if (!match.empty() || !match.lookups.empty())
                return fail("case 4 partial match leaked");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Current/start glyph is never filtered.
        //
        // Glyph 100 is a GDEF Mark. IgnoreMarks applies only while traversing
        // other glyphs; the initial lookup position is still matched directly.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3InputOnly(100);
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer;
            appendGsubContextMatchGlyph(buffer, 100);

            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0 };

            if (result != OpenTypeGsubContextMatchResult::Match)
                return fail("case 5 starting mark rejected");

            if (!gsubContextMatchPositionsEqual(match, expected, 1))
                return fail("case 5 positions");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Stored rule order.
        //
        // Both Format 1 rules match the same input sequence. The first stored
        // rule must win.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat1(true);
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubContextMatchAdjacentBuffer();
            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            if (result != OpenTypeGsubContextMatchResult::Match)
                return fail("case 6 result");

            if (!gsubContextMatchLookupsEqual(match, 90, 91))
                return fail("case 6 first stored rule not selected");

            ++passed;
        }


        // ====================================================================
        // Case 7 - Match with zero SequenceLookup actions.
        //
        // Matching context and performing substitutions are separate concepts.
        // A context containing no actions is still a successful Match.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3InputOnly(10);
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer;
            appendGsubContextMatchGlyph(buffer, 10);

            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0 };

            if (result != OpenTypeGsubContextMatchResult::Match)
                return fail("case 7 result");

            if (!gsubContextMatchPositionsEqual(match, expected, 1))
                return fail("case 7 positions");

            if (!match.lookups.empty())
                return fail("case 7 unexpected actions");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Valid NoMatch behavior.
        //
        // Exercise both initial Coverage failure and a later input mismatch.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3();
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));


            // Initial Coverage failure.

            {
                OpenTypeShapingBuffer buffer = makeGsubContextMatchAdjacentBuffer();
                buffer[0].glyphId = 99;

                OpenTypeGsubContextMatch match;

                const OpenTypeGsubContextMatchResult result =
                    matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

                if (result != OpenTypeGsubContextMatchResult::NoMatch)
                    return fail("case 8 initial Coverage");

                if (!match.empty() || !match.lookups.empty())
                    return fail("case 8 initial partial match");
            }


            // Later input mismatch.

            {
                OpenTypeShapingBuffer buffer = makeGsubContextMatchAdjacentBuffer();
                buffer[1].glyphId = 99;

                OpenTypeGsubContextMatch match;

                const OpenTypeGsubContextMatchResult result =
                    matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

                if (result != OpenTypeGsubContextMatchResult::NoMatch)
                    return fail("case 8 later input");

                if (!match.empty() || !match.lookups.empty())
                    return fail("case 8 later partial match");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Legal NULL RuleSet produces NoMatch.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat1NullRuleSet();
            const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer;
            appendGsubContextMatchGlyph(buffer, 10);

            OpenTypeGsubContextMatch match;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

            if (result != OpenTypeGsubContextMatchResult::NoMatch)
                return fail("case 9 NULL RuleSet");

            if (!match.empty() || !match.lookups.empty())
                return fail("case 9 partial match leaked");

            ++passed;
        }


        // ====================================================================
        // Case 10 - Failure paths.
        // ====================================================================

        {
            ++cases;


            // IgnoreMarks requires usable GDEF data.

            {
                const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3();
                const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0x0008u);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);

                if (filter)
                    return fail("case 10 missing GDEF accepted");
            }


            // Malformed lazy Coverage reaches the matcher as Invalid.

            {
                const std::vector<uint8_t> subtableData = makeGsubContextMatchMalformedFormat3();
                const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

                if (!subst)
                    return fail("case 10 malformed parent rejected too early");

                OpenTypeShapingBuffer buffer;
                appendGsubContextMatchGlyph(buffer, 10);

                OpenTypeGsubContextMatch match;

                const OpenTypeGsubContextMatchResult result =
                    matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

                if (result != OpenTypeGsubContextMatchResult::Invalid)
                    return fail("case 10 malformed Coverage");
            }


            // Invalid physical start position.

            {
                const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3InputOnly(10);
                const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

                OpenTypeShapingBuffer buffer;
                appendGsubContextMatchGlyph(buffer, 10);

                OpenTypeGsubContextMatch match;

                const OpenTypeGsubContextMatchResult result =
                    matchOpenTypeGsubContextSubst(subst, filter, buffer, 1, match);

                if (result != OpenTypeGsubContextMatchResult::Invalid)
                    return fail("case 10 invalid start");
            }


            // A non-OpenType glyph ID cannot participate in GSUB matching.

            {
                const std::vector<uint8_t> subtableData = makeGsubContextMatchFormat3InputOnly(10);
                const std::vector<uint8_t> lookupData = makeGsubContextMatchLookup(subtableData, 0);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGsubContextSubstView subst(lookup.subtable(0));

                OpenTypeShapingBuffer buffer;
                appendGsubContextMatchGlyph(buffer, 0x10000u);

                OpenTypeGsubContextMatch match;

                const OpenTypeGsubContextMatchResult result =
                    matchOpenTypeGsubContextSubst(subst, filter, buffer, 0, match);

                if (result != OpenTypeGsubContextMatchResult::Invalid)
                    return fail("case 10 glyph ID above 0xFFFF");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ContextSubst matcher: PASS\n"
            "  Cases:                       %u\n"
            "  Passed:                      %u\n"
            "  Format 1 filtering:          PASS\n"
            "  Format 2 filtering:          PASS\n"
            "  Format 3 filtering:          PASS\n"
            "  Unfiltered blocking:         PASS\n"
            "  Start glyph semantics:       PASS\n"
            "  Stored rule order:           PASS\n"
            "  Zero-action match:           PASS\n"
            "  No-match behavior:           PASS\n"
            "  Nullable RuleSet:            PASS\n"
            "  Failure paths:               PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs