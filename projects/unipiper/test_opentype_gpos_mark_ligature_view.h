// test_opentype_gpos_mark_ligature_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_mark_ligature_view.h"

namespace waavs
{
    static void appendGposMarkLigViewU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposMarkLigViewS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposMarkLigViewU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposMarkLigViewU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposMarkLigViewAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposMarkLigViewU16(data, 1);
        appendGposMarkLigViewS16(data, x);
        appendGposMarkLigViewS16(data, y);
    }

    static void appendGposMarkLigViewCoverage(
        std::vector<uint8_t>& data, const uint16_t* glyphs, uint16_t count)
    {
        appendGposMarkLigViewU16(data, 1);
        appendGposMarkLigViewU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposMarkLigViewU16(data, glyphs[i]);
    }


    // ====================================================================
    // MarkArray:
    //
    // mark 100 -> class 0 -> (10, 20)
    // mark 101 -> class 1 -> (30, 40)
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkLigViewMarkArray()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigViewU16(data, 2);

        appendGposMarkLigViewU16(data, 0);
        const size_t anchor0Patch = data.size();
        appendGposMarkLigViewU16(data, 0);

        appendGposMarkLigViewU16(data, 1);
        const size_t anchor1Patch = data.size();
        appendGposMarkLigViewU16(data, 0);

        patchGposMarkLigViewU16(
            data, anchor0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 10, 20);

        patchGposMarkLigViewU16(
            data, anchor1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 30, 40);

        return data;
    }


    // ====================================================================
    // LigatureAttach for glyph 500.
    //
    // Three components, two classes:
    //
    // component 0:
    //   class 0 -> (300, 400)
    //   class 1 -> NULL
    //
    // component 1:
    //   class 0 -> NULL
    //   class 1 -> (500, 600)
    //
    // component 2:
    //   class 0 -> (700, 800)
    //   class 1 -> (900, 1000)
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkLigViewAttach500()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigViewU16(data, 3);

        const size_t c0Class0Patch = data.size();
        appendGposMarkLigViewU16(data, 0);
        appendGposMarkLigViewU16(data, 0);

        appendGposMarkLigViewU16(data, 0);
        const size_t c1Class1Patch = data.size();
        appendGposMarkLigViewU16(data, 0);

        const size_t c2Class0Patch = data.size();
        appendGposMarkLigViewU16(data, 0);
        const size_t c2Class1Patch = data.size();
        appendGposMarkLigViewU16(data, 0);


        patchGposMarkLigViewU16(
            data, c0Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 300, 400);


        patchGposMarkLigViewU16(
            data, c1Class1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 500, 600);


        patchGposMarkLigViewU16(
            data, c2Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 700, 800);


        patchGposMarkLigViewU16(
            data, c2Class1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 900, 1000);

        return data;
    }


    // ====================================================================
    // LigatureAttach for glyph 600.
    //
    // Two components, two classes.
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkLigViewAttach600()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigViewU16(data, 2);

        const size_t c0Class0Patch = data.size();
        appendGposMarkLigViewU16(data, 0);

        const size_t c0Class1Patch = data.size();
        appendGposMarkLigViewU16(data, 0);

        appendGposMarkLigViewU16(data, 0);

        const size_t c1Class1Patch = data.size();
        appendGposMarkLigViewU16(data, 0);


        patchGposMarkLigViewU16(
            data, c0Class0Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 100, 200);


        patchGposMarkLigViewU16(
            data, c0Class1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 110, 210);


        patchGposMarkLigViewU16(
            data, c1Class1Patch,
            static_cast<uint16_t>(data.size()));

        appendGposMarkLigViewAnchor(data, 120, 220);

        return data;
    }


    static std::vector<uint8_t> makeGposMarkLigViewLigatureArray()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigViewU16(data, 2);

        const size_t attach500Patch = data.size();
        appendGposMarkLigViewU16(data, 0);

        const size_t attach600Patch = data.size();
        appendGposMarkLigViewU16(data, 0);


        patchGposMarkLigViewU16(
            data, attach500Patch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> attach =
                makeGposMarkLigViewAttach500();

            data.insert(data.end(), attach.begin(), attach.end());
        }


        patchGposMarkLigViewU16(
            data, attach600Patch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> attach =
                makeGposMarkLigViewAttach600();

            data.insert(data.end(), attach.begin(), attach.end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposMarkLigViewFormat1()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigViewU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposMarkLigViewU16(data, 0);

        const size_t ligatureCoveragePatch = data.size();
        appendGposMarkLigViewU16(data, 0);

        appendGposMarkLigViewU16(data, 2);

        const size_t markArrayPatch = data.size();
        appendGposMarkLigViewU16(data, 0);

        const size_t ligatureArrayPatch = data.size();
        appendGposMarkLigViewU16(data, 0);


        // MarkCoverage.

        patchGposMarkLigViewU16(
            data, markCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 101 };
            appendGposMarkLigViewCoverage(data, glyphs, 2);
        }


        // LigatureCoverage.

        patchGposMarkLigViewU16(
            data, ligatureCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 500, 600 };
            appendGposMarkLigViewCoverage(data, glyphs, 2);
        }


        // MarkArray.

        patchGposMarkLigViewU16(
            data, markArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> array =
                makeGposMarkLigViewMarkArray();

            data.insert(data.end(), array.begin(), array.end());
        }


        // LigatureArray.

        patchGposMarkLigViewU16(
            data, ligatureArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const std::vector<uint8_t> array =
                makeGposMarkLigViewLigatureArray();

            data.insert(data.end(), array.begin(), array.end());
        }

        return data;
    }


    static bool testOpenTypeGposMarkLigatureView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Mark-to-Ligature view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - LigatureAttach geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewAttach500();

            const OpenTypeGposLigatureAttachView attach(
                ByteSpan(data.data(), data.size()), 2);

            if (!attach ||
                attach.componentCount() != 3 ||
                attach.markClassCount() != 2)
            {
                return fail("case 1 LigatureAttach geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Component/class anchor selection.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewAttach500();

            const OpenTypeGposLigatureAttachView attach(
                ByteSpan(data.data(), data.size()), 2);

            const OpenTypeGposAnchorView anchor0 =
                attach.anchor(0, 0);

            const OpenTypeGposAnchorView anchor1 =
                attach.anchor(1, 1);

            const OpenTypeGposAnchorView anchor2 =
                attach.anchor(2, 1);

            if (!anchor0 ||
                anchor0.xCoordinate() != 300 ||
                anchor0.yCoordinate() != 400)
            {
                return fail("case 2 component 0 class 0");
            }

            if (!anchor1 ||
                anchor1.xCoordinate() != 500 ||
                anchor1.yCoordinate() != 600)
            {
                return fail("case 2 component 1 class 1");
            }

            if (!anchor2 ||
                anchor2.xCoordinate() != 900 ||
                anchor2.yCoordinate() != 1000)
            {
                return fail("case 2 component 2 class 1");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - NULL component anchor.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewAttach500();

            const OpenTypeGposLigatureAttachView attach(
                ByteSpan(data.data(), data.size()), 2);

            if (attach.hasAnchor(0, 1))
                return fail("case 3 unexpected component anchor");

            if (attach.anchor(0, 1))
                return fail("case 3 NULL component anchor");

            if (attach.hasAnchor(1, 0))
                return fail("case 3 second unexpected anchor");

            ++passed;
        }


        // ================================================================
        // Case 4 - LigatureArray.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewLigatureArray();

            const OpenTypeGposLigatureArrayView array(
                ByteSpan(data.data(), data.size()), 2);

            if (!array ||
                array.ligatureCount() != 2)
            {
                return fail("case 4 LigatureArray geometry");
            }

            const OpenTypeGposLigatureAttachView first =
                array.ligatureAttach(0);

            const OpenTypeGposLigatureAttachView second =
                array.ligatureAttach(1);

            if (!first || first.componentCount() != 3)
                return fail("case 4 first LigatureAttach");

            if (!second || second.componentCount() != 2)
                return fail("case 4 second LigatureAttach");

            ++passed;
        }


        // ================================================================
        // Case 5 - MarkLigPos geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewFormat1();

            const OpenTypeGposMarkLigaturePosView markLig(
                ByteSpan(data.data(), data.size()));

            if (!markLig ||
                markLig.format() != 1 ||
                markLig.markClassCount() != 2)
            {
                return fail("case 5 MarkLigPos geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Coverage mapping.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewFormat1();

            const OpenTypeGposMarkLigaturePosView markLig(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView markCoverage =
                markLig.markCoverage();

            const OpenTypeCoverageView ligatureCoverage =
                markLig.ligatureCoverage();

            if (!markCoverage || !ligatureCoverage)
                return fail("case 6 Coverage views");

            uint16_t index = 999;

            if (!markCoverage.find(100, index) || index != 0)
                return fail("case 6 mark 100");

            if (!markCoverage.find(101, index) || index != 1)
                return fail("case 6 mark 101");

            if (!ligatureCoverage.find(500, index) || index != 0)
                return fail("case 6 ligature 500");

            if (!ligatureCoverage.find(600, index) || index != 1)
                return fail("case 6 ligature 600");

            ++passed;
        }


        // ================================================================
        // Case 7 - Complete class/component path.
        //
        // mark 101:
        //   Coverage index 1
        //   mark class 1
        //
        // ligature 500:
        //   Coverage index 0
        //
        // component 1:
        //   class 1 -> (500, 600)
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposMarkLigViewFormat1();

            const OpenTypeGposMarkLigaturePosView markLig(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGposMarkArrayView marks =
                markLig.markArray();

            const OpenTypeGposLigatureArrayView ligatures =
                markLig.ligatureArray();

            if (!marks || !ligatures)
                return fail("case 7 arrays");

            uint16_t markClass = 0;

            if (!marks.markClass(1, markClass) ||
                markClass != 1)
            {
                return fail("case 7 mark class");
            }

            const OpenTypeGposLigatureAttachView attach =
                ligatures.ligatureAttach(0);

            if (!attach)
                return fail("case 7 LigatureAttach");

            const OpenTypeGposAnchorView markAnchor =
                marks.markAnchor(1);

            const OpenTypeGposAnchorView ligatureAnchor =
                attach.anchor(1, markClass);

            if (!markAnchor ||
                markAnchor.xCoordinate() != 30 ||
                markAnchor.yCoordinate() != 40)
            {
                return fail("case 7 mark anchor");
            }

            if (!ligatureAnchor ||
                ligatureAnchor.xCoordinate() != 500 ||
                ligatureAnchor.yCoordinate() != 600)
            {
                return fail("case 7 ligature anchor");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Bounds.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> attachData =
                makeGposMarkLigViewAttach500();

            const OpenTypeGposLigatureAttachView attach(
                ByteSpan(attachData.data(), attachData.size()), 2);

            uint16_t offset = 999;

            if (attach.anchorOffset(3, 0, offset) ||
                attach.anchorOffset(0, 2, offset))
            {
                return fail("case 8 LigatureAttach bounds");
            }

            const std::vector<uint8_t> arrayData =
                makeGposMarkLigViewLigatureArray();

            const OpenTypeGposLigatureArrayView array(
                ByteSpan(arrayData.data(), arrayData.size()), 2);

            if (array.ligatureAttachOffset(2, offset))
                return fail("case 8 LigatureArray bounds");

            ++passed;
        }


        // ================================================================
        // Case 9 - Structural and lazy-child failures.
        // ================================================================

        {
            ++cases;


            // Unsupported top-level format.

            {
                std::vector<uint8_t> data =
                    makeGposMarkLigViewFormat1();

                patchGposMarkLigViewU16(data, 0, 2);

                const OpenTypeGposMarkLigaturePosView markLig(
                    ByteSpan(data.data(), data.size()));

                if (markLig)
                    return fail("case 9 unsupported format");
            }


            // Zero markClassCount.

            {
                std::vector<uint8_t> data =
                    makeGposMarkLigViewFormat1();

                patchGposMarkLigViewU16(data, 6, 0);

                const OpenTypeGposMarkLigaturePosView markLig(
                    ByteSpan(data.data(), data.size()));

                if (markLig)
                    return fail("case 9 zero class count");
            }


            // Malformed LigatureAttach is detected lazily.

            {
                std::vector<uint8_t> data =
                    makeGposMarkLigViewFormat1();

                const OpenTypeGposMarkLigaturePosView initial(
                    ByteSpan(data.data(), data.size()));

                if (!initial)
                    return fail("case 9 initial parent");

                const uint16_t arrayOffset =
                    initial.ligatureArrayOffset();

                const OpenTypeGposLigatureArrayView initialArray =
                    initial.ligatureArray();

                if (!initialArray)
                    return fail("case 9 initial array");

                uint16_t attachOffset = 0;

                if (!initialArray.ligatureAttachOffset(
                    0, attachOffset))
                {
                    return fail("case 9 attach offset");
                }


                // componentCount -> zero.
                //
                // Parent and LigatureArray remain structurally valid.
                // LigatureAttach becomes invalid.

                patchGposMarkLigViewU16(
                    data,
                    size_t(arrayOffset) +
                    size_t(attachOffset),
                    0);

                const OpenTypeGposMarkLigaturePosView markLig(
                    ByteSpan(data.data(), data.size()));

                if (!markLig)
                    return fail("case 9 parent rejected child");

                const OpenTypeGposLigatureArrayView array =
                    markLig.ligatureArray();

                if (!array)
                    return fail("case 9 array rejected child");

                if (array.ligatureAttach(0))
                    return fail("case 9 malformed child accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Mark-to-Ligature view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  LigatureAttach:             PASS\n"
            "  Component anchors:          PASS\n"
            "  NULL component anchor:      PASS\n"
            "  LigatureArray:              PASS\n"
            "  MarkLigPos geometry:        PASS\n"
            "  Coverage mapping:           PASS\n"
            "  Class/component selection:  PASS\n"
            "  Bounds:                     PASS\n"
            "  Structural failures:        PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs