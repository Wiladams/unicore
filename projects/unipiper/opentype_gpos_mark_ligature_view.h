// opentype_gpos_mark_ligature_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"
#include "opentype_gpos_anchor_view.h"
#include "opentype_gpos_mark_array_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposLigatureAttachView
    //
    // LigatureAttach:
    //
    //   uint16          componentCount
    //   ComponentRecord componentRecords[componentCount]
    //
    // Each ComponentRecord contains markClassCount Offset16 values.
    //
    // Anchor offsets:
    //
    //   - are relative to the beginning of this LigatureAttach
    //   - are ordered by mark class
    //   - may be NULL
    // ====================================================================

    class OpenTypeGposLigatureAttachView
    {
    public:
        OpenTypeGposLigatureAttachView() noexcept = default;

        OpenTypeGposLigatureAttachView(ByteSpan data, uint16_t markClassCount) noexcept
            : fData(data)
            , fMarkClassCount(markClassCount)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 2 || fMarkClassCount == 0)
                return false;

            const uint16_t count = componentCount();

            if (count == 0)
                return false;

            const size_t recordSize = size_t(fMarkClassCount) * 2;

            if (count > (fData.size() - 2) / recordSize)
                return false;

            const size_t recordsEnd = 2 + size_t(count) * recordSize;

            for (uint16_t component = 0; component < count; ++component)
            {
                for (uint16_t markClass = 0; markClass < fMarkClassCount; ++markClass)
                {
                    uint16_t offset = 0;

                    if (!anchorOffset(component, markClass, offset))
                        return false;

                    if (offset != 0 && (offset < recordsEnd || offset >= fData.size()))
                        return false;
                }
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t componentCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markClassCount() const noexcept
        {
            return fMarkClassCount;
        }

        [[nodiscard]] bool anchorOffset(
            uint16_t componentIndex, uint16_t markClass,
            uint16_t& result) const noexcept
        {
            result = 0;

            if (componentIndex >= componentCount() ||
                markClass >= fMarkClassCount)
            {
                return false;
            }

            const size_t recordSize = size_t(fMarkClassCount) * 2;
            const size_t offset =
                2 + size_t(componentIndex) * recordSize + size_t(markClass) * 2;

            return readUInt16(offset, result);
        }

        [[nodiscard]] bool hasAnchor(
            uint16_t componentIndex, uint16_t markClass) const noexcept
        {
            uint16_t offset = 0;
            return anchorOffset(componentIndex, markClass, offset) && offset != 0;
        }

        [[nodiscard]] OpenTypeGposAnchorView anchor(
            uint16_t componentIndex, uint16_t markClass) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!anchorOffset(componentIndex, markClass, offset) ||
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
    // OpenTypeGposLigatureArrayView
    //
    // LigatureArray:
    //
    //   uint16   ligatureCount
    //   Offset16 ligatureAttachOffsets[ligatureCount]
    //
    // Offsets are relative to the beginning of the LigatureArray.
    // ====================================================================

    class OpenTypeGposLigatureArrayView
    {
    public:
        OpenTypeGposLigatureArrayView() noexcept = default;

        OpenTypeGposLigatureArrayView(ByteSpan data, uint16_t markClassCount) noexcept
            : fData(data)
            , fMarkClassCount(markClassCount)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 2 || fMarkClassCount == 0)
                return false;

            const uint16_t count = ligatureCount();

            if (count > (fData.size() - 2) / 2)
                return false;

            const size_t offsetsEnd = 2 + size_t(count) * 2;

            for (uint16_t i = 0; i < count; ++i)
            {
                uint16_t offset = 0;

                if (!ligatureAttachOffset(i, offset))
                    return false;

                if (offset < offsetsEnd || offset >= fData.size())
                    return false;
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t ligatureCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] uint16_t markClassCount() const noexcept
        {
            return fMarkClassCount;
        }

        [[nodiscard]] bool ligatureAttachOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (index >= ligatureCount())
                return false;

            return readUInt16(2 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGposLigatureAttachView ligatureAttach(uint16_t index) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!ligatureAttachOffset(index, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

            return OpenTypeGposLigatureAttachView(
                fData.subSpan(offset), fMarkClassCount);
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
    // OpenTypeGposMarkLigaturePosView
    //
    // GPOS LookupType 5: Mark-to-Ligature Attachment.
    //
    // Format 1:
    //
    //   uint16   posFormat
    //   Offset16 markCoverageOffset
    //   Offset16 ligatureCoverageOffset
    //   uint16   markClassCount
    //   Offset16 markArrayOffset
    //   Offset16 ligatureArrayOffset
    // ====================================================================

    class OpenTypeGposMarkLigaturePosView
    {
    public:
        OpenTypeGposMarkLigaturePosView() noexcept = default;
        explicit OpenTypeGposMarkLigaturePosView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 12)
                return false;

            if (format() != 1 || markClassCount() == 0)
                return false;

            const uint16_t offsets[] =
            {
                markCoverageOffset(),
                ligatureCoverageOffset(),
                markArrayOffset(),
                ligatureArrayOffset()
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

        [[nodiscard]] uint16_t ligatureCoverageOffset() const noexcept
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

        [[nodiscard]] uint16_t ligatureArrayOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(10, result) ? result : 0;
        }

        [[nodiscard]] OpenTypeCoverageView markCoverage() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeCoverageView(
                fData.subSpan(markCoverageOffset()));
        }

        [[nodiscard]] OpenTypeCoverageView ligatureCoverage() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeCoverageView(
                fData.subSpan(ligatureCoverageOffset()));
        }

        [[nodiscard]] OpenTypeGposMarkArrayView markArray() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeGposMarkArrayView(
                fData.subSpan(markArrayOffset()));
        }

        [[nodiscard]] OpenTypeGposLigatureArrayView ligatureArray() const noexcept
        {
            if (!isValid())
                return {};

            return OpenTypeGposLigatureArrayView(
                fData.subSpan(ligatureArrayOffset()),
                markClassCount());
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