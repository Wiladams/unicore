// opentype_gsub_single_apply.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_gsub_single_view.h"
#include "opentype_shaping_buffer.h"


namespace waavs
{
    // ====================================================================
    // applyOpenTypeGsubSingleSubtableAt
    //
    // Apply one GSUB LookupType 1 subtable to one glyph-buffer position.
    //
    // Returns:
    //
    //   false -> malformed input / invalid buffer position
    //   true  -> successfully processed
    //
    // A successfully processed glyph does not necessarily match Coverage.
    //
    // Only glyphId may change. Provenance remains untouched.
    // ====================================================================

    static inline bool applyOpenTypeGsubSingleSubtableAt(const OpenTypeGsubSingleSubstView& single,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex) noexcept
    {
        if (!single || glyphIndex >= buffer.size())
            return false;

        OpenTypeShapingGlyph& glyph = buffer[glyphIndex];

        if (glyph.glyphId > 0xFFFFu)
            return false;

        const OpenTypeCoverageView coverage = single.coverage();

        if (!coverage)
            return false;

        uint16_t coverageIndex = 0;

        if (!coverage.find(glyph.glyphId, coverageIndex))
            return true;


        uint16_t replacement = 0;

        if (single.format() == 1)
        {
            int32_t delta = 0;

            if (!single.deltaGlyphId(delta))
                return false;

            const uint16_t rawDelta = static_cast<uint16_t>(delta);

            replacement = static_cast<uint16_t>(
                static_cast<uint32_t>(glyph.glyphId) +
                static_cast<uint32_t>(rawDelta));
        }
        else if (single.format() == 2)
        {
            if (!single.substituteGlyphId(coverageIndex, replacement))
                return false;
        }
        else
        {
            return false;
        }


        glyph.glyphId = replacement;

        return true;
    }


    // ====================================================================
    // ByteSpan convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGsubSingleSubtableAt(ByteSpan subtableData,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex) noexcept
    {
        const OpenTypeGsubSingleSubstView single(subtableData);

        if (!single)
            return false;

        return applyOpenTypeGsubSingleSubtableAt(single, buffer, glyphIndex);
    }


    // ====================================================================
    // applyOpenTypeGsubSingleSubtable
    //
    // Apply a SingleSubst subtable to every glyph in the current buffer.
    //
    // This is sufficient for the initial LookupType 1 implementation.
    //
    // Later the general lookup executor will call the position-based
    // primitive so LookupFlags/GDEF can decide which glyphs are eligible.
    // ====================================================================

    static inline bool applyOpenTypeGsubSingleSubtable(ByteSpan subtableData,
        OpenTypeShapingBuffer& buffer) noexcept
    {
        const OpenTypeGsubSingleSubstView single(subtableData);

        if (!single)
            return false;

        const OpenTypeCoverageView coverage = single.coverage();

        if (!coverage)
            return false;


        // ------------------------------------------------------------
        // Preflight Format 2.
        //
        // Make sure every covered glyph maps to a valid substitute array
        // entry before changing any glyphs.
        // ------------------------------------------------------------

        if (single.format() == 2)
        {
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                const OpenTypeShapingGlyph& glyph = buffer[i];

                if (glyph.glyphId > 0xFFFFu)
                    return false;

                uint16_t coverageIndex = 0;

                if (!coverage.find(glyph.glyphId, coverageIndex))
                    continue;

                uint16_t replacement = 0;

                if (!single.substituteGlyphId(coverageIndex, replacement))
                    return false;
            }
        }


        // ------------------------------------------------------------
        // Apply.
        // ------------------------------------------------------------

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            if (!applyOpenTypeGsubSingleSubtableAt(single, buffer, i))
                return false;
        }

        return true;
    }

} // namespace waavs