// test_opentype_gsub_multiple_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_multiple_view.h"

namespace waavs
{
    static void appendGsubMultipleTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubMultipleTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeGsubMultipleCoverage1TestData()
    {
        std::vector<uint8_t> data;

        // MultipleSubst Format 1
        //
        // glyph 10 -> 100, 101
        // glyph 20 -> 200
        // glyph 30 -> 300, 301, 302

        appendGsubMultipleTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubMultipleTestU16(data, 0);

        appendGsubMultipleTestU16(data, 3);

        const size_t sequence0Patch = data.size();
        appendGsubMultipleTestU16(data, 0);

        const size_t sequence1Patch = data.size();
        appendGsubMultipleTestU16(data, 0);

        const size_t sequence2Patch = data.size();
        appendGsubMultipleTestU16(data, 0);


        // Sequence 0

        const size_t sequence0Offset = data.size();
        patchGsubMultipleTestU16(data, sequence0Patch, static_cast<uint16_t>(sequence0Offset));

        appendGsubMultipleTestU16(data, 2);
        appendGsubMultipleTestU16(data, 100);
        appendGsubMultipleTestU16(data, 101);


        // Sequence 1

        const size_t sequence1Offset = data.size();
        patchGsubMultipleTestU16(data, sequence1Patch, static_cast<uint16_t>(sequence1Offset));

        appendGsubMultipleTestU16(data, 1);
        appendGsubMultipleTestU16(data, 200);


        // Sequence 2

        const size_t sequence2Offset = data.size();
        patchGsubMultipleTestU16(data, sequence2Patch, static_cast<uint16_t>(sequence2Offset));

        appendGsubMultipleTestU16(data, 3);
        appendGsubMultipleTestU16(data, 300);
        appendGsubMultipleTestU16(data, 301);
        appendGsubMultipleTestU16(data, 302);


        // Coverage Format 1

        const size_t coverageOffset = data.size();
        patchGsubMultipleTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubMultipleTestU16(data, 1);
        appendGsubMultipleTestU16(data, 3);
        appendGsubMultipleTestU16(data, 10);
        appendGsubMultipleTestU16(data, 20);
        appendGsubMultipleTestU16(data, 30);

        return data;
    }


    static std::vector<uint8_t> makeGsubMultipleCoverage2TestData()
    {
        std::vector<uint8_t> data;

        // glyph 40 -> 500
        // glyph 41 -> 600, 601
        // glyph 50 -> 700

        appendGsubMultipleTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubMultipleTestU16(data, 0);

        appendGsubMultipleTestU16(data, 3);

        const size_t sequence0Patch = data.size();
        appendGsubMultipleTestU16(data, 0);

        const size_t sequence1Patch = data.size();
        appendGsubMultipleTestU16(data, 0);

        const size_t sequence2Patch = data.size();
        appendGsubMultipleTestU16(data, 0);


        const size_t sequence0Offset = data.size();
        patchGsubMultipleTestU16(data, sequence0Patch, static_cast<uint16_t>(sequence0Offset));

        appendGsubMultipleTestU16(data, 1);
        appendGsubMultipleTestU16(data, 500);


        const size_t sequence1Offset = data.size();
        patchGsubMultipleTestU16(data, sequence1Patch, static_cast<uint16_t>(sequence1Offset));

        appendGsubMultipleTestU16(data, 2);
        appendGsubMultipleTestU16(data, 600);
        appendGsubMultipleTestU16(data, 601);


        const size_t sequence2Offset = data.size();
        patchGsubMultipleTestU16(data, sequence2Patch, static_cast<uint16_t>(sequence2Offset));

        appendGsubMultipleTestU16(data, 1);
        appendGsubMultipleTestU16(data, 700);


        // Coverage Format 2

        const size_t coverageOffset = data.size();
        patchGsubMultipleTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubMultipleTestU16(data, 2);
        appendGsubMultipleTestU16(data, 2);

        // 40..41 -> indices 0..1

        appendGsubMultipleTestU16(data, 40);
        appendGsubMultipleTestU16(data, 41);
        appendGsubMultipleTestU16(data, 0);

        // 50 -> index 2

        appendGsubMultipleTestU16(data, 50);
        appendGsubMultipleTestU16(data, 50);
        appendGsubMultipleTestU16(data, 2);

        return data;
    }


    static bool testOpenTypeGsubMultipleView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB MultipleSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // MultipleSubst structure and Sequence offsets.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubMultipleCoverage1TestData();
            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            if (!multiple)
                return fail("case 1 MultipleSubst invalid");

            if (multiple.format() != 1 || multiple.sequenceCount() != 3)
                return fail("case 1 header");

            uint16_t offset0 = 0;
            uint16_t offset1 = 0;
            uint16_t offset2 = 0;

            if (!multiple.sequenceOffset(0, offset0) ||
                !multiple.sequenceOffset(1, offset1) ||
                !multiple.sequenceOffset(2, offset2))
            {
                return fail("case 1 sequence offsets");
            }

            if (offset0 != 12 || offset1 != 18 || offset2 != 22)
                return fail("case 1 unexpected sequence offsets");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Sequence access and glyph order.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubMultipleCoverage1TestData();
            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            const OpenTypeGsubMultipleSequenceView sequence = multiple.sequence(2);

            if (!sequence || sequence.glyphCount() != 3)
                return fail("case 2 sequence");

            const uint16_t expected[] = { 300, 301, 302 };

            for (size_t i = 0; i < 3; ++i)
            {
                uint16_t glyphId = 0;

                if (!sequence.glyphId(i, glyphId) || glyphId != expected[i])
                    return fail("case 2 glyph order");
            }

            uint16_t glyphId = 0;

            if (sequence.glyphId(3, glyphId))
                return fail("case 2 out-of-range glyph accepted");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Coverage Format 1 -> Sequence mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubMultipleCoverage1TestData();
            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            OpenTypeGsubMultipleSequenceView sequence;

            if (!multiple.sequenceForGlyph(10, sequence) || sequence.glyphCount() != 2)
                return fail("case 3 glyph 10");

            uint16_t glyph0 = 0;
            uint16_t glyph1 = 0;

            if (!sequence.glyphId(0, glyph0) || !sequence.glyphId(1, glyph1))
                return fail("case 3 glyph 10 sequence");

            if (glyph0 != 100 || glyph1 != 101)
                return fail("case 3 glyph 10 replacements");


            if (!multiple.sequenceForGlyph(20, sequence) || sequence.glyphCount() != 1)
                return fail("case 3 glyph 20");

            if (!sequence.glyphId(0, glyph0) || glyph0 != 200)
                return fail("case 3 glyph 20 replacement");


            if (multiple.sequenceForGlyph(21, sequence))
                return fail("case 3 uncovered glyph accepted");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Coverage Format 2 -> Sequence mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubMultipleCoverage2TestData();
            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage = multiple.coverage();

            if (!coverage || coverage.format() != 2)
                return fail("case 4 Coverage Format 2");

            OpenTypeGsubMultipleSequenceView sequence;

            if (!multiple.sequenceForGlyph(41, sequence) || sequence.glyphCount() != 2)
                return fail("case 4 glyph 41");

            uint16_t glyph0 = 0;
            uint16_t glyph1 = 0;

            if (!sequence.glyphId(0, glyph0) || !sequence.glyphId(1, glyph1))
                return fail("case 4 glyph sequence");

            if (glyph0 != 600 || glyph1 != 601)
                return fail("case 4 wrong sequence");

            if (!multiple.sequenceForGlyph(50, sequence))
                return fail("case 4 glyph 50");

            if (!sequence.glyphId(0, glyph0) || glyph0 != 700)
                return fail("case 4 glyph 50 replacement");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Lazy Sequence access.
        //
        // Corrupt Sequence 1's offset. The MultipleSubst and unrelated
        // Sequence tables remain usable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubMultipleCoverage1TestData();

            // Sequence offset array begins at byte 6.
            // Sequence 1 offset is at byte 8.

            patchGsubMultipleTestU16(data, 8, 0xFFFF);

            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            if (!multiple)
                return fail("case 5 MultipleSubst rejected lazy Sequence corruption");

            uint16_t offset = 0;

            if (!multiple.sequenceOffset(1, offset) || offset != 0xFFFF)
                return fail("case 5 corrupted offset unavailable");

            if (multiple.sequence(1))
                return fail("case 5 malformed Sequence accepted");

            if (!multiple.sequence(0) || !multiple.sequence(2))
                return fail("case 5 unrelated Sequence damaged");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Lazy Coverage access.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubMultipleCoverage1TestData();

            const OpenTypeGsubMultipleSubstView original(ByteSpan(data.data(), data.size()));
            const uint16_t coverageOffset = original.coverageOffset();

            patchGsubMultipleTestU16(data, coverageOffset, 99);

            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            if (!multiple)
                return fail("case 6 MultipleSubst rejected lazy Coverage corruption");

            if (multiple.coverage())
                return fail("case 6 malformed Coverage accepted");

            if (!multiple.sequence(0))
                return fail("case 6 Sequence unnecessarily affected");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Coverage Index outside Sequence array.
        //
        // Only the affected glyph fails.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubMultipleCoverage2TestData();

            const OpenTypeGsubMultipleSubstView original(ByteSpan(data.data(), data.size()));
            const uint16_t coverageOffset = original.coverageOffset();

            // Coverage Format 2:
            //
            // header                     4
            // first RangeRecord          6
            // second RangeRecord:
            //   startGlyphId             +0
            //   endGlyphId               +2
            //   startCoverageIndex       +4
            //
            // Make glyph 50 produce Coverage Index 9.

            patchGsubMultipleTestU16(data, size_t(coverageOffset) + 14, 9);

            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            OpenTypeGsubMultipleSequenceView sequence;

            if (!multiple.sequenceForGlyph(40, sequence))
                return fail("case 7 unrelated glyph damaged");

            if (multiple.sequenceForGlyph(50, sequence))
                return fail("case 7 invalid Coverage Index accepted");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Empty Sequence is invalid.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubMultipleCoverage1TestData();

            const OpenTypeGsubMultipleSubstView multiple(ByteSpan(data.data(), data.size()));

            uint16_t sequence0Offset = 0;

            if (!multiple.sequenceOffset(0, sequence0Offset))
                return fail("case 8 setup");

            patchGsubMultipleTestU16(data, sequence0Offset, 0);

            const OpenTypeGsubMultipleSubstView modified(ByteSpan(data.data(), data.size()));

            if (!modified)
                return fail("case 8 MultipleSubst damaged");

            if (modified.sequence(0))
                return fail("case 8 empty Sequence accepted");

            if (!modified.sequence(1))
                return fail("case 8 unrelated Sequence damaged");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Structural failure paths.
        // ====================================================================

        {
            ++cases;

            const uint8_t badFormatBytes[] = {
                0x00, 0x02,
                0x00, 0x08,
                0x00, 0x01,
                0x00, 0x06
            };

            const OpenTypeGsubMultipleSubstView badFormat(
                ByteSpan(badFormatBytes, sizeof(badFormatBytes)));

            if (badFormat)
                return fail("case 9 unsupported format accepted");


            const uint8_t truncatedBytes[] = {
                0x00, 0x01,
                0x00, 0x08,
                0x00, 0x02,
                0x00, 0x0A
            };

            const OpenTypeGsubMultipleSubstView truncated(
                ByteSpan(truncatedBytes, sizeof(truncatedBytes)));

            if (truncated)
                return fail("case 9 truncated sequence array accepted");


            const uint8_t badCoverageOffsetBytes[] = {
                0x00, 0x01,
                0xFF, 0xFF,
                0x00, 0x00
            };

            const OpenTypeGsubMultipleSubstView badCoverageOffset(
                ByteSpan(badCoverageOffsetBytes, sizeof(badCoverageOffsetBytes)));

            if (badCoverageOffset)
                return fail("case 9 invalid Coverage offset accepted");

            ++passed;
        }


        std::printf(
            "OpenType GSUB MultipleSubst view: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  MultipleSubst:         PASS\n"
            "  Sequence access:       PASS\n"
            "  Coverage Format 1:     PASS\n"
            "  Coverage Format 2:     PASS\n"
            "  Coverage indices:      PASS\n"
            "  Lazy Sequence access:  PASS\n"
            "  Lazy Coverage access:  PASS\n"
            "  Empty Sequence:        PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs