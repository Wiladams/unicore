// opentype_gpos_mark_base_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"
#include "opentype_gpos_anchor_view.h"
#include "opentype_gpos_mark_array_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposBaseArrayView
    //
    // BaseArray:
    //
    //   uint16     baseCount
    //   BaseRecord baseRecords[baseCount]
    //
    // Each BaseRecord contains markClassCount Offset16 values.
    //
    // Base anchor offsets:
    //
    //   - are relative to the BaseArray
    //   - are ordered by mark class
    //   - may be NULL
    // ====================================================================

    class OpenTypeGposBaseArrayView
    {
    public:
        OpenTypeGposBaseArrayView() noexcept = default;

        OpenTypeGposBaseArrayView(ByteSpan data, uint16_t markClassCount) noexcept
            : fData(data)
            , fMarkClassCount(markClassCount)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 2 || fMarkClassCount == 0)
                return false;

            const size_t recordSize = size_t(fMarkClassCount) * 2;
            const uint16_t count = baseCount();

            if (count > (fData.size() - 2) / recordSize)
                return false;

            const size_t recordsEnd = 2 + size_t(count) * recordSize;

            for (uint16_t baseIndex = 0; baseIndex < count; ++baseIndex)
            {
                for (uint16_t markClass = 0; markClass < fMarkClassCount; ++markClass)
                {
                    uint16_t offset = 0;

                    if (!baseAnchorOffset(baseIndex, markClass, offset))
                        return false;

                    if (offset != 0 && (offset < recordsEnd || offset >= fData.size()))
                        return false;
                }
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t baseCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markClassCount() const noexcept { return fMarkClassCount; }

        [[nodiscard]] bool baseAnchorOffset(
            uint16_t baseIndex, uint16_t markClass, uint16_t& result) const noexcept
        {
            result = 0;

            if (baseIndex >= baseCount() || markClass >= fMarkClassCount)
                return false;

            const size_t recordSize = size_t(fMarkClassCount) * 2;
            const size_t offset =
                2 + size_t(baseIndex) * recordSize + size_t(markClass) * 2;

            return readUInt16(offset, result);
        }

        [[nodiscard]] bool hasBaseAnchor(uint16_t baseIndex, uint16_t markClass) const noexcept
        {
            uint16_t offset = 0;
            return baseAnchorOffset(baseIndex, markClass, offset) && offset != 0;
        }

        [[nodiscard]] OpenTypeGposAnchorView baseAnchor(
            uint16_t baseIndex, uint16_t markClass) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!baseAnchorOffset(baseIndex, markClass, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

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
        uint16_t fMarkClassCount{ 0 };
    };


    // ====================================================================
    // OpenTypeGposMarkBasePosView
    //
    // GPOS LookupType 4: Mark-to-Base Attachment.
    //
    // Format 1:
    //
    //   uint16   posFormat
    //   Offset16 markCoverageOffset
    //   Offset16 baseCoverageOffset
    //   uint16   markClassCount
    //   Offset16 markArrayOffset
    //   Offset16 baseArrayOffset
    // ====================================================================

    class OpenTypeGposMarkBasePosView
    {
    public:
        OpenTypeGposMarkBasePosView() noexcept = default;
        explicit OpenTypeGposMarkBasePosView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 12)
                return false;

            if (format() != 1 || markClassCount() == 0)
                return false;

            const uint16_t offsets[] =
            {
                markCoverageOffset(),
                baseCoverageOffset(),
                markArrayOffset(),
                baseArrayOffset()
            };

            for (uint16_t offset : offsets)
            {
                if (offset < 12 || offset >= fData.size())
                    return false;
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markCoverageOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(2, result) ? result : 0;
        }

        [[nodiscard]] uint16_t baseCoverageOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(4, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markClassCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(6, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markArrayOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(8, result) ? result : 0;
        }

        [[nodiscard]] uint16_t baseArrayOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(10, result) ? result : 0;
        }

        [[nodiscard]] OpenTypeCoverageView markCoverage() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeCoverageView(fData.subSpan(markCoverageOffset()));
        }

        [[nodiscard]] OpenTypeCoverageView baseCoverage() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeCoverageView(fData.subSpan(baseCoverageOffset()));
        }

        [[nodiscard]] OpenTypeGposMarkArrayView markArray() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeGposMarkArrayView(fData.subSpan(markArrayOffset()));
        }

        [[nodiscard]] OpenTypeGposBaseArrayView baseArray() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeGposBaseArrayView(
                fData.subSpan(baseArrayOffset()), markClassCount());
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