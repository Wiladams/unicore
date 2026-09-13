// test_glyph_positioning_rtl.h
#pragma once

#include "test_core.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "glyph_positioning.h"

namespace waavs
{
    struct RTLGlyphPositionRecord
    {
        GlyphId glyphId{ 0 };
        double x{ 0.0 };
        double y{ 0.0 };
        double scale{ 0.0 };
    };


    class RTLGlyphPositionSink
    {
    public:
        bool onGlyph(const ShapedGlyph& glyph, double x, double y, double scale)
        {
            RTLGlyphPositionRecord record;
            record.glyphId = glyph.shaping.glyphId;
            record.x = x;
            record.y = y;
            record.scale = scale;

            records.push_back(record);
            return true;
        }

        std::vector<RTLGlyphPositionRecord> records;
    };


    static bool rtlPositionNearlyEqual(double a, double b) noexcept
    {
        return std::fabs(a - b) <= 1.0e-9;
    }


    static bool testGlyphPositioningRTL()
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "RTL glyph positioning: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Logical glyph sequence
        //
        // Glyphs remain:
        //
        //     10, 20, 30
        //
        // They are NOT reversed for RTL.
        // ================================================================

        ShapedGlyphBuffer buffer;

        {
            ShapedGlyph glyph;
            glyph.shaping.glyphId = 10;
            glyph.placement.advanceX = 600;
            glyph.placement.offsetX = 10;
            glyph.placement.offsetY = 20;
            buffer.pushBack(glyph);
        }

        {
            ShapedGlyph glyph;
            glyph.shaping.glyphId = 20;
            glyph.placement.advanceX = 500;
            glyph.placement.offsetX = -20;
            glyph.placement.offsetY = -10;
            buffer.pushBack(glyph);
        }

        {
            ShapedGlyph glyph;
            glyph.shaping.glyphId = 30;
            glyph.placement.advanceX = 400;
            glyph.placement.offsetX = 5;
            glyph.placement.offsetY = 0;
            buffer.pushBack(glyph);
        }


        // ================================================================
        // 100 / 1000 = 0.1
        //
        // Initial logical origin:
        //
        //     x = 300
        //
        // Expected logical origins:
        //
        //     glyph 10 origin = 300
        //     glyph 20 origin = 240
        //     glyph 30 origin = 190
        //
        // Expected final pen:
        //
        //     300 - 60 - 50 - 40 = 150
        // ================================================================

        constexpr double fontSize = 100.0;
        constexpr uint16_t unitsPerEm = 1000;
        constexpr double originX = 300.0;
        constexpr double originY = 50.0;

        ShapedGlyphView view(buffer);
        RTLGlyphPositionSink sink;
        HorizontalGlyphPositioningResult result;

        if (!positionHorizontalRTLGlyphRun(
            view,
            fontSize,
            unitsPerEm,
            originX,
            originY,
            sink,
            &result))
        {
            return fail("positioning failed");
        }


        // ================================================================
        // Buffer order must remain logical.
        // ================================================================

        if (sink.records.size() != 3)
            return fail("unexpected emitted glyph count");

        if (sink.records[0].glyphId != 10 ||
            sink.records[1].glyphId != 20 ||
            sink.records[2].glyphId != 30)
        {
            return fail("glyph sequence was reversed");
        }


        // ================================================================
        // Scale.
        // ================================================================

        if (!rtlPositionNearlyEqual(result.scale, 0.1))
            return fail("incorrect scale");


        // ================================================================
        // Glyph 10
        //
        // x = 300 + 10 * 0.1 = 301
        // y =  50 + 20 * 0.1 =  52
        // ================================================================

        if (!rtlPositionNearlyEqual(sink.records[0].x, 301.0) ||
            !rtlPositionNearlyEqual(sink.records[0].y, 52.0))
        {
            return fail("glyph 10 position");
        }


        // ================================================================
        // Glyph 20
        //
        // pen = 300 - 600 * 0.1 = 240
        //
        // x = 240 - 20 * 0.1 = 238
        // y =  50 - 10 * 0.1 =  49
        // ================================================================

        if (!rtlPositionNearlyEqual(sink.records[1].x, 238.0) ||
            !rtlPositionNearlyEqual(sink.records[1].y, 49.0))
        {
            return fail("glyph 20 position");
        }


        // ================================================================
        // Glyph 30
        //
        // pen = 240 - 500 * 0.1 = 190
        //
        // x = 190 + 5 * 0.1 = 190.5
        // y =  50
        // ================================================================

        if (!rtlPositionNearlyEqual(sink.records[2].x, 190.5) ||
            !rtlPositionNearlyEqual(sink.records[2].y, 50.0))
        {
            return fail("glyph 30 position");
        }


        // ================================================================
        // Final pen.
        // ================================================================

        if (!rtlPositionNearlyEqual(result.penX, 150.0) ||
            !rtlPositionNearlyEqual(result.penY, 50.0))
        {
            return fail("incorrect final pen");
        }


        std::printf(
            "RTL glyph positioning: PASS\n"
            "  Glyphs:      %zu\n"
            "  Scale:       %.3f\n"
            "  Origin X:    %.3f\n"
            "  Glyph 10 X:  %.3f\n"
            "  Glyph 20 X:  %.3f\n"
            "  Glyph 30 X:  %.3f\n"
            "  Final pen X: %.3f\n",
            sink.records.size(),
            result.scale,
            originX,
            sink.records[0].x,
            sink.records[1].x,
            sink.records[2].x,
            result.penX);

        return true;
    }

} // namespace waavs