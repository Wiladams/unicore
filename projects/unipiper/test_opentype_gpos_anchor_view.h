// test_opentype_gpos_anchor_view.h
#pragma once

#include "../unitils/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_anchor_view.h"

namespace waavs
{
    static void appendGposAnchorU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposAnchorS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposAnchorU16(data, static_cast<uint16_t>(value));
    }


    // ====================================================================
    // Format 1.
    // ====================================================================

    static std::vector<uint8_t> makeGposAnchorFormat1(int16_t x, int16_t y)
    {
        std::vector<uint8_t> data;

        appendGposAnchorU16(data, 1);
        appendGposAnchorS16(data, x);
        appendGposAnchorS16(data, y);

        return data;
    }


    // ====================================================================
    // Format 2.
    // ====================================================================

    static std::vector<uint8_t> makeGposAnchorFormat2(
        int16_t x, int16_t y, uint16_t anchorPoint)
    {
        std::vector<uint8_t> data;

        appendGposAnchorU16(data, 2);
        appendGposAnchorS16(data, x);
        appendGposAnchorS16(data, y);
        appendGposAnchorU16(data, anchorPoint);

        return data;
    }


    // ====================================================================
    // Format 3 with synthetic child data.
    //
    // We do not interpret the child data yet. It merely establishes valid
    // relative offsets for the Anchor view.
    // ====================================================================

    static std::vector<uint8_t> makeGposAnchorFormat3(
        int16_t x, int16_t y, bool withXDevice, bool withYDevice)
    {
        std::vector<uint8_t> data;

        appendGposAnchorU16(data, 3);
        appendGposAnchorS16(data, x);
        appendGposAnchorS16(data, y);

        const size_t xOffsetPatch = data.size();
        appendGposAnchorU16(data, 0);

        const size_t yOffsetPatch = data.size();
        appendGposAnchorU16(data, 0);

        if (withXDevice)
        {
            const uint16_t offset = static_cast<uint16_t>(data.size());

            data[xOffsetPatch] = static_cast<uint8_t>(offset >> 8);
            data[xOffsetPatch + 1] = static_cast<uint8_t>(offset);

            // Synthetic child bytes.

            appendGposAnchorU16(data, 0x1111);
            appendGposAnchorU16(data, 0x2222);
            appendGposAnchorU16(data, 0x3333);
        }

        if (withYDevice)
        {
            const uint16_t offset = static_cast<uint16_t>(data.size());

            data[yOffsetPatch] = static_cast<uint8_t>(offset >> 8);
            data[yOffsetPatch + 1] = static_cast<uint8_t>(offset);

            // Synthetic child bytes.

            appendGposAnchorU16(data, 0x4444);
            appendGposAnchorU16(data, 0x5555);
            appendGposAnchorU16(data, 0x6666);
        }

        return data;
    }


    static bool testOpenTypeGposAnchorView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Anchor view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Format 1.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposAnchorFormat1(189, -103);

            const OpenTypeGposAnchorView anchor(
                ByteSpan(data.data(), data.size()));

            if (!anchor ||
                anchor.format() != 1 ||
                anchor.xCoordinate() != 189 ||
                anchor.yCoordinate() != -103)
            {
                return fail("case 1 Format 1");
            }

            if (anchor.hasContourPoint() ||
                anchor.hasDeviceOffsets())
            {
                return fail("case 1 unexpected refinement");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 2.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposAnchorFormat2(322, 900, 13);

            const OpenTypeGposAnchorView anchor(
                ByteSpan(data.data(), data.size()));

            if (!anchor ||
                anchor.format() != 2 ||
                anchor.xCoordinate() != 322 ||
                anchor.yCoordinate() != 900 ||
                !anchor.hasContourPoint())
            {
                return fail("case 2 Format 2");
            }

            uint16_t anchorPoint = 0;

            if (!anchor.anchorPoint(anchorPoint) ||
                anchorPoint != 13)
            {
                return fail("case 2 anchorPoint");
            }

            if (anchor.hasDeviceOffsets())
                return fail("case 2 unexpected Device offsets");

            ++passed;
        }


        // ====================================================================
        // Case 3 - Format 3 with X and Y Device references.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposAnchorFormat3(279, 1301, true, true);

            const OpenTypeGposAnchorView anchor(
                ByteSpan(data.data(), data.size()));

            if (!anchor ||
                anchor.format() != 3 ||
                anchor.xCoordinate() != 279 ||
                anchor.yCoordinate() != 1301 ||
                !anchor.hasDeviceOffsets())
            {
                return fail("case 3 Format 3");
            }

            uint16_t xOffset = 0;
            uint16_t yOffset = 0;

            if (!anchor.xDeviceOffset(xOffset) ||
                !anchor.yDeviceOffset(yOffset))
            {
                return fail("case 3 Device offsets");
            }

            if (xOffset != 10 || yOffset != 16)
                return fail("case 3 relative offsets");

            const ByteSpan xDevice = anchor.xDeviceData();
            const ByteSpan yDevice = anchor.yDeviceData();

            if (!xDevice || !yDevice ||
                xDevice.begin() != data.data() + xOffset ||
                yDevice.begin() != data.data() + yOffset)
            {
                return fail("case 3 Device data");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Format 3 NULL Device offsets.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposAnchorFormat3(-50, 75, false, false);

            const OpenTypeGposAnchorView anchor(
                ByteSpan(data.data(), data.size()));

            if (!anchor ||
                anchor.xCoordinate() != -50 ||
                anchor.yCoordinate() != 75)
            {
                return fail("case 4 Format 3 NULL");
            }

            uint16_t xOffset = 999;
            uint16_t yOffset = 999;

            if (!anchor.xDeviceOffset(xOffset) ||
                !anchor.yDeviceOffset(yOffset) ||
                xOffset != 0 ||
                yOffset != 0)
            {
                return fail("case 4 NULL offsets");
            }

            if (anchor.xDeviceData() || anchor.yDeviceData())
                return fail("case 4 NULL Device data");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Signed coordinate range.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposAnchorFormat1(-32768, 32767);

            const OpenTypeGposAnchorView anchor(
                ByteSpan(data.data(), data.size()));

            if (!anchor ||
                anchor.xCoordinate() != -32768 ||
                anchor.yCoordinate() != 32767)
            {
                return fail("case 5 signed coordinates");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Anchor point zero is valid.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposAnchorFormat2(10, 20, 0);

            const OpenTypeGposAnchorView anchor(
                ByteSpan(data.data(), data.size()));

            uint16_t anchorPoint = 999;

            if (!anchor ||
                !anchor.anchorPoint(anchorPoint) ||
                anchorPoint != 0)
            {
                return fail("case 6 anchorPoint zero");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Unsupported and truncated formats.
        // ====================================================================

        {
            ++cases;

            const uint8_t unsupportedBytes[] =
            {
                0x00, 0x04,
                0x00, 0x00,
                0x00, 0x00
            };

            const OpenTypeGposAnchorView unsupported(
                ByteSpan(unsupportedBytes, sizeof(unsupportedBytes)));

            if (unsupported)
                return fail("case 7 unsupported format");

            const uint8_t truncated1Bytes[] =
            {
                0x00, 0x01,
                0x00, 0x0A
            };

            const OpenTypeGposAnchorView truncated1(
                ByteSpan(truncated1Bytes, sizeof(truncated1Bytes)));

            if (truncated1)
                return fail("case 7 truncated Format 1");

            const uint8_t truncated2Bytes[] =
            {
                0x00, 0x02,
                0x00, 0x0A,
                0x00, 0x14
            };

            const OpenTypeGposAnchorView truncated2(
                ByteSpan(truncated2Bytes, sizeof(truncated2Bytes)));

            if (truncated2)
                return fail("case 7 truncated Format 2");

            const uint8_t truncated3Bytes[] =
            {
                0x00, 0x03,
                0x00, 0x0A,
                0x00, 0x14,
                0x00, 0x00
            };

            const OpenTypeGposAnchorView truncated3(
                ByteSpan(truncated3Bytes, sizeof(truncated3Bytes)));

            if (truncated3)
                return fail("case 7 truncated Format 3");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Invalid Format 3 child offsets.
        // ====================================================================

        {
            ++cases;


            // Offset points back into the Anchor header.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x03,
                    0x00, 0x0A,
                    0x00, 0x14,
                    0x00, 0x06,
                    0x00, 0x00,
                    0x00, 0x00
                };

                const OpenTypeGposAnchorView anchor(
                    ByteSpan(bytes, sizeof(bytes)));

                if (anchor)
                    return fail("case 8 Device offset inside header");
            }


            // Offset points beyond available data.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x03,
                    0x00, 0x0A,
                    0x00, 0x14,
                    0x00, 0x20,
                    0x00, 0x00,
                    0x00, 0x00
                };

                const OpenTypeGposAnchorView anchor(
                    ByteSpan(bytes, sizeof(bytes)));

                if (anchor)
                    return fail("case 8 Device offset out of range");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Anchor view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 coordinates:      PASS\n"
            "  Format 2 contour point:    PASS\n"
            "  Format 3 Device offsets:   PASS\n"
            "  NULL Device offsets:       PASS\n"
            "  Signed coordinates:        PASS\n"
            "  Anchor point zero:         PASS\n"
            "  Format validation:         PASS\n"
            "  Offset validation:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs