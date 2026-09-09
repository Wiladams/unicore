// test_opentype_gpos_value_record.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_value_record.h"

namespace waavs
{
    static void appendGposValueU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposValueS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposValueU16(data, static_cast<uint16_t>(value));
    }


    static bool testOpenTypeGposValueRecord()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS ValueRecord: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - ValueRecord sizes.
        // ====================================================================

        {
            ++cases;

            size_t size = 999;

            if (!openTypeGposValueRecordSize(0x0000u, size) || size != 0)
                return fail("case 1 empty size");

            if (!openTypeGposValueRecordSize(0x0001u, size) || size != 2)
                return fail("case 1 one-field size");

            if (!openTypeGposValueRecordSize(0x0005u, size) || size != 4)
                return fail("case 1 sparse two-field size");

            if (!openTypeGposValueRecordSize(0x000Fu, size) || size != 8)
                return fail("case 1 four-value size");

            if (!openTypeGposValueRecordSize(0x00FFu, size) || size != 16)
                return fail("case 1 complete size");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Empty ValueRecord.
        // ====================================================================

        {
            ++cases;

            const uint8_t bytes[] = { 0xAA, 0xBB };
            OpenTypeByteStream stream(ByteSpan(bytes, sizeof(bytes)));

            OpenTypeGposValueRecord value{};

            if (!readOpenTypeGposValueRecord(stream, 0x0000u, value))
                return fail("case 2 empty record rejected");

            if (stream.remaining() != sizeof(bytes) ||
                value.valueFormat != 0 ||
                value.xPlacement != 0 ||
                value.yPlacement != 0 ||
                value.xAdvance != 0 ||
                value.yAdvance != 0)
            {
                return fail("case 2 empty record");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Single XPlacement.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data;
            appendGposValueS16(data, -125);

            OpenTypeByteStream stream(ByteSpan(data.data(), data.size()));
            OpenTypeGposValueRecord value{};

            if (!readOpenTypeGposValueRecord(
                stream, kOpenTypeGposValueXPlacement, value))
            {
                return fail("case 3 decode");
            }

            if (value.valueFormat != kOpenTypeGposValueXPlacement ||
                value.xPlacement != -125 ||
                value.yPlacement != 0 ||
                value.xAdvance != 0 ||
                value.yAdvance != 0 ||
                stream.remaining() != 0)
            {
                return fail("case 3 XPlacement");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Sparse fields remain tightly packed.
        //
        // Format 0x0005:
        //
        //   XPlacement
        //   XAdvance
        //
        // There is no encoded YPlacement field between them.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendGposValueS16(data, 25);
            appendGposValueS16(data, -60);

            OpenTypeByteStream stream(ByteSpan(data.data(), data.size()));
            OpenTypeGposValueRecord value{};

            if (!readOpenTypeGposValueRecord(stream, 0x0005u, value))
                return fail("case 4 decode");

            if (value.xPlacement != 25 ||
                value.yPlacement != 0 ||
                value.xAdvance != -60 ||
                value.yAdvance != 0)
            {
                return fail("case 4 sparse fields");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - All direct positioning fields.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendGposValueS16(data, 10);
            appendGposValueS16(data, -20);
            appendGposValueS16(data, 30);
            appendGposValueS16(data, -40);

            OpenTypeByteStream stream(ByteSpan(data.data(), data.size()));
            OpenTypeGposValueRecord value{};

            if (!readOpenTypeGposValueRecord(stream, 0x000Fu, value))
                return fail("case 5 decode");

            if (value.xPlacement != 10 ||
                value.yPlacement != -20 ||
                value.xAdvance != 30 ||
                value.yAdvance != -40)
            {
                return fail("case 5 direct values");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Device / VariationIndex offsets.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendGposValueU16(data, 0x0010u);
            appendGposValueU16(data, 0x0020u);
            appendGposValueU16(data, 0x0000u);
            appendGposValueU16(data, 0x0040u);

            OpenTypeByteStream stream(ByteSpan(data.data(), data.size()));
            OpenTypeGposValueRecord value{};

            if (!readOpenTypeGposValueRecord(stream, 0x00F0u, value))
                return fail("case 6 decode");

            if (!value.hasDeviceOffsets() ||
                value.xPlaDeviceOffset != 0x0010u ||
                value.yPlaDeviceOffset != 0x0020u ||
                value.xAdvDeviceOffset != 0x0000u ||
                value.yAdvDeviceOffset != 0x0040u)
            {
                return fail("case 6 device offsets");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Complete ValueRecord.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendGposValueS16(data, 1);
            appendGposValueS16(data, 2);
            appendGposValueS16(data, 3);
            appendGposValueS16(data, 4);

            appendGposValueU16(data, 10);
            appendGposValueU16(data, 20);
            appendGposValueU16(data, 30);
            appendGposValueU16(data, 40);

            OpenTypeByteStream stream(ByteSpan(data.data(), data.size()));
            OpenTypeGposValueRecord value{};

            if (!readOpenTypeGposValueRecord(stream, 0x00FFu, value))
                return fail("case 7 decode");

            if (value.xPlacement != 1 ||
                value.yPlacement != 2 ||
                value.xAdvance != 3 ||
                value.yAdvance != 4 ||
                value.xPlaDeviceOffset != 10 ||
                value.yPlaDeviceOffset != 20 ||
                value.xAdvDeviceOffset != 30 ||
                value.yAdvDeviceOffset != 40 ||
                stream.remaining() != 0)
            {
                return fail("case 7 complete record");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Reserved bits rejected transactionally.
        // ====================================================================

        {
            ++cases;

            const uint8_t bytes[] = { 0x00, 0x25 };
            OpenTypeByteStream stream(ByteSpan(bytes, sizeof(bytes)));

            OpenTypeGposValueRecord value{};
            value.valueFormat = 0x1234u;
            value.xPlacement = 77;

            if (readOpenTypeGposValueRecord(stream, 0x0101u, value))
                return fail("case 8 reserved format accepted");

            if (stream.remaining() != sizeof(bytes) ||
                value.valueFormat != 0x1234u ||
                value.xPlacement != 77)
            {
                return fail("case 8 transactional failure");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Truncated record rejected transactionally.
        // ====================================================================

        {
            ++cases;

            const uint8_t bytes[] = { 0x00, 0x0A };
            OpenTypeByteStream stream(ByteSpan(bytes, sizeof(bytes)));

            OpenTypeGposValueRecord value{};
            value.valueFormat = 0x7777u;
            value.xAdvance = 88;

            if (readOpenTypeGposValueRecord(stream, 0x0005u, value))
                return fail("case 9 truncated record accepted");

            if (stream.remaining() != sizeof(bytes) ||
                value.valueFormat != 0x7777u ||
                value.xAdvance != 88)
            {
                return fail("case 9 transactional failure");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10 - Apply direct adjustments.
        //
        // Device offsets are deliberately ignored by this helper.
        // ====================================================================

        {
            ++cases;

            OpenTypeGposValueRecord value{};
            value.valueFormat = 0x00FFu;

            value.xPlacement = 10;
            value.yPlacement = -20;
            value.xAdvance = 30;
            value.yAdvance = -40;

            value.xPlaDeviceOffset = 100;
            value.yPlaDeviceOffset = 200;
            value.xAdvDeviceOffset = 300;
            value.yAdvDeviceOffset = 400;

            GlyphPlacement placement{};
            placement.advanceX = 500;
            placement.advanceY = 600;
            placement.offsetX = 5;
            placement.offsetY = -5;

            applyOpenTypeGposValueRecord(value, placement);

            if (placement.advanceX != 530 ||
                placement.advanceY != 560 ||
                placement.offsetX != 15 ||
                placement.offsetY != -25)
            {
                return fail("case 10 placement application");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS ValueRecord: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Record sizing:             PASS\n"
            "  Empty record:              PASS\n"
            "  Single field:              PASS\n"
            "  Sparse fields:             PASS\n"
            "  Direct values:             PASS\n"
            "  Device offsets:            PASS\n"
            "  Complete record:           PASS\n"
            "  Reserved bits:             PASS\n"
            "  Truncation:                PASS\n"
            "  Placement application:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs