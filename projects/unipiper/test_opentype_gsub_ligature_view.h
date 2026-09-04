// test_opentype_gsub_ligature_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_ligature_view.h"

namespace waavs
{
    static void appendGsubLigatureTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubLigatureTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static std::vector<uint8_t> makeGsubLigatureCoverage1TestData()
    {
        std::vector<uint8_t> data;

        // ================================================================
        // LigatureSubst Format 1
        //
        // first glyph 10:
        //
        //   10, 11, 12 -> 500
        //   10, 11     -> 501
        //
        // first glyph 20:
        //
        //   20, 21     -> 600
        //
        // The first LigatureSet deliberately contains the longer match
        // first so we can confirm stored order is preserved.
        // ================================================================

        appendGsubLigatureTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubLigatureTestU16(data, 0);

        appendGsubLigatureTestU16(data, 2);

        const size_t set0Patch = data.size();
        appendGsubLigatureTestU16(data, 0);

        const size_t set1Patch = data.size();
        appendGsubLigatureTestU16(data, 0);


        // ================================================================
        // LigatureSet 0
        // ================================================================

        const size_t set0Offset = data.size();
        patchGsubLigatureTestU16(data, set0Patch, static_cast<uint16_t>(set0Offset));

        appendGsubLigatureTestU16(data, 2);

        const size_t ligature00Patch = data.size();
        appendGsubLigatureTestU16(data, 0);

        const size_t ligature01Patch = data.size();
        appendGsubLigatureTestU16(data, 0);


        // 10, 11, 12 -> 500
        //
        // First component 10 is implicit through Coverage.

        const size_t ligature00Offset = data.size() - set0Offset;
        patchGsubLigatureTestU16(data, ligature00Patch, static_cast<uint16_t>(ligature00Offset));

        appendGsubLigatureTestU16(data, 500);
        appendGsubLigatureTestU16(data, 3);
        appendGsubLigatureTestU16(data, 11);
        appendGsubLigatureTestU16(data, 12);


        // 10, 11 -> 501

        const size_t ligature01Offset = data.size() - set0Offset;
        patchGsubLigatureTestU16(data, ligature01Patch, static_cast<uint16_t>(ligature01Offset));

        appendGsubLigatureTestU16(data, 501);
        appendGsubLigatureTestU16(data, 2);
        appendGsubLigatureTestU16(data, 11);


        // ================================================================
        // LigatureSet 1
        // ================================================================

        const size_t set1Offset = data.size();
        patchGsubLigatureTestU16(data, set1Patch, static_cast<uint16_t>(set1Offset));

        appendGsubLigatureTestU16(data, 1);

        const size_t ligature10Patch = data.size();
        appendGsubLigatureTestU16(data, 0);


        // 20, 21 -> 600

        const size_t ligature10Offset = data.size() - set1Offset;
        patchGsubLigatureTestU16(data, ligature10Patch, static_cast<uint16_t>(ligature10Offset));

        appendGsubLigatureTestU16(data, 600);
        appendGsubLigatureTestU16(data, 2);
        appendGsubLigatureTestU16(data, 21);


        // ================================================================
        // Coverage Format 1
        // ================================================================

        const size_t coverageOffset = data.size();
        patchGsubLigatureTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubLigatureTestU16(data, 1);
        appendGsubLigatureTestU16(data, 2);
        appendGsubLigatureTestU16(data, 10);
        appendGsubLigatureTestU16(data, 20);

        return data;
    }

    static std::vector<uint8_t> makeGsubLigatureCoverage2TestData()
    {
        std::vector<uint8_t> data;

        // ================================================================
        // first glyph 30 -> 30, 31 -> 700
        // first glyph 31 -> 31, 32 -> 701
        //
        // Coverage Format 2 maps:
        //
        //   30 -> index 0
        //   31 -> index 1
        // ================================================================

        appendGsubLigatureTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubLigatureTestU16(data, 0);

        appendGsubLigatureTestU16(data, 2);

        const size_t set0Patch = data.size();
        appendGsubLigatureTestU16(data, 0);

        const size_t set1Patch = data.size();
        appendGsubLigatureTestU16(data, 0);


        // Set 0

        const size_t set0Offset = data.size();
        patchGsubLigatureTestU16(data, set0Patch, static_cast<uint16_t>(set0Offset));

        appendGsubLigatureTestU16(data, 1);

        const size_t ligature0Patch = data.size();
        appendGsubLigatureTestU16(data, 0);

        const size_t ligature0Offset = data.size() - set0Offset;
        patchGsubLigatureTestU16(data, ligature0Patch, static_cast<uint16_t>(ligature0Offset));

        appendGsubLigatureTestU16(data, 700);
        appendGsubLigatureTestU16(data, 2);
        appendGsubLigatureTestU16(data, 31);


        // Set 1

        const size_t set1Offset = data.size();
        patchGsubLigatureTestU16(data, set1Patch, static_cast<uint16_t>(set1Offset));

        appendGsubLigatureTestU16(data, 1);

        const size_t ligature1Patch = data.size();
        appendGsubLigatureTestU16(data, 0);

        const size_t ligature1Offset = data.size() - set1Offset;
        patchGsubLigatureTestU16(data, ligature1Patch, static_cast<uint16_t>(ligature1Offset));

        appendGsubLigatureTestU16(data, 701);
        appendGsubLigatureTestU16(data, 2);
        appendGsubLigatureTestU16(data, 32);


        // Coverage Format 2

        const size_t coverageOffset = data.size();
        patchGsubLigatureTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubLigatureTestU16(data, 2);
        appendGsubLigatureTestU16(data, 1);

        appendGsubLigatureTestU16(data, 30);
        appendGsubLigatureTestU16(data, 31);
        appendGsubLigatureTestU16(data, 0);

        return data;
    }

    static bool testOpenTypeGsubLigatureView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB LigatureSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // LigatureSubst structure.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();
            const OpenTypeGsubLigatureSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 1 LigatureSubst invalid");

            if (subst.format() != 1 || subst.ligatureSetCount() != 2)
                return fail("case 1 header");

            uint16_t offset0 = 0;
            uint16_t offset1 = 0;

            if (!subst.ligatureSetOffset(0, offset0) ||
                !subst.ligatureSetOffset(1, offset1))
            {
                return fail("case 1 LigatureSet offsets");
            }

            if (offset0 == 0 || offset1 <= offset0)
                return fail("case 1 unexpected LigatureSet offsets");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // LigatureSet preserves stored preference order.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();
            const OpenTypeGsubLigatureSubstView subst(ByteSpan(data.data(), data.size()));

            const OpenTypeGsubLigatureSetView set = subst.ligatureSet(0);

            if (!set || set.size() != 2)
                return fail("case 2 LigatureSet");

            const OpenTypeGsubLigatureView first = set.ligature(0);
            const OpenTypeGsubLigatureView second = set.ligature(1);

            if (!first || !second)
                return fail("case 2 Ligature access");

            if (first.ligatureGlyph() != 500 || first.componentCount() != 3)
                return fail("case 2 first Ligature");

            if (second.ligatureGlyph() != 501 || second.componentCount() != 2)
                return fail("case 2 second Ligature");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Component access.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();
            const OpenTypeGsubLigatureSubstView subst(ByteSpan(data.data(), data.size()));

            const OpenTypeGsubLigatureView ligature =
                subst.ligatureSet(0).ligature(0);

            if (!ligature)
                return fail("case 3 Ligature");

            if (ligature.componentCount() != 3 ||
                ligature.trailingComponentCount() != 2)
            {
                return fail("case 3 component count");
            }

            uint16_t component0 = 0;
            uint16_t component1 = 0;

            if (!ligature.componentGlyphId(0, component0) ||
                !ligature.componentGlyphId(1, component1))
            {
                return fail("case 3 components");
            }

            if (component0 != 11 || component1 != 12)
                return fail("case 3 wrong components");

            uint16_t dummy = 0;

            if (ligature.componentGlyphId(2, dummy))
                return fail("case 3 out-of-range component accepted");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Coverage Format 1 -> LigatureSet mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();
            const OpenTypeGsubLigatureSubstView subst(ByteSpan(data.data(), data.size()));

            OpenTypeGsubLigatureSetView set;

            if (!subst.ligatureSetForGlyph(10, set) || set.size() != 2)
                return fail("case 4 glyph 10");

            if (!subst.ligatureSetForGlyph(20, set) || set.size() != 1)
                return fail("case 4 glyph 20");

            if (subst.ligatureSetForGlyph(11, set))
                return fail("case 4 uncovered glyph accepted");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Coverage Format 2 -> LigatureSet mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubLigatureCoverage2TestData();
            const OpenTypeGsubLigatureSubstView subst(ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage = subst.coverage();

            if (!coverage || coverage.format() != 2)
                return fail("case 5 Coverage Format 2");

            OpenTypeGsubLigatureSetView set;

            if (!subst.ligatureSetForGlyph(30, set))
                return fail("case 5 glyph 30");

            if (set.ligature(0).ligatureGlyph() != 700)
                return fail("case 5 glyph 30 Ligature");

            if (!subst.ligatureSetForGlyph(31, set))
                return fail("case 5 glyph 31");

            if (set.ligature(0).ligatureGlyph() != 701)
                return fail("case 5 glyph 31 Ligature");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Lazy LigatureSet access.
        //
        // Corrupt Set 1's offset. Set 0 remains usable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();

            // LigatureSet offset array begins at byte 6.
            // Set 1 offset is at byte 8.

            patchGsubLigatureTestU16(data, 8, 0xFFFF);

            const OpenTypeGsubLigatureSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 6 LigatureSubst rejected lazy Set corruption");

            uint16_t offset = 0;

            if (!subst.ligatureSetOffset(1, offset) || offset != 0xFFFF)
                return fail("case 6 corrupted Set offset unavailable");

            if (subst.ligatureSet(1))
                return fail("case 6 malformed Set accepted");

            if (!subst.ligatureSet(0))
                return fail("case 6 unrelated Set damaged");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Lazy Ligature access.
        //
        // Corrupt Ligature 1 inside Set 0. Ligature 0 remains usable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();

            const OpenTypeGsubLigatureSubstView original(
                ByteSpan(data.data(), data.size()));

            uint16_t set0Offset = 0;

            if (!original.ligatureSetOffset(0, set0Offset))
                return fail("case 7 setup");

            // LigatureSet:
            //
            // uint16 count
            // Offset16 ligatureOffsets[2]
            //
            // Ligature 1 offset field is +4.

            patchGsubLigatureTestU16(data, size_t(set0Offset) + 4, 0xFFFF);

            const OpenTypeGsubLigatureSubstView subst(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGsubLigatureSetView set = subst.ligatureSet(0);

            if (!set)
                return fail("case 7 LigatureSet damaged");

            if (!set.ligature(0))
                return fail("case 7 unrelated Ligature damaged");

            if (set.ligature(1))
                return fail("case 7 malformed Ligature accepted");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Lazy Coverage and cross-structure failure.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();

            const OpenTypeGsubLigatureSubstView original(
                ByteSpan(data.data(), data.size()));

            const uint16_t coverageOffset = original.coverageOffset();

            // Corrupt Coverage format.

            patchGsubLigatureTestU16(data, coverageOffset, 99);

            const OpenTypeGsubLigatureSubstView subst(
                ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 8 LigatureSubst rejected lazy Coverage corruption");

            if (subst.coverage())
                return fail("case 8 malformed Coverage accepted");

            if (!subst.ligatureSet(0))
                return fail("case 8 LigatureSet unnecessarily damaged");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Invalid Ligature component count and structural failure paths.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubLigatureCoverage1TestData();

            const OpenTypeGsubLigatureSubstView original(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGsubLigatureSetView set = original.ligatureSet(0);

            uint16_t ligatureOffset = 0;
            uint16_t setOffset = 0;

            if (!original.ligatureSetOffset(0, setOffset) ||
                !set.ligatureOffset(0, ligatureOffset))
            {
                return fail("case 9 setup");
            }

            // Ligature:
            //
            // uint16 ligatureGlyph
            // uint16 componentCount
            //
            // Set componentCount to 1, which is invalid for LookupType 4.

            patchGsubLigatureTestU16(
                data,
                size_t(setOffset) + size_t(ligatureOffset) + 2,
                1);

            const OpenTypeGsubLigatureSubstView modified(
                ByteSpan(data.data(), data.size()));

            if (!modified)
                return fail("case 9 parent unexpectedly invalid");

            if (modified.ligatureSet(0).ligature(0))
                return fail("case 9 componentCount 1 accepted");


            const uint8_t badFormatBytes[] = {
                0x00, 0x02,
                0x00, 0x08,
                0x00, 0x01,
                0x00, 0x06
            };

            const OpenTypeGsubLigatureSubstView badFormat(
                ByteSpan(badFormatBytes, sizeof(badFormatBytes)));

            if (badFormat)
                return fail("case 9 unsupported format accepted");


            const uint8_t truncatedBytes[] = {
                0x00, 0x01,
                0x00, 0x08,
                0x00, 0x02,
                0x00, 0x0A
            };

            const OpenTypeGsubLigatureSubstView truncated(
                ByteSpan(truncatedBytes, sizeof(truncatedBytes)));

            if (truncated)
                return fail("case 9 truncated LigatureSet array accepted");

            ++passed;
        }


        std::printf(
            "OpenType GSUB LigatureSubst view: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  LigatureSubst:         PASS\n"
            "  LigatureSet access:    PASS\n"
            "  Ligature order:        PASS\n"
            "  Component access:      PASS\n"
            "  Coverage Format 1:     PASS\n"
            "  Coverage Format 2:     PASS\n"
            "  Lazy Set access:       PASS\n"
            "  Lazy Ligature access:  PASS\n"
            "  Lazy Coverage access:  PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs