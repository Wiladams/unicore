// opentype_shaping_glyph.h

#pragma once

#include <cstdint>


namespace waavs
{
    using GlyphId = uint32_t;


    struct OpenTypeShapingGlyph
    {
        GlyphId glyphId{ 0 };

        uint32_t scalarOffset{ 0 };
        uint32_t scalarCount{ 0 };
    };

} // namespace waavs
