// test_opentype_gsub_single_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_single_view.h"

namespace waavs
{
    static void appendGsubSingleTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubSingleTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeGsubSingleFormat1TestData()
    {
        std::vector<uint8_t> data;

        // SingleSubst Format 1
        //
        // covered glyphs:
        //   20, 50, 100
        //
        // delta = -3
        //
        // substitutions:
        //   20  -> 17
        //   50  -> 47
        //   100 -> 97

        appendGsubSingleTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubSingleTestU16(data, 0);

        appendGsubSingleTestU16(data, 0xFFFD); // -3


        const size_t coverageOffset = data.size();
        patchGsubSingleTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        // Coverage Format 1

        appendGsubSingleTestU16(data, 1);
        appendGsubSingleTestU16(data, 3);

        appendGsubSingleTestU16(data, 20);
        appendGsubSingleTestU16(data, 50);
        appendGsubSingleTestU16(data, 100);

        return data;
    }


    static std::vector<uint8_t> makeGsubSingleFormat2TestData()
    {
        std::vector<uint8_t> data;

        // SingleSubst Format 2
        //
        // Coverage Format 2:
        //
        //   glyph 10 -> coverage index 0
        //   glyph 11 -> coverage index 1
        //   glyph 20 -> coverage index 2
        //
        // substitute array:
        //
        //   0 -> 200
        //   1 -> 300
        //   2 -> 400

        appendGsubSingleTestU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubSingleTestU16(data, 0);

        appendGsubSingleTestU16(data, 3);

        appendGsubSingleTestU16(data, 200);
        appendGsubSingleTestU16(data, 300);
        appendGsubSingleTestU16(data, 400);


        const size_t coverageOffset = data.size();
        patchGsubSingleTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));


        // Coverage Format 2

        appendGsubSingleTestU16(data, 2);
        appendGsubSingleTestU16(data, 2);


        // Range 0:
        //
        // glyphs 10..11
        // coverage indices 0..1

        appendGsubSingleTestU16(data, 10);
        appendGsubSingleTestU16(data, 11);
        appendGsubSingleTestU16(data, 0);


        // Range 1:
        //
        // glyph 20
        // coverage index 2

        appendGsubSingleTestU16(data, 20);
        appendGsubSingleTestU16(data, 20);
        appendGsubSingleTestU16(data, 2);

        return data;
    }


    static bool testOpenTypeGsubSingleView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB SingleSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Format 1 structure and signed delta.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubSingleFormat1TestData();
            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            if (!single)
                return fail("case 1 Format 1 invalid");

            if (single.format() != 1)
                return fail("case 1 wrong format");

            int32_t delta = 0;

            if (!single.deltaGlyphId(delta) || delta != -3)
                return fail("case 1 wrong delta");

            if (single.glyphCount() != 0)
                return fail("case 1 unexpected Format 2 glyph count");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Format 1 substitution using Coverage Format 1.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubSingleFormat1TestData();
            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            uint16_t replacement = 0;

            if (!single.substitute(20, replacement) || replacement != 17)
                return fail("case 2 glyph 20");

            if (!single.substitute(50, replacement) || replacement != 47)
                return fail("case 2 glyph 50");

            if (!single.substitute(100, replacement) || replacement != 97)
                return fail("case 2 glyph 100");

            if (single.substitute(21, replacement))
                return fail("case 2 uncovered glyph substituted");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Format 1 arithmetic wraps modulo 65536.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendGsubSingleTestU16(data, 1);
            appendGsubSingleTestU16(data, 6);
            appendGsubSingleTestU16(data, 2);

            appendGsubSingleTestU16(data, 1);
            appendGsubSingleTestU16(data, 1);
            appendGsubSingleTestU16(data, 0xFFFF);

            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            uint16_t replacement = 0;

            if (!single.substitute(0xFFFF, replacement))
                return fail("case 3 wrapped substitution failed");

            if (replacement != 1)
                return fail("case 3 modulo arithmetic");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Format 2 structure and substitute array.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubSingleFormat2TestData();
            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            if (!single || single.format() != 2)
                return fail("case 4 Format 2 invalid");

            if (single.glyphCount() != 3)
                return fail("case 4 glyph count");

            uint16_t glyph = 0;

            if (!single.substituteGlyphId(0, glyph) || glyph != 200)
                return fail("case 4 substitute index 0");

            if (!single.substituteGlyphId(1, glyph) || glyph != 300)
                return fail("case 4 substitute index 1");

            if (!single.substituteGlyphId(2, glyph) || glyph != 400)
                return fail("case 4 substitute index 2");

            if (single.substituteGlyphId(3, glyph))
                return fail("case 4 out-of-range substitute index");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Format 2 substitution using Coverage Format 2.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubSingleFormat2TestData();
            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            uint16_t replacement = 0;

            if (!single.substitute(10, replacement) || replacement != 200)
                return fail("case 5 glyph 10");

            if (!single.substitute(11, replacement) || replacement != 300)
                return fail("case 5 glyph 11");

            if (!single.substitute(20, replacement) || replacement != 400)
                return fail("case 5 glyph 20");

            if (single.substitute(12, replacement))
                return fail("case 5 uncovered glyph 12");

            if (single.substitute(19, replacement))
                return fail("case 5 uncovered glyph 19");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Coverage Index is preserved correctly.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubSingleFormat2TestData();
            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage = single.coverage();

            if (!coverage || coverage.format() != 2)
                return fail("case 6 coverage");

            uint16_t coverageIndex = 0;

            if (!coverage.find(10, coverageIndex) || coverageIndex != 0)
                return fail("case 6 coverage index 0");

            if (!coverage.find(11, coverageIndex) || coverageIndex != 1)
                return fail("case 6 coverage index 1");

            if (!coverage.find(20, coverageIndex) || coverageIndex != 2)
                return fail("case 6 coverage index 2");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Lazy Coverage access.
        //
        // Corrupt the Coverage format. The SingleSubst local structure
        // remains valid until Coverage is actually dereferenced.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubSingleFormat1TestData();

            // Coverage begins at offset 6.
            patchGsubSingleTestU16(data, 6, 99);

            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            if (!single)
                return fail("case 7 SingleSubst rejected lazy Coverage corruption");

            if (single.coverage())
                return fail("case 7 malformed Coverage accepted");

            uint16_t replacement = 0;

            if (single.substitute(20, replacement))
                return fail("case 7 substitution succeeded with malformed Coverage");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Format 2 Coverage Index outside substitute array.
        //
        // The SingleSubst and Coverage structures are individually valid,
        // but the cross-structure relationship fails only for the affected
        // glyph.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubSingleFormat2TestData();

            // Second Coverage range currently starts at coverage index 2.
            //
            // Coverage starts at offset 12.
            // Range 1 startCoverageIndex is:
            //
            //   12 + 4 + 6 + 4 = 26

            patchGsubSingleTestU16(data, 26, 9);

            const OpenTypeGsubSingleSubstView single(ByteSpan(data.data(), data.size()));

            if (!single)
                return fail("case 8 SingleSubst invalid");

            uint16_t replacement = 0;

            if (!single.substitute(10, replacement) || replacement != 200)
                return fail("case 8 unrelated glyph damaged");

            if (single.substitute(20, replacement))
                return fail("case 8 invalid Coverage Index accepted");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Structural failure paths.
        // ====================================================================

        {
            ++cases;

            const uint8_t badFormatBytes[] = {
                0x00, 0x03,
                0x00, 0x06,
                0x00, 0x00,
                0x00, 0x01
            };

            const OpenTypeGsubSingleSubstView badFormat(
                ByteSpan(badFormatBytes, sizeof(badFormatBytes)));

            if (badFormat)
                return fail("case 9 unsupported format accepted");


            const uint8_t truncatedFormat1Bytes[] = {
                0x00, 0x01,
                0x00, 0x06
            };

            const OpenTypeGsubSingleSubstView truncatedFormat1(
                ByteSpan(truncatedFormat1Bytes, sizeof(truncatedFormat1Bytes)));

            if (truncatedFormat1)
                return fail("case 9 truncated Format 1 accepted");


            const uint8_t truncatedFormat2Bytes[] = {
                0x00, 0x02,
                0x00, 0x08,
                0x00, 0x02,
                0x00, 0x64
            };

            const OpenTypeGsubSingleSubstView truncatedFormat2(
                ByteSpan(truncatedFormat2Bytes, sizeof(truncatedFormat2Bytes)));

            if (truncatedFormat2)
                return fail("case 9 truncated Format 2 accepted");


            const std::vector<uint8_t> goodData = makeGsubSingleFormat1TestData();
            const OpenTypeGsubSingleSubstView good(ByteSpan(goodData.data(), goodData.size()));

            uint16_t replacement = 0;

            if (good.substitute(0x10000u, replacement))
                return fail("case 9 non-OpenType glyph ID accepted");

            ++passed;
        }


        std::printf(
            "OpenType GSUB SingleSubst view: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  Format 1:              PASS\n"
            "  Signed delta:          PASS\n"
            "  Delta wrapping:        PASS\n"
            "  Format 2:              PASS\n"
            "  Coverage Format 1:     PASS\n"
            "  Coverage Format 2:     PASS\n"
            "  Coverage indices:      PASS\n"
            "  Lazy Coverage access:  PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs