// svg_text_backend.h
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "font_face.h"
#include "svg_document_builder.h"
#include "svg_glyph_definition_cache.h"
#include "svg_path_data_sink.h"

namespace waavs
{
    class SVGTextBackend
    {
    public:
        void clear()
        {
            mGlyphs.clear();
            mDocument.clear();
        }


        [[nodiscard]]
        size_t fontCount() const noexcept
        {
            return mGlyphs.fontCount();
        }


        [[nodiscard]]
        size_t glyphDefinitionCount() const noexcept
        {
            return mGlyphs.glyphCount();
        }


        SVGDocumentBuilder& documentBuilder() noexcept
        {
            return mDocument;
        }


        const SVGDocumentBuilder& documentBuilder() const noexcept
        {
            return mDocument;
        }


        const SVGGlyphDefinitionCache& glyphCache() const noexcept
        {
            return mGlyphs;
        }


        // ====================================================================
        // emitGlyph
        //
        // Emit one already-positioned glyph.
        //
        // face:
        //     Supplies glyph-definition identity.
        //
        // outlines:
        //     Supplies:
        //
        //         bool emitGlyphPath(uint32_t glyphId, Sink& sink) const;
        //
        // x, y:
        //     Absolute glyph origin in SVG user space.
        //
        // scale:
        //     Font-design-units to SVG-user-units scale.
        //
        // Glyph outlines remain in font design coordinates. addGlyphUse()
        // performs the SVG y-axis inversion through scale(scale, -scale).
        // ====================================================================

        template<class OutlineSource>
        bool emitGlyph(const FontFace& face, const OutlineSource& outlines,
            uint32_t glyphId, float x, float y, float scale)
        {
            if (!face || glyphId >= face.glyphCount())
                return false;

            if (!std::isfinite(x) || !std::isfinite(y) ||
                !std::isfinite(scale) || scale <= 0.0f)
            {
                return false;
            }


            SVGGlyphDefinition def;


            // ------------------------------------------------------------
            // Existing definition.
            // ------------------------------------------------------------

            if (mGlyphs.lookup(face, glyphId, def))
                return mDocument.addGlyphUse(def.id(), x, y, scale);


            // ------------------------------------------------------------
            // Establish deterministic document-local definition identity.
            //
            // prepare() assigns the font index but deliberately does not
            // mark the glyph definition as emitted.
            // ------------------------------------------------------------

            if (!mGlyphs.prepare(face, glyphId, def))
                return false;


            // ------------------------------------------------------------
            // Produce SVG path data directly from the outline source.
            //
            // If this fails, the glyph has not been committed to the cache.
            // ------------------------------------------------------------

            SVGPathDataSink sink;

            if (!outlines.emitGlyphPath(glyphId, sink))
                return false;


            // ------------------------------------------------------------
            // Emit the reusable definition.
            // ------------------------------------------------------------

            if (!mDocument.addPathDefinition(def.id(), sink.data))
                return false;


            // ------------------------------------------------------------
            // Only now is the definition known to exist in the document.
            // ------------------------------------------------------------

            if (!mGlyphs.commit(def))
                return false;


            // ------------------------------------------------------------
            // Emit this positioned instance.
            // ------------------------------------------------------------

            return mDocument.addGlyphUse(def.id(), x, y, scale);
        }


        // ====================================================================
        // document
        // ====================================================================

        [[nodiscard]]
        std::string document(float minX, float minY, float width, float height) const
        {
            return mDocument.document(minX, minY, width, height);
        }


    private:
        SVGGlyphDefinitionCache mGlyphs;
        SVGDocumentBuilder mDocument;
    };

} // namespace waavs