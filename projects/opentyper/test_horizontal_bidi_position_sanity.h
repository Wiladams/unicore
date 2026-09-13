// test_horizontal_bidi_positioning_sanity.h
#pragma once

#include "test_core.h"

#include "glyph_positioning.h"
#include "horizontal_bidi_positioning.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace waavs
{
    static bool nearlyEqual(double a, double b, double epsilon = 1e-9) noexcept
    {
        return std::fabs(a - b) <= epsilon;
    }


    static ShapedGlyph makePositioningTestGlyph(
        GlyphId glyphId, int32_t advanceX,
        int32_t offsetX = 0, int32_t offsetY = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;
        glyph.placement.advanceY = 0;
        glyph.placement.offsetX = offsetX;
        glyph.placement.offsetY = offsetY;

        return glyph;
    }


    struct HorizontalPositionTestGlyph
    {
        GlyphId glyphId{ 0 };
        double x{ 0.0 };
        double y{ 0.0 };
        double scale{ 0.0 };
    };


    struct HorizontalPositionTestSink
    {
        std::vector<HorizontalPositionTestGlyph> glyphs{};

        bool onGlyph(const ShapedGlyph& glyph, double x, double y, double scale)
        {
            glyphs.push_back({
                glyph.shaping.glyphId,
                x,
                y,
                scale
                });

            return true;
        }
    };


    struct HorizontalBidiTestGlyph
    {
        size_t runIndex{ 0 };
        GlyphId glyphId{ 0 };
        double x{ 0.0 };
        double y{ 0.0 };
        double scale{ 0.0 };
    };


    struct HorizontalBidiTestSink
    {
        std::vector<HorizontalBidiTestGlyph> glyphs{};

        bool onGlyph(size_t runIndex, const ShapedGlyph& glyph,
            double x, double y, double scale)
        {
            glyphs.push_back({
                runIndex,
                glyph.shaping.glyphId,
                x,
                y,
                scale
                });

            return true;
        }
    };


    static bool testHorizontalBidiPositioningSanity()
    {
        size_t cases = 0;
        size_t passed = 0;


        // ================================================================
        // Case 1
        //
        // Direct RTL positioning.
        //
        // right edge = 500
        //
        // logical glyph 10:
        //     advance = 100
        //     pen     = 500 - 100 = 400
        //     x       = 400 + 7 = 407
        //
        // logical glyph 20:
        //     advance = 80
        //     pen     = 400 - 80 = 320
        //     x       = 320 - 5 = 315
        //
        // This specifically catches the old bug where the glyph was emitted
        // before its advance was consumed.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makePositioningTestGlyph(10, 100, 7, 3));
            buffer.pushBack(makePositioningTestGlyph(20, 80, -5, -2));

            HorizontalPositionTestSink sink;
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
                std::printf(
                    "Horizontal bidi positioning: FAIL: RTL primitive rejected input\n");

                return false;
            }

            if (sink.glyphs.size() != 2)
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: RTL glyph count\n"
                    "  Expected: 2\n"
                    "  Actual:   %zu\n",
                    sink.glyphs.size());

                return false;
            }

            // Logical emission order must remain unchanged.

            if (sink.glyphs[0].glyphId != 10 ||
                sink.glyphs[1].glyphId != 20)
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: RTL logical order\n");

                return false;
            }

            if (!nearlyEqual(sink.glyphs[0].x, 407.0) ||
                !nearlyEqual(sink.glyphs[0].y, 43.0) ||
                !nearlyEqual(sink.glyphs[1].x, 315.0) ||
                !nearlyEqual(sink.glyphs[1].y, 38.0))
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: RTL glyph positions\n"
                    "  Glyph 10: x=%.4f y=%.4f expected 407.0000 43.0000\n"
                    "  Glyph 20: x=%.4f y=%.4f expected 315.0000 38.0000\n",
                    sink.glyphs[0].x,
                    sink.glyphs[0].y,
                    sink.glyphs[1].x,
                    sink.glyphs[1].y);

                return false;
            }

            // originX was the right edge. Sum of advances is 180,
            // therefore final pen must be the left edge: 320.

            if (!nearlyEqual(result.penX, 320.0) ||
                !nearlyEqual(result.penY, 40.0) ||
                !nearlyEqual(result.scale, 1.0))
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: RTL final pen\n"
                    "  penX: %.4f expected 320.0000\n"
                    "  penY: %.4f expected 40.0000\n"
                    "  scale: %.8f expected 1.00000000\n",
                    result.penX,
                    result.penY,
                    result.scale);

                return false;
            }

            ++passed;
        }


        // ================================================================
        // Case 2
        //
        // Mixed LTR / RTL / LTR composition.
        //
        // origin = 100
        //
        // Run 0, LTR, width 120:
        //
        //     [100 ---------------- 220]
        //
        // Run 1, RTL, width 150:
        //
        //     [220 ---------------- 370]
        //
        //     glyph 20 advance 70 -> x = 300
        //     glyph 21 advance 80 -> x = 220
        //
        // Run 2, LTR, width 90:
        //
        //     [370 ---------------- 460]
        //
        // The important regression condition is that RTL glyph 20 must
        // begin at 300, not at the right edge 370.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer run0;
            ShapedGlyphBuffer run1;
            ShapedGlyphBuffer run2;

            run0.pushBack(makePositioningTestGlyph(10, 120));

            run1.pushBack(makePositioningTestGlyph(20, 70));
            run1.pushBack(makePositioningTestGlyph(21, 80));

            run2.pushBack(makePositioningTestGlyph(30, 90));

            std::vector<HorizontalBidiRunView> runs;

            runs.push_back({
                ShapedGlyphView(run0),
                0,
                1000
                });

            runs.push_back({
                ShapedGlyphView(run1),
                1,
                1000
                });

            runs.push_back({
                ShapedGlyphView(run2),
                0,
                1000
                });

            HorizontalBidiTestSink sink;
            HorizontalBidiPositioningResult result{};

            if (!positionHorizontalBidiRuns(
                runs,
                1000.0,
                100.0,
                50.0,
                sink,
                &result))
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: mixed compositor rejected input\n");

                return false;
            }

            if (sink.glyphs.size() != 4)
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: mixed glyph count\n"
                    "  Expected: 4\n"
                    "  Actual:   %zu\n",
                    sink.glyphs.size());

                return false;
            }

            // Visual run order is run 0, run 1, run 2. Within the RTL
            // run the glyph sequence still remains logical: 20, then 21.

            const HorizontalBidiTestGlyph& g0 = sink.glyphs[0];
            const HorizontalBidiTestGlyph& g1 = sink.glyphs[1];
            const HorizontalBidiTestGlyph& g2 = sink.glyphs[2];
            const HorizontalBidiTestGlyph& g3 = sink.glyphs[3];

            if (g0.runIndex != 0 || g0.glyphId != 10 ||
                g1.runIndex != 1 || g1.glyphId != 20 ||
                g2.runIndex != 1 || g2.glyphId != 21 ||
                g3.runIndex != 2 || g3.glyphId != 30)
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: mixed glyph/run order\n");

                return false;
            }

            if (!nearlyEqual(g0.x, 100.0) ||
                !nearlyEqual(g1.x, 300.0) ||
                !nearlyEqual(g2.x, 220.0) ||
                !nearlyEqual(g3.x, 370.0))
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: mixed positions\n"
                    "  run 0 glyph 10: %.4f expected 100.0000\n"
                    "  run 1 glyph 20: %.4f expected 300.0000\n"
                    "  run 1 glyph 21: %.4f expected 220.0000\n"
                    "  run 2 glyph 30: %.4f expected 370.0000\n",
                    g0.x,
                    g1.x,
                    g2.x,
                    g3.x);

                return false;
            }

            if (!nearlyEqual(result.penX, 460.0) ||
                !nearlyEqual(result.penY, 50.0) ||
                !nearlyEqual(result.width, 360.0) ||
                result.runCount != 3)
            {
                std::printf(
                    "Horizontal bidi positioning: FAIL: mixed final metrics\n"
                    "  penX: %.4f expected 460.0000\n"
                    "  width: %.4f expected 360.0000\n"
                    "  runs: %zu expected 3\n",
                    result.penX,
                    result.width,
                    result.runCount);

                return false;
            }

            ++passed;
        }


        // ================================================================
        // Summary
        // ================================================================

        std::printf(
            "Horizontal bidi positioning: PASS\n"
            "  Cases:                  %zu\n"
            "  Passed:                 %zu\n"
            "  RTL consume-before-emit: PASS\n"
            "  RTL logical order:       PASS\n"
            "  RTL final-left-edge:     PASS\n"
            "  Mixed run boxes:         PASS\n"
            "  Mixed final pen:         PASS\n",
            cases,
            passed);

        return passed == cases;
    }
}