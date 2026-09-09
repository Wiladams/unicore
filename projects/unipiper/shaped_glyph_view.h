// shaped_glyph_view.h
#pragma once

#include <cstddef>

#include "shaped_glyph_buffer.h"

namespace waavs
{
    // ====================================================================
    // ShapedGlyphView
    //
    // Borrowed read-only view of a shaped glyph sequence.
    //
    // Ownership remains with the ShapedGlyphBuffer or other backing store.
    // Later layout and rendering stages may consume this view without
    // modifying the underlying shaping result.
    // ====================================================================

    class ShapedGlyphView
    {
    public:
        ShapedGlyphView() noexcept = default;

        ShapedGlyphView(const ShapedGlyph* glyphs, size_t glyphCount) noexcept
            : mGlyphs(glyphs)
            , mGlyphCount(glyphCount)
        {}

        explicit ShapedGlyphView(const ShapedGlyphBuffer& buffer) noexcept
            : mGlyphs(buffer.data())
            , mGlyphCount(buffer.size())
        {}

        [[nodiscard]] size_t size() const noexcept { return mGlyphCount; }
        [[nodiscard]] bool empty() const noexcept { return mGlyphCount == 0; }

        explicit operator bool() const noexcept { return !empty(); }

        [[nodiscard]] const ShapedGlyph* data() const noexcept { return mGlyphs; }

        const ShapedGlyph& operator[](size_t index) const noexcept
        {
            return mGlyphs[index];
        }

        [[nodiscard]] const ShapedGlyph* begin() const noexcept { return mGlyphs; }

        [[nodiscard]] const ShapedGlyph* end() const noexcept
        {
            return mGlyphs ? mGlyphs + mGlyphCount : nullptr;
        }

    private:
        const ShapedGlyph* mGlyphs{ nullptr };
        size_t mGlyphCount{ 0 };
    };

} // namespace waavs