// test_opentype_gpos_single.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    static void appendGposSingleU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposSingleS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposSingleU16(data, static_cast<uint16_t>(value));
    }


    static std::vector<uint8_t> makeGposSingleCoverage(
        std::initializer_list<uint16_t> glyphs)
    {
        std::vector<uint8_t> data;

        appendGposSingleU16(data, 1);
        appendGposSingleU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyphId : glyphs)
            appendGposSingleU16(data, glyphId);

        return data;
    }


    // ====================================================================
    // Format 1.
    //
    // valueFormat = XPlacement | XAdvance = 0x0005
    // ====================================================================

    static std::vector<uint8_t> makeGposSingleFormat1(
        int16_t xPlacement, int16_t xAdvance,
        std::initializer_list<uint16_t> glyphs)
    {
        std::vector<uint8_t> data;

        const uint16_t coverageOffset = 10;

        appendGposSingleU16(data, 1);
        appendGposSingleU16(data, coverageOffset);
        appendGposSingleU16(data, 0x0005u);

        appendGposSingleS16(data, xPlacement);
        appendGposSingleS16(data, xAdvance);

        const std::vector<uint8_t> coverage =
            makeGposSingleCoverage(glyphs);

        data.insert(data.end(), coverage.begin(), coverage.end());

        return data;
    }


    // ====================================================================
    // Format 2.
    //
    // valueFormat = XPlacement | XAdvance = 0x0005
    // ====================================================================

    static std::vector<uint8_t> makeGposSingleFormat2()
    {
        std::vector<uint8_t> data;

        // Header 8 bytes + 3 records * 4 bytes = 20.

        appendGposSingleU16(data, 2);
        appendGposSingleU16(data, 20);
        appendGposSingleU16(data, 0x0005u);
        appendGposSingleU16(data, 3);

        // glyph 10
        appendGposSingleS16(data, 10);
        appendGposSingleS16(data, 100);

        // glyph 20
        appendGposSingleS16(data, 20);
        appendGposSingleS16(data, 200);

        // glyph 30
        appendGposSingleS16(data, 30);
        appendGposSingleS16(data, 300);

        const std::vector<uint8_t> coverage =
            makeGposSingleCoverage({ 10, 20, 30 });

        data.insert(data.end(), coverage.begin(), coverage.end());

        return data;
    }


    static std::vector<uint8_t> makeGposSingleLookup(
        const std::vector<uint8_t>& subtable)
    {
        std::vector<uint8_t> data;

        appendGposSingleU16(data, 1); // LookupType
        appendGposSingleU16(data, 0); // LookupFlag
        appendGposSingleU16(data, 1); // SubTableCount
        appendGposSingleU16(data, 8); // Subtable offset

        data.insert(data.end(), subtable.begin(), subtable.end());

        return data;
    }


    static std::vector<uint8_t> makeGposSingleTwoSubtableLookup(
        const std::vector<uint8_t>& first,
        const std::vector<uint8_t>& second)
    {
        std::vector<uint8_t> data;

        // Lookup header:
        //
        // type
        // flag
        // count
        // offset[0]
        // offset[1]
        //
        // 10 bytes total.

        appendGposSingleU16(data, 1);
        appendGposSingleU16(data, 0);
        appendGposSingleU16(data, 2);

        appendGposSingleU16(data, 10);
        appendGposSingleU16(
            data,
            static_cast<uint16_t>(10 + first.size()));

        data.insert(data.end(), first.begin(), first.end());
        data.insert(data.end(), second.begin(), second.end());

        return data;
    }


    static ShapedGlyph makeGposSingleGlyph(
        uint32_t glyphId, uint32_t scalarOffset,
        uint32_t scalarCount, int32_t advanceX)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = scalarOffset;
        glyph.shaping.scalarCount = scalarCount;

        glyph.placement.advanceX = advanceX;

        return glyph;
    }


    static bool testOpenTypeGposSingle()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Single Adjustment: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Format 1 view.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposSingleFormat1(25, -40, { 10, 20, 30 });

            const OpenTypeGposSinglePosView single(
                ByteSpan(data.data(), data.size()));

            if (!single ||
                single.format() != 1 ||
                single.valueFormat() != 0x0005u ||
                single.valueCount() != 1)
            {
                return fail("case 1 Format 1 geometry");
            }

            const OpenTypeCoverageView coverage = single.coverage();

            if (!coverage)
                return fail("case 1 Coverage");

            uint16_t coverageIndex = 0;

            if (!coverage.find(20, coverageIndex) || coverageIndex != 1)
                return fail("case 1 Coverage index");

            OpenTypeGposValueRecord value{};

            if (!single.valueRecordForCoverageIndex(coverageIndex, value) ||
                value.xPlacement != 25 ||
                value.xAdvance != -40)
            {
                return fail("case 1 ValueRecord");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 2 selects by Coverage index.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposSingleFormat2();

            const OpenTypeGposSinglePosView single(
                ByteSpan(data.data(), data.size()));

            if (!single ||
                single.format() != 2 ||
                single.valueCount() != 3)
            {
                return fail("case 2 Format 2 geometry");
            }

            const OpenTypeCoverageView coverage = single.coverage();

            if (!coverage)
                return fail("case 2 Coverage");

            uint16_t coverageIndex = 0;

            if (!coverage.find(20, coverageIndex) || coverageIndex != 1)
                return fail("case 2 Coverage index");

            OpenTypeGposValueRecord value{};

            if (!single.valueRecordForCoverageIndex(coverageIndex, value) ||
                value.xPlacement != 20 ||
                value.xAdvance != 200)
            {
                return fail("case 2 indexed ValueRecord");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Exact Format 1 application.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposSingleFormat1(15, -25, { 20 });

            const std::vector<uint8_t> lookupData =
                makeGposSingleLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            if (!lookup)
                return fail("case 3 lookup");

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposSingleGlyph(20, 7, 3, 600));

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(
                    lookup, buffer, 0);

            if (result != OpenTypeGposResolveResult::Match)
                return fail("case 3 result");

            if (buffer[0].placement.advanceX != 575 ||
                buffer[0].placement.offsetX != 15)
            {
                return fail("case 3 placement");
            }

            // Identity and provenance must not change.

            if (buffer[0].shaping.glyphId != 20 ||
                buffer[0].shaping.scalarOffset != 7 ||
                buffer[0].shaping.scalarCount != 3)
            {
                return fail("case 3 provenance");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Exact Format 2 application.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposSingleFormat2();

            const std::vector<uint8_t> lookupData =
                makeGposSingleLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposSingleGlyph(30, 0, 1, 500));

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(
                    lookup, buffer, 0);

            if (result != OpenTypeGposResolveResult::Match)
                return fail("case 4 result");

            if (buffer[0].placement.offsetX != 30 ||
                buffer[0].placement.advanceX != 800)
            {
                return fail("case 4 Format 2 placement");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - No Coverage match leaves placement unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposSingleFormat1(50, 75, { 10 });

            const std::vector<uint8_t> lookupData =
                makeGposSingleLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposSingleGlyph(20, 0, 1, 600));

            buffer[0].placement.offsetX = 12;

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(
                    lookup, buffer, 0);

            if (result != OpenTypeGposResolveResult::NoMatch)
                return fail("case 5 result");

            if (buffer[0].placement.advanceX != 600 ||
                buffer[0].placement.offsetX != 12)
            {
                return fail("case 5 mutation");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Adjustments accumulate.
        //
        // GPOS modifies nominal placement rather than replacing it.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposSingleFormat1(-10, 35, { 40 });

            const std::vector<uint8_t> lookupData =
                makeGposSingleLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposSingleGlyph(40, 0, 1, 700));

            buffer[0].placement.offsetX = 5;
            buffer[0].placement.offsetY = -8;

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(
                    lookup, buffer, 0);

            if (result != OpenTypeGposResolveResult::Match)
                return fail("case 6 result");

            if (buffer[0].placement.advanceX != 735 ||
                buffer[0].placement.offsetX != -5 ||
                buffer[0].placement.offsetY != -8)
            {
                return fail("case 6 accumulated placement");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - First matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> first =
                makeGposSingleFormat1(10, 20, { 50 });

            const std::vector<uint8_t> second =
                makeGposSingleFormat1(100, 200, { 50 });

            const std::vector<uint8_t> lookupData =
                makeGposSingleTwoSubtableLookup(first, second);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposSingleGlyph(50, 0, 1, 500));

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(
                    lookup, buffer, 0);

            if (result != OpenTypeGposResolveResult::Match)
                return fail("case 7 result");

            if (buffer[0].placement.offsetX != 10 ||
                buffer[0].placement.advanceX != 520)
            {
                return fail("case 7 subtable order");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Malformed subtable fails transactionally.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> bad;

            // Format 2 says three ValueRecords exist, but none follow.

            appendGposSingleU16(bad, 2);
            appendGposSingleU16(bad, 8);
            appendGposSingleU16(bad, 0x0005u);
            appendGposSingleU16(bad, 3);

            const std::vector<uint8_t> lookupData =
                makeGposSingleLookup(bad);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposSingleGlyph(10, 4, 2, 600));

            buffer[0].placement.offsetX = 9;

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(
                    lookup, buffer, 0);

            if (result != OpenTypeGposResolveResult::Invalid)
                return fail("case 8 malformed result");

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[0].shaping.scalarOffset != 4 ||
                buffer[0].shaping.scalarCount != 2 ||
                buffer[0].placement.advanceX != 600 ||
                buffer[0].placement.offsetX != 9)
            {
                return fail("case 8 transactional failure");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Out-of-range exact position.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposSingleFormat1(10, 20, { 10 });

            const std::vector<uint8_t> lookupData =
                makeGposSingleLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            if (applyOpenTypeGposSingleLookupAt(
                lookup, buffer, 0) != OpenTypeGposResolveResult::Invalid)
            {
                return fail("case 9 out-of-range position");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Single Adjustment: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 view:             PASS\n"
            "  Format 2 view:             PASS\n"
            "  Format 1 application:      PASS\n"
            "  Format 2 application:      PASS\n"
            "  Coverage no-match:         PASS\n"
            "  Placement accumulation:    PASS\n"
            "  Subtable order:            PASS\n"
            "  Transactional failure:     PASS\n"
            "  Position bounds:           PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs