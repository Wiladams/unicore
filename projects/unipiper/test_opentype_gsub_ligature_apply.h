// test_opentype_gsub_ligature_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    struct GsubLigatureApplyCandidate
    {
        uint16_t ligatureGlyph{ 0 };
        std::vector<uint16_t> components{};
    };

    struct GsubLigatureApplySet
    {
        uint16_t firstGlyph{ 0 };
        std::vector<GsubLigatureApplyCandidate> ligatures{};
    };


    static void appendGsubLigatureApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubLigatureApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeGsubLigatureApplyLookup(const std::vector<GsubLigatureApplySet>& sets)
    {
        std::vector<uint8_t> data;


        // ================================================================
        // LookupType 4
        // ================================================================

        appendGsubLigatureApplyU16(data, 4);
        appendGsubLigatureApplyU16(data, 0);
        appendGsubLigatureApplyU16(data, 1);

        const size_t subtablePatch = data.size();
        appendGsubLigatureApplyU16(data, 0);


        // ================================================================
        // LigatureSubst Format 1
        // ================================================================

        const size_t subtableOffset = data.size();
        patchGsubLigatureApplyU16(data, subtablePatch, static_cast<uint16_t>(subtableOffset));

        appendGsubLigatureApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubLigatureApplyU16(data, 0);

        appendGsubLigatureApplyU16(data, static_cast<uint16_t>(sets.size()));

        std::vector<size_t> setPatches;
        setPatches.reserve(sets.size());

        for (size_t i = 0; i < sets.size(); ++i)
        {
            setPatches.push_back(data.size());
            appendGsubLigatureApplyU16(data, 0);
        }


        // ================================================================
        // LigatureSets
        // ================================================================

        for (size_t setIndex = 0; setIndex < sets.size(); ++setIndex)
        {
            const GsubLigatureApplySet& definition = sets[setIndex];

            const size_t setOffset = data.size();
            patchGsubLigatureApplyU16(
                data,
                setPatches[setIndex],
                static_cast<uint16_t>(setOffset - subtableOffset));

            appendGsubLigatureApplyU16(
                data,
                static_cast<uint16_t>(definition.ligatures.size()));

            std::vector<size_t> ligaturePatches;
            ligaturePatches.reserve(definition.ligatures.size());

            for (size_t i = 0; i < definition.ligatures.size(); ++i)
            {
                ligaturePatches.push_back(data.size());
                appendGsubLigatureApplyU16(data, 0);
            }


            for (size_t i = 0; i < definition.ligatures.size(); ++i)
            {
                const GsubLigatureApplyCandidate& candidate =
                    definition.ligatures[i];

                const size_t ligatureOffset = data.size();

                patchGsubLigatureApplyU16(
                    data,
                    ligaturePatches[i],
                    static_cast<uint16_t>(ligatureOffset - setOffset));

                appendGsubLigatureApplyU16(data, candidate.ligatureGlyph);

                appendGsubLigatureApplyU16(
                    data,
                    static_cast<uint16_t>(candidate.components.size() + 1));

                for (uint16_t component : candidate.components)
                    appendGsubLigatureApplyU16(data, component);
            }
        }


        // ================================================================
        // Coverage Format 1
        //
        // Test definitions are supplied in sorted first-glyph order.
        // ================================================================

        const size_t coverageOffset = data.size();
        patchGsubLigatureApplyU16(
            data,
            coveragePatch,
            static_cast<uint16_t>(coverageOffset - subtableOffset));

        appendGsubLigatureApplyU16(data, 1);
        appendGsubLigatureApplyU16(data, static_cast<uint16_t>(sets.size()));

        for (const GsubLigatureApplySet& set : sets)
            appendGsubLigatureApplyU16(data, set.firstGlyph);

        return data;
    }


    static void appendGsubLigatureApplyGlyph(OpenTypeShapingBuffer& buffer,
        uint32_t glyphId, uint32_t scalarOffset, uint32_t scalarCount)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;
        buffer.pushBack(glyph);
    }


    static bool testOpenTypeGsubLigatureApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB LigatureSubst apply: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Basic N -> 1 contraction and provenance union.
        //
        //   10, 11, 12 -> 500
        //
        // Provenance:
        //
        //   10 -> [2, 3)
        //   11 -> [3, 5)
        //   12 -> [5, 6)
        //
        // Result:
        //
        //   500 -> [2, 6)
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubLigatureApplySet> definitions = {
                {
                    10,
                    {
                        { 500, { 11, 12 } }
                    }
                }
            };

            const std::vector<uint8_t> lookupData =
                makeGsubLigatureApplyLookup(definitions);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubLigatureApplyGlyph(buffer, 10, 2, 1);
            appendGsubLigatureApplyGlyph(buffer, 11, 3, 2);
            appendGsubLigatureApplyGlyph(buffer, 12, 5, 1);
            appendGsubLigatureApplyGlyph(buffer, 50, 6, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 1 application failed");

            if (buffer.size() != 2)
                return fail("case 1 wrong buffer size");

            if (buffer[0].glyphId != 500 || buffer[1].glyphId != 50)
                return fail("case 1 wrong glyph sequence");

            if (buffer[0].scalarOffset != 2 || buffer[0].scalarCount != 4)
                return fail("case 1 wrong ligature provenance");

            if (buffer[1].scalarOffset != 6 || buffer[1].scalarCount != 1)
                return fail("case 1 trailing provenance changed");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Stored Ligature order wins, and the generated ligature is not
        // reprocessed by this same lookup.
        //
        // First glyph 10:
        //
        //   candidate 0: 10, 11     -> 100
        //   candidate 1: 10, 11, 12 -> 500
        //
        // First glyph 100:
        //
        //   100, 12 -> 900
        //
        // Input:
        //
        //   10, 11, 12
        //
        // Correct result:
        //
        //   100, 12
        //
        // Not:
        //
        //   500
        //
        // and not:
        //
        //   900
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubLigatureApplySet> definitions = {
                {
                    10,
                    {
                        { 100, { 11 } },
                        { 500, { 11, 12 } }
                    }
                },
                {
                    100,
                    {
                        { 900, { 12 } }
                    }
                }
            };

            const std::vector<uint8_t> lookupData =
                makeGsubLigatureApplyLookup(definitions);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubLigatureApplyGlyph(buffer, 10, 0, 1);
            appendGsubLigatureApplyGlyph(buffer, 11, 1, 1);
            appendGsubLigatureApplyGlyph(buffer, 12, 2, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 2 application failed");

            if (buffer.size() != 2)
                return fail("case 2 wrong buffer size");

            if (buffer[0].glyphId != 100 || buffer[1].glyphId != 12)
                return fail("case 2 stored order or same-lookup skipping");

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 2)
                return fail("case 2 provenance");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Post-MultipleSubst provenance union.
        //
        // These two glyphs represent the result of a previous MultipleSubst:
        //
        //   200 -> [4, 6)
        //   201 -> [4, 6)
        //
        // Ligature:
        //
        //   200, 201 -> 700
        //
        // The result remains [4, 6), not a scalarCount of 4.
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubLigatureApplySet> definitions = {
                {
                    200,
                    {
                        { 700, { 201 } }
                    }
                }
            };

            const std::vector<uint8_t> lookupData =
                makeGsubLigatureApplyLookup(definitions);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubLigatureApplyGlyph(buffer, 200, 4, 2);
            appendGsubLigatureApplyGlyph(buffer, 201, 4, 2);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 3 application failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 700)
                return fail("case 3 wrong ligature");

            if (buffer[0].scalarOffset != 4 || buffer[0].scalarCount != 2)
                return fail("case 3 provenance was summed instead of unioned");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Transactional failure.
        //
        // The first two glyphs form a valid ligature:
        //
        //   10, 11 -> 500
        //
        // A later glyph has an invalid OpenType glyph ID.
        //
        // The working copy may contract before discovering the bad glyph,
        // but the caller's buffer must remain completely unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubLigatureApplySet> definitions = {
                {
                    10,
                    {
                        { 500, { 11 } }
                    }
                }
            };

            const std::vector<uint8_t> lookupData =
                makeGsubLigatureApplyLookup(definitions);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubLigatureApplyGlyph(buffer, 10, 0, 1);
            appendGsubLigatureApplyGlyph(buffer, 11, 1, 1);
            appendGsubLigatureApplyGlyph(buffer, 0x10000u, 2, 1);

            if (applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 4 invalid glyph accepted");

            if (buffer.size() != 3)
                return fail("case 4 caller buffer contracted");

            if (buffer[0].glyphId != 10 ||
                buffer[1].glyphId != 11 ||
                buffer[2].glyphId != 0x10000u)
            {
                return fail("case 4 caller glyphs changed");
            }

            if (buffer[0].scalarOffset != 0 ||
                buffer[1].scalarOffset != 1 ||
                buffer[2].scalarOffset != 2)
            {
                return fail("case 4 caller provenance changed");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB LigatureSubst apply: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  Buffer contraction:    PASS\n"
            "  Ligature order:        PASS\n"
            "  Same-lookup skipping:  PASS\n"
            "  Provenance union:      PASS\n"
            "  Post-Multiple span:    PASS\n"
            "  Transactional failure: PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs