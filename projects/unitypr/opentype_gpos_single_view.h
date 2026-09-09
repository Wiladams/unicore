// opentype_gpos_single_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_coverage_view.h"
#include "opentype_gpos_value_record.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposSinglePosView
    //
    // GPOS LookupType 1: Single Adjustment Positioning.
    //
    // Format 1:
    //
    //   uint16   posFormat
    //   Offset16 coverageOffset
    //   uint16   valueFormat
    //   ValueRecord valueRecord
    //
    // One ValueRecord applies to every glyph in Coverage.
    //
    // Format 2:
    //
    //   uint16   posFormat
    //   Offset16 coverageOffset
    //   uint16   valueFormat
    //   uint16   valueCount
    //   ValueRecord valueRecords[valueCount]
    //
    // Coverage index selects the corresponding ValueRecord.
    // ====================================================================

    class OpenTypeGposSinglePosView
    {
    public:
        OpenTypeGposSinglePosView() noexcept = default;
        explicit OpenTypeGposSinglePosView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t posFormat = 0;

            if (!readUInt16(0, posFormat))
                return false;

            if (posFormat == 1)
                return isFormat1Valid();

            if (posFormat == 2)
                return isFormat2Valid();

            return false;
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

        [[nodiscard]] uint16_t valueFormat() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(4, result) ? result : 0;
        }

        [[nodiscard]] uint16_t valueCount() const noexcept
        {
            if (format() != 2)
                return format() == 1 ? 1 : 0;

            uint16_t result = 0;
            return readUInt16(6, result) ? result : 0;
        }

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            const uint16_t offset = coverageOffset();

            if (!isValid() || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // valueRecord
        //
        // Format 1:
        //   index must be zero.
        //
        // Format 2:
        //   index selects one ValueRecord.
        // ================================================================

        [[nodiscard]]
        bool valueRecord(uint16_t index, OpenTypeGposValueRecord& result) const noexcept
        {
            if (!isValid())
                return false;

            const uint16_t posFormat = format();
            const uint16_t valueFmt = valueFormat();

            size_t recordSize = 0;

            if (!openTypeGposValueRecordSize(valueFmt, recordSize))
                return false;

            size_t recordOffset = 0;

            if (posFormat == 1)
            {
                if (index != 0)
                    return false;

                recordOffset = 6;
            }
            else if (posFormat == 2)
            {
                if (index >= valueCount())
                    return false;

                recordOffset = 8 + size_t(index) * recordSize;
            }
            else
            {
                return false;
            }

            if (recordSize == 0)
            {
                result = {};
                result.valueFormat = valueFmt;
                return true;
            }

            if (recordOffset > fData.size() || recordSize > fData.size() - recordOffset)
                return false;

            OpenTypeByteStream stream(fData.subSpan(recordOffset, recordSize));
            return readOpenTypeGposValueRecord(stream, valueFmt, result);
        }


        // ================================================================
        // valueRecordForCoverageIndex
        //
        // Normalize the difference between Format 1 and Format 2.
        // ================================================================

        [[nodiscard]]
        bool valueRecordForCoverageIndex(
            uint16_t coverageIndex, OpenTypeGposValueRecord& result) const noexcept
        {
            if (format() == 1)
                return valueRecord(0, result);

            if (format() == 2)
                return valueRecord(coverageIndex, result);

            return false;
        }

    private:
        [[nodiscard]]
        bool readUInt16(size_t offset, uint16_t& result) const noexcept
        {
            result = 0;

            if (offset > fData.size() || fData.size() - offset < 2)
                return false;

            const uint8_t* p = fData.begin() + offset;
            result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
            return true;
        }


        [[nodiscard]]
        bool isFormat1Valid() const noexcept
        {
            if (fData.size() < 6)
                return false;

            uint16_t coverageOff = 0;
            uint16_t valueFmt = 0;

            if (!readUInt16(2, coverageOff) ||
                !readUInt16(4, valueFmt))
            {
                return false;
            }

            size_t recordSize = 0;

            if (!openTypeGposValueRecordSize(valueFmt, recordSize))
                return false;

            const size_t recordsEnd = 6 + recordSize;

            if (recordsEnd > fData.size())
                return false;

            if (coverageOff < recordsEnd || coverageOff >= fData.size())
                return false;

            return true;
        }


        [[nodiscard]]
        bool isFormat2Valid() const noexcept
        {
            if (fData.size() < 8)
                return false;

            uint16_t coverageOff = 0;
            uint16_t valueFmt = 0;
            uint16_t count = 0;

            if (!readUInt16(2, coverageOff) ||
                !readUInt16(4, valueFmt) ||
                !readUInt16(6, count))
            {
                return false;
            }

            size_t recordSize = 0;

            if (!openTypeGposValueRecordSize(valueFmt, recordSize))
                return false;

            if (recordSize != 0 && count > (fData.size() - 8) / recordSize)
                return false;

            const size_t recordsEnd = 8 + size_t(count) * recordSize;

            if (recordsEnd > fData.size())
                return false;

            if (coverageOff < recordsEnd || coverageOff >= fData.size())
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs