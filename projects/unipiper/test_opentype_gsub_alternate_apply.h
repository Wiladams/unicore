// test_opentype_gsub_alternate_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    struct GsubAlternateApplySet
    {
        uint16_t inputGlyph{ 0 };
        std::vector<uint16_t> alternates{};
    };


    static void appendGsubAlternateApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubAlternateApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static size_t appendGsubAlternateApplySubtable(std::vector<uint8_t>& data,
        const std::vector<GsubAlternateApplySet>& sets)
    {
        const size_t subtableOffset = data.size();

        appendGsubAlternateApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubAlternateApplyU16(data, 0);

        appendGsubAlternateApplyU16(data, static_cast<uint16_t>(sets.size()));

        std::vector<size_t> setPatches;
        setPatches.reserve(sets.size());

        for (size_t i = 0; i < sets.size(); ++i)
        {
            setPatches.push_back(data.size());
            appendGsubAlternateApplyU16(data, 0);
        }


        // AlternateSets

        for (size_t i = 0; i < sets.size(); ++i)
        {
            const size_t setOffset = data.size();

            patchGsubAlternateApplyU16(data, setPatches[i],
                static_cast<uint16_t>(setOffset - subtableOffset));

            appendGsubAlternateApplyU16(data, static_cast<uint16_t>(sets[i].alternates.size()));

            for (uint16_t glyphId : sets[i].alternates)
                appendGsubAlternateApplyU16(data, glyphId);
        }


        // Coverage Format 1.
        //
        // Test definitions are supplied in sorted input-glyph order.

        const size_t coverageOffset = data.size();

        patchGsubAlternateApplyU16(data, coveragePatch,
            static_cast<uint16_t>(coverageOffset - subtableOffset));

        appendGsubAlternateApplyU16(data, 1);
        appendGsubAlternateApplyU16(data, static_cast<uint16_t>(sets.size()));

        for (const GsubAlternateApplySet& set : sets)
            appendGsubAlternateApplyU16(data, set.inputGlyph);

        return subtableOffset;
    }


    static std::vector<uint8_t> makeGsubAlternateApplyLookup(
        const std::vector<GsubAlternateApplySet>& sets)
    {
        std::vector<uint8_t> data;

        // LookupType 3

        appendGsubAlternateApplyU16(data, 3);
        appendGsubAlternateApplyU16(data, 0);
        appendGsubAlternateApplyU16(data, 1);

        const size_t subtablePatch = data.size();
        appendGsubAlternateApplyU16(data, 0);

        const size_t subtableOffset =
            appendGsubAlternateApplySubtable(data, sets);

        patchGsubAlternateApplyU16(data, subtablePatch,
            static_cast<uint16_t>(subtableOffset));

        return data;
    }


    static std::vector<uint8_t> makeGsubAlternateTransactionalFailureLookup()
    {
        std::vector<uint8_t> data;

        // LookupType 3 with two subtables.

        appendGsubAlternateApplyU16(data, 3);
        appendGsubAlternateApplyU16(data, 0);
        appendGsubAlternateApplyU16(data, 2);

        const size_t subtable0Patch = data.size();
        appendGsubAlternateApplyU16(data, 0);

        const size_t subtable1Patch = data.size();
        appendGsubAlternateApplyU16(data, 0);


        // Valid first subtable:
        //
        // 10 -> 100

        const size_t subtable0Offset = appendGsubAlternateApplySubtable(
            data,
            {
                { 10, { 100 } }
            });

        patchGsubAlternateApplyU16(data, subtable0Patch,
            static_cast<uint16_t>(subtable0Offset));


        // Malformed second subtable.
        //
        // Format 99 is invalid for AlternateSubst.

        const size_t subtable1Offset = data.size();

        patchGsubAlternateApplyU16(data, subtable1Patch,
            static_cast<uint16_t>(subtable1Offset));

        appendGsubAlternateApplyU16(data, 99);

        return data;
    }


    static void appendGsubAlternateApplyGlyph(OpenTypeShapingBuffer& buffer,
        uint32_t glyphId, uint32_t scalarOffset, uint32_t scalarCount)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;
        buffer.pushBack(glyph);
    }


    static bool testOpenTypeGsubAlternateApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB AlternateSubst apply: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // First alternate is selected.
        //
        //   10 -> { 100, 101, 102 }
        //   20 -> { 200, 201 }
        //
        // Result:
        //
        //   10, 20 -> 100, 200
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubAlternateApplyLookup(
                    {
                        { 10, { 100, 101, 102 } },
                        { 20, { 200, 201 } }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubAlternateApplyGlyph(buffer, 10, 0, 1);
            appendGsubAlternateApplyGlyph(buffer, 20, 1, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 1 application failed");

            if (buffer.size() != 2)
                return fail("case 1 buffer size");

            if (buffer[0].glyphId != 100 || buffer[1].glyphId != 200)
                return fail("case 1 first alternate not selected");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Provenance remains unchanged.
        //
        // AlternateSubst is 1 -> 1.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubAlternateApplyLookup(
                    {
                        { 40, { 400 } }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubAlternateApplyGlyph(buffer, 40, 7, 3);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 2 application failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 400)
                return fail("case 2 replacement");

            if (buffer[0].scalarOffset != 7 || buffer[0].scalarCount != 3)
                return fail("case 2 provenance changed");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Generated alternate is not reprocessed by the same lookup.
        //
        //   10  -> 100
        //   100 -> 900
        //
        // Input 10 must become 100, not 900.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubAlternateApplyLookup(
                    {
                        { 10, { 100 } },
                        { 100, { 900 } }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubAlternateApplyGlyph(buffer, 10, 4, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 3 application failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 100)
                return fail("case 3 generated glyph reprocessed");

            if (buffer[0].scalarOffset != 4 || buffer[0].scalarCount != 1)
                return fail("case 3 provenance");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Transactional failure.
        //
        // First glyph successfully substitutes through subtable 0:
        //
        //   10 -> 100
        //
        // The second glyph does not match subtable 0 and therefore reaches
        // malformed subtable 1.
        //
        // The entire lookup must fail without changing the caller's buffer.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubAlternateTransactionalFailureLookup();

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubAlternateApplyGlyph(buffer, 10, 0, 1);
            appendGsubAlternateApplyGlyph(buffer, 20, 1, 2);

            if (applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 4 malformed subtable accepted");

            if (buffer.size() != 2)
                return fail("case 4 caller buffer size changed");

            if (buffer[0].glyphId != 10 || buffer[1].glyphId != 20)
                return fail("case 4 caller glyphs changed");

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 1 ||
                buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 2)
            {
                return fail("case 4 caller provenance changed");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB AlternateSubst apply: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  First alternate:       PASS\n"
            "  Provenance:            PASS\n"
            "  Same-lookup skipping:  PASS\n"
            "  Transactional failure: PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs