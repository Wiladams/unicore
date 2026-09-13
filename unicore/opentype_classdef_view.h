#// opentype_classdef_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeClassDefView
    //
    // Lazy non-owning view of the common OpenType ClassDef structure.
    //
    // Format 1:
    //
    //   uint16 classFormat
    //   uint16 startGlyphId
    //   uint16 glyphCount
    //   uint16 classValueArray[glyphCount]
    //
    // Format 2:
    //
    //   uint16 classFormat
    //   uint16 classRangeCount
    //   ClassRangeRecord classRangeRecords[classRangeCount]
    //
    // A glyph not explicitly assigned to another class belongs to class 0.
    // ====================================================================

    class OpenTypeClassDefView
    {
    public:
        OpenTypeClassDefView() noexcept = default;
        explicit OpenTypeClassDefView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t format = 0;

            if (!readFormat(format))
                return false;

            switch (format)
            {
            case 1:
                return validateFormat1();

            case 2:
                return validateFormat2();

            default:
                return false;
            }
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t result = 0;
            return readFormat(result) ? result : 0;
        }


        // ================================================================
        // classValue
        //
        // Returns false only when the ClassDef itself is invalid or the
        // glyph ID cannot be represented by OpenType.
        //
        // A valid glyph not explicitly assigned by the ClassDef has class 0.
        // ================================================================

        [[nodiscard]] bool classValue(uint32_t glyphId, uint16_t& result) const noexcept
        {
            result = 0;

            if (glyphId > 0xFFFFu)
                return false;

            uint16_t format = 0;

            if (!readFormat(format))
                return false;

            switch (format)
            {
            case 1:
                return classValueFormat1(static_cast<uint16_t>(glyphId), result);

            case 2:
                return classValueFormat2(static_cast<uint16_t>(glyphId), result);

            default:
                return false;
            }
        }

        [[nodiscard]] uint16_t classValue(uint32_t glyphId) const noexcept
        {
            uint16_t result = 0;
            classValue(glyphId, result);
            return result;
        }


        // ================================================================
        // Format 1 access
        // ================================================================

        [[nodiscard]] uint16_t startGlyphId() const noexcept
        {
            if (format() != 1)
                return 0;

            OpenTypeByteStream stream(fData);

            uint16_t ignored = 0;
            uint16_t startGlyph = 0;

            if (!stream.readUInt16(ignored) ||
                !stream.readUInt16(startGlyph))
            {
                return 0;
            }

            return startGlyph;
        }

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            if (format() != 1)
                return 0;

            OpenTypeByteStream stream(fData);

            uint16_t ignored = 0;
            uint16_t count = 0;

            if (!stream.readUInt16(ignored) ||
                !stream.readUInt16(ignored) ||
                !stream.readUInt16(count))
            {
                return 0;
            }

            return count;
        }


        // ================================================================
        // Format 2 access
        // ================================================================

        [[nodiscard]] uint16_t rangeCount() const noexcept
        {
            if (format() != 2)
                return 0;

            OpenTypeByteStream stream(fData);

            uint16_t ignored = 0;
            uint16_t count = 0;

            if (!stream.readUInt16(ignored) ||
                !stream.readUInt16(count))
            {
                return 0;
            }

            return count;
        }

    private:
        bool readFormat(uint16_t& result) const noexcept
        {
            OpenTypeByteStream stream(fData);
            return stream.readUInt16(result);
        }


        // ================================================================
        // Format 1
        // ================================================================

        bool validateFormat1() const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;
            uint16_t startGlyph = 0;
            uint16_t glyphCount = 0;

            if (!stream.readUInt16(format) ||
                !stream.readUInt16(startGlyph) ||
                !stream.readUInt16(glyphCount))
            {
                return false;
            }

            if (format != 1)
                return false;

            return glyphCount <= stream.remaining() / 2;
        }

        bool classValueFormat1(uint16_t glyphId, uint16_t& result) const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;
            uint16_t startGlyph = 0;
            uint16_t glyphCount = 0;

            if (!stream.readUInt16(format) ||
                !stream.readUInt16(startGlyph) ||
                !stream.readUInt16(glyphCount))
            {
                return false;
            }

            if (format != 1 || glyphCount > stream.remaining() / 2)
                return false;


            if (glyphId < startGlyph)
            {
                result = 0;
                return true;
            }

            const uint32_t index =
                uint32_t(glyphId) - uint32_t(startGlyph);

            if (index >= glyphCount)
            {
                result = 0;
                return true;
            }


            if (!stream.seek(6 + size_t(index) * 2))
                return false;

            return stream.readUInt16(result);
        }


        // ================================================================
        // Format 2
        //
        // ClassRangeRecords are sorted by startGlyphId, so lookup uses
        // binary search.
        //
        // Each record:
        //
        //   uint16 startGlyphId
        //   uint16 endGlyphId
        //   uint16 classValue
        // ================================================================

        bool validateFormat2() const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;
            uint16_t rangeCount = 0;

            if (!stream.readUInt16(format) ||
                !stream.readUInt16(rangeCount))
            {
                return false;
            }

            if (format != 2)
                return false;

            return rangeCount <= stream.remaining() / 6;
        }

        bool classValueFormat2(uint16_t glyphId, uint16_t& result) const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;
            uint16_t rangeCount = 0;

            if (!stream.readUInt16(format) ||
                !stream.readUInt16(rangeCount))
            {
                return false;
            }

            if (format != 2 || rangeCount > stream.remaining() / 6)
                return false;


            size_t low = 0;
            size_t high = rangeCount;

            while (low < high)
            {
                const size_t mid = low + (high - low) / 2;

                if (!stream.seek(4 + mid * 6))
                    return false;

                uint16_t startGlyph = 0;
                uint16_t endGlyph = 0;
                uint16_t classValue = 0;

                if (!stream.readUInt16(startGlyph) ||
                    !stream.readUInt16(endGlyph) ||
                    !stream.readUInt16(classValue))
                {
                    return false;
                }

                if (startGlyph > endGlyph)
                    return false;


                if (glyphId < startGlyph)
                {
                    high = mid;
                    continue;
                }

                if (glyphId > endGlyph)
                {
                    low = mid + 1;
                    continue;
                }


                result = classValue;
                return true;
            }


            // Glyphs not otherwise classified belong to class 0.

            result = 0;
            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs