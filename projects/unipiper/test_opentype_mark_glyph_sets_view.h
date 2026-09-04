// test_opentype_mark_glyph_sets_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_mark_glyph_sets_view.h"

namespace waavs
{
    static void appendMarkGlyphSetsTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendMarkGlyphSetsTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchMarkGlyphSetsTestU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // MarkGlyphSetsDef
    //
    // Set 0, Coverage Format 1:
    //
    //   10, 20, 30
    //
    // Set 1, Coverage Format 2:
    //
    //   40..42
    //   50
    // ====================================================================

    static std::vector<uint8_t> makeMarkGlyphSetsTestData()
    {
        std::vector<uint8_t> data;

        appendMarkGlyphSetsTestU16(data, 1);
        appendMarkGlyphSetsTestU16(data, 2);

        const size_t coverage0Patch = data.size();
        appendMarkGlyphSetsTestU32(data, 0);

        const size_t coverage1Patch = data.size();
        appendMarkGlyphSetsTestU32(data, 0);


        // ================================================================
        // Coverage 0 - Format 1
        // ================================================================

        const size_t coverage0Offset = data.size();

        patchMarkGlyphSetsTestU32(
            data,
            coverage0Patch,
            static_cast<uint32_t>(coverage0Offset));

        appendMarkGlyphSetsTestU16(data, 1);
        appendMarkGlyphSetsTestU16(data, 3);
        appendMarkGlyphSetsTestU16(data, 10);
        appendMarkGlyphSetsTestU16(data, 20);
        appendMarkGlyphSetsTestU16(data, 30);


        // ================================================================
        // Coverage 1 - Format 2
        // ================================================================

        const size_t coverage1Offset = data.size();

        patchMarkGlyphSetsTestU32(
            data,
            coverage1Patch,
            static_cast<uint32_t>(coverage1Offset));

        appendMarkGlyphSetsTestU16(data, 2);
        appendMarkGlyphSetsTestU16(data, 2);

        // 40..42 -> Coverage indices 0..2

        appendMarkGlyphSetsTestU16(data, 40);
        appendMarkGlyphSetsTestU16(data, 42);
        appendMarkGlyphSetsTestU16(data, 0);

        // 50 -> Coverage index 3

        appendMarkGlyphSetsTestU16(data, 50);
        appendMarkGlyphSetsTestU16(data, 50);
        appendMarkGlyphSetsTestU16(data, 3);

        return data;
    }


    static bool testOpenTypeMarkGlyphSetsView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType MarkGlyphSetsDef view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Header and Offset32 array.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeMarkGlyphSetsTestData();

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            if (!sets)
                return fail("case 1 MarkGlyphSetsDef invalid");

            if (sets.format() != 1 ||
                sets.size() != 2 ||
                sets.markGlyphSetCount() != 2)
            {
                return fail("case 1 header");
            }

            uint32_t offset0 = 0;
            uint32_t offset1 = 0;

            if (!sets.coverageOffset(0, offset0) ||
                !sets.coverageOffset(1, offset1))
            {
                return fail("case 1 Coverage offsets");
            }

            if (offset0 == 0 || offset1 <= offset0)
                return fail("case 1 unexpected Coverage offsets");

            uint32_t dummy = 0;

            if (sets.coverageOffset(2, dummy))
                return fail("case 1 out-of-range Coverage offset accepted");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Coverage Format 1 access.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeMarkGlyphSetsTestData();

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage =
                sets.coverage(0);

            if (!coverage || coverage.format() != 1)
                return fail("case 2 Coverage Format 1");

            if (!coverage.contains(10) ||
                !coverage.contains(20) ||
                !coverage.contains(30))
            {
                return fail("case 2 expected glyph missing");
            }

            if (coverage.contains(11))
                return fail("case 2 unexpected glyph present");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Coverage Format 2 access.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeMarkGlyphSetsTestData();

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage =
                sets.coverage(1);

            if (!coverage || coverage.format() != 2)
                return fail("case 3 Coverage Format 2");

            if (!coverage.contains(40) ||
                !coverage.contains(41) ||
                !coverage.contains(42) ||
                !coverage.contains(50))
            {
                return fail("case 3 expected glyph missing");
            }

            if (coverage.contains(43) ||
                coverage.contains(49))
            {
                return fail("case 3 unexpected glyph present");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Membership helper distinguishes membership from structural failure.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeMarkGlyphSetsTestData();

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            bool member = false;

            if (!sets.contains(0, 20, member) || !member)
                return fail("case 4 set 0 member");

            if (!sets.contains(0, 25, member) || member)
                return fail("case 4 set 0 non-member");

            if (!sets.contains(1, 41, member) || !member)
                return fail("case 4 set 1 member");

            if (sets.contains(2, 41, member))
                return fail("case 4 invalid set index accepted");

            if (sets.contains(0, 0x10000u, member))
                return fail("case 4 oversized glyph accepted");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Lazy Coverage offset corruption.
        //
        // Corrupt only Set 1. Parent and Set 0 remain usable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                makeMarkGlyphSetsTestData();

            // Coverage offset array:
            //
            // format             0
            // count              2
            // coverageOffset[0]  4
            // coverageOffset[1]  8

            patchMarkGlyphSetsTestU32(data, 8, 0xFFFFFFFFu);

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            if (!sets)
                return fail("case 5 parent rejected lazy offset corruption");

            if (!sets.coverage(0))
                return fail("case 5 unrelated Coverage damaged");

            if (sets.coverage(1))
                return fail("case 5 malformed Coverage offset accepted");

            bool member = false;

            if (!sets.contains(0, 10, member) || !member)
                return fail("case 5 unrelated membership damaged");

            if (sets.contains(1, 50, member))
                return fail("case 5 malformed set membership succeeded");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Lazy Coverage content corruption.
        //
        // Coverage offset remains valid, but Coverage format is malformed.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                makeMarkGlyphSetsTestData();

            const OpenTypeMarkGlyphSetsView original(
                ByteSpan(data.data(), data.size()));

            uint32_t coverage1Offset = 0;

            if (!original.coverageOffset(1, coverage1Offset))
                return fail("case 6 setup");

            data[coverage1Offset] = 0;
            data[coverage1Offset + 1] = 99;

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            if (!sets)
                return fail("case 6 parent rejected lazy Coverage corruption");

            if (!sets.coverage(0))
                return fail("case 6 unrelated Coverage damaged");

            if (sets.coverage(1))
                return fail("case 6 malformed Coverage accepted");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Real Offset32.
        //
        // Put one Coverage beyond 64K to prove Offset32 is preserved.
        // ====================================================================

        {
            ++cases;

            static constexpr uint32_t kCoverageOffset =
                0x00010020u;

            std::vector<uint8_t> data;

            appendMarkGlyphSetsTestU16(data, 1);
            appendMarkGlyphSetsTestU16(data, 1);
            appendMarkGlyphSetsTestU32(data, kCoverageOffset);

            data.resize(kCoverageOffset, 0);

            appendMarkGlyphSetsTestU16(data, 1);
            appendMarkGlyphSetsTestU16(data, 1);
            appendMarkGlyphSetsTestU16(data, 123);

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data.data(), data.size()));

            if (!sets)
                return fail("case 7 large-offset table invalid");

            uint32_t offset = 0;

            if (!sets.coverageOffset(0, offset) ||
                offset != kCoverageOffset)
            {
                return fail("case 7 Offset32 truncated");
            }

            bool member = false;

            if (!sets.contains(0, 123, member) || !member)
                return fail("case 7 large-offset Coverage lookup");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Empty mark glyph set table.
        //
        // Zero sets is structurally valid.
        // ====================================================================

        {
            ++cases;

            const uint8_t data[] = {
                0x00, 0x01,
                0x00, 0x00
            };

            const OpenTypeMarkGlyphSetsView sets(
                ByteSpan(data, sizeof(data)));

            if (!sets)
                return fail("case 8 empty table rejected");

            if (sets.size() != 0)
                return fail("case 8 empty table count");

            if (sets.coverage(0))
                return fail("case 8 nonexistent Coverage exposed");

            bool member = false;

            if (sets.contains(0, 10, member))
                return fail("case 8 nonexistent set accepted");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Structural failure paths.
        // ====================================================================

        {
            ++cases;


            // Unsupported format.

            const uint8_t badFormat[] = {
                0x00, 0x02,
                0x00, 0x00
            };

            if (OpenTypeMarkGlyphSetsView(
                ByteSpan(badFormat, sizeof(badFormat))))
            {
                return fail("case 9 unsupported format accepted");
            }


            // Truncated offset array.

            const uint8_t truncatedOffsets[] = {
                0x00, 0x01,
                0x00, 0x02,
                0x00, 0x00, 0x00, 0x08
            };

            if (OpenTypeMarkGlyphSetsView(
                ByteSpan(truncatedOffsets, sizeof(truncatedOffsets))))
            {
                return fail("case 9 truncated offset array accepted");
            }


            // Coverage offset points into the header/offset array.

            const uint8_t insideHeader[] = {
                0x00, 0x01,
                0x00, 0x01,
                0x00, 0x00, 0x00, 0x04
            };

            const OpenTypeMarkGlyphSetsView inside(
                ByteSpan(insideHeader, sizeof(insideHeader)));

            if (!inside)
                return fail("case 9 parent rejected lazy child offset");

            if (inside.coverage(0))
                return fail("case 9 header-relative Coverage accepted");


            // Truncated header.

            const uint8_t truncatedHeader[] = {
                0x00, 0x01,
                0x00
            };

            if (OpenTypeMarkGlyphSetsView(
                ByteSpan(truncatedHeader, sizeof(truncatedHeader))))
            {
                return fail("case 9 truncated header accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType MarkGlyphSetsDef view: PASS\n"
            "  Cases:                  %u\n"
            "  Passed:                 %u\n"
            "  MarkGlyphSetsDef:       PASS\n"
            "  Coverage Format 1:      PASS\n"
            "  Coverage Format 2:      PASS\n"
            "  Membership:             PASS\n"
            "  Lazy offset access:     PASS\n"
            "  Lazy Coverage access:   PASS\n"
            "  Offset32:               PASS\n"
            "  Empty set list:         PASS\n"
            "  Failure paths:          PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs