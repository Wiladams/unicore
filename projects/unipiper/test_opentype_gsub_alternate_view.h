// test_opentype_gsub_alternate_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_alternate_view.h"

namespace waavs
{
    static void appendGsubAlternateTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubAlternateTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static std::vector<uint8_t> makeGsubAlternateCoverage1TestData()
    {
        std::vector<uint8_t> data;

        // ================================================================
        // AlternateSubst Format 1
        //
        // glyph 10 -> { 100, 101, 102 }
        // glyph 20 -> { 200 }
        // glyph 30 -> { 300, 301 }
        // ================================================================

        appendGsubAlternateTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubAlternateTestU16(data, 0);

        appendGsubAlternateTestU16(data, 3);

        const size_t set0Patch = data.size();
        appendGsubAlternateTestU16(data, 0);

        const size_t set1Patch = data.size();
        appendGsubAlternateTestU16(data, 0);

        const size_t set2Patch = data.size();
        appendGsubAlternateTestU16(data, 0);


        // Set 0

        const size_t set0Offset = data.size();
        patchGsubAlternateTestU16(data, set0Patch, static_cast<uint16_t>(set0Offset));

        appendGsubAlternateTestU16(data, 3);
        appendGsubAlternateTestU16(data, 100);
        appendGsubAlternateTestU16(data, 101);
        appendGsubAlternateTestU16(data, 102);


        // Set 1

        const size_t set1Offset = data.size();
        patchGsubAlternateTestU16(data, set1Patch, static_cast<uint16_t>(set1Offset));

        appendGsubAlternateTestU16(data, 1);
        appendGsubAlternateTestU16(data, 200);


        // Set 2

        const size_t set2Offset = data.size();
        patchGsubAlternateTestU16(data, set2Patch, static_cast<uint16_t>(set2Offset));

        appendGsubAlternateTestU16(data, 2);
        appendGsubAlternateTestU16(data, 300);
        appendGsubAlternateTestU16(data, 301);


        // Coverage Format 1

        const size_t coverageOffset = data.size();
        patchGsubAlternateTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubAlternateTestU16(data, 1);
        appendGsubAlternateTestU16(data, 3);
        appendGsubAlternateTestU16(data, 10);
        appendGsubAlternateTestU16(data, 20);
        appendGsubAlternateTestU16(data, 30);

        return data;
    }

    static std::vector<uint8_t> makeGsubAlternateCoverage2TestData()
    {
        std::vector<uint8_t> data;

        // ================================================================
        // glyph 40 -> { 400 }
        // glyph 41 -> { 500, 501 }
        // glyph 50 -> { 600 }
        //
        // Coverage Format 2:
        //
        //   40 -> index 0
        //   41 -> index 1
        //   50 -> index 2
        // ================================================================

        appendGsubAlternateTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubAlternateTestU16(data, 0);

        appendGsubAlternateTestU16(data, 3);

        const size_t set0Patch = data.size();
        appendGsubAlternateTestU16(data, 0);

        const size_t set1Patch = data.size();
        appendGsubAlternateTestU16(data, 0);

        const size_t set2Patch = data.size();
        appendGsubAlternateTestU16(data, 0);


        const size_t set0Offset = data.size();
        patchGsubAlternateTestU16(data, set0Patch, static_cast<uint16_t>(set0Offset));

        appendGsubAlternateTestU16(data, 1);
        appendGsubAlternateTestU16(data, 400);


        const size_t set1Offset = data.size();
        patchGsubAlternateTestU16(data, set1Patch, static_cast<uint16_t>(set1Offset));

        appendGsubAlternateTestU16(data, 2);
        appendGsubAlternateTestU16(data, 500);
        appendGsubAlternateTestU16(data, 501);


        const size_t set2Offset = data.size();
        patchGsubAlternateTestU16(data, set2Patch, static_cast<uint16_t>(set2Offset));

        appendGsubAlternateTestU16(data, 1);
        appendGsubAlternateTestU16(data, 600);


        // Coverage Format 2

        const size_t coverageOffset = data.size();
        patchGsubAlternateTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubAlternateTestU16(data, 2);
        appendGsubAlternateTestU16(data, 2);

        // 40..41 -> coverage indices 0..1

        appendGsubAlternateTestU16(data, 40);
        appendGsubAlternateTestU16(data, 41);
        appendGsubAlternateTestU16(data, 0);

        // 50 -> coverage index 2

        appendGsubAlternateTestU16(data, 50);
        appendGsubAlternateTestU16(data, 50);
        appendGsubAlternateTestU16(data, 2);

        return data;
    }

    static bool testOpenTypeGsubAlternateView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB AlternateSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // AlternateSubst structure.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubAlternateCoverage1TestData();
            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 1 AlternateSubst invalid");

            if (subst.format() != 1 || subst.alternateSetCount() != 3)
                return fail("case 1 header");

            uint16_t offset0 = 0;
            uint16_t offset1 = 0;
            uint16_t offset2 = 0;

            if (!subst.alternateSetOffset(0, offset0) ||
                !subst.alternateSetOffset(1, offset1) ||
                !subst.alternateSetOffset(2, offset2))
            {
                return fail("case 1 AlternateSet offsets");
            }

            if (offset0 == 0 || offset1 <= offset0 || offset2 <= offset1)
                return fail("case 1 unexpected AlternateSet ordering");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // AlternateSet access and stored alternate order.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubAlternateCoverage1TestData();
            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            const OpenTypeGsubAlternateSetView set = subst.alternateSet(0);

            if (!set || set.glyphCount() != 3)
                return fail("case 2 AlternateSet");

            const uint16_t expected[] = { 100, 101, 102 };

            for (size_t i = 0; i < 3; ++i)
            {
                uint16_t glyphId = 0;

                if (!set.glyphId(i, glyphId) || glyphId != expected[i])
                    return fail("case 2 alternate order");
            }

            uint16_t glyphId = 0;

            if (set.glyphId(3, glyphId))
                return fail("case 2 out-of-range alternate accepted");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Coverage Format 1 -> AlternateSet mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubAlternateCoverage1TestData();
            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            OpenTypeGsubAlternateSetView set;

            if (!subst.alternateSetForGlyph(10, set) || set.glyphCount() != 3)
                return fail("case 3 glyph 10");

            uint16_t glyphId = 0;

            if (!set.glyphId(0, glyphId) || glyphId != 100)
                return fail("case 3 glyph 10 first alternate");

            if (!subst.alternateSetForGlyph(20, set) || set.glyphCount() != 1)
                return fail("case 3 glyph 20");

            if (!set.glyphId(0, glyphId) || glyphId != 200)
                return fail("case 3 glyph 20 alternate");

            if (subst.alternateSetForGlyph(21, set))
                return fail("case 3 uncovered glyph accepted");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Coverage Format 2 -> AlternateSet mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubAlternateCoverage2TestData();
            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage = subst.coverage();

            if (!coverage || coverage.format() != 2)
                return fail("case 4 Coverage Format 2");

            OpenTypeGsubAlternateSetView set;
            uint16_t glyphId = 0;

            if (!subst.alternateSetForGlyph(41, set) || set.glyphCount() != 2)
                return fail("case 4 glyph 41");

            if (!set.glyphId(0, glyphId) || glyphId != 500)
                return fail("case 4 glyph 41 alternate 0");

            if (!set.glyphId(1, glyphId) || glyphId != 501)
                return fail("case 4 glyph 41 alternate 1");

            if (!subst.alternateSetForGlyph(50, set))
                return fail("case 4 glyph 50");

            if (!set.glyphId(0, glyphId) || glyphId != 600)
                return fail("case 4 glyph 50 alternate");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Lazy AlternateSet access.
        //
        // Corrupt Set 1. Parent and unrelated Sets remain usable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubAlternateCoverage1TestData();

            // AlternateSet offset array starts at byte 6.
            // Set 1's offset is at byte 8.

            patchGsubAlternateTestU16(data, 8, 0xFFFF);

            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 5 AlternateSubst rejected lazy Set corruption");

            uint16_t offset = 0;

            if (!subst.alternateSetOffset(1, offset) || offset != 0xFFFF)
                return fail("case 5 corrupted Set offset unavailable");

            if (subst.alternateSet(1))
                return fail("case 5 malformed Set accepted");

            if (!subst.alternateSet(0) || !subst.alternateSet(2))
                return fail("case 5 unrelated Set damaged");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Lazy Coverage access.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubAlternateCoverage1TestData();

            const OpenTypeGsubAlternateSubstView original(ByteSpan(data.data(), data.size()));
            const uint16_t coverageOffset = original.coverageOffset();

            patchGsubAlternateTestU16(data, coverageOffset, 99);

            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 6 AlternateSubst rejected lazy Coverage corruption");

            if (subst.coverage())
                return fail("case 6 malformed Coverage accepted");

            if (!subst.alternateSet(0))
                return fail("case 6 AlternateSet unnecessarily affected");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Coverage Index outside AlternateSet array.
        //
        // Only the affected glyph fails.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubAlternateCoverage2TestData();

            const OpenTypeGsubAlternateSubstView original(ByteSpan(data.data(), data.size()));
            const uint16_t coverageOffset = original.coverageOffset();

            // Coverage Format 2:
            //
            // header                   4
            // first RangeRecord        6
            // second RangeRecord:
            //   startGlyphId           +0
            //   endGlyphId             +2
            //   startCoverageIndex     +4
            //
            // Make glyph 50 produce Coverage Index 9.

            patchGsubAlternateTestU16(data, size_t(coverageOffset) + 14, 9);

            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));

            OpenTypeGsubAlternateSetView set;

            if (!subst.alternateSetForGlyph(40, set))
                return fail("case 7 unrelated glyph damaged");

            if (subst.alternateSetForGlyph(50, set))
                return fail("case 7 invalid Coverage Index accepted");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Empty AlternateSet remains structurally readable.
        //
        // The view layer exposes what the font contains; selection policy
        // will decide whether an empty set can produce a substitution.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubAlternateCoverage1TestData();

            const OpenTypeGsubAlternateSubstView original(ByteSpan(data.data(), data.size()));

            uint16_t set0Offset = 0;

            if (!original.alternateSetOffset(0, set0Offset))
                return fail("case 8 setup");

            patchGsubAlternateTestU16(data, set0Offset, 0);

            const OpenTypeGsubAlternateSubstView subst(ByteSpan(data.data(), data.size()));
            const OpenTypeGsubAlternateSetView set = subst.alternateSet(0);

            if (!set)
                return fail("case 8 empty AlternateSet rejected");

            if (set.glyphCount() != 0)
                return fail("case 8 empty AlternateSet count");

            uint16_t glyphId = 0;

            if (set.glyphId(0, glyphId))
                return fail("case 8 empty AlternateSet exposed glyph");

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
                0x00, 0x02,
                0x00, 0x08,
                0x00, 0x01,
                0x00, 0x06
            };

            const OpenTypeGsubAlternateSubstView badFormat(
                ByteSpan(badFormatBytes, sizeof(badFormatBytes)));

            if (badFormat)
                return fail("case 9 unsupported format accepted");


            const uint8_t truncatedSubstBytes[] = {
                0x00, 0x01,
                0x00, 0x08,
                0x00, 0x02,
                0x00, 0x0A
            };

            const OpenTypeGsubAlternateSubstView truncatedSubst(
                ByteSpan(truncatedSubstBytes, sizeof(truncatedSubstBytes)));

            if (truncatedSubst)
                return fail("case 9 truncated AlternateSet array accepted");


            const uint8_t truncatedSetBytes[] = {
                0x00, 0x02,
                0x00, 0x64
            };

            const OpenTypeGsubAlternateSetView truncatedSet(
                ByteSpan(truncatedSetBytes, sizeof(truncatedSetBytes)));

            if (truncatedSet)
                return fail("case 9 truncated AlternateSet accepted");

            ++passed;
        }


        std::printf(
            "OpenType GSUB AlternateSubst view: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  AlternateSubst:        PASS\n"
            "  AlternateSet access:   PASS\n"
            "  Alternate order:       PASS\n"
            "  Coverage Format 1:     PASS\n"
            "  Coverage Format 2:     PASS\n"
            "  Coverage indices:      PASS\n"
            "  Lazy Set access:       PASS\n"
            "  Lazy Coverage access:  PASS\n"
            "  Empty AlternateSet:    PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs