// glyph_positioning.h
#pragma once

#include <cmath>
#include <cstdint>

#include "font_run.h"
#include "shaped_glyph_view.h"

namespace waavs
{
    // ====================================================================
    // HorizontalGlyphPositioningResult
    //
    // Positions are in backend-neutral layout units.
    //
    // Coordinate convention at this layer:
    //
    //     +X -> right
    //     +Y -> up
    //
    // SVG or another backend is responsible for mapping this coordinate
    // system into its own presentation coordinate system.
    // ====================================================================

    struct HorizontalGlyphPositioningResult
    {
        double penX{ 0.0 };
        double penY{ 0.0 };
        double scale{ 0.0 };
    };


    // ====================================================================
    // positionHorizontalLTRGlyphRun
    //
    // Convert shaped placement from font design units into absolute layout
    // positions.
    //
    // Sink interface:
    //
    //     bool onGlyph(const ShapedGlyph& glyph,
    //         double x, double y, double scale);
    //
    // The ShapedGlyph itself remains unchanged and remains in design units.
    //
    // scale:
    //
    //     fontSize / unitsPerEm
    //
    // For each glyph:
    //
    //     x = penX + offsetX * scale
    //     y = penY + offsetY * scale
    //
    // followed by:
    //
    //     penX += advanceX * scale
    //     penY += advanceY * scale
    //
    // This function performs no bidi reordering.
    // ====================================================================

    template<class Sink>
    [[nodiscard]]
    static bool positionHorizontalLTRGlyphRun(const ShapedGlyphView& glyphs,
        double fontSize, uint16_t unitsPerEm, double originX, double originY,
        Sink& sink, HorizontalGlyphPositioningResult* result = nullptr)
    {
        if (!(fontSize > 0.0) || !std::isfinite(fontSize) || unitsPerEm == 0)
            return false;

        if (!std::isfinite(originX) || !std::isfinite(originY))
            return false;

        if (glyphs.size() != 0 && !glyphs.data())
            return false;

        const double scale = fontSize / static_cast<double>(unitsPerEm);

        if (!(scale > 0.0) || !std::isfinite(scale))
            return false;

        double penX = originX;
        double penY = originY;

        for (const ShapedGlyph& glyph : glyphs)
        {
            const GlyphPlacement& placement = glyph.placement;

            const double x = penX + static_cast<double>(placement.offsetX) * scale;
            const double y = penY + static_cast<double>(placement.offsetY) * scale;

            if (!std::isfinite(x) || !std::isfinite(y))
                return false;

            if (!sink.onGlyph(glyph, x, y, scale))
                return false;

            penX += static_cast<double>(placement.advanceX) * scale;
            penY += static_cast<double>(placement.advanceY) * scale;

            if (!std::isfinite(penX) || !std::isfinite(penY))
                return false;
        }

        if (result)
        {
            result->penX = penX;
            result->penY = penY;
            result->scale = scale;
        }

        return true;
    }


    // ====================================================================
    // FontRunView convenience overload
    //
    // The face supplies unitsPerEm. This first positioning primitive is
    // deliberately LTR-only.
    // ====================================================================

    template<class Sink>
    [[nodiscard]]
    static bool positionHorizontalLTRGlyphRun(const FontRunView& run,
        const ShapedGlyphView& glyphs, double fontSize,
        double originX, double originY, Sink& sink,
        HorizontalGlyphPositioningResult* result = nullptr)
    {
        if (!run.face)
            return false;

        if ((run.bidiLevel & 1u) != 0)
            return false;

        return positionHorizontalLTRGlyphRun(
            glyphs,
            fontSize,
            run.face.unitsPerEm(),
            originX,
            originY,
            sink,
            result);
    }




    // ====================================================================
    // positionHorizontalRTLGlyphRun
    //
    // Convert shaped placement from font design units into absolute layout
    // positions for an RTL run.
    //
    // The glyph buffer remains in logical order.
    //
    // originX is the RIGHT edge of the visual run box.
    //
    // Horizontal advances remain positive logical magnitudes. Therefore,
    // before emitting each glyph, consume its advance toward decreasing X:
    //
    //     penX -= advanceX * scale
    //
    // then apply the glyph's GPOS placement offset:
    //
    //     x = penX + offsetX * scale
    //
    // After the complete run, penX is the LEFT edge of the run box.
    // ====================================================================

    template<class Sink>
    [[nodiscard]]
    static bool positionHorizontalRTLGlyphRun(const ShapedGlyphView& glyphs,
        double fontSize, uint16_t unitsPerEm, double originX, double originY,
        Sink& sink, HorizontalGlyphPositioningResult* result = nullptr)
    {
        if (!(fontSize > 0.0) || !std::isfinite(fontSize) || unitsPerEm == 0)
            return false;

        if (!std::isfinite(originX) || !std::isfinite(originY))
            return false;

        if (glyphs.size() != 0 && !glyphs.data())
            return false;

        const double scale = fontSize / static_cast<double>(unitsPerEm);

        if (!(scale > 0.0) || !std::isfinite(scale))
            return false;

        double penX = originX;
        double penY = originY;

        // Glyphs remain in logical order. RTL progression consumes each
        // positive horizontal advance toward decreasing X before emission.
        for (const ShapedGlyph& glyph : glyphs)
        {
            const GlyphPlacement& placement = glyph.placement;

            penX -= static_cast<double>(placement.advanceX) * scale;

            if (!std::isfinite(penX))
                return false;

            const double x = penX + static_cast<double>(placement.offsetX) * scale;
            const double y = penY + static_cast<double>(placement.offsetY) * scale;

            if (!std::isfinite(x) || !std::isfinite(y))
                return false;

            if (!sink.onGlyph(glyph, x, y, scale))
                return false;

            penY += static_cast<double>(placement.advanceY) * scale;

            if (!std::isfinite(penY))
                return false;
        }

        if (result)
        {
            result->penX = penX;
            result->penY = penY;
            result->scale = scale;
        }

        return true;
    }


    template<class Sink>
    [[nodiscard]]
    static bool positionHorizontalRTLGlyphRun(const FontRunView& run,
        const ShapedGlyphView& glyphs, double fontSize,
        double originX, double originY, Sink& sink,
        HorizontalGlyphPositioningResult* result = nullptr)
    {
        if (!run.face)
            return false;

        if ((run.bidiLevel & 1u) == 0)
            return false;

        return positionHorizontalRTLGlyphRun(
            glyphs,
            fontSize,
            run.face.unitsPerEm(),
            originX,
            originY,
            sink,
            result);
    }
} // namespace waavs