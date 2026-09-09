// opentype_gpos_mark_mark_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"
#include "opentype_gpos_anchor_view.h"
#include "opentype_gpos_mark_array_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposMark2ArrayView
    //
    // Mark2Array:
    //
    //   uint16      mark2Count
    //   Mark2Record mark2Records[mark2Count]
    //
    // Each Mark2Record contains markClassCount Offset16 values.
    //
    // Anchor offsets:
    //
    //   - are relative to the beginning of the Mark2Array
    //   - are ordered by Mark1 class
    //   - may be NULL
    // ====================================================================

    class OpenTypeGposMark2ArrayView
    {
    public:
        OpenTypeGposMark2ArrayView() noexcept = default;

        OpenTypeGposMark2ArrayView(ByteSpan data, uint16_t markClassCount) noexcept
            : fData(data)
            , fMarkClassCount(markClassCount)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 2 || fMarkClassCount == 0)
                return false;

            const uint16_t count = mark2Count();
            const size_t recordSize = size_t(fMarkClassCount) * 2;

            if (count > (fData.size() - 2) / recordSize)
                return false;

            const size_t recordsEnd = 2 + size_t(count) * recordSize;

            for (uint16_t mark2Index = 0; mark2Index < count; ++mark2Index)
            {
                for (uint16_t markClass = 0; markClass < fMarkClassCount; ++markClass)
                {
                    uint16_t offset = 0;

                    if (!anchorOffset(mark2Index, markClass, offset))
                        return false;

                    if (offset != 0 && (offset < recordsEnd || offset >= fData.size()))
                        return false;
                }
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t mark2Count() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markClassCount() const noexcept
        {
            return fMarkClassCount;
        }

        [[nodiscard]] bool anchorOffset(
            uint16_t mark2Index, uint16_t markClass, uint16_t& result) const noexcept
        {
            result = 0;

            if (mark2Index >= mark2Count() || markClass >= fMarkClassCount)
                return false;

            const size_t recordSize = size_t(fMarkClassCount) * 2;
            const size_t offset =
                2 + size_t(mark2Index) * recordSize + size_t(markClass) * 2;

            return readUInt16(offset, result);
        }

        [[nodiscard]] bool hasAnchor(uint16_t mark2Index, uint16_t markClass) const noexcept
        {
            uint16_t offset = 0;
            return anchorOffset(mark2Index, markClass, offset) && offset != 0;
        }

        [[nodiscard]] OpenTypeGposAnchorView anchor(
            uint16_t mark2Index, uint16_t markClass) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!anchorOffset(mark2Index, markClass, offset) ||
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
    // OpenTypeGposMarkMarkPosView
    //
    // GPOS LookupType 6: Mark-to-Mark Attachment.
    //
    // Format 1:
    //
    //   uint16   posFormat
    //   Offset16 mark1CoverageOffset
    //   Offset16 mark2CoverageOffset
    //   uint16   markClassCount
    //   Offset16 mark1ArrayOffset
    //   Offset16 mark2ArrayOffset
    // ====================================================================

    class OpenTypeGposMarkMarkPosView
    {
    public:
        OpenTypeGposMarkMarkPosView() noexcept = default;
        explicit OpenTypeGposMarkMarkPosView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 12)
                return false;

            if (format() != 1 || markClassCount() == 0)
                return false;

            const uint16_t offsets[] =
            {
                mark1CoverageOffset(),
                mark2CoverageOffset(),
                mark1ArrayOffset(),
                mark2ArrayOffset()
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

        [[nodiscard]] uint16_t mark1CoverageOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(2, result) ? result : 0;
        }

        [[nodiscard]] uint16_t mark2CoverageOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(4, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markClassCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(6, result) ? result : 0;
        }

        [[nodiscard]] uint16_t mark1ArrayOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(8, result) ? result : 0;
        }

        [[nodiscard]] uint16_t mark2ArrayOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(10, result) ? result : 0;
        }

        [[nodiscard]] OpenTypeCoverageView mark1Coverage() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeCoverageView(fData.subSpan(mark1CoverageOffset()));
        }

        [[nodiscard]] OpenTypeCoverageView mark2Coverage() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeCoverageView(fData.subSpan(mark2CoverageOffset()));
        }

        [[nodiscard]] OpenTypeGposMarkArrayView mark1Array() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeGposMarkArrayView(fData.subSpan(mark1ArrayOffset()));
        }

        [[nodiscard]] OpenTypeGposMark2ArrayView mark2Array() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeGposMark2ArrayView(
                fData.subSpan(mark2ArrayOffset()), markClassCount());
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