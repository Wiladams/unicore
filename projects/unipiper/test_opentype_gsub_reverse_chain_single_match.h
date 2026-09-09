// test_opentype_gsub_reverse_chain_single_match.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "opentype_gsub_reverse_chain_single_match.h"
#include "opentype_gdef_view.h"
#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGsubReverseChainMatchU16(
        std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubReverseChainMatchU16(
        std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGsubReverseChainMatchCoverage(
        std::vector<uint8_t>& data, std::initializer_list<uint16_t> glyphs)
    {
        appendGsubReverseChainMatchU16(data, 1);
        appendGsubReverseChainMatchU16(
            data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyphId : glyphs)
            appendGsubReverseChainMatchU16(data, glyphId);
    }


    // ====================================================================
    // Type 8 Format 1.
    //
    // Logical:
    //
    //     11 12 | 20/21 | 30 31
    //
    // Backtrack:
    //
    //     index 0 -> 12
    //     index 1 -> 11
    //
    // Input:
    //
    //     20 -> 200
    //     21 -> 201
    //
    // Lookahead:
    //
    //     index 0 -> 30
    //     index 1 -> 31
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainMatchSubtable()
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainMatchU16(data, 1);

        const size_t inputCoveragePatch = data.size();
        appendGsubReverseChainMatchU16(data, 0);


        appendGsubReverseChainMatchU16(data, 2);

        const size_t backtrack0Patch = data.size();
        appendGsubReverseChainMatchU16(data, 0);

        const size_t backtrack1Patch = data.size();
        appendGsubReverseChainMatchU16(data, 0);


        appendGsubReverseChainMatchU16(data, 2);

        const size_t lookahead0Patch = data.size();
        appendGsubReverseChainMatchU16(data, 0);

        const size_t lookahead1Patch = data.size();
        appendGsubReverseChainMatchU16(data, 0);


        appendGsubReverseChainMatchU16(data, 2);
        appendGsubReverseChainMatchU16(data, 200);
        appendGsubReverseChainMatchU16(data, 201);


        auto appendCoverage =
            [&](size_t patch, std::initializer_list<uint16_t> glyphs)
            {
                const size_t offset = data.size();

                patchGsubReverseChainMatchU16(
                    data, patch, static_cast<uint16_t>(offset));

                appendGsubReverseChainMatchCoverage(data, glyphs);
            };


        appendCoverage(inputCoveragePatch, { 20, 21 });

        appendCoverage(backtrack0Patch, { 12 });
        appendCoverage(backtrack1Patch, { 11 });

        appendCoverage(lookahead0Patch, { 30 });
        appendCoverage(lookahead1Patch, { 31 });

        return data;
    }


    // ====================================================================
    // One-glyph, empty-context Type 8.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainMatchInputOnlySubtable(
        uint16_t inputGlyph, uint16_t substituteGlyph)
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubReverseChainMatchU16(data, 0);

        appendGsubReverseChainMatchU16(data, 0);
        appendGsubReverseChainMatchU16(data, 0);

        appendGsubReverseChainMatchU16(data, 1);
        appendGsubReverseChainMatchU16(data, substituteGlyph);

        const size_t coverageOffset = data.size();

        patchGsubReverseChainMatchU16(
            data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubReverseChainMatchCoverage(data, { inputGlyph });

        return data;
    }


    // ====================================================================
    // Coverage/substitute mismatch.
    //
    // Coverage:
    //
    //     20 -> index 0
    //     21 -> index 1
    //
    // Substitute array contains only index 0.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainMatchShortSubstitute()
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainMatchU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubReverseChainMatchU16(data, 0);

        appendGsubReverseChainMatchU16(data, 0);
        appendGsubReverseChainMatchU16(data, 0);

        appendGsubReverseChainMatchU16(data, 1);
        appendGsubReverseChainMatchU16(data, 200);

        const size_t coverageOffset = data.size();

        patchGsubReverseChainMatchU16(
            data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubReverseChainMatchCoverage(data, { 20, 21 });

        return data;
    }


    // ====================================================================
    // Lookup wrapper.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainMatchLookup(
        const std::vector<uint8_t>& subtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainMatchU16(data, 8);
        appendGsubReverseChainMatchU16(data, lookupFlag);
        appendGsubReverseChainMatchU16(data, 1);
        appendGsubReverseChainMatchU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());

        return data;
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseChainMatchGdef()
    {
        std::vector<uint8_t> data;

        appendGsubReverseChainMatchU16(data, 1);
        appendGsubReverseChainMatchU16(data, 0);

        appendGsubReverseChainMatchU16(data, 12);
        appendGsubReverseChainMatchU16(data, 0);
        appendGsubReverseChainMatchU16(data, 0);
        appendGsubReverseChainMatchU16(data, 0);

        appendGsubReverseChainMatchU16(data, 2);
        appendGsubReverseChainMatchU16(data, 1);

        appendGsubReverseChainMatchU16(data, 100);
        appendGsubReverseChainMatchU16(data, 102);
        appendGsubReverseChainMatchU16(data, 3);

        return data;
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static void appendGsubReverseChainMatchGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId,
        uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubReverseChainMatchBuffer(
        uint32_t inputGlyph = 20)
    {
        OpenTypeShapingBuffer buffer;

        appendGsubReverseChainMatchGlyph(buffer, 11, 0);
        appendGsubReverseChainMatchGlyph(buffer, 12, 1);
        appendGsubReverseChainMatchGlyph(buffer, inputGlyph, 2);
        appendGsubReverseChainMatchGlyph(buffer, 30, 3);
        appendGsubReverseChainMatchGlyph(buffer, 31, 4);

        return buffer;
    }


    // Physical:
    //
    //     11 M 12 M 20 M 30 M 31
    //
    // With IgnoreMarks:
    //
    //     11 12 | 20 | 30 31

    static OpenTypeShapingBuffer makeGsubReverseChainMatchFilteredBuffer()
    {
        OpenTypeShapingBuffer buffer;

        appendGsubReverseChainMatchGlyph(buffer, 11, 0);
        appendGsubReverseChainMatchGlyph(buffer, 100, 1);

        appendGsubReverseChainMatchGlyph(buffer, 12, 2);
        appendGsubReverseChainMatchGlyph(buffer, 101, 3);

        appendGsubReverseChainMatchGlyph(buffer, 20, 4);
        appendGsubReverseChainMatchGlyph(buffer, 102, 5);

        appendGsubReverseChainMatchGlyph(buffer, 30, 6);
        appendGsubReverseChainMatchGlyph(buffer, 100, 7);

        appendGsubReverseChainMatchGlyph(buffer, 31, 8);

        return buffer;
    }


    static bool testOpenTypeGsubReverseChainSingleMatch()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB ReverseChainSingleSubst matcher: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Basic exact-position match.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchSubtable();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer =
                makeGsubReverseChainMatchBuffer();

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 2, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::Match ||
                match.substituteGlyph != 200)
            {
                return fail("case 1 basic match");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Coverage index chooses the second substitute.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchSubtable();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer =
                makeGsubReverseChainMatchBuffer(21);

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 2, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::Match ||
                match.substituteGlyph != 201)
            {
                return fail("case 2 Coverage mapping");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - IgnoreMarks across both context directions.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchSubtable();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const std::vector<uint8_t> gdefData =
                makeGsubReverseChainMatchGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            if (!gdef)
                return fail("case 3 GDEF");

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer =
                makeGsubReverseChainMatchFilteredBuffer();

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 4, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::Match ||
                match.substituteGlyph != 200)
            {
                return fail("case 3 filtered match");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Without IgnoreMarks, interspersed marks block matching.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchSubtable();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const std::vector<uint8_t> gdefData =
                makeGsubReverseChainMatchGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer =
                makeGsubReverseChainMatchFilteredBuffer();

            OpenTypeGsubReverseChainSingleMatch match;
            match.substituteGlyph = 999;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 4, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::NoMatch)
                return fail("case 4 unfiltered result");

            if (match.substituteGlyph != 0)
                return fail("case 4 partial match leaked");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Backtrack is nearest-first.
        //
        // Swap 11 and 12. If traversal/order is wrong, this could falsely
        // match.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchSubtable();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubReverseChainMatchGlyph(buffer, 12, 0);
            appendGsubReverseChainMatchGlyph(buffer, 11, 1);
            appendGsubReverseChainMatchGlyph(buffer, 20, 2);
            appendGsubReverseChainMatchGlyph(buffer, 30, 3);
            appendGsubReverseChainMatchGlyph(buffer, 31, 4);

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 2, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::NoMatch)
                return fail("case 5 backtrack order");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Backtrack/lookahead gate the substitution.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchSubtable();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));


            // Bad backtrack.

            {
                OpenTypeShapingBuffer buffer;

                appendGsubReverseChainMatchGlyph(buffer, 11, 0);
                appendGsubReverseChainMatchGlyph(buffer, 13, 1);
                appendGsubReverseChainMatchGlyph(buffer, 20, 2);
                appendGsubReverseChainMatchGlyph(buffer, 30, 3);
                appendGsubReverseChainMatchGlyph(buffer, 31, 4);

                OpenTypeGsubReverseChainSingleMatch match;

                if (matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 2, match) !=
                    OpenTypeGsubReverseChainSingleMatchResult::NoMatch)
                {
                    return fail("case 6 bad backtrack");
                }
            }


            // Bad lookahead.

            {
                OpenTypeShapingBuffer buffer;

                appendGsubReverseChainMatchGlyph(buffer, 11, 0);
                appendGsubReverseChainMatchGlyph(buffer, 12, 1);
                appendGsubReverseChainMatchGlyph(buffer, 20, 2);
                appendGsubReverseChainMatchGlyph(buffer, 32, 3);
                appendGsubReverseChainMatchGlyph(buffer, 31, 4);

                OpenTypeGsubReverseChainSingleMatch match;

                if (matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 2, match) !=
                    OpenTypeGsubReverseChainSingleMatchResult::NoMatch)
                {
                    return fail("case 6 bad lookahead");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Current/start glyph is never filtered.
        //
        // Glyph 100 is a Mark, and the Lookup has IgnoreMarks. Since it is
        // the current input glyph, it must still be considered normally.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchInputOnlySubtable(100, 300);

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const std::vector<uint8_t> gdefData =
                makeGsubReverseChainMatchGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubReverseChainMatchGlyph(buffer, 100, 0);

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 0, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::Match ||
                match.substituteGlyph != 300)
            {
                return fail("case 7 current glyph filtering");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Empty backtrack/lookahead is legal.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchInputOnlySubtable(20, 200);

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubReverseChainMatchGlyph(buffer, 20, 0);

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 0, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::Match ||
                match.substituteGlyph != 200)
            {
                return fail("case 8 empty context");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Coverage/substitute mismatch.
        //
        // Coverage index 1 has no corresponding substitute glyph.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchShortSubstitute();

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubReverseChainMatchGlyph(buffer, 21, 0);

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult result =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 0, match);

            if (result != OpenTypeGsubReverseChainSingleMatchResult::Invalid)
                return fail("case 9 short substitute array");

            if (match.substituteGlyph != 0)
                return fail("case 9 invalid result leaked");

            ++passed;
        }


        // ====================================================================
        // Case 10 - Failure paths.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtableData =
                makeGsubReverseChainMatchInputOnlySubtable(20, 200);

            const std::vector<uint8_t> lookupData =
                makeGsubReverseChainMatchLookup(subtableData);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGsubReverseChainSingleSubstView subst(
                ByteSpan(subtableData.data(), subtableData.size()));


            // Out-of-range physical position.

            {
                OpenTypeShapingBuffer buffer;
                appendGsubReverseChainMatchGlyph(buffer, 20, 0);

                OpenTypeGsubReverseChainSingleMatch match;

                if (matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 1, match) !=
                    OpenTypeGsubReverseChainSingleMatchResult::Invalid)
                {
                    return fail("case 10 out-of-range position");
                }
            }


            // Glyph IDs in a shaping buffer must fit OpenType GlyphID.

            {
                OpenTypeShapingBuffer buffer;
                appendGsubReverseChainMatchGlyph(buffer, 0x10000u, 0);

                OpenTypeGsubReverseChainSingleMatch match;

                if (matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, 0, match) !=
                    OpenTypeGsubReverseChainSingleMatchResult::Invalid)
                {
                    return fail("case 10 oversized current glyph");
                }
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ReverseChainSingleSubst matcher: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Basic match:              PASS\n"
            "  Coverage mapping:         PASS\n"
            "  LookupFlag filtering:     PASS\n"
            "  Unfiltered blocking:      PASS\n"
            "  Backtrack order:          PASS\n"
            "  Context gating:           PASS\n"
            "  Start glyph semantics:    PASS\n"
            "  Empty context:            PASS\n"
            "  Coverage/substitute:      PASS\n"
            "  Failure paths:            PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs