// opentype_shaping_glyph.h
#pragma once

#include <cstdint>

namespace waavs
{
    using GlyphId = uint32_t;


    // ====================================================================
    // OpenTypeLigatureProvenance
    //
    // Transient GSUB/GPOS association metadata.
    //
    // id:
    //   Non-zero identity allocated when a base ligature is formed.
    //
    // component:
    //   For a mark associated with a ligature, one-based component index.
    //   Zero means the glyph itself is the ligature base or has no component
    //   association.
    //
    // componentCount:
    //   Number of logical components represented by a ligature base.
    //   Zero for normal glyphs and associated marks.
    //
    // This is not source provenance. scalarOffset/scalarCount remain the
    // logical source extent carried to downstream consumers.
    // ====================================================================

    struct OpenTypeLigatureProvenance
    {
        uint32_t id{ 0 };
        uint16_t component{ 0 };
        uint16_t componentCount{ 0 };

        void clear() noexcept
        {
            id = 0;
            component = 0;
            componentCount = 0;
        }

        [[nodiscard]] bool associated() const noexcept
        {
            return id != 0 && component != 0;
        }

        [[nodiscard]] bool ligatureBase() const noexcept
        {
            return id != 0 && component == 0 && componentCount != 0;
        }

        [[nodiscard]] uint16_t effectiveComponentCount() const noexcept
        {
            return componentCount != 0 ? componentCount : 1;
        }
    };


    struct OpenTypeShapingGlyph
    {
        GlyphId glyphId{ 0 };

        uint32_t scalarOffset{ 0 };
        uint32_t scalarCount{ 0 };

        OpenTypeLigatureProvenance ligature{};
    };

} // namespace waavs
