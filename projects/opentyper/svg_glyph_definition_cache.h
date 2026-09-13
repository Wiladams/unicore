// svg_glyph_definition_cache.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include "font_face.h"

namespace waavs
{
    struct SVGGlyphDefinition
    {
        uint32_t fontIndex{ 0 };
        uint32_t glyphId{ 0 };

        [[nodiscard]] std::string id() const
        {
            return "f" + std::to_string(fontIndex) + "g" + std::to_string(glyphId);
        }
    };


    class SVGGlyphDefinitionCache
    {
    public:
        void clear()
        {
            mFonts.clear();
            mGlyphs.clear();
        }

        [[nodiscard]] size_t fontCount() const noexcept
        {
            return mFonts.size();
        }

        [[nodiscard]] size_t glyphCount() const noexcept
        {
            return mGlyphs.size();
        }


        // ------------------------------------------------------------
        // lookup
        //
        // Find an already committed glyph definition.
        // Does not modify the cache.
        // ------------------------------------------------------------

        [[nodiscard]]
        bool lookup(const FontFace& face, uint32_t glyphId, SVGGlyphDefinition& out) const noexcept
        {
            uint32_t fontIndex = 0;

            if (!findFont(face, fontIndex))
                return false;

            if (mGlyphs.find(makeKey(fontIndex, glyphId)) == mGlyphs.end())
                return false;

            out.fontIndex = fontIndex;
            out.glyphId = glyphId;
            return true;
        }


        [[nodiscard]]
        bool contains(const FontFace& face, uint32_t glyphId) const noexcept
        {
            SVGGlyphDefinition def;
            return lookup(face, glyphId, def);
        }


        // ------------------------------------------------------------
        // prepare
        //
        // Establish the document-local font index and produce the SVG
        // definition identity.
        //
        // The glyph itself is NOT committed yet. This allows outline
        // decoding and SVG definition emission to fail without leaving
        // a false glyph-cache hit behind.
        // ------------------------------------------------------------

        bool prepare(const FontFace& face, uint32_t glyphId, SVGGlyphDefinition& out)
        {
            uint32_t fontIndex = 0;

            if (!internFont(face, fontIndex))
                return false;

            out.fontIndex = fontIndex;
            out.glyphId = glyphId;
            return true;
        }


        // ------------------------------------------------------------
        // commit
        //
        // Mark a successfully emitted SVG glyph definition as present.
        //
        // Returns false if the definition is invalid or was already
        // committed.
        // ------------------------------------------------------------

        bool commit(const SVGGlyphDefinition& def)
        {
            if (def.fontIndex >= mFonts.size())
                return false;

            return mGlyphs.insert(makeKey(def.fontIndex, def.glyphId)).second;
        }


        [[nodiscard]]
        const FontFace* font(uint32_t fontIndex) const noexcept
        {
            return fontIndex < mFonts.size() ? &mFonts[fontIndex] : nullptr;
        }


    private:
        std::vector<FontFace> mFonts;
        std::unordered_set<uint64_t> mGlyphs;


        static uint64_t makeKey(uint32_t fontIndex, uint32_t glyphId) noexcept
        {
            return (static_cast<uint64_t>(fontIndex) << 32) | glyphId;
        }


        [[nodiscard]]
        bool findFont(const FontFace& face, uint32_t& index) const noexcept
        {
            if (!face)
                return false;

            for (size_t i = 0; i < mFonts.size(); ++i)
            {
                if (mFonts[i] == face)
                {
                    index = static_cast<uint32_t>(i);
                    return true;
                }
            }

            return false;
        }


        bool internFont(const FontFace& face, uint32_t& index)
        {
            if (findFont(face, index))
                return true;

            if (!face || mFonts.size() >= std::numeric_limits<uint32_t>::max())
                return false;

            index = static_cast<uint32_t>(mFonts.size());
            mFonts.push_back(face);
            return true;
        }
    };

} // namespace waavs