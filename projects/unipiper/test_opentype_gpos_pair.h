// test_opentype_gpos_pair.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    static void appendGposPairU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposPairS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposPairU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposPairU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposPairCoverage1(
        std::vector<uint8_t>& data,
        const uint16_t* glyphs, uint16_t count)
    {
        appendGposPairU16(data, 1);
        appendGposPairU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposPairU16(data, glyphs[i]);
    }

    static void appendGposPairClassDef2(
        std::vector<uint8_t>& data,
        const uint16_t* starts, const uint16_t* ends,
        const uint16_t* classes, uint16_t count)
    {
        appendGposPairU16(data, 2);
        appendGposPairU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
        {
            appendGposPairU16(data, starts[i]);
            appendGposPairU16(data, ends[i]);
            appendGposPairU16(data, classes[i]);
        }
    }


    // ====================================================================
    // Format 1.
    //
    // First glyph: 10
    //
    // second 20:
    //   first.xAdvance  = -50
    //   second.offsetX  = +5
    //
    // second 30:
    //   first.xAdvance  = -70
    //   second.offsetX  = +7
    // ====================================================================

    static std::vector<uint8_t> makeGposPairFormat1()
    {
        std::vector<uint8_t> data;

        appendGposPairU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposPairU16(data, 0);

        appendGposPairU16(data, kOpenTypeGposValueXAdvance);
        appendGposPairU16(data, kOpenTypeGposValueXPlacement);

        appendGposPairU16(data, 1);

        const size_t pairSetPatch = data.size();
        appendGposPairU16(data, 0);


        // PairSet.

        patchGposPairU16(
            data, pairSetPatch,
            static_cast<uint16_t>(data.size()));

        appendGposPairU16(data, 2);

        appendGposPairU16(data, 20);
        appendGposPairS16(data, -50);
        appendGposPairS16(data, 5);

        appendGposPairU16(data, 30);
        appendGposPairS16(data, -70);
        appendGposPairS16(data, 7);


        // Coverage.

        patchGposPairU16(
            data, coveragePatch,
            static_cast<uint16_t>(data.size()));

        const uint16_t coverageGlyphs[] = { 10 };
        appendGposPairCoverage1(data, coverageGlyphs, 1);

        return data;
    }


    // ====================================================================
    // Format 2.
    //
    // class1:
    //
    //   glyph 10..11 -> class 1
    //
    // class2:
    //
    //   glyph 20 -> class 1
    //   glyph 30 -> class 2
    //
    // Matrix values:
    //
    //   [1][0] = -10 / +1
    //   [1][1] = -20 / +2
    //   [1][2] = -80 / +12
    // ====================================================================

    static std::vector<uint8_t> makeGposPairFormat2()
    {
        std::vector<uint8_t> data;

        appendGposPairU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposPairU16(data, 0);

        appendGposPairU16(data, kOpenTypeGposValueXAdvance);
        appendGposPairU16(data, kOpenTypeGposValueXPlacement);

        const size_t classDef1Patch = data.size();
        appendGposPairU16(data, 0);

        const size_t classDef2Patch = data.size();
        appendGposPairU16(data, 0);

        appendGposPairU16(data, 2);
        appendGposPairU16(data, 3);


        // Class 0 row.

        for (uint16_t i = 0; i < 3; ++i)
        {
            appendGposPairS16(data, 0);
            appendGposPairS16(data, 0);
        }


        // Class 1 row.

        appendGposPairS16(data, -10);
        appendGposPairS16(data, 1);

        appendGposPairS16(data, -20);
        appendGposPairS16(data, 2);

        appendGposPairS16(data, -80);
        appendGposPairS16(data, 12);


        // Coverage.

        patchGposPairU16(
            data, coveragePatch,
            static_cast<uint16_t>(data.size()));

        const uint16_t coverageGlyphs[] = { 10, 11 };
        appendGposPairCoverage1(data, coverageGlyphs, 2);


        // ClassDef1.

        patchGposPairU16(
            data, classDef1Patch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t starts[] = { 10 };
            const uint16_t ends[] = { 11 };
            const uint16_t classes[] = { 1 };

            appendGposPairClassDef2(
                data, starts, ends, classes, 1);
        }


        // ClassDef2.

        patchGposPairU16(
            data, classDef2Patch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t starts[] = { 20, 30 };
            const uint16_t ends[] = { 20, 30 };
            const uint16_t classes[] = { 1, 2 };

            appendGposPairClassDef2(
                data, starts, ends, classes, 2);
        }

        return data;
    }


    static std::vector<uint8_t> makeGposPairLookup(
        const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposPairU16(data, 2);
        appendGposPairU16(data, lookupFlag);
        appendGposPairU16(data, 1);
        appendGposPairU16(data, 8);

        data.insert(
            data.end(),
            subtable.begin(), subtable.end());

        return data;
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyph 100 is a Mark.
    // ====================================================================

    static std::vector<uint8_t> makeGposPairGdef()
    {
        std::vector<uint8_t> data;

        appendGposPairU16(data, 1);
        appendGposPairU16(data, 0);

        appendGposPairU16(data, 12);
        appendGposPairU16(data, 0);
        appendGposPairU16(data, 0);
        appendGposPairU16(data, 0);

        appendGposPairU16(data, 2);
        appendGposPairU16(data, 1);

        appendGposPairU16(data, 100);
        appendGposPairU16(data, 100);
        appendGposPairU16(data, 3);

        return data;
    }


    static ShapedGlyph makeGposPairGlyph(uint32_t glyphId, int32_t advanceX)
    {
        ShapedGlyph glyph{};
        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;
        glyph.placement.advanceX = advanceX;
        return glyph;
    }


    static bool testOpenTypeGposPair()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Pair Adjustment: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - Format 1 PairSet and binary lookup.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposPairFormat1();

            const OpenTypeGposPairPosView pair(
                ByteSpan(data.data(), data.size()));

            if (!pair ||
                pair.format() != 1 ||
                pair.pairSetCount() != 1)
            {
                return fail("case 1 Format 1 geometry");
            }

            const OpenTypeGposPairSetView set =
                pair.pairSet(0);

            if (!set || set.size() != 2)
                return fail("case 1 PairSet");

            OpenTypeGposValueRecord value1{};
            OpenTypeGposValueRecord value2{};

            if (!set.find(30, value1, value2) ||
                value1.xAdvance != -70 ||
                value2.xPlacement != 7)
            {
                return fail("case 1 PairValueRecord");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Format 1 NoMatch.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposPairFormat1();

            const OpenTypeGposPairPosView pair(
                ByteSpan(data.data(), data.size()));

            OpenTypeGposValueRecord value1{};
            OpenTypeGposValueRecord value2{};

            if (resolveOpenTypeGposPairSubtable(
                pair, 10, 25, value1, value2) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 2 Format 1 NoMatch");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Format 2 class-pair lookup.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposPairFormat2();

            const OpenTypeGposPairPosView pair(
                ByteSpan(data.data(), data.size()));

            if (!pair ||
                pair.format() != 2 ||
                pair.class1Count() != 2 ||
                pair.class2Count() != 3)
            {
                return fail("case 3 Format 2 geometry");
            }

            const OpenTypeClassDefView classDef1 =
                pair.classDef1();

            const OpenTypeClassDefView classDef2 =
                pair.classDef2();

            if (!classDef1 || !classDef2)
                return fail("case 3 ClassDefs");

            OpenTypeGposValueRecord value1{};
            OpenTypeGposValueRecord value2{};

            if (resolveOpenTypeGposPairSubtable(
                pair, 10, 30, value1, value2) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 3 resolve");
            }

            if (value1.xAdvance != -80 ||
                value2.xPlacement != 12)
            {
                return fail("case 3 class pair values");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Format 2 Class 0.
        //
        // Glyph 99 is absent from ClassDef2 and therefore class 0.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposPairFormat2();

            const OpenTypeGposPairPosView pair(
                ByteSpan(data.data(), data.size()));

            OpenTypeGposValueRecord value1{};
            OpenTypeGposValueRecord value2{};

            if (resolveOpenTypeGposPairSubtable(
                pair, 10, 99, value1, value2) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 4 Class 0 resolve");
            }

            if (value1.xAdvance != -10 ||
                value2.xPlacement != 1)
            {
                return fail("case 4 Class 0 values");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Exact application.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposPairFormat1();

            const std::vector<uint8_t> lookupData =
                makeGposPairLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposPairGlyph(10, 600));
            buffer.pushBack(makeGposPairGlyph(20, 500));

            const OpenTypeGdefView gdef{};

            size_t resume = 0;

            const OpenTypeGposResolveResult result =
                applyOpenTypeGposPairLookupAt(
                    lookup, gdef, buffer, 0, &resume);

            if (result != OpenTypeGposResolveResult::Match)
                return fail("case 5 result");

            if (buffer[0].placement.advanceX != 550 ||
                buffer[1].placement.offsetX != 5)
            {
                return fail("case 5 placement");
            }

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[1].shaping.glyphId != 20)
            {
                return fail("case 5 identity");
            }

            if (resume != 2)
                return fail("case 5 resume");

            ++passed;
        }


        // ================================================================
        // Case 6 - LookupFlag filtered second glyph.
        //
        // Physical:
        //
        //   10 mark 20
        //
        // IgnoreMarks makes the participating pair:
        //
        //   10 20
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> gdefData =
                makeGposPairGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            if (!gdef)
                return fail("case 6 GDEF");

            const std::vector<uint8_t> subtable =
                makeGposPairFormat1();

            const std::vector<uint8_t> lookupData =
                makeGposPairLookup(subtable, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposPairGlyph(10, 600));
            buffer.pushBack(makeGposPairGlyph(100, 0));
            buffer.pushBack(makeGposPairGlyph(20, 500));

            OpenTypeGposPairMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposPairLookup(
                    lookup, filter, buffer, 0, match);

            if (result != OpenTypeGposResolveResult::Match)
                return fail("case 6 filtered result");

            if (match.firstIndex != 0 ||
                match.secondIndex != 2)
            {
                return fail("case 6 filtered positions");
            }

            if (applyOpenTypeGposPairLookupAt(
                lookup, gdef, buffer, 0) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 6 filtered application");
            }

            if (buffer[0].placement.advanceX != 550 ||
                buffer[1].placement.advanceX != 0 ||
                buffer[1].placement.offsetX != 0 ||
                buffer[2].placement.offsetX != 5)
            {
                return fail("case 6 filtered placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Whole lookup.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposPairFormat1();

            const std::vector<uint8_t> lookupData =
                makeGposPairLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposPairGlyph(10, 600));
            buffer.pushBack(makeGposPairGlyph(20, 500));
            buffer.pushBack(makeGposPairGlyph(40, 700));

            if (!applyOpenTypeGposPairLookup(
                lookup, buffer))
            {
                return fail("case 7 whole lookup");
            }

            if (buffer[0].placement.advanceX != 550 ||
                buffer[1].placement.offsetX != 5 ||
                buffer[2].placement.advanceX != 700)
            {
                return fail("case 7 whole placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Failure paths.
        // ================================================================

        {
            ++cases;


            // Unsupported format.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x03
                };

                const OpenTypeGposPairPosView pair(
                    ByteSpan(bytes, sizeof(bytes)));

                if (pair)
                    return fail("case 8 unsupported format");
            }


            // Reserved ValueFormat.

            {
                std::vector<uint8_t> data =
                    makeGposPairFormat1();

                patchGposPairU16(data, 4, 0x0100u);

                const OpenTypeGposPairPosView pair(
                    ByteSpan(data.data(), data.size()));

                if (pair)
                    return fail("case 8 reserved ValueFormat");
            }


            // Unsorted PairSet.

            {
                std::vector<uint8_t> data =
                    makeGposPairFormat1();

                // PairSet begins at offset 12.
                //
                // PairValueRecord secondGlyphs occur at:
                //
                //   14 -> 20
                //   20 -> 30
                //
                // Make second record smaller than first.

                patchGposPairU16(data, 20, 15);

                const OpenTypeGposPairPosView pair(
                    ByteSpan(data.data(), data.size()));

                if (!pair)
                    return fail("case 8 parent unexpectedly invalid");

                if (pair.pairSet(0))
                    return fail("case 8 unsorted PairSet accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Pair Adjustment: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 PairSet:          PASS\n"
            "  Format 1 NoMatch:          PASS\n"
            "  Format 2 classes:          PASS\n"
            "  Class 0:                   PASS\n"
            "  Exact application:         PASS\n"
            "  LookupFlag filtering:      PASS\n"
            "  Whole lookup:              PASS\n"
            "  Failure paths:             PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs