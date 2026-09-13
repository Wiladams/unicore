// opentype_shaping_buffer.h

#pragma once

#include <cstdint>
#include <vector>

#include "font_run.h"
#include "opentype_shaping_glyph.h"


namespace waavs
{
    class OpenTypeShapingBuffer
    {
    public:
        void clear() noexcept
        {
            mGlyphs.clear();
            mInput = nullptr;
        }

        void reset(const FontRunView& input)
        {
            mGlyphs.clear();
            mInput = &input;
            mGlyphs.reserve(input.scalarCount);
        }

        [[nodiscard]] const FontRunView* input() const noexcept { return mInput; }

        [[nodiscard]] size_t size() const noexcept { return mGlyphs.size(); }
        [[nodiscard]] bool empty() const noexcept { return mGlyphs.empty(); }

        OpenTypeShapingGlyph& operator[](size_t index) noexcept { return mGlyphs[index]; }
        const OpenTypeShapingGlyph& operator[](size_t index) const noexcept { return mGlyphs[index]; }

        auto begin() noexcept { return mGlyphs.begin(); }
        auto end() noexcept { return mGlyphs.end(); }
        auto begin() const noexcept { return mGlyphs.begin(); }
        auto end() const noexcept { return mGlyphs.end(); }

        void pushBack(const OpenTypeShapingGlyph& glyph)
        {
            mGlyphs.push_back(glyph);
        }

        std::vector<OpenTypeShapingGlyph>& glyphs() noexcept { return mGlyphs; }
        const std::vector<OpenTypeShapingGlyph>& glyphs() const noexcept { return mGlyphs; }

    private:
        const FontRunView* mInput{ nullptr };
        std::vector<OpenTypeShapingGlyph> mGlyphs{};
    };

} // namespace waavs
