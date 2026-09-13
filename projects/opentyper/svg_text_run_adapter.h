// svg_text_run_adapter.h
#pragma once

#include <cmath>
#include <limits>

#include "glyph_positioning.h"
#include "svg_text_backend.h"

namespace waavs
{
    // ====================================================================
    // SVGPositionedGlyphSink
    //
    // Adapter from backend-neutral positioned glyphs to SVGTextBackend.
    //
    // Layout coordinates:
    //
    //     +X -> right
    //     +Y -> up
    //
    // SVG coordinates:
    //
    //     +X -> right
    //     +Y -> down
    //
    // The run baseline is the axis about which Y is reflected:
    //
    //     svgY = baselineY - (layoutY - baselineY)
    //
    // The glyph outline itself remains in font design units. SVGTextBackend
    // handles the outline Y inversion through scale(scale, -scale).
    // ====================================================================

    template<class OutlineSource>
    class SVGPositionedGlyphSink
    {
    public:
        SVGPositionedGlyphSink(const FontFace& face, const OutlineSource& outlines,
            SVGTextBackend& backend, double baselineY) noexcept
            : mFace(face)
            , mOutlines(&outlines)
            , mBackend(&backend)
            , mBaselineY(baselineY)
        {}

        bool onGlyph(const ShapedGlyph& glyph, double x, double y, double scale)
        {
            if (!mFace || !mOutlines || !mBackend)
                return false;

            const double svgY = mBaselineY - (y - mBaselineY);

            if (!std::isfinite(x) || !std::isfinite(svgY) || !std::isfinite(scale))
                return false;

            const double floatMax = static_cast<double>(std::numeric_limits<float>::max());

            if (std::fabs(x) > floatMax || std::fabs(svgY) > floatMax || scale > floatMax)
                return false;

            return mBackend->emitGlyph(
                mFace,
                *mOutlines,
                glyph.shaping.glyphId,
                static_cast<float>(x),
                static_cast<float>(svgY),
                static_cast<float>(scale));
        }

    private:
        FontFace mFace{};
        const OutlineSource* mOutlines{ nullptr };
        SVGTextBackend* mBackend{ nullptr };
        double mBaselineY{ 0.0 };
    };


    // ====================================================================
    // emitHorizontalLTRSVGRun
    //
    // Complete first composition-to-SVG bridge:
    //
    //     ShapedGlyphView
    //         -> design-unit scaling
    //         -> absolute pen positioning
    //         -> SVG coordinate mapping
    //         -> SVGTextBackend
    //
    // No shaping or bidi processing occurs here.
    // ====================================================================

    template<class OutlineSource>
    [[nodiscard]]
    static bool emitHorizontalLTRSVGRun(const FontRunView& run,
        const ShapedGlyphView& glyphs, const OutlineSource& outlines,
        double fontSize, double originX, double baselineY,
        SVGTextBackend& backend,
        HorizontalGlyphPositioningResult* result = nullptr)
    {
        if (!run.face)
            return false;

        SVGPositionedGlyphSink<OutlineSource> sink(
            run.face,
            outlines,
            backend,
            baselineY);

        return positionHorizontalLTRGlyphRun(
            run,
            glyphs,
            fontSize,
            originX,
            baselineY,
            sink,
            result);
    }

} // namespace waavs