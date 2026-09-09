// opentype_gpos_value_record.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "glyph_placement.h"

namespace waavs
{
    // ====================================================================
    // ValueFormat
    // ====================================================================

    static constexpr uint16_t kOpenTypeGposValueXPlacement = 0x0001u;
    static constexpr uint16_t kOpenTypeGposValueYPlacement = 0x0002u;
    static constexpr uint16_t kOpenTypeGposValueXAdvance = 0x0004u;
    static constexpr uint16_t kOpenTypeGposValueYAdvance = 0x0008u;
    static constexpr uint16_t kOpenTypeGposValueXPlaDevice = 0x0010u;
    static constexpr uint16_t kOpenTypeGposValueYPlaDevice = 0x0020u;
    static constexpr uint16_t kOpenTypeGposValueXAdvDevice = 0x0040u;
    static constexpr uint16_t kOpenTypeGposValueYAdvDevice = 0x0080u;

    static constexpr uint16_t kOpenTypeGposValueDefinedMask = 0x00FFu;
    static constexpr uint16_t kOpenTypeGposValueReservedMask = 0xFF00u;


    // ====================================================================
    // OpenTypeGposValueRecord
    //
    // Decoded ValueRecord.
    //
    // valueFormat records which fields were actually present. Fields not
    // present in the encoded record remain zero.
    //
    // Device offsets may refer to either a Device table or VariationIndex
    // table. They are relative to the immediate parent positioning table,
    // not to the ValueRecord itself.
    // ====================================================================

    struct OpenTypeGposValueRecord
    {
        uint16_t valueFormat{ 0 };

        int16_t xPlacement{ 0 };
        int16_t yPlacement{ 0 };
        int16_t xAdvance{ 0 };
        int16_t yAdvance{ 0 };

        uint16_t xPlaDeviceOffset{ 0 };
        uint16_t yPlaDeviceOffset{ 0 };
        uint16_t xAdvDeviceOffset{ 0 };
        uint16_t yAdvDeviceOffset{ 0 };

        [[nodiscard]] bool has(uint16_t mask) const noexcept
        {
            return (valueFormat & mask) != 0;
        }

        [[nodiscard]] bool hasDeviceOffsets() const noexcept
        {
            return (valueFormat & 0x00F0u) != 0;
        }
    };


    // ====================================================================
    // openTypeGposValueRecordSize
    //
    // Every defined ValueFormat bit contributes one 16-bit field.
    //
    // ValueFormat 0 is valid and produces a zero-byte ValueRecord.
    // Reserved bits make the format invalid.
    // ====================================================================

    [[nodiscard]]
    static inline bool openTypeGposValueRecordSize(uint16_t valueFormat, size_t& result) noexcept
    {
        result = 0;

        if ((valueFormat & kOpenTypeGposValueReservedMask) != 0)
            return false;

        uint16_t bits = valueFormat & kOpenTypeGposValueDefinedMask;

        while (bits != 0)
        {
            result += 2;
            bits &= static_cast<uint16_t>(bits - 1u);
        }

        return true;
    }


    // ====================================================================
    // readOpenTypeGposValueRecord
    //
    // Decode one ValueRecord from the current stream position.
    //
    // The operation is transactional:
    //
    //   - on success, stream advances by the encoded ValueRecord size
    //   - on failure, stream and result remain unchanged
    // ====================================================================

    [[nodiscard]]
    static inline bool readOpenTypeGposValueRecord(
        OpenTypeByteStream& stream, uint16_t valueFormat,
        OpenTypeGposValueRecord& result) noexcept
    {
        size_t recordSize = 0;

        if (!openTypeGposValueRecordSize(valueFormat, recordSize))
            return false;

        if (recordSize > stream.remaining())
            return false;

        OpenTypeByteStream working = stream;
        OpenTypeGposValueRecord value{};
        value.valueFormat = valueFormat;

        if ((valueFormat & kOpenTypeGposValueXPlacement) != 0 &&
            !working.readInt16(value.xPlacement))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueYPlacement) != 0 &&
            !working.readInt16(value.yPlacement))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueXAdvance) != 0 &&
            !working.readInt16(value.xAdvance))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueYAdvance) != 0 &&
            !working.readInt16(value.yAdvance))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueXPlaDevice) != 0 &&
            !working.readOffset16(value.xPlaDeviceOffset))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueYPlaDevice) != 0 &&
            !working.readOffset16(value.yPlaDeviceOffset))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueXAdvDevice) != 0 &&
            !working.readOffset16(value.xAdvDeviceOffset))
        {
            return false;
        }

        if ((valueFormat & kOpenTypeGposValueYAdvDevice) != 0 &&
            !working.readOffset16(value.yAdvDeviceOffset))
        {
            return false;
        }

        stream = working;
        result = value;

        return true;
    }


    // ====================================================================
    // applyOpenTypeGposValueRecord
    //
    // Apply the design-unit adjustments directly encoded in a ValueRecord.
    //
    // Device / VariationIndex adjustments are deliberately not applied here.
    // Those require the immediate parent table and shaping-instance state.
    // ====================================================================

    static inline void applyOpenTypeGposValueRecord(
        const OpenTypeGposValueRecord& value, GlyphPlacement& placement) noexcept
    {
        placement.offsetX += static_cast<int32_t>(value.xPlacement);
        placement.offsetY += static_cast<int32_t>(value.yPlacement);
        placement.advanceX += static_cast<int32_t>(value.xAdvance);
        placement.advanceY += static_cast<int32_t>(value.yAdvance);
    }

} // namespace waavs