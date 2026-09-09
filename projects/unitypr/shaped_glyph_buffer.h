// shaped_glyph_buffer.h
#pragma once

#include <cstddef>
#include <vector>

#include "shaped_glyph.h"

namespace waavs
{
    // ====================================================================
    // ShapedGlyphBuffer
    //
    // Owning mutable glyph sequence after GSUB.
    //
    // Nominal metrics initialize placement.
    // GPOS or kern may subsequently adjust placement.
    //
    // Glyph identity and provenance remain available in ShapedGlyph::shaping.
    //
    // Glyph count and identity are normally stable once this buffer has been
    // constructed from the completed GSUB result.
    // ====================================================================

    class ShapedGlyphBuffer
    {
    public:
        void clear() noexcept
        {
            mGlyphs.clear();
        }

        void reserve(size_t count)
        {
            mGlyphs.reserve(count);
        }

        [[nodiscard]] size_t size() const noexcept { return mGlyphs.size(); }
        [[nodiscard]] bool empty() const noexcept { return mGlyphs.empty(); }

        [[nodiscard]] ShapedGlyph* data() noexcept { return mGlyphs.data(); }
        [[nodiscard]] const ShapedGlyph* data() const noexcept { return mGlyphs.data(); }

        ShapedGlyph& operator[](size_t index) noexcept { return mGlyphs[index]; }
        const ShapedGlyph& operator[](size_t index) const noexcept { return mGlyphs[index]; }

        auto begin() noexcept { return mGlyphs.begin(); }
        auto end() noexcept { return mGlyphs.end(); }
        auto begin() const noexcept { return mGlyphs.begin(); }
        auto end() const noexcept { return mGlyphs.end(); }

        void pushBack(const ShapedGlyph& glyph)
        {
            mGlyphs.push_back(glyph);
        }

        std::vector<ShapedGlyph>& glyphs() noexcept { return mGlyphs; }
        const std::vector<ShapedGlyph>& glyphs() const noexcept { return mGlyphs; }

    private:
        std::vector<ShapedGlyph> mGlyphs{};
    };

} // namespace waavs
