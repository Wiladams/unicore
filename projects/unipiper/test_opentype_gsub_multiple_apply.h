// test_opentype_gsub_multiple_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    static void appendGsubMultipleApplyTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubMultipleApplyTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeGsubMultipleApplyLookup(
        uint16_t inputGlyph, const std::vector<uint16_t>& replacements)
    {
        std::vector<uint8_t> data;

        // LookupType 2, one subtable.

        appendGsubMultipleApplyTestU16(data, 2);
        appendGsubMultipleApplyTestU16(data, 0);
        appendGsubMultipleApplyTestU16(data, 1);

        const size_t subtablePatch = data.size();
        appendGsubMultipleApplyTestU16(data, 0);


        const size_t subtableOffset = data.size();
        patchGsubMultipleApplyTestU16(data, subtablePatch, static_cast<uint16_t>(subtableOffset));

        // MultipleSubst Format 1.

        appendGsubMultipleApplyTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubMultipleApplyTestU16(data, 0);

        appendGsubMultipleApplyTestU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubMultipleApplyTestU16(data, 0);


        // Sequence.

        const size_t sequenceOffset = data.size() - subtableOffset;
        patchGsubMultipleApplyTestU16(data, sequencePatch, static_cast<uint16_t>(sequenceOffset));

        appendGsubMultipleApplyTestU16(data, static_cast<uint16_t>(replacements.size()));

        for (uint16_t glyphId : replacements)
            appendGsubMultipleApplyTestU16(data, glyphId);


        // Coverage Format 1.

        const size_t coverageOffset = data.size() - subtableOffset;
        patchGsubMultipleApplyTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubMultipleApplyTestU16(data, 1);
        appendGsubMultipleApplyTestU16(data, 1);
        appendGsubMultipleApplyTestU16(data, inputGlyph);

        return data;
    }


    static std::vector<uint8_t> makeGsubSingleApplyLookup(
        const std::vector<std::pair<uint16_t, uint16_t>>& substitutions)
    {
        std::vector<uint8_t> data;

        // LookupType 1.
        //
        // One SingleSubst subtable per substitution.

        appendGsubMultipleApplyTestU16(data, 1);
        appendGsubMultipleApplyTestU16(data, 0);
        appendGsubMultipleApplyTestU16(data, static_cast<uint16_t>(substitutions.size()));

        std::vector<size_t> subtablePatches;

        for (size_t i = 0; i < substitutions.size(); ++i)
        {
            subtablePatches.push_back(data.size());
            appendGsubMultipleApplyTestU16(data, 0);
        }


        for (size_t i = 0; i < substitutions.size(); ++i)
        {
            const size_t subtableOffset = data.size();
            patchGsubMultipleApplyTestU16(
                data, subtablePatches[i], static_cast<uint16_t>(subtableOffset));

            const uint16_t inputGlyph = substitutions[i].first;
            const uint16_t outputGlyph = substitutions[i].second;

            // SingleSubst Format 1.

            appendGsubMultipleApplyTestU16(data, 1);
            appendGsubMultipleApplyTestU16(data, 6);

            const uint16_t delta =
                static_cast<uint16_t>(uint32_t(outputGlyph) - uint32_t(inputGlyph));

            appendGsubMultipleApplyTestU16(data, delta);

            // Coverage Format 1.

            appendGsubMultipleApplyTestU16(data, 1);
            appendGsubMultipleApplyTestU16(data, 1);
            appendGsubMultipleApplyTestU16(data, inputGlyph);
        }

        return data;
    }


    static OpenTypeShapingBuffer makeGsubMultipleApplyTestBuffer()
    {
        OpenTypeShapingBuffer buffer;

        OpenTypeShapingGlyph glyph0{};
        glyph0.glyphId = 10;
        glyph0.scalarOffset = 0;
        glyph0.scalarCount = 1;
        buffer.pushBack(glyph0);

        OpenTypeShapingGlyph glyph1{};
        glyph1.glyphId = 20;
        glyph1.scalarOffset = 1;
        glyph1.scalarCount = 2;
        buffer.pushBack(glyph1);

        OpenTypeShapingGlyph glyph2{};
        glyph2.glyphId = 30;
        glyph2.scalarOffset = 3;
        glyph2.scalarCount = 1;
        buffer.pushBack(glyph2);

        return buffer;
    }


    static bool testOpenTypeGsubMultipleApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB MultipleSubst apply: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Buffer expansion and provenance.
        //
        //   20 -> 200, 201, 202
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubMultipleApplyLookup(20, { 200, 201, 202 });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer = makeGsubMultipleApplyTestBuffer();

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 1 application failed");

            if (buffer.size() != 5)
                return fail("case 1 wrong buffer size");

            const uint32_t expectedGlyphs[] = {
                10, 200, 201, 202, 30
            };

            for (size_t i = 0; i < 5; ++i)
            {
                if (buffer[i].glyphId != expectedGlyphs[i])
                    return fail("case 1 wrong glyph sequence");
            }

            for (size_t i = 1; i <= 3; ++i)
            {
                if (buffer[i].scalarOffset != 1 || buffer[i].scalarCount != 2)
                    return fail("case 1 replacement provenance");
            }

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 1)
                return fail("case 1 leading provenance");

            if (buffer[4].scalarOffset != 3 || buffer[4].scalarCount != 1)
                return fail("case 1 trailing provenance");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Generated glyphs are not reprocessed by the same lookup.
        //
        // Subtable 0:
        //   20 -> 21, 22
        //
        // Subtable 1:
        //   21 -> 99
        //
        // A correct single lookup leaves 21 alone after it was generated.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> lookupData;

            appendGsubMultipleApplyTestU16(lookupData, 2);
            appendGsubMultipleApplyTestU16(lookupData, 0);
            appendGsubMultipleApplyTestU16(lookupData, 2);

            const size_t subtable0Patch = lookupData.size();
            appendGsubMultipleApplyTestU16(lookupData, 0);

            const size_t subtable1Patch = lookupData.size();
            appendGsubMultipleApplyTestU16(lookupData, 0);


            // Subtable 0: 20 -> 21, 22.

            const size_t subtable0Offset = lookupData.size();
            patchGsubMultipleApplyTestU16(
                lookupData, subtable0Patch, static_cast<uint16_t>(subtable0Offset));

            appendGsubMultipleApplyTestU16(lookupData, 1);

            const size_t coverage0Patch = lookupData.size();
            appendGsubMultipleApplyTestU16(lookupData, 0);

            appendGsubMultipleApplyTestU16(lookupData, 1);

            const size_t sequence0Patch = lookupData.size();
            appendGsubMultipleApplyTestU16(lookupData, 0);

            const size_t sequence0Offset = lookupData.size() - subtable0Offset;
            patchGsubMultipleApplyTestU16(
                lookupData, sequence0Patch, static_cast<uint16_t>(sequence0Offset));

            appendGsubMultipleApplyTestU16(lookupData, 2);
            appendGsubMultipleApplyTestU16(lookupData, 21);
            appendGsubMultipleApplyTestU16(lookupData, 22);

            const size_t coverage0Offset = lookupData.size() - subtable0Offset;
            patchGsubMultipleApplyTestU16(
                lookupData, coverage0Patch, static_cast<uint16_t>(coverage0Offset));

            appendGsubMultipleApplyTestU16(lookupData, 1);
            appendGsubMultipleApplyTestU16(lookupData, 1);
            appendGsubMultipleApplyTestU16(lookupData, 20);


            // Subtable 1: 21 -> 99.

            const size_t subtable1Offset = lookupData.size();
            patchGsubMultipleApplyTestU16(
                lookupData, subtable1Patch, static_cast<uint16_t>(subtable1Offset));

            appendGsubMultipleApplyTestU16(lookupData, 1);

            const size_t coverage1Patch = lookupData.size();
            appendGsubMultipleApplyTestU16(lookupData, 0);

            appendGsubMultipleApplyTestU16(lookupData, 1);

            const size_t sequence1Patch = lookupData.size();
            appendGsubMultipleApplyTestU16(lookupData, 0);

            const size_t sequence1Offset = lookupData.size() - subtable1Offset;
            patchGsubMultipleApplyTestU16(
                lookupData, sequence1Patch, static_cast<uint16_t>(sequence1Offset));

            appendGsubMultipleApplyTestU16(lookupData, 1);
            appendGsubMultipleApplyTestU16(lookupData, 99);

            const size_t coverage1Offset = lookupData.size() - subtable1Offset;
            patchGsubMultipleApplyTestU16(
                lookupData, coverage1Patch, static_cast<uint16_t>(coverage1Offset));

            appendGsubMultipleApplyTestU16(lookupData, 1);
            appendGsubMultipleApplyTestU16(lookupData, 1);
            appendGsubMultipleApplyTestU16(lookupData, 21);


            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            OpenTypeShapingGlyph glyph{};
            glyph.glyphId = 20;
            glyph.scalarOffset = 4;
            glyph.scalarCount = 1;
            buffer.pushBack(glyph);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 2 application failed");

            if (buffer.size() != 2)
                return fail("case 2 wrong buffer size");

            if (buffer[0].glyphId != 21 || buffer[1].glyphId != 22)
                return fail("case 2 generated glyph reprocessed");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Later lookup may operate on MultipleSubst output.
        //
        // Lookup A:
        //   20 -> 200, 201
        //
        // Lookup B:
        //   200 -> 300
        //   201 -> 301
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> multipleData =
                makeGsubMultipleApplyLookup(20, { 200, 201 });

            const std::vector<uint8_t> singleData =
                makeGsubSingleApplyLookup({
                    { 200, 300 },
                    { 201, 301 }
                    });

            const OpenTypeLayoutLookupView multipleLookup(
                ByteSpan(multipleData.data(), multipleData.size()));

            const OpenTypeLayoutLookupView singleLookup(
                ByteSpan(singleData.data(), singleData.size()));

            OpenTypeShapingBuffer buffer;

            OpenTypeShapingGlyph glyph{};
            glyph.glyphId = 20;
            glyph.scalarOffset = 6;
            glyph.scalarCount = 3;
            buffer.pushBack(glyph);


            if (!applyOpenTypeGsubLookup(multipleLookup, buffer))
                return fail("case 3 MultipleSubst failed");

            if (buffer.size() != 2 ||
                buffer[0].glyphId != 200 ||
                buffer[1].glyphId != 201)
            {
                return fail("case 3 intermediate sequence");
            }


            if (!applyOpenTypeGsubLookup(singleLookup, buffer))
                return fail("case 3 later lookup failed");

            if (buffer[0].glyphId != 300 || buffer[1].glyphId != 301)
                return fail("case 3 lookup feed-through");

            for (size_t i = 0; i < 2; ++i)
            {
                if (buffer[i].scalarOffset != 6 || buffer[i].scalarCount != 3)
                    return fail("case 3 provenance");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB MultipleSubst apply: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  Buffer expansion:      PASS\n"
            "  Provenance:            PASS\n"
            "  Same-lookup skipping:  PASS\n"
            "  Later lookup feed:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs