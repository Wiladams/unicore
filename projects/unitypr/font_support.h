// font_support.h

#pragma once

#include "font_face.h"
#include "unicode_database.h"
#include "unicode_shaping_run.h"


namespace waavs
{
    // ========================================================================
    // fontFallbackScalarRequiresGlyph
    //
    // Return true when font fallback requires a nominal glyph for cp.
    //
    // This is deliberately separate from FontFace::hasGlyph() because some
    // Unicode shaping/control scalars participate in text processing without
    // requiring an independently encoded glyph.
    //
    // This implementation will become database-driven when
    // Default_Ignorable_Code_Point is available in UnicodeDatabase.
    // ========================================================================

    [[nodiscard]]
    static inline bool fontFallbackScalarRequiresGlyph(uint32_t cp, const UnicodeDatabase& database) noexcept
    {
        return !database.isDefaultIgnorableCodePoint(cp);
    }


    // ========================================================================
    // fontSupportsCluster
    //
    // A grapheme cluster is the atomic unit of font fallback.
    //
    // Every scalar requiring a nominal glyph must map to a non-zero glyph in
    // the selected face.
    //
    // Scalars such as join controls and variation selectors may participate
    // in shaping without requiring their own nominal glyph.
    // ========================================================================

    [[nodiscard]]
    static inline bool fontSupportsCluster(const FontFace& face,
        const ShapingRunView& run, const ShapingCluster& cluster,
        const UnicodeDatabase& database) noexcept
    {
        if (!face || !run.scalars || cluster.scalarCount == 0)
            return false;

        if (cluster.scalarOffset > run.scalarCount)
            return false;

        if (cluster.scalarCount > run.scalarCount - cluster.scalarOffset)
            return false;

        for (uint32_t i = 0; i < cluster.scalarCount; ++i)
        {
            const uint32_t cp =
                run.scalars[cluster.scalarOffset + i].value;

            if (!fontFallbackScalarRequiresGlyph(cp, database))
                continue;

            if (!face.hasGlyph(cp))
                return false;
        }

        return true;
    }

} // namespace waavs
