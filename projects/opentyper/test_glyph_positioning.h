// test_glyph_positioning.h
#pragma once

#include "test_core.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "glyph_positioning.h"

namespace waavs
{
    struct GlyphPositioningTestRecord
    {
        GlyphId glyphId{ 0 };
        uint32_t scalarOffset{ 0 };
        uint32_t scalarCount{ 0 };

        double x{ 0.0 };
        double y{ 0.0 };
        double scale{ 0.0 };
    };


    struct GlyphPositioningTestSink
    {
        std::vector<GlyphPositioningTestRecord> records;

        bool onGlyph(const ShapedGlyph& glyph, double x, double y, double scale)
        {
            GlyphPositioningTestRecord record{};

            record.glyphId = glyph.shaping.glyphId;
            record.scalarOffset = glyph.shaping.scalarOffset;
            record.scalarCount = glyph.shaping.scalarCount;
            record.x = x;
            record.y = y;
            record.scale = scale;

            records.push_back(record);
            return true;
        }
    };


    static bool glyphPositioningNearlyEqual(double a, double b) noexcept
    {
        return std::fabs(a - b) <= 1.0e-9;
    }


    static bool testGlyphPositioning()
    {
        auto fail = [](const char* message)
            {
                std::printf("Horizontal glyph positioning: FAIL: %s\n", message);
                return false;
            };


        // ================================================================
        // Three shaped glyphs in design units.
        //
        // unitsPerEm = 1000
        // fontSize   = 20
        // scale      = 0.02
        //
        // origin = (100, 200)
        //
        // glyph 10:
        //     origin  = (100 + 20 * .02, 200 + 50 * .02)
        //             = (100.4, 201.0)
        //     advance = 600 * .02 = 12
        //
        // glyph 20:
        //     pen     = (112, 200)
        //     origin  = (112 - 30 * .02, 200 - 20 * .02)
        //             = (111.4, 199.6)
        //     advance = 500 * .02 = 10
        //
        // glyph 30:
        //     pen     = (122, 200)
        //     origin  = (122, 200)
        //     advance = 400 * .02 = 8
        //
        // final pen = (130, 200)
        // ================================================================

        ShapedGlyph shaped[3]{};

        shaped[0].shaping.glyphId = 10;
        shaped[0].shaping.scalarOffset = 0;
        shaped[0].shaping.scalarCount = 1;
        shaped[0].placement.advanceX = 600;
        shaped[0].placement.offsetX = 20;
        shaped[0].placement.offsetY = 50;

        shaped[1].shaping.glyphId = 20;
        shaped[1].shaping.scalarOffset = 1;
        shaped[1].shaping.scalarCount = 2;
        shaped[1].placement.advanceX = 500;
        shaped[1].placement.offsetX = -30;
        shaped[1].placement.offsetY = -20;

        shaped[2].shaping.glyphId = 30;
        shaped[2].shaping.scalarOffset = 3;
        shaped[2].shaping.scalarCount = 1;
        shaped[2].placement.advanceX = 400;

        const ShapedGlyphView glyphs(shaped, 3);

        GlyphPositioningTestSink sink;
        HorizontalGlyphPositioningResult result{};

        if (!positionHorizontalLTRGlyphRun(glyphs, 20.0, 1000, 100.0, 200.0, sink, &result))
            return fail("positioning failed");


        // ================================================================
        // Scale
        // ================================================================

        if (!glyphPositioningNearlyEqual(result.scale, 0.02))
            return fail("incorrect design-unit scale");


        // ================================================================
        // Glyph count
        // ================================================================

        if (sink.records.size() != 3)
            return fail("incorrect emitted glyph count");


        // ================================================================
        // Glyph 0
        // ================================================================

        if (sink.records[0].glyphId != 10 ||
            !glyphPositioningNearlyEqual(sink.records[0].x, 100.4) ||
            !glyphPositioningNearlyEqual(sink.records[0].y, 201.0))
        {
            return fail("glyph 0 position");
        }


        // ================================================================
        // Glyph 1
        // ================================================================

        if (sink.records[1].glyphId != 20 ||
            !glyphPositioningNearlyEqual(sink.records[1].x, 111.4) ||
            !glyphPositioningNearlyEqual(sink.records[1].y, 199.6))
        {
            return fail("glyph 1 position");
        }


        // ================================================================
        // Glyph 2
        // ================================================================

        if (sink.records[2].glyphId != 30 ||
            !glyphPositioningNearlyEqual(sink.records[2].x, 122.0) ||
            !glyphPositioningNearlyEqual(sink.records[2].y, 200.0))
        {
            return fail("glyph 2 position");
        }


        // ================================================================
        // Provenance must pass through unchanged.
        // ================================================================

        if (sink.records[0].scalarOffset != 0 ||
            sink.records[0].scalarCount != 1 ||
            sink.records[1].scalarOffset != 1 ||
            sink.records[1].scalarCount != 2 ||
            sink.records[2].scalarOffset != 3 ||
            sink.records[2].scalarCount != 1)
        {
            return fail("glyph provenance changed");
        }


        // ================================================================
        // Final pen
        // ================================================================

        if (!glyphPositioningNearlyEqual(result.penX, 130.0) ||
            !glyphPositioningNearlyEqual(result.penY, 200.0))
        {
            return fail("incorrect final pen");
        }


        // ================================================================
        // Empty run
        // ================================================================

        {
            GlyphPositioningTestSink emptySink;
            HorizontalGlyphPositioningResult emptyResult{};
            ShapedGlyphView emptyView;

            if (!positionHorizontalLTRGlyphRun(
                emptyView, 20.0, 1000, 17.0, 23.0,
                emptySink, &emptyResult))
            {
                return fail("empty run rejected");
            }

            if (!emptySink.records.empty() ||
                !glyphPositioningNearlyEqual(emptyResult.penX, 17.0) ||
                !glyphPositioningNearlyEqual(emptyResult.penY, 23.0) ||
                !glyphPositioningNearlyEqual(emptyResult.scale, 0.02))
            {
                return fail("empty run result");
            }
        }


        // ================================================================
        // Invalid inputs
        // ================================================================

        {
            GlyphPositioningTestSink invalidSink;

            if (positionHorizontalLTRGlyphRun(
                glyphs, 20.0, 0, 0.0, 0.0, invalidSink))
            {
                return fail("zero unitsPerEm accepted");
            }

            if (positionHorizontalLTRGlyphRun(
                glyphs, 0.0, 1000, 0.0, 0.0, invalidSink))
            {
                return fail("zero font size accepted");
            }

            if (positionHorizontalLTRGlyphRun(
                glyphs,
                std::numeric_limits<double>::quiet_NaN(),
                1000, 0.0, 0.0, invalidSink))
            {
                return fail("NaN font size accepted");
            }

            if (!invalidSink.records.empty())
                return fail("invalid input emitted glyphs");
        }


        std::printf(
            "Horizontal glyph positioning: PASS\n"
            "  Glyphs:             %zu\n"
            "  Units per em:       %u\n"
            "  Font size:          %.2f\n"
            "  Scale:              %.6f\n"
            "  Origin:             %.2f, %.2f\n"
            "  Final pen:          %.2f, %.2f\n"
            "  GPOS offsets:       PASS\n"
            "  Provenance:         PASS\n"
            "  Validation:         PASS\n",
            sink.records.size(),
            1000u,
            20.0,
            result.scale,
            100.0,
            200.0,
            result.penX,
            result.penY);

        return true;
    }

} // namespace waavs