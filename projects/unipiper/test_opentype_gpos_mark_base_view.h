// test_opentype_gpos_mark_base_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_mark_base_view.h"

namespace waavs
{
    static void appendGposMarkBaseU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposMarkBaseS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposMarkBaseU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposMarkBaseU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposMarkBaseAnchor1(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposMarkBaseU16(data, 1);
        appendGposMarkBaseS16(data, x);
        appendGposMarkBaseS16(data, y);
    }

    static std::vector<uint8_t> makeGposMarkBaseCoverage(
        const uint16_t* glyphs, uint16_t count)
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseU16(data, 1);
        appendGposMarkBaseU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposMarkBaseU16(data, glyphs[i]);

        return data;
    }


    // ====================================================================
    // MarkArray:
    //
    // mark 0 -> class 0, anchor (10, 20)
    // mark 1 -> class 1, anchor (30, 40)
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkBaseMarkArray()
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseU16(data, 2);

        appendGposMarkBaseU16(data, 0);
        const size_t anchor0Patch = data.size();
        appendGposMarkBaseU16(data, 0);

        appendGposMarkBaseU16(data, 1);
        const size_t anchor1Patch = data.size();
        appendGposMarkBaseU16(data, 0);

        patchGposMarkBaseU16(data, anchor0Patch, static_cast<uint16_t>(data.size()));
        appendGposMarkBaseAnchor1(data, 10, 20);

        patchGposMarkBaseU16(data, anchor1Patch, static_cast<uint16_t>(data.size()));
        appendGposMarkBaseAnchor1(data, 30, 40);

        return data;
    }


    // ====================================================================
    // BaseArray, two classes.
    //
    // base 0:
    //   class 0 -> (300, 400)
    //   class 1 -> (500, 600)
    //
    // base 1:
    //   class 0 -> (700, 800)
    //   class 1 -> NULL
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkBaseBaseArray()
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseU16(data, 2);

        const size_t base0Class0Patch = data.size();
        appendGposMarkBaseU16(data, 0);

        const size_t base0Class1Patch = data.size();
        appendGposMarkBaseU16(data, 0);

        const size_t base1Class0Patch = data.size();
        appendGposMarkBaseU16(data, 0);

        appendGposMarkBaseU16(data, 0);


        patchGposMarkBaseU16(
            data, base0Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkBaseAnchor1(data, 300, 400);


        patchGposMarkBaseU16(
            data, base0Class1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkBaseAnchor1(data, 500, 600);


        patchGposMarkBaseU16(
            data, base1Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkBaseAnchor1(data, 700, 800);

        return data;
    }


    static std::vector<uint8_t> makeGposMarkBaseFormat1()
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposMarkBaseU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposMarkBaseU16(data, 0);

        appendGposMarkBaseU16(data, 2);

        const size_t markArrayPatch = data.size();
        appendGposMarkBaseU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposMarkBaseU16(data, 0);


        // Mark Coverage.

        patchGposMarkBaseU16(
            data, markCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 101 };
            const std::vector<uint8_t> coverage =
                makeGposMarkBaseCoverage(glyphs, 2);

            data.insert(data.end(), coverage.begin(), coverage.end());
        }


        // Base Coverage.

        patchGposMarkBaseU16(
            data, baseCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 20, 30 };
            const std::vector<uint8_t> coverage =
                makeGposMarkBaseCoverage(glyphs, 2);

            data.insert(data.end(), coverage.begin(), coverage.end());
        }


        // MarkArray.

        patchGposMarkBaseU16(
            data, markArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> array =
                makeGposMarkBaseMarkArray();

            data.insert(data.end(), array.begin(), array.end());
        }


        // BaseArray.

        patchGposMarkBaseU16(
            data, baseArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> array =
                makeGposMarkBaseBaseArray();

            data.insert(data.end(), array.begin(), array.end());
        }

        return data;
    }


    static bool testOpenTypeGposMarkBaseView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Mark-to-Base view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - MarkArray.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkBaseMarkArray();

            const OpenTypeGposMarkArrayView marks(
                ByteSpan(data.data(), data.size()));

            if (!marks || marks.markCount() != 2)
                return fail("case 1 MarkArray geometry");

            uint16_t markClass = 999;

            if (!marks.markClass(0, markClass) || markClass != 0)
                return fail("case 1 class 0");

            if (!marks.markClass(1, markClass) || markClass != 1)
                return fail("case 1 class 1");

            const OpenTypeGposAnchorView anchor0 =
                marks.markAnchor(0);

            const OpenTypeGposAnchorView anchor1 =
                marks.markAnchor(1);

            if (!anchor0 ||
                anchor0.xCoordinate() != 10 ||
                anchor0.yCoordinate() != 20)
            {
                return fail("case 1 anchor 0");
            }

            if (!anchor1 ||
                anchor1.xCoordinate() != 30 ||
                anchor1.yCoordinate() != 40)
            {
                return fail("case 1 anchor 1");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - BaseArray.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkBaseBaseArray();

            const OpenTypeGposBaseArrayView bases(
                ByteSpan(data.data(), data.size()), 2);

            if (!bases ||
                bases.baseCount() != 2 ||
                bases.markClassCount() != 2)
            {
                return fail("case 2 BaseArray geometry");
            }

            const OpenTypeGposAnchorView anchor00 =
                bases.baseAnchor(0, 0);

            const OpenTypeGposAnchorView anchor01 =
                bases.baseAnchor(0, 1);

            const OpenTypeGposAnchorView anchor10 =
                bases.baseAnchor(1, 0);

            if (!anchor00 ||
                anchor00.xCoordinate() != 300 ||
                anchor00.yCoordinate() != 400)
            {
                return fail("case 2 base 0 class 0");
            }

            if (!anchor01 ||
                anchor01.xCoordinate() != 500 ||
                anchor01.yCoordinate() != 600)
            {
                return fail("case 2 base 0 class 1");
            }

            if (!anchor10 ||
                anchor10.xCoordinate() != 700 ||
                anchor10.yCoordinate() != 800)
            {
                return fail("case 2 base 1 class 0");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - NULL BaseAnchor.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkBaseBaseArray();

            const OpenTypeGposBaseArrayView bases(
                ByteSpan(data.data(), data.size()), 2);

            if (bases.hasBaseAnchor(1, 1))
                return fail("case 3 unexpected anchor");

            if (bases.baseAnchor(1, 1))
                return fail("case 3 NULL anchor");

            ++passed;
        }


        // ====================================================================
        // Case 4 - MarkBasePos geometry.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkBaseFormat1();

            const OpenTypeGposMarkBasePosView markBase(
                ByteSpan(data.data(), data.size()));

            if (!markBase ||
                markBase.format() != 1 ||
                markBase.markClassCount() != 2)
            {
                return fail("case 4 MarkBasePos geometry");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Coverage mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkBaseFormat1();

            const OpenTypeGposMarkBasePosView markBase(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView markCoverage =
                markBase.markCoverage();

            const OpenTypeCoverageView baseCoverage =
                markBase.baseCoverage();

            if (!markCoverage || !baseCoverage)
                return fail("case 5 Coverage views");

            uint16_t index = 999;

            if (!markCoverage.find(100, index) || index != 0)
                return fail("case 5 mark 100");

            if (!markCoverage.find(101, index) || index != 1)
                return fail("case 5 mark 101");

            if (!baseCoverage.find(20, index) || index != 0)
                return fail("case 5 base 20");

            if (!baseCoverage.find(30, index) || index != 1)
                return fail("case 5 base 30");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Complete class-selected anchor path.
        //
        // mark glyph 101:
        //   Coverage index 1
        //   mark class 1
        //
        // base glyph 20:
        //   Coverage index 0
        //   class 1 anchor = (500, 600)
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkBaseFormat1();

            const OpenTypeGposMarkBasePosView markBase(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGposMarkArrayView marks =
                markBase.markArray();

            const OpenTypeGposBaseArrayView bases =
                markBase.baseArray();

            if (!marks || !bases)
                return fail("case 6 arrays");

            uint16_t markClass = 0;

            if (!marks.markClass(1, markClass) ||
                markClass != 1)
            {
                return fail("case 6 mark class");
            }

            const OpenTypeGposAnchorView markAnchor =
                marks.markAnchor(1);

            const OpenTypeGposAnchorView baseAnchor =
                bases.baseAnchor(0, markClass);

            if (!markAnchor ||
                markAnchor.xCoordinate() != 30 ||
                markAnchor.yCoordinate() != 40)
            {
                return fail("case 6 mark anchor");
            }

            if (!baseAnchor ||
                baseAnchor.xCoordinate() != 500 ||
                baseAnchor.yCoordinate() != 600)
            {
                return fail("case 6 base anchor");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Bounds.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> markData =
                makeGposMarkBaseMarkArray();

            const OpenTypeGposMarkArrayView marks(
                ByteSpan(markData.data(), markData.size()));

            uint16_t value = 999;

            if (marks.markClass(2, value) ||
                marks.markAnchorOffset(2, value))
            {
                return fail("case 7 MarkArray bounds");
            }

            const std::vector<uint8_t> baseData =
                makeGposMarkBaseBaseArray();

            const OpenTypeGposBaseArrayView bases(
                ByteSpan(baseData.data(), baseData.size()), 2);

            if (bases.baseAnchorOffset(2, 0, value) ||
                bases.baseAnchorOffset(0, 2, value))
            {
                return fail("case 7 BaseArray bounds");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Structural failures.
        // ====================================================================

        {
            ++cases;


            // Unsupported MarkBasePos format.

            {
                std::vector<uint8_t> data =
                    makeGposMarkBaseFormat1();

                patchGposMarkBaseU16(data, 0, 2);

                const OpenTypeGposMarkBasePosView markBase(
                    ByteSpan(data.data(), data.size()));

                if (markBase)
                    return fail("case 8 unsupported format");
            }


            // Zero markClassCount.

            {
                std::vector<uint8_t> data =
                    makeGposMarkBaseFormat1();

                patchGposMarkBaseU16(data, 6, 0);

                const OpenTypeGposMarkBasePosView markBase(
                    ByteSpan(data.data(), data.size()));

                if (markBase)
                    return fail("case 8 zero class count");
            }


            // Child offset points into parent header.

            {
                std::vector<uint8_t> data =
                    makeGposMarkBaseFormat1();

                patchGposMarkBaseU16(data, 8, 6);

                const OpenTypeGposMarkBasePosView markBase(
                    ByteSpan(data.data(), data.size()));

                if (markBase)
                    return fail("case 8 child inside header");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Lazy child validation.
        //
        // Parent remains structurally valid if a child table itself is bad.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                makeGposMarkBaseFormat1();

            const OpenTypeGposMarkBasePosView initial(
                ByteSpan(data.data(), data.size()));

            if (!initial)
                return fail("case 9 initial parent");

            const uint16_t markArrayOffset =
                initial.markArrayOffset();

            // MarkRecord[0].markAnchorOffset is at MarkArray + 4.
            // Point it back into the MarkArray record area.

            patchGposMarkBaseU16(
                data, size_t(markArrayOffset) + 4, 2);

            const OpenTypeGposMarkBasePosView markBase(
                ByteSpan(data.data(), data.size()));

            if (!markBase)
                return fail("case 9 parent rejected child");

            if (markBase.markArray())
                return fail("case 9 malformed MarkArray accepted");

            ++passed;
        }


        std::printf(
            "OpenType GPOS Mark-to-Base view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  MarkArray:                 PASS\n"
            "  BaseArray:                 PASS\n"
            "  NULL BaseAnchor:           PASS\n"
            "  MarkBasePos geometry:      PASS\n"
            "  Coverage mapping:          PASS\n"
            "  Class-selected anchors:    PASS\n"
            "  Bounds:                    PASS\n"
            "  Structural failures:       PASS\n"
            "  Lazy child validation:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs