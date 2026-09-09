// test_opentype_gsub_reverse_chain_single_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "opentype_gsub_reverse_chain_single_view.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGsubReverseChainViewU16(
        std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubReverseChainViewU16(
        std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGsubReverseChainViewCoverage(
        std::vector<uint8_t>& data, std::initializer_list<uint16_t> glyphs)
    {
        appendGsubReverseChainViewU16(data, 1);
        appendGsubReverseChainViewU16(
            data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyphId : glyphs)
            appendGsubReverseChainViewU16(data, glyphId);
    }


    // ====================================================================
    // Main Format 1 subtable.
    //
    // Logical context:
    //
    //   11 12 | 20/21 | 30 31
    //
    // Backtrack storage:
    //
    //   index 0 -> 12
    //   index 1 -> 11
    //
    // Input Coverage:
    //
    //   20 -> coverage index 0 -> 200
    //   21 -> coverage index 1 -> 201
    //
    // Lookahead:
    //
    //   index 0 -> 30
    //   index 1 -> 31
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainViewFormat1()
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainViewU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubReverseChainViewU16(data, 0);


        // Backtrack Coverage array.

        appendGsubReverseChainViewU16(data, 2);

        const size_t backtrack0Patch = data.size();
        appendGsubReverseChainViewU16(data, 0);

        const size_t backtrack1Patch = data.size();
        appendGsubReverseChainViewU16(data, 0);


        // Lookahead Coverage array.

        appendGsubReverseChainViewU16(data, 2);

        const size_t lookahead0Patch = data.size();
        appendGsubReverseChainViewU16(data, 0);

        const size_t lookahead1Patch = data.size();
        appendGsubReverseChainViewU16(data, 0);


        // Substitutes.

        appendGsubReverseChainViewU16(data, 2);
        appendGsubReverseChainViewU16(data, 200);
        appendGsubReverseChainViewU16(data, 201);


        auto appendCoverage =
            [&](size_t patch, std::initializer_list<uint16_t> glyphs)
            {
                const size_t offset = data.size();

                patchGsubReverseChainViewU16(
                    data, patch, static_cast<uint16_t>(offset));

                appendGsubReverseChainViewCoverage(data, glyphs);
            };


        appendCoverage(coveragePatch, { 20, 21 });

        appendCoverage(backtrack0Patch, { 12 });
        appendCoverage(backtrack1Patch, { 11 });

        appendCoverage(lookahead0Patch, { 30 });
        appendCoverage(lookahead1Patch, { 31 });

        return data;
    }


    // ====================================================================
    // Empty backtrack/lookahead.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainViewEmptyContext()
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainViewU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubReverseChainViewU16(data, 0);

        appendGsubReverseChainViewU16(data, 0);
        appendGsubReverseChainViewU16(data, 0);

        appendGsubReverseChainViewU16(data, 1);
        appendGsubReverseChainViewU16(data, 200);

        const size_t coverageOffset = data.size();

        patchGsubReverseChainViewU16(
            data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubReverseChainViewCoverage(data, { 20 });

        return data;
    }


    // ====================================================================
    // Parent geometry is valid, but child Coverage offsets are deliberately
    // invalid. Child validation must remain lazy.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainViewInvalidChildren()
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainViewU16(data, 1);

        // NULL input Coverage.

        appendGsubReverseChainViewU16(data, 0);


        appendGsubReverseChainViewU16(data, 1);

        // NULL backtrack Coverage.

        appendGsubReverseChainViewU16(data, 0);


        appendGsubReverseChainViewU16(data, 1);

        // Out-of-range lookahead Coverage.

        appendGsubReverseChainViewU16(data, 0xFFFFu);


        appendGsubReverseChainViewU16(data, 1);
        appendGsubReverseChainViewU16(data, 200);

        return data;
    }


    static bool testOpenTypeGsubReverseChainSingleView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB ReverseChainSingleSubst view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Parent geometry.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewFormat1();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            if (!subst ||
                subst.format() != 1 ||
                subst.backtrackGlyphCount() != 2 ||
                subst.lookaheadGlyphCount() != 2 ||
                subst.glyphCount() != 2)
            {
                return fail("case 1 parent geometry");
            }

            uint16_t coverageOffset = 0;

            if (!subst.coverageOffset(coverageOffset) || coverageOffset == 0)
                return fail("case 1 input Coverage offset");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Input Coverage and Coverage-index mapping.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewFormat1();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage = subst.coverage();

            if (!coverage)
                return fail("case 2 input Coverage");

            uint16_t coverageIndex = 0;

            if (!coverage.find(20, coverageIndex) || coverageIndex != 0)
                return fail("case 2 glyph 20 Coverage index");

            uint16_t substitute = 0;

            if (!subst.substituteGlyphId(coverageIndex, substitute) ||
                substitute != 200)
            {
                return fail("case 2 substitute 200");
            }

            if (!coverage.find(21, coverageIndex) || coverageIndex != 1)
                return fail("case 2 glyph 21 Coverage index");

            if (!subst.substituteGlyphId(coverageIndex, substitute) ||
                substitute != 201)
            {
                return fail("case 2 substitute 201");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Backtrack Coverage geometry.
        //
        // Logical:
        //
        //   11 12 | input
        //
        // Encoded:
        //
        //   backtrackCoverage(0) -> 12
        //   backtrackCoverage(1) -> 11
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewFormat1();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView backtrack0 =
                subst.backtrackCoverage(0);

            const OpenTypeCoverageView backtrack1 =
                subst.backtrackCoverage(1);

            if (!backtrack0 || !backtrack1)
                return fail("case 3 backtrack Coverages");

            uint16_t coverageIndex = 0;

            if (!backtrack0.find(12, coverageIndex))
                return fail("case 3 nearest backtrack");

            if (!backtrack1.find(11, coverageIndex))
                return fail("case 3 far backtrack");

            if (backtrack0.find(11, coverageIndex) ||
                backtrack1.find(12, coverageIndex))
            {
                return fail("case 3 backtrack order");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Lookahead Coverage geometry.
        //
        // Logical:
        //
        //   input | 30 31
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewFormat1();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView lookahead0 =
                subst.lookaheadCoverage(0);

            const OpenTypeCoverageView lookahead1 =
                subst.lookaheadCoverage(1);

            if (!lookahead0 || !lookahead1)
                return fail("case 4 lookahead Coverages");

            uint16_t coverageIndex = 0;

            if (!lookahead0.find(30, coverageIndex))
                return fail("case 4 nearest lookahead");

            if (!lookahead1.find(31, coverageIndex))
                return fail("case 4 far lookahead");

            if (lookahead0.find(31, coverageIndex) ||
                lookahead1.find(30, coverageIndex))
            {
                return fail("case 4 lookahead order");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Raw offsets and substitute bounds.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewFormat1();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            uint16_t offset = 0;

            if (!subst.backtrackCoverageOffset(0, offset) || offset == 0)
                return fail("case 5 backtrack offset 0");

            if (!subst.backtrackCoverageOffset(1, offset) || offset == 0)
                return fail("case 5 backtrack offset 1");

            if (!subst.lookaheadCoverageOffset(0, offset) || offset == 0)
                return fail("case 5 lookahead offset 0");

            if (!subst.lookaheadCoverageOffset(1, offset) || offset == 0)
                return fail("case 5 lookahead offset 1");

            uint16_t glyphId = 0;

            if (subst.backtrackCoverageOffset(2, offset) ||
                subst.lookaheadCoverageOffset(2, offset) ||
                subst.substituteGlyphId(2, glyphId))
            {
                return fail("case 5 bounds");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Empty backtrack and lookahead are legal.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewEmptyContext();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            if (!subst ||
                subst.backtrackGlyphCount() != 0 ||
                subst.lookaheadGlyphCount() != 0 ||
                subst.glyphCount() != 1)
            {
                return fail("case 6 empty context");
            }

            const OpenTypeCoverageView coverage = subst.coverage();

            uint16_t coverageIndex = 0;
            uint16_t substitute = 0;

            if (!coverage ||
                !coverage.find(20, coverageIndex) ||
                coverageIndex != 0 ||
                !subst.substituteGlyphId(0, substitute) ||
                substitute != 200)
            {
                return fail("case 6 substitution");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Lazy child validation.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubReverseChainViewInvalidChildren();

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 7 parent rejected");

            if (subst.coverage())
                return fail("case 7 NULL input Coverage");

            if (subst.backtrackCoverage(0))
                return fail("case 7 NULL backtrack Coverage");

            if (subst.lookaheadCoverage(0))
                return fail("case 7 out-of-range lookahead Coverage");

            uint16_t substitute = 0;

            if (!subst.substituteGlyphId(0, substitute) ||
                substitute != 200)
            {
                return fail("case 7 substitute access");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Failure paths.
        // ====================================================================

        {
            ++cases;


            // Wrong format.

            {
                const uint8_t raw[] =
                {
                    0x00, 0x02,
                    0x00, 0x00,
                    0x00, 0x00,
                    0x00, 0x00,
                    0x00, 0x00
                };

                const OpenTypeGsubReverseChainSingleSubstView subst(
                    ByteSpan(raw, sizeof(raw)));

                if (subst)
                    return fail("case 8 wrong format accepted");
            }


            // Truncated backtrack Coverage array.

            {
                const uint8_t raw[] =
                {
                    0x00, 0x01,
                    0x00, 0x10,
                    0x00, 0x02,
                    0x00, 0x08
                };

                const OpenTypeGsubReverseChainSingleSubstView subst(
                    ByteSpan(raw, sizeof(raw)));

                if (subst)
                    return fail("case 8 truncated backtrack array");
            }


            // glyphCount says two substitutes, but only one exists.

            {
                const uint8_t raw[] =
                {
                    0x00, 0x01,
                    0x00, 0x00,

                    0x00, 0x00,

                    0x00, 0x00,

                    0x00, 0x02,
                    0x00, 0xC8
                };

                const OpenTypeGsubReverseChainSingleSubstView subst(
                    ByteSpan(raw, sizeof(raw)));

                if (subst)
                    return fail("case 8 truncated substitute array");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ReverseChainSingleSubst view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 geometry:         PASS\n"
            "  Coverage mapping:          PASS\n"
            "  Backtrack geometry:        PASS\n"
            "  Lookahead geometry:        PASS\n"
            "  Offset/bounds access:       PASS\n"
            "  Empty context:             PASS\n"
            "  Lazy child validation:     PASS\n"
            "  Failure paths:              PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs