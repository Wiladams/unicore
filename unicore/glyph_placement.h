// glyph_placement.h
#pragma once

#include <cstdint>

namespace waavs
{
    // ====================================================================
    // GlyphPlacement
    //
    // Relative glyph placement produced by shaping.
    //
    // advanceX/advanceY move the text cursor after the glyph.
    // offsetX/offsetY move the glyph relative to that cursor position.
    //
    // Values remain in font design units through shaping. Scaling into
    // device/user coordinates belongs to later layout/rendering stages.
    // ====================================================================

    struct GlyphPlacement
    {
        int32_t advanceX{ 0 };
        int32_t advanceY{ 0 };
        int32_t offsetX{ 0 };
        int32_t offsetY{ 0 };
    };

} // namespace waavs