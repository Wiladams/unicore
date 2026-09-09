// test_opentype_nominal_metrics.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_nominal_metrics.h"
#include "shaped_glyph_view.h"

namespace waavs
{
    static void appendNominalMetricsU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendNominalMetricsS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendNominalMetricsU16(data, static_cast<uint16_t>(value));
    }


    // glyphCount = 6
    // numberOfHMetrics = 3
    //
    // glyph  advance  lsb
    //
    //   0      500     10
    //   1      600    -20
    //   2      700     30
    //   3      700     40
    //   4      700    -50
    //   5      700     60

    static std::vector<uint8_t> makeNominalMetricsHmtx()
    {
        std::vector<uint8_t> data;

        appendNominalMetricsU16(data, 500);
        appendNominalMetricsS16(data, 10);

        appendNominalMetricsU16(data, 600);
        appendNominalMetricsS16(data, -20);

        appendNominalMetricsU16(data, 700);
        appendNominalMetricsS16(data, 30);

        appendNominalMetricsS16(data, 40);
        appendNominalMetricsS16(data, -50);
        appendNominalMetricsS16(data, 60);

        return data;
    }


    static void appendNominalMetricsGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId,
        uint32_t scalarOffset, uint32_t scalarCount)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;

        buffer.pushBack(glyph);
    }


    static bool testOpenTypeNominalMetrics()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType nominal horizontal metrics: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> hmtxData = makeNominalMetricsHmtx();
        const OpenTypeHmtxView hmtx(ByteSpan(hmtxData.data(), hmtxData.size()), 6, 3);

        if (!hmtx)
            return fail("synthetic hmtx invalid");


        // ====================================================================
        // Case 1 - Basic conversion.
        //
        // Deliberately use nontrivial provenance values so this verifies more
        // than just glyph IDs.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;

            appendNominalMetricsGlyph(input, 0, 0, 1);
            appendNominalMetricsGlyph(input, 1, 1, 2);
            appendNominalMetricsGlyph(input, 2, 3, 1);

            ShapedGlyphBuffer output;

            if (!buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 1 conversion");

            if (output.size() != 3)
                return fail("case 1 output size");

            const uint32_t expectedGlyph[] = { 0, 1, 2 };
            const uint32_t expectedOffset[] = { 0, 1, 3 };
            const uint32_t expectedCount[] = { 1, 2, 1 };
            const int32_t expectedAdvance[] = { 500, 600, 700 };

            for (size_t i = 0; i < output.size(); ++i)
            {
                if (output[i].shaping.glyphId != expectedGlyph[i] ||
                    output[i].shaping.scalarOffset != expectedOffset[i] ||
                    output[i].shaping.scalarCount != expectedCount[i] ||
                    output[i].placement.advanceX != expectedAdvance[i])
                {
                    return fail("case 1 glyph transfer");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Initial non-horizontal placement is zero.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;
            appendNominalMetricsGlyph(input, 1, 4, 1);

            ShapedGlyphBuffer output;

            if (!buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 2 conversion");

            if (output.size() != 1 ||
                output[0].placement.advanceX != 600 ||
                output[0].placement.advanceY != 0 ||
                output[0].placement.offsetX != 0 ||
                output[0].placement.offsetY != 0)
            {
                return fail("case 2 initial placement");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Trailing glyph uses shared final advanceWidth.
        //
        // Glyph 5 has its own LSB but reuses glyph 2's advance 700.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;
            appendNominalMetricsGlyph(input, 5, 7, 2);

            ShapedGlyphBuffer output;

            if (!buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 3 conversion");

            if (output.size() != 1 ||
                output[0].shaping.glyphId != 5 ||
                output[0].placement.advanceX != 700)
            {
                return fail("case 3 shared advance");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Glyph zero is valid.
        //
        // .notdef is still a real glyph and has ordinary metrics.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;
            appendNominalMetricsGlyph(input, 0, 9, 1);

            ShapedGlyphBuffer output;

            if (!buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 4 conversion");

            if (output.size() != 1 ||
                output[0].shaping.glyphId != 0 ||
                output[0].placement.advanceX != 500)
            {
                return fail("case 4 glyph zero");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Empty post-GSUB stream.
        //
        // A valid metrics table plus an empty glyph stream produces an empty
        // shaped result.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;
            ShapedGlyphBuffer output;

            ShapedGlyph sentinel{};
            sentinel.shaping.glyphId = 123;
            output.pushBack(sentinel);

            if (!buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 5 conversion");

            if (!output.empty())
                return fail("case 5 empty result");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Transactional failure.
        //
        // Glyph 6 is outside the declared glyphCount of 6.
        // The destination must remain unchanged.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;

            appendNominalMetricsGlyph(input, 1, 0, 1);
            appendNominalMetricsGlyph(input, 6, 1, 1);

            ShapedGlyphBuffer output;

            ShapedGlyph sentinel{};
            sentinel.shaping.glyphId = 999;
            sentinel.shaping.scalarOffset = 77;
            sentinel.shaping.scalarCount = 3;
            sentinel.placement.advanceX = 1234;

            output.pushBack(sentinel);

            if (buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 6 invalid glyph accepted");

            if (output.size() != 1 ||
                output[0].shaping.glyphId != 999 ||
                output[0].shaping.scalarOffset != 77 ||
                output[0].shaping.scalarCount != 3 ||
                output[0].placement.advanceX != 1234)
            {
                return fail("case 6 transactional rollback");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - ShapedGlyphView exposes the completed result.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer input;

            appendNominalMetricsGlyph(input, 1, 10, 1);
            appendNominalMetricsGlyph(input, 4, 11, 2);

            ShapedGlyphBuffer output;

            if (!buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output))
                return fail("case 7 conversion");

            const ShapedGlyphView view(output);

            if (!view ||
                view.size() != 2 ||
                view[0].shaping.glyphId != 1 ||
                view[0].placement.advanceX != 600 ||
                view[1].shaping.glyphId != 4 ||
                view[1].placement.advanceX != 700)
            {
                return fail("case 7 shaped view");
            }

            size_t iterated = 0;

            for (const ShapedGlyph& glyph : view)
            {
                if (glyph.shaping.glyphId != (iterated == 0 ? 1u : 4u))
                    return fail("case 7 view iteration");

                ++iterated;
            }

            if (iterated != 2)
                return fail("case 7 iteration count");

            ++passed;
        }


        std::printf(
            "OpenType nominal horizontal metrics: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Glyph/provenance transfer: PASS\n"
            "  Initial placement:         PASS\n"
            "  Shared final advance:      PASS\n"
            "  Glyph zero:                PASS\n"
            "  Empty stream:              PASS\n"
            "  Transactional failure:     PASS\n"
            "  ShapedGlyphView:           PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs