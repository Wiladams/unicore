// opentype_nominal_glyphs.h
#pragma once

#include "script_shaping_buffer.h"
#include "opentype_shaping_buffer.h"

namespace waavs
{
    // ========================================================================
    // mapOpenTypeNominalGlyphs
    //
    // Map the scalar-domain shaping sequence through the selected face cmap.
    //
    // The real FontRunView retained by ScriptShapingBuffer remains the source
    // of the selected face and is retained by OpenTypeShapingBuffer.
    //
    // Scalar provenance comes from ScriptShapingItem rather than being
    // reconstructed here.
    // ========================================================================

    [[nodiscard]]
    static inline bool mapOpenTypeNominalGlyphs(const ScriptShapingBuffer& input, OpenTypeShapingBuffer& output)
    {
        output.clear();

        const FontRunView* run = input.input();

        if (!run || !run->face)
            return false;

        output.reset(*run);

        for (const ScriptShapingItem& item : input)
        {
            OpenTypeShapingGlyph glyph{};

            glyph.glyphId = run->face.glyphIndex(item.value);
            glyph.scalarOffset = item.scalarOffset;
            glyph.scalarCount = item.scalarCount;

            output.pushBack(glyph);
        }

        return true;
    }


    // ========================================================================
    // FontRunView convenience overload
    //
    // Preserve the existing API while routing it through the new scalar-domain
    // shaping representation.
    //
    // Existing callers therefore remain behaviorally unchanged.
    // ========================================================================

    [[nodiscard]]
    static inline bool mapOpenTypeNominalGlyphs(const FontRunView& input, OpenTypeShapingBuffer& output)
    {
        ScriptShapingBuffer shapingInput;

        if (!shapingInput.reset(input))
            return false;

        return mapOpenTypeNominalGlyphs(shapingInput, output);
    }

} // namespace waavs