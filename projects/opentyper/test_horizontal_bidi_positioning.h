#pragma once

#include "test_core.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "horizontal_bidi_positioning.h"

namespace waavs
{
    struct HorizontalGlyphTestRecord
    {
        GlyphId glyphId{ 0 };
        double x{ 0.0 };
        double y{ 0.0 };
        double scale{ 0.0 };
    };


    class HorizontalGlyphTestSink
    {
    public:
        bool onGlyph(const ShapedGlyph& glyph, double x, double y, double scale)
        {
            HorizontalGlyphTestRecord record;

            record.glyphId = glyph.shaping.glyphId;
            record.x = x;
            record.y = y;
            record.scale = scale;

            records.push_back(record);
            return true;
        }

        std::vector<HorizontalGlyphTestRecord> records;
    };


    struct HorizontalBidiTestRecord
    {
        size_t runIndex{ 0 };
        GlyphId glyphId{ 0 };
        double x{ 0.0 };
        double y{ 0.0 };
        double scale{ 0.0 };
    };


    class HorizontalBidiTestSink
    {
    public:
        bool onGlyph(size_t runIndex, const ShapedGlyph& glyph, double x, double y, double scale)
        {
            HorizontalBidiTestRecord record;

            record.runIndex = runIndex;
            record.glyphId = glyph.shaping.glyphId;
            record.x = x;
            record.y = y;
            record.scale = scale;

            records.push_back(record);
            return true;
        }

        std::vector<HorizontalBidiTestRecord> records;
    };


    static bool horizontalBidiNearlyEqual(double a, double b) noexcept
    {
        return std::fabs(a - b) <= 1.0e-9;
    }


    static void appendHorizontalBidiGlyph(ShapedGlyphBuffer& buffer, GlyphId glyphId,
        int32_t advanceX, int32_t offsetX = 0, int32_t offsetY = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.placement.advanceX = advanceX;
        glyph.placement.advanceY = 0;
        glyph.placement.offsetX = offsetX;
        glyph.placement.offsetY = offsetY;

        buffer.pushBack(glyph);
    }


    static bool testHorizontalBidiPositioning()
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        size_t cases = 0;
        size_t passed = 0;


        // ================================================================
        // Case 1
        //
        // Direct RTL positioning regression.
        //
        // originX is the RIGHT edge of the run.
        //
        // scale = 1000 / 1000 = 1
        //
        // glyph 10:
        //     advance = 100
        //     pen     = 500 - 100 = 400
        //     offsetX = 7
        //     x       = 407
        //
        // glyph 20:
        //     advance = 80
        //     pen     = 400 - 80 = 320
        //     offsetX = -5
        //     x       = 315
        //
        // This specifically proves that RTL consumes the advance BEFORE
        // emitting the glyph.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            appendHorizontalBidiGlyph(buffer, 10, 100, 7, 3);
            appendHorizontalBidiGlyph(buffer, 20, 80, -5, -2);

            HorizontalGlyphTestSink sink;
            HorizontalGlyphPositioningResult result{};

            if (!positionHorizontalRTLGlyphRun(
                ShapedGlyphView(buffer),
                1000.0,
                1000,
                500.0,
                40.0,
                sink,
                &result))
            {
                return fail("direct RTL positioning failed");
            }

            if (sink.records.size() != 2)
                return fail("direct RTL emitted glyph count mismatch");

            if (sink.records[0].glyphId != 10 ||
                sink.records[1].glyphId != 20)
            {
                return fail("direct RTL logical glyph order changed");
            }

            if (!horizontalBidiNearlyEqual(sink.records[0].x, 407.0) ||
                !horizontalBidiNearlyEqual(sink.records[0].y, 43.0))
            {
                return fail("direct RTL first glyph position mismatch");
            }

            if (!horizontalBidiNearlyEqual(sink.records[1].x, 315.0) ||
                !horizontalBidiNearlyEqual(sink.records[1].y, 38.0))
            {
                return fail("direct RTL second glyph position mismatch");
            }

            if (!horizontalBidiNearlyEqual(sink.records[0].scale, 1.0) ||
                !horizontalBidiNearlyEqual(sink.records[1].scale, 1.0))
            {
                return fail("direct RTL glyph scale mismatch");
            }

            // The right edge began at 500. Total advance is 180.
            // Final pen must therefore be the left edge at 320.

            if (!horizontalBidiNearlyEqual(result.penX, 320.0))
                return fail("direct RTL final pen is not left edge");

            if (!horizontalBidiNearlyEqual(result.penY, 40.0))
                return fail("direct RTL final baseline mismatch");

            if (!horizontalBidiNearlyEqual(result.scale, 1.0))
                return fail("direct RTL result scale mismatch");

            // Positioning must never mutate or reorder the shaped buffer.

            if (buffer.size() != 2 ||
                buffer[0].shaping.glyphId != 10 ||
                buffer[1].shaping.glyphId != 20)
            {
                return fail("direct RTL shaped buffer was modified");
            }

            ++passed;
        }


        // ================================================================
        // Case 2
        //
        // Five logical runs.
        //
        // Levels:
        //
        //     0 1 2 1 0
        //
        // L2 visual order:
        //
        //     0 3 2 1 4
        //
        // fontSize / unitsPerEm:
        //
        //     100 / 1000 = 0.1
        //
        // This proves:
        //
        //     - run-level L2 ordering
        //     - RTL consume-before-emit behavior
        //     - glyph buffers remain logical
        //     - run boxes remain contiguous
        //     - paragraph visual extent is correct
        // ================================================================

        {
            ++cases;

            std::vector<ShapedGlyphBuffer> buffers(5);

            appendHorizontalBidiGlyph(buffers[0], 10, 100);

            appendHorizontalBidiGlyph(buffers[1], 20, 120);
            appendHorizontalBidiGlyph(buffers[1], 21, 80);

            appendHorizontalBidiGlyph(buffers[2], 30, 100);

            appendHorizontalBidiGlyph(buffers[3], 40, 150);

            appendHorizontalBidiGlyph(buffers[4], 50, 50);


            std::vector<HorizontalBidiRunView> runs =
            {
                { ShapedGlyphView(buffers[0]), 0, 1000 },
                { ShapedGlyphView(buffers[1]), 1, 1000 },
                { ShapedGlyphView(buffers[2]), 2, 1000 },
                { ShapedGlyphView(buffers[3]), 1, 1000 },
                { ShapedGlyphView(buffers[4]), 0, 1000 }
            };


            // ------------------------------------------------------------
            // Run widths:
            //
            //     run 0 = 10
            //     run 1 = 20
            //     run 2 = 10
            //     run 3 = 15
            //     run 4 =  5
            //
            // Total = 60
            //
            // Visual boxes:
            //
            //     run 0: [100,110] LTR
            //     run 3: [110,125] RTL
            //     run 2: [125,135] LTR
            //     run 1: [135,155] RTL
            //     run 4: [155,160] LTR
            //
            // RTL origins after consuming each positive advance:
            //
            //     run 3 glyph 40:
            //         125 - 15 = 110
            //
            //     run 1 glyph 20:
            //         155 - 12 = 143
            //
            //     run 1 glyph 21:
            //         143 - 8 = 135
            // ------------------------------------------------------------

            HorizontalBidiTestSink sink;
            HorizontalBidiPositioningResult result{};

            if (!positionHorizontalBidiRuns(
                runs,
                100.0,
                100.0,
                50.0,
                sink,
                &result))
            {
                return fail("mixed paragraph positioning failed");
            }

            if (sink.records.size() != 6)
                return fail("mixed paragraph emitted glyph count mismatch");


            // ------------------------------------------------------------
            // Visual run ordering.
            //
            // Emission:
            //
            //     run 0 -> glyph 10
            //     run 3 -> glyph 40
            //     run 2 -> glyph 30
            //     run 1 -> glyph 20, glyph 21
            //     run 4 -> glyph 50
            //
            // Glyphs 20 and 21 remain in LOGICAL order.
            // ------------------------------------------------------------

            const GlyphId expectedGlyphs[] =
            {
                10,
                40,
                30,
                20,
                21,
                50
            };

            const size_t expectedRuns[] =
            {
                0,
                3,
                2,
                1,
                1,
                4
            };

            for (size_t i = 0; i < 6; ++i)
            {
                if (sink.records[i].glyphId != expectedGlyphs[i])
                    return fail("mixed visual glyph emission order mismatch");

                if (sink.records[i].runIndex != expectedRuns[i])
                    return fail("mixed visual logical-run index mismatch");
            }


            // ------------------------------------------------------------
            // Corrected glyph origins.
            //
            // run 0 LTR:
            //
            //     glyph 10 = 100
            //
            // run 3 RTL:
            //
            //     right edge = 125
            //     glyph 40   = 125 - 15 = 110
            //
            // run 2 LTR:
            //
            //     glyph 30 = 125
            //
            // run 1 RTL:
            //
            //     right edge = 155
            //     glyph 20   = 155 - 12 = 143
            //     glyph 21   = 143 - 8  = 135
            //
            // run 4 LTR:
            //
            //     glyph 50 = 155
            // ------------------------------------------------------------

            const double expectedX[] =
            {
                100.0,
                110.0,
                125.0,
                143.0,
                135.0,
                155.0
            };

            for (size_t i = 0; i < 6; ++i)
            {
                if (!horizontalBidiNearlyEqual(sink.records[i].x, expectedX[i]))
                    return fail("mixed glyph X position mismatch");

                if (!horizontalBidiNearlyEqual(sink.records[i].y, 50.0))
                    return fail("mixed glyph Y position mismatch");

                if (!horizontalBidiNearlyEqual(sink.records[i].scale, 0.1))
                    return fail("mixed glyph scale mismatch");
            }


            // ------------------------------------------------------------
            // Final visual extent.
            // ------------------------------------------------------------

            if (!horizontalBidiNearlyEqual(result.width, 60.0))
                return fail("mixed paragraph width mismatch");

            if (!horizontalBidiNearlyEqual(result.penX, 160.0))
                return fail("mixed final visual pen mismatch");

            if (!horizontalBidiNearlyEqual(result.penY, 50.0))
                return fail("mixed final visual baseline mismatch");

            if (result.runCount != 5)
                return fail("mixed run count mismatch");


            // ------------------------------------------------------------
            // Prove RTL buffers themselves were untouched.
            // ------------------------------------------------------------

            if (buffers[1].size() != 2 ||
                buffers[1][0].shaping.glyphId != 20 ||
                buffers[1][1].shaping.glyphId != 21)
            {
                return fail("mixed RTL glyph buffer was reordered");
            }

            if (buffers[3].size() != 1 ||
                buffers[3][0].shaping.glyphId != 40)
            {
                return fail("single-glyph RTL buffer was modified");
            }

            ++passed;
        }


        // ================================================================
        // Case 3
        //
        // Empty paragraph is a successful no-op.
        // ================================================================

        {
            ++cases;

            std::vector<HorizontalBidiRunView> runs;
            HorizontalBidiTestSink sink;
            HorizontalBidiPositioningResult result{};

            if (!positionHorizontalBidiRuns(
                runs,
                100.0,
                75.0,
                25.0,
                sink,
                &result))
            {
                return fail("empty paragraph positioning failed");
            }

            if (!sink.records.empty())
                return fail("empty paragraph emitted glyphs");

            if (!horizontalBidiNearlyEqual(result.penX, 75.0) ||
                !horizontalBidiNearlyEqual(result.penY, 25.0) ||
                !horizontalBidiNearlyEqual(result.width, 0.0) ||
                result.runCount != 0)
            {
                return fail("empty paragraph result mismatch");
            }

            ++passed;
        }


        // ================================================================
        // Summary
        // ================================================================

        std::printf(
            "Horizontal bidi positioning: PASS\n"
            "  Cases:                   %zu\n"
            "  Passed:                  %zu\n"
            "  RTL consume-before-emit: PASS\n"
            "  RTL offsets:             PASS\n"
            "  RTL logical order:       PASS\n"
            "  RTL final-left-edge:     PASS\n"
            "  L2 visual run order:     PASS\n"
            "  Mixed run boxes:         PASS\n"
            "  Mixed final pen:         PASS\n"
            "  Buffer preservation:     PASS\n"
            "  Empty paragraph:         PASS\n",
            cases,
            passed);

        return passed == cases;
    }

} // namespace waavs