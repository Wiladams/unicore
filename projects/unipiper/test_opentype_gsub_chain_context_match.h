// test_opentype_gsub_chain_context_match.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_chain_context_match.h"
#include "opentype_gdef_view.h"
#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"

namespace waavs
{
    // ====================================================================
    // Binary construction helpers.
    // ====================================================================

    static void appendGsubChainMatchU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubChainMatchU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGsubChainMatchCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, glyphId);
    }


    static void appendGsubChainMatchClassDef2(std::vector<uint8_t>& data, const uint16_t* starts,
        const uint16_t* ends, const uint16_t* classes, uint16_t count)
    {
        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
        {
            appendGsubChainMatchU16(data, starts[i]);
            appendGsubChainMatchU16(data, ends[i]);
            appendGsubChainMatchU16(data, classes[i]);
        }
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchGdef()
    {
        std::vector<uint8_t> data;

        // GDEF 1.0.

        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, 0);

        // GlyphClassDef offset.

        appendGsubChainMatchU16(data, 12);

        // AttachList, LigCaretList, MarkAttachClassDef.

        appendGsubChainMatchU16(data, 0);
        appendGsubChainMatchU16(data, 0);
        appendGsubChainMatchU16(data, 0);


        // GlyphClassDef Format 2:
        //
        // 100..102 -> class 3 (Mark).

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, 1);

        appendGsubChainMatchU16(data, 100);
        appendGsubChainMatchU16(data, 102);
        appendGsubChainMatchU16(data, 3);

        return data;
    }


    // ====================================================================
    // Wrap one Type 6 subtable in one Lookup.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchLookup(const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag)
    {
        std::vector<uint8_t> data;

        appendGsubChainMatchU16(data, 6);
        appendGsubChainMatchU16(data, lookupFlag);
        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    // ====================================================================
    // Format 1 rule.
    //
    // Logical chain:
    //
    //   11 12 | 10 20 30 | 40 50
    //
    // Backtrack is encoded nearest-first:
    //
    //   12 11
    //
    // Input 10 comes from parent Coverage. The rule stores only 20, 30.
    // ====================================================================

    static void appendGsubChainMatchFormat1Rule(std::vector<uint8_t>& data, uint16_t firstLookup)
    {
        // Backtrack.

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, 12);
        appendGsubChainMatchU16(data, 11);


        // Input.

        appendGsubChainMatchU16(data, 3);
        appendGsubChainMatchU16(data, 20);
        appendGsubChainMatchU16(data, 30);


        // Lookahead.

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, 40);
        appendGsubChainMatchU16(data, 50);


        // SequenceLookup records.

        appendGsubChainMatchU16(data, 2);

        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, firstLookup);

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, static_cast<uint16_t>(firstLookup + 1));
    }


    // ====================================================================
    // Format 1.
    //
    // Coverage:
    //
    //   10 -> RuleSet 0
    //
    // twoRules emits two identical matching rules with different actions so
    // stored rule order can be verified.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchFormat1(bool twoRules = false)
    {
        std::vector<uint8_t> data;

        appendGsubChainMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubChainMatchU16(data, 0);

        appendGsubChainMatchU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGsubChainMatchU16(data, 0);


        // RuleSet.

        const size_t ruleSetBase = data.size();
        patchGsubChainMatchU16(data, ruleSetPatch, static_cast<uint16_t>(ruleSetBase));

        appendGsubChainMatchU16(data, twoRules ? 2 : 1);

        const size_t rule0Patch = data.size();
        appendGsubChainMatchU16(data, 0);

        size_t rule1Patch = 0;

        if (twoRules)
        {
            rule1Patch = data.size();
            appendGsubChainMatchU16(data, 0);
        }


        // Rule 0.

        const size_t rule0Offset = data.size() - ruleSetBase;
        patchGsubChainMatchU16(data, rule0Patch, static_cast<uint16_t>(rule0Offset));

        appendGsubChainMatchFormat1Rule(data, twoRules ? 90 : 7);


        // Optional Rule 1.

        if (twoRules)
        {
            const size_t rule1Offset = data.size() - ruleSetBase;
            patchGsubChainMatchU16(data, rule1Patch, static_cast<uint16_t>(rule1Offset));

            appendGsubChainMatchFormat1Rule(data, 100);
        }


        // Coverage.

        const size_t coverageOffset = data.size();
        patchGsubChainMatchU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainMatchCoverage1(data, 10);

        return data;
    }


    // ====================================================================
    // Format 2.
    //
    // Backtrack ClassDef:
    //
    //   12 -> 1
    //   11 -> 2
    //
    // Input ClassDef:
    //
    //   10 -> 2
    //   20 -> 3
    //   30 -> 4
    //
    // Lookahead ClassDef:
    //
    //   40 -> 5
    //   50 -> 6
    //
    // Current input class 2 selects ClassSet 2.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchFormat2()
    {
        std::vector<uint8_t> data;

        appendGsubChainMatchU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t backtrackClassDefPatch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t inputClassDefPatch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t lookaheadClassDefPatch = data.size();
        appendGsubChainMatchU16(data, 0);


        // ClassSet count = 4.

        appendGsubChainMatchU16(data, 4);

        appendGsubChainMatchU16(data, 0);
        appendGsubChainMatchU16(data, 0);

        const size_t classSet2Patch = data.size();
        appendGsubChainMatchU16(data, 0);

        appendGsubChainMatchU16(data, 0);


        // ClassSet 2.

        const size_t classSetBase = data.size();
        patchGsubChainMatchU16(data, classSet2Patch, static_cast<uint16_t>(classSetBase));

        appendGsubChainMatchU16(data, 1);

        const size_t classRulePatch = data.size();
        appendGsubChainMatchU16(data, 0);


        // ClassRule offset is relative to ClassSet.

        const size_t classRuleOffset = data.size() - classSetBase;
        patchGsubChainMatchU16(data, classRulePatch, static_cast<uint16_t>(classRuleOffset));


        // Backtrack classes, nearest-first: 1, 2.

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, 2);


        // Input classes: current is class 2 from ClassSet selection;
        // remaining classes are 3, 4.

        appendGsubChainMatchU16(data, 3);
        appendGsubChainMatchU16(data, 3);
        appendGsubChainMatchU16(data, 4);


        // Lookahead classes.

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, 5);
        appendGsubChainMatchU16(data, 6);


        // SequenceLookup records.

        appendGsubChainMatchU16(data, 2);

        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, 7);

        appendGsubChainMatchU16(data, 2);
        appendGsubChainMatchU16(data, 8);


        // Coverage.

        const size_t coverageOffset = data.size();
        patchGsubChainMatchU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainMatchCoverage1(data, 10);


        // Backtrack ClassDef.

        const size_t backtrackClassDefOffset = data.size();
        patchGsubChainMatchU16(data, backtrackClassDefPatch,
            static_cast<uint16_t>(backtrackClassDefOffset));

        {
            const uint16_t starts[] = { 11, 12 };
            const uint16_t ends[] = { 11, 12 };
            const uint16_t classes[] = { 2, 1 };

            appendGsubChainMatchClassDef2(data, starts, ends, classes, 2);
        }


        // Input ClassDef.

        const size_t inputClassDefOffset = data.size();
        patchGsubChainMatchU16(data, inputClassDefPatch,
            static_cast<uint16_t>(inputClassDefOffset));

        {
            const uint16_t starts[] = { 10, 20, 30 };
            const uint16_t ends[] = { 10, 20, 30 };
            const uint16_t classes[] = { 2, 3, 4 };

            appendGsubChainMatchClassDef2(data, starts, ends, classes, 3);
        }


        // Lookahead ClassDef.

        const size_t lookaheadClassDefOffset = data.size();
        patchGsubChainMatchU16(data, lookaheadClassDefPatch,
            static_cast<uint16_t>(lookaheadClassDefOffset));

        {
            const uint16_t starts[] = { 40, 50 };
            const uint16_t ends[] = { 40, 50 };
            const uint16_t classes[] = { 5, 6 };

            appendGsubChainMatchClassDef2(data, starts, ends, classes, 2);
        }

        return data;
    }


    // ====================================================================
    // Format 3.
    //
    // Logical chain:
    //
    //   11 12 | 10 20 30 | 40 50
    //
    // Backtrack Coverage offsets are stored:
    //
    //   [12] [11]
    //
    // sequenceIndex0/1 are deliberately configurable so the matcher can be
    // tested for preserving dynamically interpreted SequenceLookup values.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchFormat3(uint16_t sequenceIndex0 = 1,
        uint16_t sequenceIndex1 = 2)
    {
        std::vector<uint8_t> data;

        appendGsubChainMatchU16(data, 3);


        // Backtrack Coverages.

        appendGsubChainMatchU16(data, 2);

        const size_t backtrack0Patch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t backtrack1Patch = data.size();
        appendGsubChainMatchU16(data, 0);


        // Input Coverages.

        appendGsubChainMatchU16(data, 3);

        const size_t input0Patch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t input1Patch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t input2Patch = data.size();
        appendGsubChainMatchU16(data, 0);


        // Lookahead Coverages.

        appendGsubChainMatchU16(data, 2);

        const size_t lookahead0Patch = data.size();
        appendGsubChainMatchU16(data, 0);

        const size_t lookahead1Patch = data.size();
        appendGsubChainMatchU16(data, 0);


        // SequenceLookup records.

        appendGsubChainMatchU16(data, 2);

        appendGsubChainMatchU16(data, sequenceIndex0);
        appendGsubChainMatchU16(data, 7);

        appendGsubChainMatchU16(data, sequenceIndex1);
        appendGsubChainMatchU16(data, 8);


        auto appendCoverage = [&](size_t patch, uint16_t glyphId)
            {
                const size_t offset = data.size();

                patchGsubChainMatchU16(data, patch, static_cast<uint16_t>(offset));
                appendGsubChainMatchCoverage1(data, glyphId);
            };


        appendCoverage(backtrack0Patch, 12);
        appendCoverage(backtrack1Patch, 11);

        appendCoverage(input0Patch, 10);
        appendCoverage(input1Patch, 20);
        appendCoverage(input2Patch, 30);

        appendCoverage(lookahead0Patch, 40);
        appendCoverage(lookahead1Patch, 50);

        return data;
    }


    // ====================================================================
    // Format 3 with:
    //
    //   zero backtrack
    //   one input glyph
    //   zero lookahead
    //   zero SequenceLookup records
    //
    // This is a successful match with no substitutions.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchFormat3InputOnly(uint16_t glyphId)
    {
        std::vector<uint8_t> data;

        appendGsubChainMatchU16(data, 3);

        appendGsubChainMatchU16(data, 0);

        appendGsubChainMatchU16(data, 1);

        const size_t inputPatch = data.size();
        appendGsubChainMatchU16(data, 0);

        appendGsubChainMatchU16(data, 0);
        appendGsubChainMatchU16(data, 0);


        const size_t coverageOffset = data.size();
        patchGsubChainMatchU16(data, inputPatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainMatchCoverage1(data, glyphId);

        return data;
    }


    // ====================================================================
    // Structurally valid Format 3 parent with malformed input Coverage.
    //
    // Coverage format 9 is unsupported. Parent validation remains lazy;
    // matcher access to the Coverage must produce Invalid.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainMatchMalformedFormat3()
    {
        std::vector<uint8_t> data;

        appendGsubChainMatchU16(data, 3);

        appendGsubChainMatchU16(data, 0);

        appendGsubChainMatchU16(data, 1);
        appendGsubChainMatchU16(data, 12);

        appendGsubChainMatchU16(data, 0);
        appendGsubChainMatchU16(data, 0);


        // Coverage begins at offset 12.

        appendGsubChainMatchU16(data, 9);

        return data;
    }


    // ====================================================================
    // Shaping-buffer helpers.
    // ====================================================================

    static void appendGsubChainMatchGlyph(OpenTypeShapingBuffer& buffer, uint32_t glyphId)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = static_cast<uint32_t>(buffer.size());
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    // Physical:
    //
    //   11 M 12 M 10 M 20 M 30 M 40 M 50
    //    0 1  2 3  4 5  6 7  8 9 10 11 12
    //
    // M = GDEF Mark.
    //
    // With IgnoreMarks:
    //
    //   backtrack: 12, 11 -> physical 2, 0
    //   input:     10,20,30 -> physical 4, 6, 8
    //   lookahead: 40,50    -> physical 10,12

    static OpenTypeShapingBuffer makeGsubChainMatchFilteredBuffer()
    {
        OpenTypeShapingBuffer buffer;

        const uint32_t glyphs[] =
        {
            11, 101,
            12, 100,
            10, 101,
            20, 102,
            30, 101,
            40, 102,
            50
        };

        for (uint32_t glyphId : glyphs)
            appendGsubChainMatchGlyph(buffer, glyphId);

        return buffer;
    }


    // Logical and physical positions are adjacent:
    //
    //   11 12 | 10 20 30 | 40 50
    //    0  1    2  3  4    5  6

    static OpenTypeShapingBuffer makeGsubChainMatchAdjacentBuffer()
    {
        OpenTypeShapingBuffer buffer;

        const uint32_t glyphs[] =
        {
            11, 12,
            10, 20, 30,
            40, 50
        };

        for (uint32_t glyphId : glyphs)
            appendGsubChainMatchGlyph(buffer, glyphId);

        return buffer;
    }


    static bool gsubChainMatchPositionsEqual(const OpenTypeGsubChainContextMatch& match,
        const size_t* expected, size_t count) noexcept
    {
        if (match.inputPositions.size() != count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (match.inputPositions[i] != expected[i])
                return false;
        }

        return true;
    }


    static bool gsubChainMatchLookupsEqual(const OpenTypeGsubChainContextMatch& match,
        uint16_t sequenceIndex0, uint16_t lookup0,
        uint16_t sequenceIndex1, uint16_t lookup1) noexcept
    {
        if (match.lookups.size() != 2)
            return false;

        return match.lookups[0].sequenceIndex == sequenceIndex0 &&
            match.lookups[0].lookupListIndex == lookup0 &&
            match.lookups[1].sequenceIndex == sequenceIndex1 &&
            match.lookups[1].lookupListIndex == lookup1;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGsubChainContextMatch()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB ChainContextSubst matcher: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> gdefData = makeGsubChainMatchGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

        if (!gdef)
            return fail("synthetic GDEF invalid");


        // ====================================================================
        // Case 1 - Format 1 filtered traversal.
        //
        // Proves:
        //
        //   backtrack uses previous()
        //   input remainder uses next()
        //   lookahead uses next()
        //   ignored marks are not retained as input positions
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubChainMatchFormat1();
            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchFilteredBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 4, match);

            const size_t expected[] = { 4, 6, 8 };

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 1 result");

            if (!gsubChainMatchPositionsEqual(match, expected, 3))
                return fail("case 1 input positions");

            if (!gsubChainMatchLookupsEqual(match, 1, 7, 2, 8))
                return fail("case 1 SequenceLookups");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 2 filtered traversal.
        //
        // Proves that three independent ClassDefs are used.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubChainMatchFormat2();
            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchFilteredBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 4, match);

            const size_t expected[] = { 4, 6, 8 };

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 2 result");

            if (!gsubChainMatchPositionsEqual(match, expected, 3))
                return fail("case 2 input positions");

            if (!gsubChainMatchLookupsEqual(match, 1, 7, 2, 8))
                return fail("case 2 SequenceLookups");

            ++passed;
        }


        // ====================================================================
        // Case 3 - Format 3 filtered traversal.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubChainMatchFormat3();
            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchFilteredBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 4, match);

            const size_t expected[] = { 4, 6, 8 };

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 3 result");

            if (!gsubChainMatchPositionsEqual(match, expected, 3))
                return fail("case 3 input positions");

            if (!gsubChainMatchLookupsEqual(match, 1, 7, 2, 8))
                return fail("case 3 SequenceLookups");

            ++passed;
        }


        // ====================================================================
        // Case 4 - Without IgnoreMarks, interspersed marks block matching.
        //
        // A mark exists in each traversal region, so this exercises outer
        // filtering across backtrack, input, and lookahead.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubChainMatchFormat3();
            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);

            if (!filter)
                return fail("case 4 zero-flag filter invalid");

            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchFilteredBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 4, match);

            if (result != OpenTypeGsubChainContextMatchResult::NoMatch)
                return fail("case 4 marks did not block");

            if (!match.empty() || !match.lookups.empty())
                return fail("case 4 partial match leaked");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Backtrack is nearest-first.
        //
        // Logical:
        //
        //   11 12 | 10
        //
        // Encoded:
        //
        //   12 11
        //
        // Reading the encoded sequence in logical forward order would fail.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGsubChainMatchFormat3();
            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchAdjacentBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 2, match);

            const size_t expected[] = { 2, 3, 4 };

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 5 result");

            if (!gsubChainMatchPositionsEqual(match, expected, 3))
                return fail("case 5 positions");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Zero backtrack, lookahead, and actions.
        //
        // A chained context may contain only its required input sequence.
        // Matching with zero SequenceLookup records is still Match.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubChainMatchFormat3InputOnly(10);

            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer;
            appendGsubChainMatchGlyph(buffer, 10);

            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0 };

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 6 result");

            if (!gsubChainMatchPositionsEqual(match, expected, 1))
                return fail("case 6 positions");

            if (!match.lookups.empty())
                return fail("case 6 unexpected actions");

            ++passed;
        }


        // ====================================================================
        // Case 7 - Current glyph is never filtered.
        //
        // Glyph 100 is a GDEF Mark. IgnoreMarks applies while traversing other
        // glyphs, not to the current physical lookup position.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubChainMatchFormat3InputOnly(100);

            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer;
            appendGsubChainMatchGlyph(buffer, 100);

            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 0, match);

            const size_t expected[] = { 0 };

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 7 starting mark rejected");

            if (!gsubChainMatchPositionsEqual(match, expected, 1))
                return fail("case 7 positions");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Stored rule order.
        //
        // Both Format 1 rules match. The first stored rule wins.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubChainMatchFormat1(true);

            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchAdjacentBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 2, match);

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 8 result");

            if (!gsubChainMatchLookupsEqual(match, 1, 90, 2, 91))
                return fail("case 8 first stored rule not selected");

            ++passed;
        }


        // ====================================================================
        // Case 9 - Dynamic SequenceLookup preservation.
        //
        // sequenceIndex belongs to the mutable current input sequence during
        // execution. The matcher must not validate it against the original
        // inputGlyphCount.
        //
        // 0xFFFF is intentionally impossible for this original 3-glyph input,
        // but must survive matching unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubChainMatchFormat3(1, 0xFFFFu);

            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

            OpenTypeShapingBuffer buffer = makeGsubChainMatchAdjacentBuffer();
            OpenTypeGsubChainContextMatch match;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 2, match);

            if (result != OpenTypeGsubChainContextMatchResult::Match)
                return fail("case 9 result");

            if (!gsubChainMatchLookupsEqual(match, 1, 7, 0xFFFFu, 8))
                return fail("case 9 dynamic sequenceIndex changed");

            ++passed;
        }


        // ====================================================================
        // Case 10 - Valid NoMatch behavior.
        //
        // Exercise both backtrack and lookahead mismatch paths.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubChainMatchFormat3();

            const std::vector<uint8_t> lookupData =
                makeGsubChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));


            // Bad nearest backtrack.

            {
                OpenTypeShapingBuffer buffer = makeGsubChainMatchAdjacentBuffer();
                buffer[1].glyphId = 99;

                OpenTypeGsubChainContextMatch match;

                const OpenTypeGsubChainContextMatchResult result =
                    matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 2, match);

                if (result != OpenTypeGsubChainContextMatchResult::NoMatch)
                    return fail("case 10 bad backtrack");

                if (!match.empty() || !match.lookups.empty())
                    return fail("case 10 backtrack partial match leaked");
            }


            // Bad final lookahead.

            {
                OpenTypeShapingBuffer buffer = makeGsubChainMatchAdjacentBuffer();
                buffer[6].glyphId = 99;

                OpenTypeGsubChainContextMatch match;

                const OpenTypeGsubChainContextMatchResult result =
                    matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 2, match);

                if (result != OpenTypeGsubChainContextMatchResult::NoMatch)
                    return fail("case 10 bad lookahead");

                if (!match.empty() || !match.lookups.empty())
                    return fail("case 10 lookahead partial match leaked");
            }

            ++passed;
        }


        // ====================================================================
        // Case 11 - Failure paths.
        // ====================================================================

        {
            ++cases;


            // IgnoreMarks requires usable GDEF information.

            {
                const std::vector<uint8_t> subtableData =
                    makeGsubChainMatchFormat3();

                const std::vector<uint8_t> lookupData =
                    makeGsubChainMatchLookup(subtableData, 0x0008u);

                const OpenTypeLayoutLookupView lookup(
                    ByteSpan(lookupData.data(), lookupData.size()));

                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);

                if (filter)
                    return fail("case 11 missing GDEF accepted");
            }


            // Malformed lazy Coverage reaches matcher as Invalid.

            {
                const std::vector<uint8_t> subtableData =
                    makeGsubChainMatchMalformedFormat3();

                const std::vector<uint8_t> lookupData =
                    makeGsubChainMatchLookup(subtableData, 0);

                const OpenTypeLayoutLookupView lookup(
                    ByteSpan(lookupData.data(), lookupData.size()));

                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

                if (!subst)
                    return fail("case 11 malformed parent rejected too early");

                OpenTypeShapingBuffer buffer;
                appendGsubChainMatchGlyph(buffer, 10);

                OpenTypeGsubChainContextMatch match;

                const OpenTypeGsubChainContextMatchResult result =
                    matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 0, match);

                if (result != OpenTypeGsubChainContextMatchResult::Invalid)
                    return fail("case 11 malformed Coverage");
            }


            // Invalid start position.

            {
                const std::vector<uint8_t> subtableData =
                    makeGsubChainMatchFormat3InputOnly(10);

                const std::vector<uint8_t> lookupData =
                    makeGsubChainMatchLookup(subtableData, 0);

                const OpenTypeLayoutLookupView lookup(
                    ByteSpan(lookupData.data(), lookupData.size()));

                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

                OpenTypeShapingBuffer buffer;
                appendGsubChainMatchGlyph(buffer, 10);

                OpenTypeGsubChainContextMatch match;

                const OpenTypeGsubChainContextMatchResult result =
                    matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 1, match);

                if (result != OpenTypeGsubChainContextMatchResult::Invalid)
                    return fail("case 11 invalid start");
            }


            // OpenType glyph IDs are 16-bit.

            {
                const std::vector<uint8_t> subtableData =
                    makeGsubChainMatchFormat3InputOnly(10);

                const std::vector<uint8_t> lookupData =
                    makeGsubChainMatchLookup(subtableData, 0);

                const OpenTypeLayoutLookupView lookup(
                    ByteSpan(lookupData.data(), lookupData.size()));

                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGsubChainContextSubstView subst(lookup.subtable(0));

                OpenTypeShapingBuffer buffer;
                appendGsubChainMatchGlyph(buffer, 0x10000u);

                OpenTypeGsubChainContextMatch match;

                const OpenTypeGsubChainContextMatchResult result =
                    matchOpenTypeGsubChainContextSubst(subst, filter, buffer, 0, match);

                if (result != OpenTypeGsubChainContextMatchResult::Invalid)
                    return fail("case 11 glyph ID above 0xFFFF");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ChainContextSubst matcher: PASS\n"
            "  Cases:                       %u\n"
            "  Passed:                      %u\n"
            "  Format 1 filtering:          PASS\n"
            "  Format 2 filtering:          PASS\n"
            "  Format 3 filtering:          PASS\n"
            "  Unfiltered blocking:         PASS\n"
            "  Nearest-first backtrack:     PASS\n"
            "  Empty backtrack/lookahead:   PASS\n"
            "  Start glyph semantics:       PASS\n"
            "  Stored rule order:           PASS\n"
            "  Dynamic SequenceLookup:      PASS\n"
            "  No-match behavior:           PASS\n"
            "  Failure paths:               PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs