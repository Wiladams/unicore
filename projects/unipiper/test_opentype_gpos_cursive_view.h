// test_opentype_gpos_cursive_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_cursive_view.h"

namespace waavs
{
    static void appendGposCursiveU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposCursiveS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposCursiveU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposCursiveU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposCursiveAnchor1(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposCursiveU16(data, 1);
        appendGposCursiveS16(data, x);
        appendGposCursiveS16(data, y);
    }

    static void appendGposCursiveCoverage1(
        std::vector<uint8_t>& data, const uint16_t* glyphs, uint16_t count)
    {
        appendGposCursiveU16(data, 1);
        appendGposCursiveU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposCursiveU16(data, glyphs[i]);
    }


    // ====================================================================
    // Synthetic CursivePos.
    //
    // Coverage:
    //
    //   index 0 -> glyph 10
    //   index 1 -> glyph 20
    //   index 2 -> glyph 30
    //
    // Glyph 10:
    //   entry = NULL
    //   exit  = (450, 20)
    //
    // Glyph 20:
    //   entry = (50, 100)
    //   exit  = (430, -10)
    //
    // Glyph 30:
    //   entry = (30, 80)
    //   exit  = NULL
    //
    // This forms a natural chain:
    //
    //   10 -> 20 -> 30
    // ====================================================================

    static std::vector<uint8_t> makeGposCursiveViewFormat1()
    {
        std::vector<uint8_t> data;

        appendGposCursiveU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposCursiveU16(data, 0);

        appendGposCursiveU16(data, 3);


        // Glyph 10 EntryExitRecord.

        appendGposCursiveU16(data, 0);

        const size_t glyph10ExitPatch = data.size();
        appendGposCursiveU16(data, 0);


        // Glyph 20 EntryExitRecord.

        const size_t glyph20EntryPatch = data.size();
        appendGposCursiveU16(data, 0);

        const size_t glyph20ExitPatch = data.size();
        appendGposCursiveU16(data, 0);


        // Glyph 30 EntryExitRecord.

        const size_t glyph30EntryPatch = data.size();
        appendGposCursiveU16(data, 0);

        appendGposCursiveU16(data, 0);


        // Coverage.

        patchGposCursiveU16(
            data, coveragePatch,
            static_cast<uint16_t>(data.size()));

        const uint16_t glyphs[] = { 10, 20, 30 };
        appendGposCursiveCoverage1(data, glyphs, 3);


        // Glyph 10 exit.

        patchGposCursiveU16(
            data, glyph10ExitPatch,
            static_cast<uint16_t>(data.size()));

        appendGposCursiveAnchor1(data, 450, 20);


        // Glyph 20 entry.

        patchGposCursiveU16(
            data, glyph20EntryPatch,
            static_cast<uint16_t>(data.size()));

        appendGposCursiveAnchor1(data, 50, 100);


        // Glyph 20 exit.

        patchGposCursiveU16(
            data, glyph20ExitPatch,
            static_cast<uint16_t>(data.size()));

        appendGposCursiveAnchor1(data, 430, -10);


        // Glyph 30 entry.

        patchGposCursiveU16(
            data, glyph30EntryPatch,
            static_cast<uint16_t>(data.size()));

        appendGposCursiveAnchor1(data, 30, 80);

        return data;
    }


    static bool testOpenTypeGposCursiveView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS CursivePos view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Format 1 geometry.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposCursiveViewFormat1();

            const OpenTypeGposCursivePosView cursive(
                ByteSpan(data.data(), data.size()));

            if (!cursive ||
                cursive.format() != 1 ||
                cursive.entryExitCount() != 3)
            {
                return fail("case 1 Format 1 geometry");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Coverage index mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposCursiveViewFormat1();

            const OpenTypeGposCursivePosView cursive(
                ByteSpan(data.data(), data.size()));

            uint16_t index = 999;

            if (!cursive.coverageIndex(10, index) || index != 0)
                return fail("case 2 glyph 10 Coverage");

            if (!cursive.coverageIndex(20, index) || index != 1)
                return fail("case 2 glyph 20 Coverage");

            if (!cursive.coverageIndex(30, index) || index != 2)
                return fail("case 2 glyph 30 Coverage");

            if (cursive.coverageIndex(40, index))
                return fail("case 2 unexpected Coverage match");

            ++passed;
        }


        // ====================================================================
        // Case 3 - Entry/exit offsets.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposCursiveViewFormat1();

            const OpenTypeGposCursivePosView cursive(
                ByteSpan(data.data(), data.size()));

            uint16_t entryOffset = 0;
            uint16_t exitOffset = 0;

            if (!cursive.entryAnchorOffset(1, entryOffset) ||
                !cursive.exitAnchorOffset(1, exitOffset) ||
                entryOffset == 0 ||
                exitOffset == 0)
            {
                return fail("case 3 glyph 20 offsets");
            }

            if (!cursive.hasEntryAnchor(1) ||
                !cursive.hasExitAnchor(1))
            {
                return fail("case 3 glyph 20 presence");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Anchor contents.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposCursiveViewFormat1();

            const OpenTypeGposCursivePosView cursive(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGposAnchorView exit10 =
                cursive.exitAnchor(0);

            const OpenTypeGposAnchorView entry20 =
                cursive.entryAnchor(1);

            const OpenTypeGposAnchorView exit20 =
                cursive.exitAnchor(1);

            const OpenTypeGposAnchorView entry30 =
                cursive.entryAnchor(2);

            if (!exit10 ||
                exit10.xCoordinate() != 450 ||
                exit10.yCoordinate() != 20)
            {
                return fail("case 4 glyph 10 exit");
            }

            if (!entry20 ||
                entry20.xCoordinate() != 50 ||
                entry20.yCoordinate() != 100)
            {
                return fail("case 4 glyph 20 entry");
            }

            if (!exit20 ||
                exit20.xCoordinate() != 430 ||
                exit20.yCoordinate() != -10)
            {
                return fail("case 4 glyph 20 exit");
            }

            if (!entry30 ||
                entry30.xCoordinate() != 30 ||
                entry30.yCoordinate() != 80)
            {
                return fail("case 4 glyph 30 entry");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - NULL anchors.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposCursiveViewFormat1();

            const OpenTypeGposCursivePosView cursive(
                ByteSpan(data.data(), data.size()));

            if (cursive.hasEntryAnchor(0))
                return fail("case 5 glyph 10 unexpected entry");

            if (cursive.entryAnchor(0))
                return fail("case 5 glyph 10 NULL entry");

            if (cursive.hasExitAnchor(2))
                return fail("case 5 glyph 30 unexpected exit");

            if (cursive.exitAnchor(2))
                return fail("case 5 glyph 30 NULL exit");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Record bounds.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposCursiveViewFormat1();

            const OpenTypeGposCursivePosView cursive(
                ByteSpan(data.data(), data.size()));

            uint16_t offset = 999;

            if (cursive.entryAnchorOffset(3, offset) ||
                cursive.exitAnchorOffset(3, offset))
            {
                return fail("case 6 out-of-range record");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Unsupported and truncated tables.
        // ====================================================================

        {
            ++cases;


            // Unsupported format.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x02,
                    0x00, 0x06,
                    0x00, 0x00
                };

                const OpenTypeGposCursivePosView cursive(
                    ByteSpan(bytes, sizeof(bytes)));

                if (cursive)
                    return fail("case 7 unsupported format");
            }


            // Truncated header.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x06
                };

                const OpenTypeGposCursivePosView cursive(
                    ByteSpan(bytes, sizeof(bytes)));

                if (cursive)
                    return fail("case 7 truncated header");
            }


            // Record count exceeds available EntryExitRecords.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x0A,
                    0x00, 0x02,

                    0x00, 0x00,
                    0x00, 0x00
                };

                const OpenTypeGposCursivePosView cursive(
                    ByteSpan(bytes, sizeof(bytes)));

                if (cursive)
                    return fail("case 7 truncated records");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Invalid child offsets and lazy child validation.
        // ====================================================================

        {
            ++cases;


            // Anchor offset points inside EntryExitRecord array.

            {
                std::vector<uint8_t> data =
                    makeGposCursiveViewFormat1();

                // Glyph 10 exit offset is at byte 8.

                patchGposCursiveU16(data, 8, 10);

                const OpenTypeGposCursivePosView cursive(
                    ByteSpan(data.data(), data.size()));

                if (cursive)
                    return fail("case 8 anchor inside record array");
            }


            // Structurally valid child offset, malformed Anchor table.
            //
            // Parent remains valid. Child validation is lazy.

            {
                std::vector<uint8_t> data =
                    makeGposCursiveViewFormat1();

                // Locate glyph 10 exit anchor.

                uint16_t exitOffset =
                    static_cast<uint16_t>(
                        (uint16_t(data[8]) << 8) |
                        uint16_t(data[9]));

                if (exitOffset >= data.size())
                    return fail("case 8 synthetic offset");

                // Change Anchor format from 1 to unsupported format 4.

                data[exitOffset] = 0;
                data[exitOffset + 1] = 4;

                const OpenTypeGposCursivePosView cursive(
                    ByteSpan(data.data(), data.size()));

                if (!cursive)
                    return fail("case 8 parent rejected lazy child");

                if (cursive.exitAnchor(0))
                    return fail("case 8 malformed child accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS CursivePos view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 geometry:         PASS\n"
            "  Coverage mapping:          PASS\n"
            "  Entry/exit offsets:        PASS\n"
            "  Anchor contents:           PASS\n"
            "  NULL anchors:              PASS\n"
            "  Record bounds:             PASS\n"
            "  Format validation:         PASS\n"
            "  Lazy child validation:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs