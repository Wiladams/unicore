// opentype_gpos_cursive_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"
#include "opentype_gpos_anchor_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposCursivePosView
    //
    // GPOS LookupType 3: Cursive Attachment Positioning.
    //
    // There is one format:
    //
    //   uint16   posFormat
    //   Offset16 coverageOffset
    //   uint16   entryExitCount
    //   EntryExitRecord entryExitRecords[entryExitCount]
    //
    // EntryExitRecord:
    //
    //   Offset16 entryAnchorOffset
    //   Offset16 exitAnchorOffset
    //
    // Anchor offsets are relative to the beginning of this CursivePos
    // subtable and may be NULL.
    // ====================================================================

    class OpenTypeGposCursivePosView
    {
    public:
        OpenTypeGposCursivePosView() noexcept = default;
        explicit OpenTypeGposCursivePosView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 6)
                return false;

            uint16_t posFormat = 0;
            uint16_t coverageOff = 0;
            uint16_t count = 0;

            if (!readUInt16(0, posFormat) ||
                !readUInt16(2, coverageOff) ||
                !readUInt16(4, count))
            {
                return false;
            }

            if (posFormat != 1)
                return false;

            if (count > (fData.size() - 6) / 4)
                return false;

            const size_t recordsEnd = 6 + size_t(count) * 4;

            if (coverageOff < recordsEnd || coverageOff >= fData.size())
                return false;

            for (uint16_t i = 0; i < count; ++i)
            {
                uint16_t entryOffset = 0;
                uint16_t exitOffset = 0;

                if (!entryAnchorOffset(i, entryOffset) ||
                    !exitAnchorOffset(i, exitOffset))
                {
                    return false;
                }

                if (!validAnchorOffset(entryOffset, recordsEnd) ||
                    !validAnchorOffset(exitOffset, recordsEnd))
                {
                    return false;
                }
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

        [[nodiscard]] uint16_t coverageOffset() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(2, result) ? result : 0;
        }

        [[nodiscard]] uint16_t entryExitCount() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(4, result) ? result : 0;
        }

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            if (!isValid())
                return {};

            const uint16_t offset = coverageOffset();

            if (offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // EntryExitRecord access.
        // ================================================================

        [[nodiscard]] bool entryAnchorOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (index >= entryExitCount())
                return false;

            return readUInt16(6 + size_t(index) * 4, result);
        }

        [[nodiscard]] bool exitAnchorOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (index >= entryExitCount())
                return false;

            return readUInt16(8 + size_t(index) * 4, result);
        }

        [[nodiscard]] bool hasEntryAnchor(uint16_t index) const noexcept
        {
            uint16_t offset = 0;
            return entryAnchorOffset(index, offset) && offset != 0;
        }

        [[nodiscard]] bool hasExitAnchor(uint16_t index) const noexcept
        {
            uint16_t offset = 0;
            return exitAnchorOffset(index, offset) && offset != 0;
        }

        [[nodiscard]] OpenTypeGposAnchorView entryAnchor(uint16_t index) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!entryAnchorOffset(index, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

            return OpenTypeGposAnchorView(fData.subSpan(offset));
        }

        [[nodiscard]] OpenTypeGposAnchorView exitAnchor(uint16_t index) const noexcept
        {
            if (!isValid())
                return {};

            uint16_t offset = 0;

            if (!exitAnchorOffset(index, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

            return OpenTypeGposAnchorView(fData.subSpan(offset));
        }


        // ================================================================
        // Glyph convenience.
        //
        // Coverage index selects the corresponding EntryExitRecord.
        // ================================================================

        [[nodiscard]] bool coverageIndex(uint32_t glyphId, uint16_t& result) const noexcept
        {
            result = 0;

            if (!isValid() || glyphId > 0xFFFFu)
                return false;

            const OpenTypeCoverageView cov = coverage();

            if (!cov)
                return false;

            return cov.find(glyphId, result);
        }

    private:
        [[nodiscard]] bool validAnchorOffset(uint16_t offset, size_t recordsEnd) const noexcept
        {
            if (offset == 0)
                return true;

            return offset >= recordsEnd && offset < fData.size();
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

    private:
        ByteSpan fData{};
    };

} // namespace waavs