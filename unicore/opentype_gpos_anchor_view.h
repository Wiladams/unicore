// opentype_gpos_anchor_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "lang_span.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposAnchorView
    //
    // OpenType GPOS Anchor table.
    //
    // Format 1:
    //
    //   uint16 format
    //   int16  xCoordinate
    //   int16  yCoordinate
    //
    // Format 2:
    //
    //   uint16 format
    //   int16  xCoordinate
    //   int16  yCoordinate
    //   uint16 anchorPoint
    //
    // Format 3:
    //
    //   uint16   format
    //   int16    xCoordinate
    //   int16    yCoordinate
    //   Offset16 xDeviceOffset
    //   Offset16 yDeviceOffset
    //
    // Format 3 offsets are relative to the beginning of this Anchor table.
    // They may refer to Device tables or VariationIndex tables.
    //
    // Device/VariationIndex tables are deliberately left unresolved here.
    // ====================================================================

    class OpenTypeGposAnchorView
    {
    public:
        OpenTypeGposAnchorView() noexcept = default;
        explicit OpenTypeGposAnchorView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t anchorFormat = 0;

            if (!readUInt16(0, anchorFormat))
                return false;

            if (anchorFormat == 1)
                return fData.size() >= 6;

            if (anchorFormat == 2)
                return fData.size() >= 8;

            if (anchorFormat == 3)
            {
                if (fData.size() < 10)
                    return false;

                uint16_t xOffset = 0;
                uint16_t yOffset = 0;

                if (!readUInt16(6, xOffset) || !readUInt16(8, yOffset))
                    return false;

                if (!validDeviceOffset(xOffset) || !validDeviceOffset(yOffset))
                    return false;

                return true;
            }

            return false;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] int16_t xCoordinate() const noexcept
        {
            int16_t result = 0;
            return readInt16(2, result) ? result : 0;
        }

        [[nodiscard]] int16_t yCoordinate() const noexcept
        {
            int16_t result = 0;
            return readInt16(4, result) ? result : 0;
        }

        [[nodiscard]] bool hasContourPoint() const noexcept
        {
            return isValid() && format() == 2;
        }

        [[nodiscard]] bool anchorPoint(uint16_t& result) const noexcept
        {
            result = 0;

            if (!isValid() || format() != 2)
                return false;

            return readUInt16(6, result);
        }

        [[nodiscard]] bool hasDeviceOffsets() const noexcept
        {
            return isValid() && format() == 3;
        }

        [[nodiscard]] bool xDeviceOffset(uint16_t& result) const noexcept
        {
            result = 0;

            if (!isValid() || format() != 3)
                return false;

            return readUInt16(6, result);
        }

        [[nodiscard]] bool yDeviceOffset(uint16_t& result) const noexcept
        {
            result = 0;

            if (!isValid() || format() != 3)
                return false;

            return readUInt16(8, result);
        }

        [[nodiscard]] ByteSpan xDeviceData() const noexcept
        {
            uint16_t offset = 0;

            if (!xDeviceOffset(offset) || offset == 0)
                return {};

            return fData.subSpan(offset);
        }

        [[nodiscard]] ByteSpan yDeviceData() const noexcept
        {
            uint16_t offset = 0;

            if (!yDeviceOffset(offset) || offset == 0)
                return {};

            return fData.subSpan(offset);
        }

    private:
        [[nodiscard]] bool validDeviceOffset(uint16_t offset) const noexcept
        {
            if (offset == 0)
                return true;

            // A non-NULL child offset cannot point back into the 10-byte
            // Anchor Format 3 header.

            return offset >= 10 && offset < fData.size();
        }

        [[nodiscard]] bool readUInt16(size_t offset, uint16_t& result) const noexcept
        {
            result = 0;

            if (offset > fData.size() || fData.size() - offset < 2)
                return false;

            const uint8_t* p = fData.begin() + offset;
            result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
            return true;
        }

        [[nodiscard]] bool readInt16(size_t offset, int16_t& result) const noexcept
        {
            uint16_t value = 0;

            if (!readUInt16(offset, value))
                return false;

            result = static_cast<int16_t>(value);
            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs
