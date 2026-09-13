// opentype_nominal_glyphs.h

#pragma once

#include "opentype_shaping_buffer.h"


namespace waavs
{
    [[nodiscard]]
    static inline bool mapOpenTypeNominalGlyphs(const FontRunView& input, OpenTypeShapingBuffer& output)
    {
        output.clear();

        if (!input.face)
            return false;

        if (input.scalarCount != 0 && !input.scalars)
            return false;

        output.reset(input);

        for (uint32_t i = 0; i < input.scalarCount; ++i)
        {
            OpenTypeShapingGlyph glyph{};

            glyph.glyphId = input.face.glyphIndex(input.scalars[i].value);
            glyph.scalarOffset = i;
            glyph.scalarCount = 1;

            output.pushBack(glyph);
        }

        return true;
    }

} // namespace waavs

