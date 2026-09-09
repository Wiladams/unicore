// shaped_glyph.h
#pragma once

#include "glyph_placement.h"
#include "opentype_shaping_glyph.h"

namespace waavs
{
    // ====================================================================
    // ShapedGlyph
    //
    // Final glyph identity/provenance paired with relative placement.
    //
    // shaping:
    //   Glyph identity and source provenance established by cmap/GSUB.
    //
    // placement:
    //   Nominal metrics followed by GPOS/kern adjustments.
    //
    // Later layout stages may consume this without modifying it.
    // ====================================================================

    struct ShapedGlyph
    {
        OpenTypeShapingGlyph shaping{};
        GlyphPlacement placement{};
    };

} // namespace waavs