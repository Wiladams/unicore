// opentype_maxp_view.h
#pragma once

#include "font_face_view.h"
#include "opentype_bytestream.h"

namespace waavs
{
    class MaxpView
    {
    private:
        uint32_t fGlyphCount{ 0 };
        bool fValid{ false };

    public:
        MaxpView() = default;

        explicit MaxpView(const FontFaceView& face) noexcept
        {
            reset(face);
        }

        bool reset(const FontFaceView& face) noexcept
        {
            fGlyphCount = 0;
            fValid = false;

            const TableRecord* table = face.getTable(TagConstants::MAXP);

            if (!table || table->data.size() < 6)
                return false;

            OpenTypeByteStream stream(table->data);

            if (!stream.skip(4))
                return false;

            uint16_t glyphCount = 0;

            if (!stream.readUInt16(glyphCount))
                return false;

            fGlyphCount = glyphCount;
            fValid = true;
            return true;
        }

        explicit operator bool() const noexcept
        {
            return fValid;
        }

        uint32_t glyphCount() const noexcept
        {
            return fGlyphCount;
        }
    };
}