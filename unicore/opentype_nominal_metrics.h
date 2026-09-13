// opentype_nominal_metrics.h
#pragma once

#include <utility>

#include "opentype_shaping_buffer.h"
#include "shaped_glyph_buffer.h"
#include "opentype_face_tables.h"
#include "opentype_hmtx_view.h"
#include "opentype_hhea_view.h"

namespace waavs
{
    // ====================================================================
    // buildOpenTypeHorizontalShapedGlyphs
    //
    // Convert the final post-GSUB glyph sequence into the initial shaped
    // glyph sequence using nominal horizontal metrics.
    //
    // Glyph identity and provenance are copied unchanged.
    //
    // Initial placement:
    //
    //     advanceX = hmtx advanceWidth
    //     advanceY = 0
    //     offsetX  = 0
    //     offsetY  = 0
    //
    // leftSideBearing is deliberately not converted into offsetX. It
    // describes glyph-outline geometry relative to the glyph origin rather
    // than an additional shaping placement adjustment.
    //
    // The operation is transactional. On failure, output is unchanged.
    // ====================================================================

    [[nodiscard]]
    static inline bool buildOpenTypeHorizontalShapedGlyphs(
        const OpenTypeShapingBuffer& input, const OpenTypeHmtxView& hmtx,
        ShapedGlyphBuffer& output)
    {
        if (!hmtx)
            return false;

        ShapedGlyphBuffer working;
        working.reserve(input.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            const OpenTypeShapingGlyph& inputGlyph = input[i];

            OpenTypeHorizontalMetric metric;

            if (!hmtx.metric(inputGlyph.glyphId, metric))
                return false;

            ShapedGlyph shaped{};
            shaped.shaping = inputGlyph;
            shaped.placement.advanceX = static_cast<int32_t>(metric.advanceWidth);

            working.pushBack(shaped);
        }

        output = std::move(working);
        return true;
    }

    // ====================================================================
// buildOpenTypeHorizontalShapedGlyphs
//
// Integrated horizontal nominal-metrics path.
//
// The FontFace comes from the original FontRunView retained by the
// OpenTypeShapingBuffer. The face supplies hhea/hmtx and glyphCount.
//
// This overload performs only table discovery and view construction,
// then delegates to the lower-level tested conversion routine.
// ====================================================================

    [[nodiscard]]
    static inline bool buildOpenTypeHorizontalShapedGlyphs(
        const OpenTypeShapingBuffer& input, ShapedGlyphBuffer& output)
    {
        const FontRunView* run = input.input();

        if (!run || !run->face)
            return false;

        const IProvideOpenTypeTables* tables =
            openTypeTableProvider(run->face);

        if (!tables)
            return false;

        const TableRecord* hheaTable = tables->getTable(OTAG("hhea"));
        const TableRecord* hmtxTable = tables->getTable(OTAG("hmtx"));

        if (!hheaTable || !hmtxTable)
            return false;

        const OpenTypeHheaView hhea(hheaTable->data);

        if (!hhea)
            return false;

        const OpenTypeHmtxView hmtx(
            hmtxTable->data,
            run->face->glyphCount(),
            hhea.numberOfHMetrics());

        if (!hmtx)
            return false;

        return buildOpenTypeHorizontalShapedGlyphs(input, hmtx, output);
    }
} // namespace waavs