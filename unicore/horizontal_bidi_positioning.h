// horizontal_bidi_positioning.h
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "glyph_positioning.h"
#include "unicode_bidi_visual_order.h"

namespace waavs
{
    // ====================================================================
    // HorizontalBidiRunView
    //
    // One already-shaped logical run.
    //
    // glyphs remain in logical order.
    // bidiLevel determines positioning direction.
    // unitsPerEm belongs to the font which produced the glyphs.
    // ====================================================================

    struct HorizontalBidiRunView
    {
        ShapedGlyphView glyphs{};
        UnicodeBidiLevel bidiLevel{ 0 };
        uint16_t unitsPerEm{ 0 };

        [[nodiscard]] bool rightToLeft() const noexcept
        {
            return (bidiLevel & 1u) != 0;
        }

        [[nodiscard]] bool leftToRight() const noexcept
        {
            return !rightToLeft();
        }
    };


    // ====================================================================
    // HorizontalBidiPositioningResult
    // ====================================================================

    struct HorizontalBidiPositioningResult
    {
        double penX{ 0.0 };
        double penY{ 0.0 };
        double width{ 0.0 };
        size_t runCount{ 0 };
    };


    // ====================================================================
    // measureHorizontalBidiRun
    //
    // Return the visual advance width of one shaped run.
    //
    // Offsets do not contribute to advance width.
    // ====================================================================

    [[nodiscard]]
    static bool measureHorizontalBidiRun(const HorizontalBidiRunView& run,
        double fontSize, double& width) noexcept
    {
        width = 0.0;

        if (!(fontSize > 0.0) || !std::isfinite(fontSize) || run.unitsPerEm == 0)
            return false;

        if (run.bidiLevel > kUnicodeBidiMaxDepth)
            return false;

        if (run.glyphs.size() != 0 && !run.glyphs.data())
            return false;

        const double scale = fontSize / static_cast<double>(run.unitsPerEm);

        if (!(scale > 0.0) || !std::isfinite(scale))
            return false;

        for (const ShapedGlyph& glyph : run.glyphs)
        {
            width += static_cast<double>(glyph.placement.advanceX) * scale;

            if (!std::isfinite(width))
                return false;
        }

        // Horizontal advance is a positive logical magnitude.
        if (width < 0.0)
            return false;

        return true;
    }


    // ====================================================================
    // HorizontalBidiRunSink
    //
    // Adapt the ordinary glyph-positioning sink contract:
    //
    //     onGlyph(glyph, x, y, scale)
    //
    // into the paragraph-level contract:
    //
    //     onGlyph(logicalRunIndex, glyph, x, y, scale)
    // ====================================================================

    template<class Sink>
    class HorizontalBidiRunSink
    {
    public:
        HorizontalBidiRunSink(size_t logicalRunIndex, Sink& sink) noexcept
            : mLogicalRunIndex(logicalRunIndex)
            , mSink(&sink)
        {}

        bool onGlyph(const ShapedGlyph& glyph, double x, double y, double scale)
        {
            return mSink &&
                mSink->onGlyph(mLogicalRunIndex, glyph, x, y, scale);
        }

    private:
        size_t mLogicalRunIndex{ 0 };
        Sink* mSink{ nullptr };
    };


    // ====================================================================
    // positionHorizontalBidiRuns
    //
    // runs are supplied in LOGICAL order.
    //
    // Processing:
    //
    //     logical runs
    //         -> UAX #9 L2 visual run order
    //         -> assign each run a left-to-right visual box
    //         -> LTR run starts at box left
    //         -> RTL run starts at box right
    //         -> position glyphs without reversing the glyph buffer
    //
    // originX is currently the visual left edge of the complete line.
    //
    // This is deliberately line composition, not text anchoring. RTL
    // paragraph anchoring can be layered above this later.
    // ====================================================================

    template<class Sink>
    [[nodiscard]]
    static bool positionHorizontalBidiRuns(const HorizontalBidiRunView* runs,
        size_t runCount, double fontSize, double originX, double originY,
        Sink& sink, HorizontalBidiPositioningResult* result = nullptr)
    {
        if (!(fontSize > 0.0) || !std::isfinite(fontSize))
            return false;

        if (!std::isfinite(originX) || !std::isfinite(originY))
            return false;

        if (runCount != 0 && !runs)
            return false;


        // ---------------------------------------------------------------
        // Empty line.
        // ---------------------------------------------------------------

        if (runCount == 0)
        {
            if (result)
            {
                result->penX = originX;
                result->penY = originY;
                result->width = 0.0;
                result->runCount = 0;
            }

            return true;
        }


        // ---------------------------------------------------------------
        // Collect levels and visual widths in logical-run order.
        // ---------------------------------------------------------------

        std::vector<UnicodeBidiLevel> levels;
        std::vector<double> widths;

        levels.reserve(runCount);
        widths.resize(runCount);

        double totalWidth = 0.0;

        for (size_t i = 0; i < runCount; ++i)
        {
            const HorizontalBidiRunView& run = runs[i];

            if (run.bidiLevel > kUnicodeBidiMaxDepth)
                return false;

            levels.push_back(run.bidiLevel);

            if (!measureHorizontalBidiRun(run, fontSize, widths[i]))
                return false;

            totalWidth += widths[i];

            if (!std::isfinite(totalWidth))
                return false;
        }


        // ---------------------------------------------------------------
        // Logical run indices in visual left-to-right order.
        // ---------------------------------------------------------------

        std::vector<size_t> visualOrder;

        if (!makeBidiVisualRunOrder(levels, visualOrder))
            return false;

        if (visualOrder.size() != runCount)
            return false;


        // ---------------------------------------------------------------
        // Place visual run boxes from left to right.
        // ---------------------------------------------------------------

        double visualPenX = originX;

        for (size_t visualIndex = 0; visualIndex < runCount; ++visualIndex)
        {
            const size_t logicalRunIndex = visualOrder[visualIndex];

            if (logicalRunIndex >= runCount)
                return false;

            const HorizontalBidiRunView& run = runs[logicalRunIndex];
            const double width = widths[logicalRunIndex];

            const double leftX = visualPenX;
            const double rightX = leftX + width;

            if (!std::isfinite(rightX))
                return false;

            HorizontalBidiRunSink<Sink> runSink(logicalRunIndex, sink);
            HorizontalGlyphPositioningResult runResult{};


            // -----------------------------------------------------------
            // The visual box always progresses left-to-right.
            //
            // LTR:
            //
            //     leftX -> glyphs ->
            //
            // RTL:
            //
            //     <- glyphs <- rightX
            // -----------------------------------------------------------

            if (run.rightToLeft())
            {
                if (!positionHorizontalRTLGlyphRun(
                    run.glyphs,
                    fontSize,
                    run.unitsPerEm,
                    rightX,
                    originY,
                    runSink,
                    &runResult))
                {
                    return false;
                }
            }
            else
            {
                if (!positionHorizontalLTRGlyphRun(
                    run.glyphs,
                    fontSize,
                    run.unitsPerEm,
                    leftX,
                    originY,
                    runSink,
                    &runResult))
                {
                    return false;
                }
            }


            // The paragraph visual pen always advances to the right edge
            // of the run box, regardless of that run's logical direction.

            visualPenX = rightX;
        }


        if (result)
        {
            result->penX = visualPenX;
            result->penY = originY;
            result->width = totalWidth;
            result->runCount = runCount;
        }

        return true;
    }


    template<class Sink>
    [[nodiscard]]
    static bool positionHorizontalBidiRuns(const std::vector<HorizontalBidiRunView>& runs,
        double fontSize, double originX, double originY, Sink& sink,
        HorizontalBidiPositioningResult* result = nullptr)
    {
        return positionHorizontalBidiRuns(
            runs.empty() ? nullptr : runs.data(),
            runs.size(),
            fontSize,
            originX,
            originY,
            sink,
            result);
    }

} // namespace waavs
