// test_opentype_gpos_context_match.h
#pragma once

#include "../unitils/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_context_match.h"
#include "opentype_gdef_view.h"
#include "opentype_layout_view.h"

namespace waavs
{
    static void appendGposContextMatchU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGposContextMatchU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGposContextMatchCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposContextMatchU16(data, 1);
        appendGposContextMatchU16(data, 1);
        appendGposContextMatchU16(data, glyphId);
    }


    static void appendGposContextMatchClassRange(
        std::vector<uint8_t>& data, uint16_t first, uint16_t last, uint16_t classValue)
    {
        appendGposContextMatchU16(data, first);
        appendGposContextMatchU16(data, last);
        appendGposContextMatchU16(data, classValue);
    }


    static std::vector<uint8_t> makeGposContextMatchFormat1()
    {
        std::vector<uint8_t> data;

        appendGposContextMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposContextMatchU16(data, 0);

        appendGposContextMatchU16(data, 1);

        const size_t setPatch = data.size();
        appendGposContextMatchU16(data, 0);


        patchGposContextMatchU16(
            data, coveragePatch,
            static_cast<uint16_t>(data.size()));

        appendGposContextMatchCoverage1(data, 10);


        patchGposContextMatchU16(
            data, setPatch,
            static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposContextMatchU16(data, 2);

        const size_t rule0Patch = data.size();
        appendGposContextMatchU16(data, 0);

        const size_t rule1Patch = data.size();
        appendGposContextMatchU16(data, 0);


        // Rule 0 deliberately does not match:
        //
        //   10 99 30

        patchGposContextMatchU16(
            data, rule0Patch,
            static_cast<uint16_t>(data.size() - setBegin));

        appendGposContextMatchU16(data, 3);
        appendGposContextMatchU16(data, 1);

        appendGposContextMatchU16(data, 99);
        appendGposContextMatchU16(data, 30);

        appendGposContextMatchU16(data, 1);
        appendGposContextMatchU16(data, 8);


        // Rule 1 matches:
        //
        //   10 20 30

        patchGposContextMatchU16(
            data, rule1Patch,
            static_cast<uint16_t>(data.size() - setBegin));

        appendGposContextMatchU16(data, 3);
        appendGposContextMatchU16(data, 2);

        appendGposContextMatchU16(data, 20);
        appendGposContextMatchU16(data, 30);

        appendGposContextMatchU16(data, 0);
        appendGposContextMatchU16(data, 4);

        appendGposContextMatchU16(data, 2);
        appendGposContextMatchU16(data, 5);

        return data;
    }


    static std::vector<uint8_t> makeGposContextMatchFormat2()
    {
        std::vector<uint8_t> data;

        appendGposContextMatchU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposContextMatchU16(data, 0);

        const size_t classDefPatch = data.size();
        appendGposContextMatchU16(data, 0);

        appendGposContextMatchU16(data, 2);

        appendGposContextMatchU16(data, 0);

        const size_t classSetPatch = data.size();
        appendGposContextMatchU16(data, 0);


        patchGposContextMatchU16(
            data, coveragePatch,
            static_cast<uint16_t>(data.size()));

        appendGposContextMatchCoverage1(data, 10);


        patchGposContextMatchU16(
            data, classDefPatch,
            static_cast<uint16_t>(data.size()));

        appendGposContextMatchU16(data, 2);
        appendGposContextMatchU16(data, 3);

        appendGposContextMatchClassRange(data, 10, 10, 1);
        appendGposContextMatchClassRange(data, 20, 20, 2);
        appendGposContextMatchClassRange(data, 30, 30, 3);


        patchGposContextMatchU16(
            data, classSetPatch,
            static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposContextMatchU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposContextMatchU16(data, 0);

        patchGposContextMatchU16(
            data, rulePatch,
            static_cast<uint16_t>(data.size() - setBegin));

        appendGposContextMatchU16(data, 3);
        appendGposContextMatchU16(data, 1);

        appendGposContextMatchU16(data, 2);
        appendGposContextMatchU16(data, 3);

        appendGposContextMatchU16(data, 1);
        appendGposContextMatchU16(data, 7);

        return data;
    }


    static std::vector<uint8_t> makeGposContextMatchFormat3()
    {
        std::vector<uint8_t> data;

        appendGposContextMatchU16(data, 3);
        appendGposContextMatchU16(data, 3);
        appendGposContextMatchU16(data, 2);

        const size_t coverage0Patch = data.size();
        appendGposContextMatchU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGposContextMatchU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGposContextMatchU16(data, 0);

        appendGposContextMatchU16(data, 0);
        appendGposContextMatchU16(data, 4);

        appendGposContextMatchU16(data, 2);
        appendGposContextMatchU16(data, 5);


        patchGposContextMatchU16(
            data, coverage0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposContextMatchCoverage1(data, 10);


        patchGposContextMatchU16(
            data, coverage1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposContextMatchCoverage1(data, 20);


        patchGposContextMatchU16(
            data, coverage2Patch,
            static_cast<uint16_t>(data.size()));

        appendGposContextMatchCoverage1(data, 30);

        return data;
    }


    static std::vector<uint8_t> makeGposContextMatchLookup(
        const std::vector<uint8_t>& subtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposContextMatchU16(data, 7);
        appendGposContextMatchU16(data, lookupFlag);
        appendGposContextMatchU16(data, 1);
        appendGposContextMatchU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    // Glyph 100 is a Mark.

    static std::vector<uint8_t> makeGposContextMatchGdef()
    {
        std::vector<uint8_t> data;

        appendGposContextMatchU16(data, 1);
        appendGposContextMatchU16(data, 0);

        appendGposContextMatchU16(data, 12);
        appendGposContextMatchU16(data, 0);
        appendGposContextMatchU16(data, 0);
        appendGposContextMatchU16(data, 0);

        appendGposContextMatchU16(data, 2);
        appendGposContextMatchU16(data, 1);

        appendGposContextMatchU16(data, 100);
        appendGposContextMatchU16(data, 100);
        appendGposContextMatchU16(data, 3);

        return data;
    }


    static ShapedGlyph makeGposContextMatchGlyph(uint32_t glyphId)
    {
        ShapedGlyph glyph{};
        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;
        return glyph;
    }


    static ShapedGlyphBuffer makeGposContextMatchBuffer(bool interspersedMarks = false)
    {
        ShapedGlyphBuffer buffer;

        buffer.pushBack(makeGposContextMatchGlyph(10));

        if (interspersedMarks)
            buffer.pushBack(makeGposContextMatchGlyph(100));

        buffer.pushBack(makeGposContextMatchGlyph(20));

        if (interspersedMarks)
            buffer.pushBack(makeGposContextMatchGlyph(100));

        buffer.pushBack(makeGposContextMatchGlyph(30));

        return buffer;
    }


    static bool gposContextMatchPositionsEqual(
        const OpenTypeGposContextMatch& match,
        const size_t* expected, size_t count)
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


    static bool testOpenTypeGposContextMatch()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Context match: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        const std::vector<uint8_t> gdefData =
            makeGposContextMatchGdef();

        const OpenTypeGdefView gdef(
            ByteSpan(gdefData.data(), gdefData.size()));


        // ================================================================
        // Case 1 - Format 1 exact glyph match and rule order.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposContextMatchFormat1();

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer();

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Match)
            {
                return fail("case 1 Format 1 result");
            }

            const size_t expected[] = { 0, 1, 2 };

            if (!gposContextMatchPositionsEqual(
                match, expected, 3))
            {
                return fail("case 1 positions");
            }

            if (match.lookups.size() != 2 ||
                match.lookups[0].sequenceIndex != 0 ||
                match.lookups[0].lookupListIndex != 4 ||
                match.lookups[1].sequenceIndex != 2 ||
                match.lookups[1].lookupListIndex != 5)
            {
                return fail("case 1 actions");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Format 2 classes.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposContextMatchFormat2();

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer();

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Match)
            {
                return fail("case 2 Format 2 result");
            }

            const size_t expected[] = { 0, 1, 2 };

            if (!gposContextMatchPositionsEqual(
                match, expected, 3))
            {
                return fail("case 2 positions");
            }

            if (match.lookups.size() != 1 ||
                match.lookups[0].sequenceIndex != 1 ||
                match.lookups[0].lookupListIndex != 7)
            {
                return fail("case 2 action");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Format 3 Coverage sequence.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposContextMatchFormat3();

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer();

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Match)
            {
                return fail("case 3 Format 3 result");
            }

            const size_t expected[] = { 0, 1, 2 };

            if (!gposContextMatchPositionsEqual(
                match, expected, 3))
            {
                return fail("case 3 positions");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - LookupFlag filtering.
        //
        // Physical:
        //
        //   10 mark 20 mark 30
        //
        // IgnoreMarks produces logical context:
        //
        //   10 20 30
        //
        // Physical positions:
        //
        //   { 0, 2, 4 }
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposContextMatchFormat1();

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(
                    subtable, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer(true);

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Match)
            {
                return fail("case 4 filtered result");
            }

            const size_t expected[] = { 0, 2, 4 };

            if (!gposContextMatchPositionsEqual(
                match, expected, 3))
            {
                return fail("case 4 filtered positions");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Starting glyph is never filtered.
        //
        // Lookup says IgnoreMarks, but the current glyph itself is a mark.
        // Format 3 with one input position must still match it.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> subtable;

            appendGposContextMatchU16(subtable, 3);
            appendGposContextMatchU16(subtable, 1);
            appendGposContextMatchU16(subtable, 0);

            appendGposContextMatchU16(subtable, 8);

            appendGposContextMatchCoverage1(
                subtable, 100);

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(
                    subtable, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer;
            buffer.pushBack(
                makeGposContextMatchGlyph(100));

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Match)
            {
                return fail("case 5 starting mark");
            }

            const size_t expected[] = { 0 };

            if (!gposContextMatchPositionsEqual(
                match, expected, 1))
            {
                return fail("case 5 positions");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Bad middle glyph gives NoMatch.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposContextMatchFormat1();

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer();

            buffer[1].shaping.glyphId = 77;

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::NoMatch)
            {
                return fail("case 6 bad middle glyph");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Too-short buffer gives NoMatch.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposContextMatchFormat3();

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer;
            buffer.pushBack(makeGposContextMatchGlyph(10));
            buffer.pushBack(makeGposContextMatchGlyph(20));

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::NoMatch)
            {
                return fail("case 7 short buffer");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Dynamic SequenceLookup values are preserved.
        //
        // Matching does not decide whether sequenceIndex is executable.
        // Type 7 execution owns that check.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> subtable =
                makeGposContextMatchFormat3();

            // First SequenceLookupRecord begins at byte 12.

            patchGposContextMatchU16(
                subtable, 12, 0xFFFFu);

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer();

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Match)
            {
                return fail("case 8 context match");
            }

            if (match.lookups.size() != 2 ||
                match.lookups[0].sequenceIndex != 0xFFFFu)
            {
                return fail("case 8 SequenceLookup preservation");
            }

            ++passed;
        }


        // ================================================================
        // Case 9 - Malformed child is Invalid.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> subtable =
                makeGposContextMatchFormat3();

            // Coverage 0 offset is at byte 6.

            const uint16_t coverageOffset =
                static_cast<uint16_t>(
                    (uint16_t(subtable[6]) << 8) |
                    uint16_t(subtable[7]));

            patchGposContextMatchU16(
                subtable, coverageOffset, 9);

            const std::vector<uint8_t> lookupData =
                makeGposContextMatchLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposContextPosView pos(
                lookup.subtable(0));

            ShapedGlyphBuffer buffer =
                makeGposContextMatchBuffer();

            OpenTypeGposContextMatch match;

            if (matchOpenTypeGposContextPos(
                pos, filter, buffer, 0, match) !=
                OpenTypeGposContextMatchResult::Invalid)
            {
                return fail("case 9 malformed Coverage");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Context match: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 glyph rules:       PASS\n"
            "  Format 2 class rules:       PASS\n"
            "  Format 3 Coverage:          PASS\n"
            "  LookupFlag filtering:       PASS\n"
            "  Starting glyph unfiltered:  PASS\n"
            "  Valid NoMatch:              PASS\n"
            "  Short context:              PASS\n"
            "  SequenceLookup preservation:PASS\n"
            "  Malformed child:            PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs