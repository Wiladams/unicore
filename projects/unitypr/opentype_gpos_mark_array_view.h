// opentype_gpos_mark_array_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_gpos_anchor_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposMarkArrayView
    //
    // Shared by GPOS Types 4, 5 and 6.
    //
    // MarkArray:
    //
    //   uint16     markCount
    //   MarkRecord markRecords[markCount]
    //
    // MarkRecord:
    //
    //   uint16   markClass
    //   Offset16 markAnchorOffset
    //
    // markAnchorOffset is relative to the beginning of the MarkArray.
    // ====================================================================

    class OpenTypeGposMarkArrayView
    {
    public:
        OpenTypeGposMarkArrayView() noexcept = default;
        explicit OpenTypeGposMarkArrayView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 2)
                return false;

            const uint16_t count = markCount();

            if (count > (fData.size() - 2) / 4)
                return false;

            const size_t recordsEnd = 2 + size_t(count) * 4;

            for (uint16_t i = 0; i < count; ++i)
            {
                uint16_t anchorOffset = 0;

                if (!markAnchorOffset(i, anchorOffset))
                    return false;

                // Every MarkRecord defines an Anchor. Unlike BaseArray
                // anchors, this offset is not nullable.

                if (anchorOffset < recordsEnd || anchorOffset >= fData.size())
                    return false;
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t markCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] bool markClass(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (index >= markCount())
                return false;

            return readUInt16(2 + size_t(index) * 4, result);
        }

        [[nodiscard]] bool markAnchorOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (index >= markCount())
                return false;

            return readUInt16(4 + size_t(index) * 4, result);
        }

        [[nodiscard]] OpenTypeGposAnchorView markAnchor(uint16_t index) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!markAnchorOffset(index, offset) || offset >= fData.size())
                return {};

            return OpenTypeGposAnchorView(fData.subSpan(offset));
        }

    private:
        [[nodiscard]] bool readUInt16(size_t offset, uint16_t& result) const noexcept
        {
            result = 0;

            if (offset > fData.size() || fData.size() - offset < 2)
                return false;

            const uint8_t* p = fData.begin() + offset;
            result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs