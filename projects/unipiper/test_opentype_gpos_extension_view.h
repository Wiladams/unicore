// test_opentype_gpos_extension_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_extension_view.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGposExtensionU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposExtensionU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static std::vector<uint8_t> makeGposExtension(
        uint16_t lookupType, uint32_t offset = 8,
        uint16_t payload = 0x1234)
    {
        std::vector<uint8_t> data;

        appendGposExtensionU16(data, 1);
        appendGposExtensionU16(data, lookupType);
        appendGposExtensionU32(data, offset);

        if (offset > data.size())
            data.resize(offset, 0);

        appendGposExtensionU16(data, payload);

        return data;
    }


    static bool readGposExtensionPayload(ByteSpan data, uint16_t& value)
    {
        value = 0;

        OpenTypeByteStream stream(data);
        return stream.readUInt16(value);
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposExtensionView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Extension view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - Basic Format 1 geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposExtension(1, 8, 0x1234);

            const OpenTypeGposExtensionPosView extension(
                ByteSpan(data.data(), data.size()));

            if (!extension ||
                extension.format() != 1 ||
                extension.extensionLookupType() != 1 ||
                extension.extensionOffset() != 8)
            {
                return fail("case 1 geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Extension subtable.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposExtension(2, 8, 0xBEEF);

            const OpenTypeGposExtensionPosView extension(
                ByteSpan(data.data(), data.size()));

            const ByteSpan subtable =
                extension.extensionSubtable();

            uint16_t payload = 0;

            if (!subtable ||
                !readGposExtensionPayload(subtable, payload) ||
                payload != 0xBEEFu)
            {
                return fail("case 2 extension subtable");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - GPOS Type 7 and Type 8 are valid targets.
        //
        // This is the important difference from GSUB ExtensionSubst.
        // ================================================================

        {
            ++cases;

            {
                const std::vector<uint8_t> data =
                    makeGposExtension(7);

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(data.data(), data.size()));

                if (!extension ||
                    extension.extensionLookupType() != 7)
                {
                    return fail("case 3 Type 7 target");
                }
            }

            {
                const std::vector<uint8_t> data =
                    makeGposExtension(8);

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(data.data(), data.size()));

                if (!extension ||
                    extension.extensionLookupType() != 8)
                {
                    return fail("case 3 Type 8 target");
                }
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Real Offset32 behavior.
        //
        // Use an offset greater than 65535 so this cannot accidentally be
        // implemented using Offset16.
        // ================================================================

        {
            ++cases;

            constexpr uint32_t offset = 0x00010008u;

            const std::vector<uint8_t> data =
                makeGposExtension(4, offset, 0xCAFE);

            const OpenTypeGposExtensionPosView extension(
                ByteSpan(data.data(), data.size()));

            if (!extension ||
                extension.extensionOffset() != offset)
            {
                return fail("case 4 Offset32");
            }

            const ByteSpan subtable =
                extension.extensionSubtable();

            uint16_t payload = 0;

            if (!subtable ||
                !readGposExtensionPayload(subtable, payload) ||
                payload != 0xCAFEu)
            {
                return fail("case 4 Offset32 target");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Invalid extension lookup types.
        // ================================================================

        {
            ++cases;

            {
                const std::vector<uint8_t> data =
                    makeGposExtension(0);

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(data.data(), data.size()));

                if (extension)
                    return fail("case 5 Type 0 accepted");
            }

            {
                const std::vector<uint8_t> data =
                    makeGposExtension(9);

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(data.data(), data.size()));

                if (extension)
                    return fail("case 5 recursive Type 9 accepted");
            }

            {
                const std::vector<uint8_t> data =
                    makeGposExtension(10);

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(data.data(), data.size()));

                if (extension)
                    return fail("case 5 unknown Type 10 accepted");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Unsupported format.
        // ================================================================

        {
            ++cases;

            const uint8_t bytes[] =
            {
                0x00, 0x02,
                0x00, 0x01,
                0x00, 0x00, 0x00, 0x08,
                0x12, 0x34
            };

            const OpenTypeGposExtensionPosView extension(
                ByteSpan(bytes, sizeof(bytes)));

            if (extension)
                return fail("case 6 unsupported format");

            ++passed;
        }


        // ================================================================
        // Case 7 - Truncated headers.
        // ================================================================

        {
            ++cases;

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01
                };

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(bytes, sizeof(bytes)));

                if (extension)
                    return fail("case 7 truncated format");
            }

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x01
                };

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(bytes, sizeof(bytes)));

                if (extension)
                    return fail("case 7 missing Offset32");
            }

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x01,
                    0x00, 0x00, 0x00
                };

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(bytes, sizeof(bytes)));

                if (extension)
                    return fail("case 7 truncated Offset32");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Invalid offsets.
        // ================================================================

        {
            ++cases;


            // Offset inside header.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x01,
                    0x00, 0x00, 0x00, 0x06,
                    0x12, 0x34
                };

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(bytes, sizeof(bytes)));

                if (extension)
                    return fail("case 8 header offset accepted");
            }


            // Offset exactly at end.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x01,
                    0x00, 0x00, 0x00, 0x08
                };

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(bytes, sizeof(bytes)));

                if (extension)
                    return fail("case 8 end offset accepted");
            }


            // Offset beyond end.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x01,
                    0x00, 0x00, 0x00, 0x20,
                    0x12, 0x34
                };

                const OpenTypeGposExtensionPosView extension(
                    ByteSpan(bytes, sizeof(bytes)));

                if (extension)
                    return fail("case 8 beyond-end offset accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Extension view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 geometry:          PASS\n"
            "  Extension subtable:         PASS\n"
            "  Type 7/8 targets:           PASS\n"
            "  Offset32:                   PASS\n"
            "  Lookup type validation:     PASS\n"
            "  Format validation:          PASS\n"
            "  Truncated headers:          PASS\n"
            "  Offset validation:          PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs