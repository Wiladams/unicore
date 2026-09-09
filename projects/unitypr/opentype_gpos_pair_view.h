// opentype_gpos_pair_view.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "opentype_bytestream.h"
#include "opentype_classdef_view.h"
#include "opentype_coverage_view.h"
#include "opentype_gpos_value_record.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposPairSetView
    //
    // PairPos Format 1 PairSet.
    //
    // PairValueRecords are sorted by secondGlyph.
    // ====================================================================

    class OpenTypeGposPairSetView
    {
    public:
        OpenTypeGposPairSetView() noexcept = default;

        OpenTypeGposPairSetView(
            ByteSpan data, uint16_t valueFormat1,
            uint16_t valueFormat2) noexcept
            : fData(data)
            , fValueFormat1(valueFormat1)
            , fValueFormat2(valueFormat2)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 2)
                return false;

            size_t valueSize1 = 0;
            size_t valueSize2 = 0;

            if (!openTypeGposValueRecordSize(fValueFormat1, valueSize1) ||
                !openTypeGposValueRecordSize(fValueFormat2, valueSize2))
            {
                return false;
            }

            const size_t recordSize = 2 + valueSize1 + valueSize2;
            const uint16_t count = size();

            if (count > (fData.size() - 2) / recordSize)
                return false;

            uint16_t previousGlyph = 0;
            bool havePrevious = false;

            for (uint16_t i = 0; i < count; ++i)
            {
                uint16_t glyphId = 0;

                if (!secondGlyph(i, glyphId))
                    return false;

                if (havePrevious && glyphId <= previousGlyph)
                    return false;

                previousGlyph = glyphId;
                havePrevious = true;
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(0, result) ? result : 0;
        }

        [[nodiscard]] uint16_t valueFormat1() const noexcept { return fValueFormat1; }
        [[nodiscard]] uint16_t valueFormat2() const noexcept { return fValueFormat2; }

        [[nodiscard]]
        bool secondGlyph(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            size_t recordSize = 0;

            if (!pairRecordSize(recordSize) || index >= size())
                return false;

            return readUInt16(2 + size_t(index) * recordSize, result);
        }

        [[nodiscard]]
        bool valueRecords(
            uint16_t index, OpenTypeGposValueRecord& value1,
            OpenTypeGposValueRecord& value2) const noexcept
        {
            if (!isValid() || index >= size())
                return false;

            size_t valueSize1 = 0;
            size_t valueSize2 = 0;

            if (!openTypeGposValueRecordSize(fValueFormat1, valueSize1) ||
                !openTypeGposValueRecordSize(fValueFormat2, valueSize2))
            {
                return false;
            }

            const size_t recordSize = 2 + valueSize1 + valueSize2;
            const size_t offset = 2 + size_t(index) * recordSize;

            OpenTypeByteStream stream(
                fData.subSpan(offset + 2, valueSize1 + valueSize2));

            OpenTypeGposValueRecord first{};
            OpenTypeGposValueRecord second{};

            if (!readOpenTypeGposValueRecord(stream, fValueFormat1, first) ||
                !readOpenTypeGposValueRecord(stream, fValueFormat2, second))
            {
                return false;
            }

            value1 = first;
            value2 = second;
            return true;
        }

        [[nodiscard]]
        bool find(
            uint16_t secondGlyphId, OpenTypeGposValueRecord& value1,
            OpenTypeGposValueRecord& value2) const noexcept
        {
            if (!isValid())
                return false;

            size_t first = 0;
            size_t last = size();

            while (first < last)
            {
                const size_t mid = first + (last - first) / 2;

                uint16_t glyphId = 0;

                if (!secondGlyph(static_cast<uint16_t>(mid), glyphId))
                    return false;

                if (glyphId < secondGlyphId)
                    first = mid + 1;
                else
                    last = mid;
            }

            if (first >= size())
                return false;

            uint16_t glyphId = 0;

            if (!secondGlyph(static_cast<uint16_t>(first), glyphId) ||
                glyphId != secondGlyphId)
            {
                return false;
            }

            return valueRecords(
                static_cast<uint16_t>(first),
                value1, value2);
        }

    private:
        [[nodiscard]]
        bool pairRecordSize(size_t& result) const noexcept
        {
            size_t valueSize1 = 0;
            size_t valueSize2 = 0;

            if (!openTypeGposValueRecordSize(fValueFormat1, valueSize1) ||
                !openTypeGposValueRecordSize(fValueFormat2, valueSize2))
            {
                return false;
            }

            result = 2 + valueSize1 + valueSize2;
            return true;
        }

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

    private:
        ByteSpan fData{};
        uint16_t fValueFormat1{ 0 };
        uint16_t fValueFormat2{ 0 };
    };


    // ====================================================================
    // OpenTypeGposPairPosView
    //
    // GPOS LookupType 2: Pair Adjustment Positioning.
    //
    // Format 1:
    //
    //   Coverage -> PairSet -> second glyph
    //
    // Format 2:
    //
    //   Coverage
    //       +
    //   ClassDef1 / ClassDef2
    //       ->
    //   class1 x class2 ValueRecord matrix
    // ====================================================================

    class OpenTypeGposPairPosView
    {
    public:
        OpenTypeGposPairPosView() noexcept = default;
        explicit OpenTypeGposPairPosView(ByteSpan data) noexcept : fData(data) {}

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

        [[nodiscard]] uint16_t valueFormat1() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(4, result) ? result : 0;
        }

        [[nodiscard]] uint16_t valueFormat2() const noexcept
        {
            uint16_t result = 0;
            return readUInt16(6, result) ? result : 0;
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
        // Format 1.
        // ================================================================

        [[nodiscard]] uint16_t pairSetCount() const noexcept
        {
            if (format() != 1)
                return 0;

            uint16_t result = 0;
            return readUInt16(8, result) ? result : 0;
        }

        [[nodiscard]]
        bool pairSetOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (format() != 1 || index >= pairSetCount())
                return false;

            return readUInt16(10 + size_t(index) * 2, result);
        }

        [[nodiscard]]
        OpenTypeGposPairSetView pairSet(uint16_t index) const noexcept
        {
            if (!isValid() || format() != 1)
                return {};

            uint16_t offset = 0;

            if (!pairSetOffset(index, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

            return OpenTypeGposPairSetView(
                fData.subSpan(offset),
                valueFormat1(),
                valueFormat2());
        }


        // ================================================================
        // Format 2.
        // ================================================================

        [[nodiscard]] uint16_t classDef1Offset() const noexcept
        {
            if (format() != 2)
                return 0;

            uint16_t result = 0;
            return readUInt16(8, result) ? result : 0;
        }

        [[nodiscard]] uint16_t classDef2Offset() const noexcept
        {
            if (format() != 2)
                return 0;

            uint16_t result = 0;
            return readUInt16(10, result) ? result : 0;
        }

        [[nodiscard]] uint16_t class1Count() const noexcept
        {
            if (format() != 2)
                return 0;

            uint16_t result = 0;
            return readUInt16(12, result) ? result : 0;
        }

        [[nodiscard]] uint16_t class2Count() const noexcept
        {
            if (format() != 2)
                return 0;

            uint16_t result = 0;
            return readUInt16(14, result) ? result : 0;
        }

        [[nodiscard]] OpenTypeClassDefView classDef1() const noexcept
        {
            if (!isValid() || format() != 2)
                return {};

            const uint16_t offset = classDef1Offset();

            if (offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeClassDefView(fData.subSpan(offset));
        }

        [[nodiscard]] OpenTypeClassDefView classDef2() const noexcept
        {
            if (!isValid() || format() != 2)
                return {};

            const uint16_t offset = classDef2Offset();

            if (offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeClassDefView(fData.subSpan(offset));
        }

        [[nodiscard]]
        bool classValueRecords(
            uint16_t class1, uint16_t class2,
            OpenTypeGposValueRecord& value1,
            OpenTypeGposValueRecord& value2) const noexcept
        {
            if (!isValid() || format() != 2 ||
                class1 >= class1Count() || class2 >= class2Count())
            {
                return false;
            }

            size_t valueSize1 = 0;
            size_t valueSize2 = 0;

            if (!openTypeGposValueRecordSize(valueFormat1(), valueSize1) ||
                !openTypeGposValueRecordSize(valueFormat2(), valueSize2))
            {
                return false;
            }

            const size_t recordSize = valueSize1 + valueSize2;
            const size_t recordIndex =
                size_t(class1) * size_t(class2Count()) + size_t(class2);
            const size_t offset = 16 + recordIndex * recordSize;

            OpenTypeByteStream stream(
                fData.subSpan(offset, recordSize));

            OpenTypeGposValueRecord first{};
            OpenTypeGposValueRecord second{};

            if (!readOpenTypeGposValueRecord(stream, valueFormat1(), first) ||
                !readOpenTypeGposValueRecord(stream, valueFormat2(), second))
            {
                return false;
            }

            value1 = first;
            value2 = second;
            return true;
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
        bool valueSizes(size_t& valueSize1, size_t& valueSize2) const noexcept
        {
            return
                openTypeGposValueRecordSize(valueFormat1(), valueSize1) &&
                openTypeGposValueRecordSize(valueFormat2(), valueSize2);
        }

        [[nodiscard]]
        bool isFormat1Valid() const noexcept
        {
            if (fData.size() < 10)
                return false;

            size_t valueSize1 = 0;
            size_t valueSize2 = 0;

            if (!valueSizes(valueSize1, valueSize2))
                return false;

            uint16_t count = 0;

            if (!readUInt16(8, count))
                return false;

            if (count > (fData.size() - 10) / 2)
                return false;

            const size_t headerEnd = 10 + size_t(count) * 2;
            const uint16_t coverageOff = coverageOffset();

            if (coverageOff < headerEnd || coverageOff >= fData.size())
                return false;

            for (uint16_t i = 0; i < count; ++i)
            {
                uint16_t offset = 0;

                if (!readUInt16(10 + size_t(i) * 2, offset))
                    return false;

                if (offset < headerEnd || offset >= fData.size())
                    return false;
            }

            return true;
        }

        [[nodiscard]]
        bool isFormat2Valid() const noexcept
        {
            if (fData.size() < 16)
                return false;

            size_t valueSize1 = 0;
            size_t valueSize2 = 0;

            if (!valueSizes(valueSize1, valueSize2))
                return false;

            const uint16_t count1 = class1Count();
            const uint16_t count2 = class2Count();

            if (count1 == 0 || count2 == 0)
                return false;

            const size_t recordSize = valueSize1 + valueSize2;
            const uint64_t recordCount =
                uint64_t(count1) * uint64_t(count2);
            const uint64_t matrixBytes =
                recordCount * uint64_t(recordSize);

            if (matrixBytes >
                uint64_t(std::numeric_limits<size_t>::max()) - 16u)
            {
                return false;
            }

            const size_t recordsEnd =
                16 + static_cast<size_t>(matrixBytes);

            if (recordsEnd > fData.size())
                return false;

            const uint16_t coverageOff = coverageOffset();
            const uint16_t class1Off = classDef1Offset();
            const uint16_t class2Off = classDef2Offset();

            if (coverageOff < recordsEnd || coverageOff >= fData.size() ||
                class1Off < recordsEnd || class1Off >= fData.size() ||
                class2Off < recordsEnd || class2Off >= fData.size())
            {
                return false;
            }

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs