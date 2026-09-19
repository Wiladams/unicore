// test_opentype_gpos_mark_mark_view.h
#pragma once

#include "../unitils/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_mark_mark_view.h"

namespace waavs
{
    static void appendGposMarkMarkViewU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposMarkMarkViewS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposMarkMarkViewU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposMarkMarkViewU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposMarkMarkViewAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposMarkMarkViewU16(data, 1);
        appendGposMarkMarkViewS16(data, x);
        appendGposMarkMarkViewS16(data, y);
    }

    static void appendGposMarkMarkViewCoverage(
        std::vector<uint8_t>& data, const uint16_t* glyphs, uint16_t count)
    {
        appendGposMarkMarkViewU16(data, 1);
        appendGposMarkMarkViewU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposMarkMarkViewU16(data, glyphs[i]);
    }


    // ====================================================================
    // Mark1Array:
    //
    // mark 100 -> class 0 -> (10, 20)
    // mark 101 -> class 1 -> (30, 40)
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkMarkViewMark1Array()
    {
        std::vector<uint8_t> data;

        appendGposMarkMarkViewU16(data, 2);

        appendGposMarkMarkViewU16(data, 0);
        const size_t anchor0Patch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        appendGposMarkMarkViewU16(data, 1);
        const size_t anchor1Patch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        patchGposMarkMarkViewU16(
            data, anchor0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkMarkViewAnchor(data, 10, 20);

        patchGposMarkMarkViewU16(
            data, anchor1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkMarkViewAnchor(data, 30, 40);

        return data;
    }


    // ====================================================================
    // Mark2Array, two classes.
    //
    // mark2 200:
    //   class 0 -> (300, 400)
    //   class 1 -> (500, 600)
    //
    // mark2 201:
    //   class 0 -> (700, 800)
    //   class 1 -> NULL
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkMarkViewMark2Array()
    {
        std::vector<uint8_t> data;

        appendGposMarkMarkViewU16(data, 2);

        const size_t mark200Class0Patch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        const size_t mark200Class1Patch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        const size_t mark201Class0Patch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        appendGposMarkMarkViewU16(data, 0);


        patchGposMarkMarkViewU16(
            data, mark200Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkMarkViewAnchor(data, 300, 400);


        patchGposMarkMarkViewU16(
            data, mark200Class1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkMarkViewAnchor(data, 500, 600);


        patchGposMarkMarkViewU16(
            data, mark201Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkMarkViewAnchor(data, 700, 800);

        return data;
    }


    static std::vector<uint8_t> makeGposMarkMarkViewFormat1()
    {
        std::vector<uint8_t> data;

        appendGposMarkMarkViewU16(data, 1);

        const size_t mark1CoveragePatch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        const size_t mark2CoveragePatch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        appendGposMarkMarkViewU16(data, 2);

        const size_t mark1ArrayPatch = data.size();
        appendGposMarkMarkViewU16(data, 0);

        const size_t mark2ArrayPatch = data.size();
        appendGposMarkMarkViewU16(data, 0);


        // Mark1Coverage.

        patchGposMarkMarkViewU16(
            data, mark1CoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 101 };
            appendGposMarkMarkViewCoverage(data, glyphs, 2);
        }


        // Mark2Coverage.

        patchGposMarkMarkViewU16(
            data, mark2CoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 200, 201 };
            appendGposMarkMarkViewCoverage(data, glyphs, 2);
        }


        // Mark1Array.

        patchGposMarkMarkViewU16(
            data, mark1ArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> array =
                makeGposMarkMarkViewMark1Array();

            data.insert(data.end(), array.begin(), array.end());
        }


        // Mark2Array.

        patchGposMarkMarkViewU16(
            data, mark2ArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> array =
                makeGposMarkMarkViewMark2Array();

            data.insert(data.end(), array.begin(), array.end());
        }

        return data;
    }


    static bool testOpenTypeGposMarkMarkView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Mark-to-Mark view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Mark2Array geometry.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewMark2Array();

            const OpenTypeGposMark2ArrayView mark2(
                ByteSpan(data.data(), data.size()), 2);

            if (!mark2 ||
                mark2.mark2Count() != 2 ||
                mark2.markClassCount() != 2)
            {
                return fail("case 1 Mark2Array geometry");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Mark2 class-selected anchors.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewMark2Array();

            const OpenTypeGposMark2ArrayView mark2(
                ByteSpan(data.data(), data.size()), 2);

            const OpenTypeGposAnchorView anchor00 =
                mark2.anchor(0, 0);

            const OpenTypeGposAnchorView anchor01 =
                mark2.anchor(0, 1);

            const OpenTypeGposAnchorView anchor10 =
                mark2.anchor(1, 0);

            if (!anchor00 ||
                anchor00.xCoordinate() != 300 ||
                anchor00.yCoordinate() != 400)
            {
                return fail("case 2 mark2 0 class 0");
            }

            if (!anchor01 ||
                anchor01.xCoordinate() != 500 ||
                anchor01.yCoordinate() != 600)
            {
                return fail("case 2 mark2 0 class 1");
            }

            if (!anchor10 ||
                anchor10.xCoordinate() != 700 ||
                anchor10.yCoordinate() != 800)
            {
                return fail("case 2 mark2 1 class 0");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - NULL Mark2 anchor.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewMark2Array();

            const OpenTypeGposMark2ArrayView mark2(
                ByteSpan(data.data(), data.size()), 2);

            if (mark2.hasAnchor(1, 1))
                return fail("case 3 unexpected anchor");

            if (mark2.anchor(1, 1))
                return fail("case 3 NULL anchor");

            ++passed;
        }


        // ====================================================================
        // Case 4 - MarkMarkPos geometry.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewFormat1();

            const OpenTypeGposMarkMarkPosView markMark(
                ByteSpan(data.data(), data.size()));

            if (!markMark ||
                markMark.format() != 1 ||
                markMark.markClassCount() != 2)
            {
                return fail("case 4 MarkMarkPos geometry");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Coverage mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewFormat1();

            const OpenTypeGposMarkMarkPosView markMark(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView mark1Coverage =
                markMark.mark1Coverage();

            const OpenTypeCoverageView mark2Coverage =
                markMark.mark2Coverage();

            if (!mark1Coverage || !mark2Coverage)
                return fail("case 5 Coverage views");

            uint16_t index = 999;

            if (!mark1Coverage.find(100, index) || index != 0)
                return fail("case 5 mark1 100");

            if (!mark1Coverage.find(101, index) || index != 1)
                return fail("case 5 mark1 101");

            if (!mark2Coverage.find(200, index) || index != 0)
                return fail("case 5 mark2 200");

            if (!mark2Coverage.find(201, index) || index != 1)
                return fail("case 5 mark2 201");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Complete class-selected path.
        //
        // mark1 101:
        //   Coverage index 1
        //   class 1
        //   anchor (30,40)
        //
        // mark2 200:
        //   Coverage index 0
        //   class 1 anchor (500,600)
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewFormat1();

            const OpenTypeGposMarkMarkPosView markMark(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGposMarkArrayView mark1 =
                markMark.mark1Array();

            const OpenTypeGposMark2ArrayView mark2 =
                markMark.mark2Array();

            if (!mark1 || !mark2)
                return fail("case 6 arrays");

            uint16_t markClass = 0;

            if (!mark1.markClass(1, markClass) ||
                markClass != 1)
            {
                return fail("case 6 mark class");
            }

            const OpenTypeGposAnchorView mark1Anchor =
                mark1.markAnchor(1);

            const OpenTypeGposAnchorView mark2Anchor =
                mark2.anchor(0, markClass);

            if (!mark1Anchor ||
                mark1Anchor.xCoordinate() != 30 ||
                mark1Anchor.yCoordinate() != 40)
            {
                return fail("case 6 mark1 anchor");
            }

            if (!mark2Anchor ||
                mark2Anchor.xCoordinate() != 500 ||
                mark2Anchor.yCoordinate() != 600)
            {
                return fail("case 6 mark2 anchor");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Bounds.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkMarkViewMark2Array();

            const OpenTypeGposMark2ArrayView mark2(
                ByteSpan(data.data(), data.size()), 2);

            uint16_t offset = 999;

            if (mark2.anchorOffset(2, 0, offset) ||
                mark2.anchorOffset(0, 2, offset))
            {
                return fail("case 7 Mark2Array bounds");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Structural failures.
        // ====================================================================

        {
            ++cases;


            // Unsupported format.

            {
                std::vector<uint8_t> data =
                    makeGposMarkMarkViewFormat1();

                patchGposMarkMarkViewU16(data, 0, 2);

                const OpenTypeGposMarkMarkPosView markMark(
                    ByteSpan(data.data(), data.size()));

                if (markMark)
                    return fail("case 8 unsupported format");
            }


            // Zero markClassCount.

            {
                std::vector<uint8_t> data =
                    makeGposMarkMarkViewFormat1();

                patchGposMarkMarkViewU16(data, 6, 0);

                const OpenTypeGposMarkMarkPosView markMark(
                    ByteSpan(data.data(), data.size()));

                if (markMark)
                    return fail("case 8 zero class count");
            }


            // Child points into parent header.

            {
                std::vector<uint8_t> data =
                    makeGposMarkMarkViewFormat1();

                patchGposMarkMarkViewU16(data, 10, 6);

                const OpenTypeGposMarkMarkPosView markMark(
                    ByteSpan(data.data(), data.size()));

                if (markMark)
                    return fail("case 8 child inside header");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Lazy child validation.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                makeGposMarkMarkViewFormat1();

            const OpenTypeGposMarkMarkPosView initial(
                ByteSpan(data.data(), data.size()));

            if (!initial)
                return fail("case 9 initial parent");

            const uint16_t mark2ArrayOffset =
                initial.mark2ArrayOffset();

            // Mark2Record[0], class 0 begins at Mark2Array + 2.
            // Point it back into the Mark2 record area.

            patchGposMarkMarkViewU16(
                data, size_t(mark2ArrayOffset) + 2, 2);

            const OpenTypeGposMarkMarkPosView markMark(
                ByteSpan(data.data(), data.size()));

            if (!markMark)
                return fail("case 9 parent rejected child");

            if (markMark.mark2Array())
                return fail("case 9 malformed Mark2Array accepted");

            ++passed;
        }


        std::printf(
            "OpenType GPOS Mark-to-Mark view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Mark2Array:                PASS\n"
            "  Mark2 anchors:              PASS\n"
            "  NULL Mark2 anchor:          PASS\n"
            "  MarkMarkPos geometry:       PASS\n"
            "  Coverage mapping:           PASS\n"
            "  Class-selected anchors:     PASS\n"
            "  Bounds:                     PASS\n"
            "  Structural failures:        PASS\n"
            "  Lazy child validation:      PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs