// test_opentype_gpos_chain_context_match.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_chain_context_match.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGposChainMatchU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGposChainMatchU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGposChainMatchCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, glyphId);
    }


    static void appendGposChainMatchClassRange(
        std::vector<uint8_t>& data, uint16_t first, uint16_t last, uint16_t classValue)
    {
        appendGposChainMatchU16(data, first);
        appendGposChainMatchU16(data, last);
        appendGposChainMatchU16(data, classValue);
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainMatchGdef()
    {
        std::vector<uint8_t> data;

        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, 0);

        appendGposChainMatchU16(data, 12);
        appendGposChainMatchU16(data, 0);
        appendGposChainMatchU16(data, 0);
        appendGposChainMatchU16(data, 0);

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 1);

        appendGposChainMatchU16(data, 100);
        appendGposChainMatchU16(data, 102);
        appendGposChainMatchU16(data, 3);

        return data;
    }


    // ====================================================================
    // Lookup wrapper.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainMatchLookup(
        const std::vector<uint8_t>& subtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposChainMatchU16(data, 8);
        appendGposChainMatchU16(data, lookupFlag);
        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    // ====================================================================
    // Format 1.
    //
    // Logical:
    //
    //   11 12 | 10 20 30 | 40 50
    //
    // Stored backtrack:
    //
    //   12 11
    // ====================================================================

    static void appendGposChainMatchFormat1Rule(
        std::vector<uint8_t>& data, uint16_t firstLookup)
    {
        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 12);
        appendGposChainMatchU16(data, 11);

        appendGposChainMatchU16(data, 3);
        appendGposChainMatchU16(data, 20);
        appendGposChainMatchU16(data, 30);

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 40);
        appendGposChainMatchU16(data, 50);

        appendGposChainMatchU16(data, 2);

        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, firstLookup);

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, static_cast<uint16_t>(firstLookup + 1));
    }


    static std::vector<uint8_t> makeGposChainMatchFormat1(bool twoRules = false)
    {
        std::vector<uint8_t> data;

        appendGposChainMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposChainMatchU16(data, 0);

        appendGposChainMatchU16(data, 1);

        const size_t setPatch = data.size();
        appendGposChainMatchU16(data, 0);


        patchGposChainMatchU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainMatchCoverage(data, 10);


        patchGposChainMatchU16(data, setPatch, static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposChainMatchU16(data, twoRules ? 2 : 1);

        const size_t rule0Patch = data.size();
        appendGposChainMatchU16(data, 0);

        size_t rule1Patch = 0;

        if (twoRules)
        {
            rule1Patch = data.size();
            appendGposChainMatchU16(data, 0);
        }

        patchGposChainMatchU16(data, rule0Patch, static_cast<uint16_t>(data.size() - setBegin));
        appendGposChainMatchFormat1Rule(data, 7);

        if (twoRules)
        {
            patchGposChainMatchU16(data, rule1Patch, static_cast<uint16_t>(data.size() - setBegin));
            appendGposChainMatchFormat1Rule(data, 9);
        }

        return data;
    }


    // ====================================================================
    // Format 2.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainMatchFormat2()
    {
        std::vector<uint8_t> data;

        appendGposChainMatchU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t backtrackClassDefPatch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t inputClassDefPatch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t lookaheadClassDefPatch = data.size();
        appendGposChainMatchU16(data, 0);

        appendGposChainMatchU16(data, 2);

        appendGposChainMatchU16(data, 0);

        const size_t classSetPatch = data.size();
        appendGposChainMatchU16(data, 0);


        patchGposChainMatchU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainMatchCoverage(data, 10);


        patchGposChainMatchU16(data, backtrackClassDefPatch, static_cast<uint16_t>(data.size()));

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 2);
        appendGposChainMatchClassRange(data, 11, 11, 3);
        appendGposChainMatchClassRange(data, 12, 12, 2);


        patchGposChainMatchU16(data, inputClassDefPatch, static_cast<uint16_t>(data.size()));

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 3);
        appendGposChainMatchClassRange(data, 10, 10, 1);
        appendGposChainMatchClassRange(data, 20, 20, 2);
        appendGposChainMatchClassRange(data, 30, 30, 3);


        patchGposChainMatchU16(data, lookaheadClassDefPatch, static_cast<uint16_t>(data.size()));

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 2);
        appendGposChainMatchClassRange(data, 40, 40, 4);
        appendGposChainMatchClassRange(data, 50, 50, 5);


        patchGposChainMatchU16(data, classSetPatch, static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposChainMatchU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposChainMatchU16(data, 0);

        patchGposChainMatchU16(data, rulePatch, static_cast<uint16_t>(data.size() - setBegin));


        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 3);

        appendGposChainMatchU16(data, 3);
        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 3);

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 4);
        appendGposChainMatchU16(data, 5);

        appendGposChainMatchU16(data, 2);

        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, 7);

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 8);

        return data;
    }


    // ====================================================================
    // Format 3.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainMatchFormat3()
    {
        std::vector<uint8_t> data;

        appendGposChainMatchU16(data, 3);

        appendGposChainMatchU16(data, 2);

        const size_t backtrack0Patch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t backtrack1Patch = data.size();
        appendGposChainMatchU16(data, 0);


        appendGposChainMatchU16(data, 3);

        const size_t input0Patch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t input1Patch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t input2Patch = data.size();
        appendGposChainMatchU16(data, 0);


        appendGposChainMatchU16(data, 2);

        const size_t lookahead0Patch = data.size();
        appendGposChainMatchU16(data, 0);

        const size_t lookahead1Patch = data.size();
        appendGposChainMatchU16(data, 0);


        appendGposChainMatchU16(data, 2);

        appendGposChainMatchU16(data, 1);
        appendGposChainMatchU16(data, 7);

        appendGposChainMatchU16(data, 2);
        appendGposChainMatchU16(data, 8);


        auto appendCoverage = [&](size_t patch, uint16_t glyphId)
            {
                patchGposChainMatchU16(data, patch, static_cast<uint16_t>(data.size()));
                appendGposChainMatchCoverage(data, glyphId);
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


    static std::vector<uint8_t> makeGposChainMatchFormat3InputOnly(uint16_t glyphId)
    {
        std::vector<uint8_t> data;

        appendGposChainMatchU16(data, 3);

        appendGposChainMatchU16(data, 0);

        appendGposChainMatchU16(data, 1);

        const size_t inputPatch = data.size();
        appendGposChainMatchU16(data, 0);

        appendGposChainMatchU16(data, 0);
        appendGposChainMatchU16(data, 0);

        patchGposChainMatchU16(data, inputPatch, static_cast<uint16_t>(data.size()));
        appendGposChainMatchCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Buffers.
    // ====================================================================

    static void appendGposChainMatchGlyph(ShapedGlyphBuffer& buffer, uint32_t glyphId)
    {
        ShapedGlyph glyph{};
        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;
        buffer.pushBack(glyph);
    }


    // Physical:
    //
    //   11 M 12 M 10 M 20 M 30 M 40 M 50

    static ShapedGlyphBuffer makeGposChainMatchFilteredBuffer()
    {
        ShapedGlyphBuffer buffer;

        const uint32_t glyphs[] =
        {
            11, 100,
            12, 101,
            10, 102,
            20, 100,
            30, 101,
            40, 102,
            50
        };

        for (uint32_t glyphId : glyphs)
            appendGposChainMatchGlyph(buffer, glyphId);

        return buffer;
    }


    static ShapedGlyphBuffer makeGposChainMatchAdjacentBuffer()
    {
        ShapedGlyphBuffer buffer;

        const uint32_t glyphs[] =
        {
            11, 12,
            10, 20, 30,
            40, 50
        };

        for (uint32_t glyphId : glyphs)
            appendGposChainMatchGlyph(buffer, glyphId);

        return buffer;
    }


    static bool gposChainMatchPositionsEqual(
        const OpenTypeGposChainContextMatch& match,
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


    static bool gposChainMatchLookupsEqual(
        const OpenTypeGposChainContextMatch& match,
        uint16_t firstLookup, uint16_t secondLookup) noexcept
    {
        if (match.lookups.size() != 2)
            return false;

        return
            match.lookups[0].sequenceIndex == 1 &&
            match.lookups[0].lookupListIndex == firstLookup &&
            match.lookups[1].sequenceIndex == 2 &&
            match.lookups[1].lookupListIndex == secondLookup;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposChainContextMatch()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS ChainContext match: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> gdefData = makeGposChainMatchGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

        if (!gdef)
            return fail("synthetic GDEF invalid");


        // ================================================================
        // Case 1 - Format 1 filtered traversal.
        //
        // Physical input positions:
        //
        //   {4,6,8}
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat1();
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer = makeGposChainMatchFilteredBuffer();
            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 4, match);

            const size_t expected[] = { 4, 6, 8 };

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 1 result");

            if (!gposChainMatchPositionsEqual(match, expected, 3))
                return fail("case 1 input positions");

            if (!gposChainMatchLookupsEqual(match, 7, 8))
                return fail("case 1 SequenceLookups");

            ++passed;
        }


        // ================================================================
        // Case 2 - Format 2 filtered traversal.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat2();
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer = makeGposChainMatchFilteredBuffer();
            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 4, match);

            const size_t expected[] = { 4, 6, 8 };

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 2 result");

            if (!gposChainMatchPositionsEqual(match, expected, 3))
                return fail("case 2 input positions");

            if (!gposChainMatchLookupsEqual(match, 7, 8))
                return fail("case 2 SequenceLookups");

            ++passed;
        }


        // ================================================================
        // Case 3 - Format 3 filtered traversal.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat3();
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer = makeGposChainMatchFilteredBuffer();
            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 4, match);

            const size_t expected[] = { 4, 6, 8 };

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 3 result");

            if (!gposChainMatchPositionsEqual(match, expected, 3))
                return fail("case 3 input positions");

            if (!gposChainMatchLookupsEqual(match, 7, 8))
                return fail("case 3 SequenceLookups");

            ++passed;
        }


        // ================================================================
        // Case 4 - Without IgnoreMarks, marks block the chain.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat3();
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData, 0);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer = makeGposChainMatchFilteredBuffer();
            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 4, match);

            if (result != OpenTypeGposChainContextMatchResult::NoMatch)
                return fail("case 4 blocking result");

            if (!match.empty() || !match.lookups.empty())
                return fail("case 4 partial match leaked");

            ++passed;
        }


        // ================================================================
        // Case 5 - Adjacent unfiltered matching.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat3();
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer = makeGposChainMatchAdjacentBuffer();
            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 2, match);

            const size_t expected[] = { 2, 3, 4 };

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 5 result");

            if (!gposChainMatchPositionsEqual(match, expected, 3))
                return fail("case 5 input positions");

            ++passed;
        }


        // ================================================================
        // Case 6 - Empty backtrack and lookahead.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat3InputOnly(10);
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer;
            appendGposChainMatchGlyph(buffer, 10);

            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 0, match);

            const size_t expected[] = { 0 };

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 6 result");

            if (!gposChainMatchPositionsEqual(match, expected, 1))
                return fail("case 6 positions");

            if (!match.lookups.empty())
                return fail("case 6 actions");

            ++passed;
        }


        // ================================================================
        // Case 7 - Start glyph is never filtered.
        //
        // Glyph 100 is a GDEF Mark and lookup has IgnoreMarks.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat3InputOnly(100);
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer;
            appendGposChainMatchGlyph(buffer, 100);

            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 0, match);

            const size_t expected[] = { 0 };

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 7 starting mark rejected");

            if (!gposChainMatchPositionsEqual(match, expected, 1))
                return fail("case 7 positions");

            ++passed;
        }


        // ================================================================
        // Case 8 - Stored rule order.
        //
        // Both Format 1 rules match. First stored rule must win.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat1(true);
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer = makeGposChainMatchAdjacentBuffer();
            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 2, match);

            if (result != OpenTypeGposChainContextMatchResult::Match)
                return fail("case 8 result");

            if (!gposChainMatchLookupsEqual(match, 7, 8))
                return fail("case 8 stored rule order");

            ++passed;
        }


        // ================================================================
        // Case 9 - Valid NoMatch clears result.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData = makeGposChainMatchFormat1();
            const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView emptyGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
            const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

            ShapedGlyphBuffer buffer;

            const uint32_t glyphs[] =
            {
                11, 13,
                10, 20, 30,
                40, 50
            };

            for (uint32_t glyphId : glyphs)
                appendGposChainMatchGlyph(buffer, glyphId);

            OpenTypeGposChainContextMatch match;
            match.inputPositions.push_back(99);
            match.lookups.push_back({ 99, 99 });

            const OpenTypeGposChainContextMatchResult result =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, 2, match);

            if (result != OpenTypeGposChainContextMatchResult::NoMatch)
                return fail("case 9 result");

            if (!match.empty() || !match.lookups.empty())
                return fail("case 9 stale result");

            ++passed;
        }


        // ================================================================
        // Case 10 - Failure paths.
        // ================================================================

        {
            ++cases;


            // Malformed input Coverage child.

            {
                std::vector<uint8_t> subtableData = makeGposChainMatchFormat3();

                // First input Coverage offset lives at byte 10.

                const uint16_t coverageOffset =
                    static_cast<uint16_t>(
                        (uint16_t(subtableData[10]) << 8) |
                        uint16_t(subtableData[11]));

                patchGposChainMatchU16(subtableData, coverageOffset, 9);

                const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

                ShapedGlyphBuffer buffer = makeGposChainMatchAdjacentBuffer();
                OpenTypeGposChainContextMatch match;

                const OpenTypeGposChainContextMatchResult result =
                    matchOpenTypeGposChainContextPos(pos, filter, buffer, 2, match);

                if (result != OpenTypeGposChainContextMatchResult::Invalid)
                    return fail("case 10 malformed child");
            }


            // Invalid start index.

            {
                const std::vector<uint8_t> subtableData = makeGposChainMatchFormat3InputOnly(10);
                const std::vector<uint8_t> lookupData = makeGposChainMatchLookup(subtableData);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView emptyGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, emptyGdef);
                const OpenTypeGposChainContextPosView pos(lookup.subtable(0));

                ShapedGlyphBuffer buffer;
                appendGposChainMatchGlyph(buffer, 10);

                OpenTypeGposChainContextMatch match;

                const OpenTypeGposChainContextMatchResult result =
                    matchOpenTypeGposChainContextPos(pos, filter, buffer, 1, match);

                if (result != OpenTypeGposChainContextMatchResult::Invalid)
                    return fail("case 10 invalid start");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS ChainContext match: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 filtering:         PASS\n"
            "  Format 2 filtering:         PASS\n"
            "  Format 3 filtering:         PASS\n"
            "  Unfiltered blocking:        PASS\n"
            "  Adjacent matching:          PASS\n"
            "  Empty context sides:        PASS\n"
            "  Start glyph semantics:      PASS\n"
            "  Stored rule order:          PASS\n"
            "  No-match behavior:          PASS\n"
            "  Failure paths:              PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs