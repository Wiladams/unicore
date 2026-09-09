// test_opentype_gpos_mark_mark_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    static void appendGposMarkMarkApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposMarkMarkApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposMarkMarkApplyU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposMarkMarkApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposMarkMarkApplyAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposMarkMarkApplyU16(data, 1);
        appendGposMarkMarkApplyS16(data, x);
        appendGposMarkMarkApplyS16(data, y);
    }

    static void appendGposMarkMarkApplyCoverage(
        std::vector<uint8_t>& data, const uint16_t* glyphs, uint16_t count)
    {
        appendGposMarkMarkApplyU16(data, 1);
        appendGposMarkMarkApplyU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposMarkMarkApplyU16(data, glyphs[i]);
    }


    // ====================================================================
    // Synthetic Type 6.
    //
    // Mark1:
    //
    //   100 -> class 0 -> (10,20)
    //   101 -> class 1 -> (30,40)
    //
    // Mark2:
    //
    //   100:
    //       class 0 -> (150,250)
    //       class 1 -> (350,450)
    //
    //   200:
    //       class 0 -> (300,400)
    //       class 1 -> (500,600)
    //
    //   201:
    //       class 0 -> (700,800)
    //       class 1 -> NULL
    //
    // Including glyph 100 in Mark2Coverage allows:
    //
    //   200 <- 100 <- 101
    //
    // to exercise real stacked mark propagation.
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkMarkApplySubtable()
    {
        std::vector<uint8_t> data;

        appendGposMarkMarkApplyU16(data, 1);

        const size_t mark1CoveragePatch = data.size();
        appendGposMarkMarkApplyU16(data, 0);

        const size_t mark2CoveragePatch = data.size();
        appendGposMarkMarkApplyU16(data, 0);

        appendGposMarkMarkApplyU16(data, 2);

        const size_t mark1ArrayPatch = data.size();
        appendGposMarkMarkApplyU16(data, 0);

        const size_t mark2ArrayPatch = data.size();
        appendGposMarkMarkApplyU16(data, 0);


        // Mark1Coverage.

        patchGposMarkMarkApplyU16(
            data, mark1CoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 101 };
            appendGposMarkMarkApplyCoverage(data, glyphs, 2);
        }


        // Mark2Coverage.

        patchGposMarkMarkApplyU16(
            data, mark2CoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 200, 201 };
            appendGposMarkMarkApplyCoverage(data, glyphs, 3);
        }


        // Mark1Array.

        patchGposMarkMarkApplyU16(
            data, mark1ArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposMarkMarkApplyU16(data, 2);

            appendGposMarkMarkApplyU16(data, 0);
            const size_t mark100Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);

            appendGposMarkMarkApplyU16(data, 1);
            const size_t mark101Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);

            patchGposMarkMarkApplyU16(
                data, mark100Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 10, 20);

            patchGposMarkMarkApplyU16(
                data, mark101Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 30, 40);
        }


        // Mark2Array.

        patchGposMarkMarkApplyU16(
            data, mark2ArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposMarkMarkApplyU16(data, 3);


            // Glyph 100.

            const size_t m100Class0Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);

            const size_t m100Class1Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);


            // Glyph 200.

            const size_t m200Class0Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);

            const size_t m200Class1Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);


            // Glyph 201.

            const size_t m201Class0Patch = data.size();
            appendGposMarkMarkApplyU16(data, 0);

            appendGposMarkMarkApplyU16(data, 0);


            patchGposMarkMarkApplyU16(
                data, m100Class0Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 150, 250);


            patchGposMarkMarkApplyU16(
                data, m100Class1Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 350, 450);


            patchGposMarkMarkApplyU16(
                data, m200Class0Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 300, 400);


            patchGposMarkMarkApplyU16(
                data, m200Class1Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 500, 600);


            patchGposMarkMarkApplyU16(
                data, m201Class0Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkMarkApplyAnchor(data, 700, 800);
        }

        return data;
    }


    static std::vector<uint8_t> makeGposMarkMarkApplyLookup(
        const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposMarkMarkApplyU16(data, 6);
        appendGposMarkMarkApplyU16(data, lookupFlag);
        appendGposMarkMarkApplyU16(data, 1);
        appendGposMarkMarkApplyU16(data, 8);

        data.insert(
            data.end(),
            subtable.begin(),
            subtable.end());

        return data;
    }


    // ====================================================================
    // GDEF:
    //
    //   50       Base
    //   60       Ligature
    //   100..101 Mark
    //   200..202 Mark
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkMarkApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposMarkMarkApplyU16(data, 1);
        appendGposMarkMarkApplyU16(data, 0);

        appendGposMarkMarkApplyU16(data, 12);
        appendGposMarkMarkApplyU16(data, 0);
        appendGposMarkMarkApplyU16(data, 0);
        appendGposMarkMarkApplyU16(data, 0);

        appendGposMarkMarkApplyU16(data, 2);
        appendGposMarkMarkApplyU16(data, 4);

        appendGposMarkMarkApplyU16(data, 50);
        appendGposMarkMarkApplyU16(data, 50);
        appendGposMarkMarkApplyU16(data, 1);

        appendGposMarkMarkApplyU16(data, 60);
        appendGposMarkMarkApplyU16(data, 60);
        appendGposMarkMarkApplyU16(data, 2);

        appendGposMarkMarkApplyU16(data, 100);
        appendGposMarkMarkApplyU16(data, 101);
        appendGposMarkMarkApplyU16(data, 3);

        appendGposMarkMarkApplyU16(data, 200);
        appendGposMarkMarkApplyU16(data, 202);
        appendGposMarkMarkApplyU16(data, 3);

        return data;
    }


    static ShapedGlyph makeGposMarkMarkApplyGlyph(
        uint32_t glyphId, int32_t advanceX,
        int32_t offsetX = 0, int32_t offsetY = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;
        glyph.placement.offsetX = offsetX;
        glyph.placement.offsetY = offsetY;

        return glyph;
    }


    static void setGposMarkMarkApplyLigature(
        ShapedGlyph& glyph,
        uint32_t id, uint16_t component)
    {
        glyph.shaping.ligature.id = id;
        glyph.shaping.ligature.component = component;
        glyph.shaping.ligature.componentCount = 0;
    }


    static bool testOpenTypeGposMarkMarkApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Mark-to-Mark apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        const std::vector<uint8_t> subtable =
            makeGposMarkMarkApplySubtable();

        const std::vector<uint8_t> lookupData =
            makeGposMarkMarkApplyLookup(subtable);

        const std::vector<uint8_t> gdefData =
            makeGposMarkMarkApplyGdef();

        const OpenTypeLayoutLookupView lookup(
            ByteSpan(
                lookupData.data(),
                lookupData.size()));

        const OpenTypeGdefView gdef(
            ByteSpan(
                gdefData.data(),
                gdefData.size()));


        // ================================================================
        // Case 1 - Basic LTR Mark1 -> Mark2.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 50, 5, 10));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    100, 0));

            if (!applyOpenTypeGposMarkMarkLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 1 application");
            }

            // local:
            //
            //   (300,400) - (10,20)
            //   = (290,380)
            //
            // final:
            //
            //   X = 290 + 5 - 50 = 245
            //   Y = 380 + 10     = 390

            if (buffer[1].placement.offsetX != 245 ||
                buffer[1].placement.offsetY != 390)
            {
                return fail("case 1 placement");
            }

            if (buffer[0].placement.advanceX != 50 ||
                buffer[0].placement.offsetX != 5 ||
                buffer[0].placement.offsetY != 10)
            {
                return fail("case 1 Mark2 changed");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Class-selected anchor.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    101, 0));

            OpenTypeGposMarkMarkMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            if (resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer, 1, match) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 2 resolve");
            }

            if (match.mark2X != 500 ||
                match.mark2Y != 600 ||
                match.mark1X != 30 ||
                match.mark1Y != 40)
            {
                return fail("case 2 class anchor");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - LookupFlag filtering.
        //
        // IgnoreBaseGlyphs:
        //
        //   200 base50 101
        //
        // base50 is skipped, so 101 attaches to 200.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> filteredLookupData =
                makeGposMarkMarkApplyLookup(
                    subtable, 0x0002u);

            const OpenTypeLayoutLookupView filteredLookup(
                ByteSpan(
                    filteredLookupData.data(),
                    filteredLookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    50, 400));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    101, 0));

            if (!applyOpenTypeGposMarkMarkLookup(
                filteredLookup, gdef,
                buffer, false))
            {
                return fail("case 3 application");
            }

            // local class-1 delta = (470,560).
            // Intervening base advance = 400.

            if (buffer[2].placement.offsetX != 70 ||
                buffer[2].placement.offsetY != 560)
            {
                return fail("case 3 filtered placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - First eligible glyph is authoritative.
        //
        //   200 202 101
        //
        // 202 is the preceding eligible glyph but is not Mark2Coverage.
        // Do not continue backward to 200.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    202, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    101, 0));

            OpenTypeGposMarkMarkMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            if (resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer, 2, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 4 searched past candidate");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - NULL Mark2 class anchor.
        //
        // 201 has no class-1 anchor.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    201, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    101, 0));

            OpenTypeGposMarkMarkMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            if (resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer, 1, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 5 NULL anchor");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Same ligature component is accepted.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            ShapedGlyph mark2 =
                makeGposMarkMarkApplyGlyph(
                    200, 0);

            ShapedGlyph mark1 =
                makeGposMarkMarkApplyGlyph(
                    101, 0);

            setGposMarkMarkApplyLigature(
                mark2, 77, 2);

            setGposMarkMarkApplyLigature(
                mark1, 77, 2);

            buffer.pushBack(mark2);
            buffer.pushBack(mark1);

            OpenTypeGposMarkMarkMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            if (resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer, 1, match) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 6 compatible component");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Different ligature components are rejected.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            ShapedGlyph mark2 =
                makeGposMarkMarkApplyGlyph(
                    200, 0);

            ShapedGlyph mark1 =
                makeGposMarkMarkApplyGlyph(
                    101, 0);

            setGposMarkMarkApplyLigature(
                mark2, 77, 1);

            setGposMarkMarkApplyLigature(
                mark1, 77, 2);

            buffer.pushBack(mark2);
            buffer.pushBack(mark1);

            OpenTypeGposMarkMarkMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            if (resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer, 1, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 7 cross-component attachment");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Ligature-base exception.
        //
        // Different IDs are allowed if one mark is itself represented as
        // a ligature base.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            ShapedGlyph mark2 =
                makeGposMarkMarkApplyGlyph(
                    200, 0);

            ShapedGlyph mark1 =
                makeGposMarkMarkApplyGlyph(
                    101, 0);

            setGposMarkMarkApplyLigature(
                mark2, 77, 0);

            setGposMarkMarkApplyLigature(
                mark1, 88, 2);

            buffer.pushBack(mark2);
            buffer.pushBack(mark1);

            OpenTypeGposMarkMarkMatch match;

            const OpenTypeLookupGlyphFilter filter(
                lookup, gdef);

            if (resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer, 1, match) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 8 ligature-base exception");
            }

            ++passed;
        }


        // ================================================================
        // Case 9 - Logical RTL compensation.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 50, 5, 10));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    100, 0));

            if (!applyOpenTypeGposMarkMarkLookup(
                lookup, gdef, buffer, true))
            {
                return fail("case 9 application");
            }

            // local = (290,380)
            //
            // RTL:
            //
            //   X = 290 + 5 + 50 = 345
            //   Y = 380 + 10      = 390

            if (buffer[1].placement.offsetX != 345 ||
                buffer[1].placement.offsetY != 390)
            {
                return fail("case 9 RTL placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 10 - Whole stacked-mark chain.
        //
        //   200 <- 100 <- 101
        //
        // 100 first attaches to 200.
        // 101 then attaches to 100.
        //
        // Delayed finalization must propagate both levels.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    100, 0));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    101, 0));

            if (!applyOpenTypeGposMarkMarkLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 10 whole lookup");
            }

            // 100 -> 200:
            //
            //   local = (300,400) - (10,20)
            //         = (290,380)
            //
            // 101 -> 100:
            //
            //   local = (350,450) - (30,40)
            //         = (320,410)
            //
            // final 101:
            //
            //   (320,410) + (290,380)
            //   = (610,790)

            if (buffer[1].placement.offsetX != 290 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 10 first level");
            }

            if (buffer[2].placement.offsetX != 610 ||
                buffer[2].placement.offsetY != 790)
            {
                return fail("case 10 second level");
            }

            ++passed;
        }


        // ================================================================
        // Case 11 - Transactional malformed child.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> badSubtable =
                makeGposMarkMarkApplySubtable();

            const OpenTypeGposMarkMarkPosView initial(
                ByteSpan(
                    badSubtable.data(),
                    badSubtable.size()));

            if (!initial)
                return fail("case 11 initial subtable");

            const uint16_t mark1ArrayOffset =
                initial.mark1ArrayOffset();

            const OpenTypeGposMarkArrayView mark1Array =
                initial.mark1Array();

            if (!mark1Array)
                return fail("case 11 Mark1Array");

            uint16_t anchorOffset = 0;

            if (!mark1Array.markAnchorOffset(
                0, anchorOffset))
            {
                return fail("case 11 anchor offset");
            }

            const size_t absoluteAnchor =
                size_t(mark1ArrayOffset) +
                size_t(anchorOffset);

            if (absoluteAnchor + 2 >
                badSubtable.size())
            {
                return fail("case 11 synthetic bounds");
            }

            badSubtable[absoluteAnchor] = 0;
            badSubtable[absoluteAnchor + 1] = 4;

            const std::vector<uint8_t> badLookupData =
                makeGposMarkMarkApplyLookup(
                    badSubtable);

            const OpenTypeLayoutLookupView badLookup(
                ByteSpan(
                    badLookupData.data(),
                    badLookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    200, 50, 5, 10));

            buffer.pushBack(
                makeGposMarkMarkApplyGlyph(
                    100, 0, 7, 8));

            if (applyOpenTypeGposMarkMarkLookup(
                badLookup, gdef,
                buffer, false))
            {
                return fail(
                    "case 11 malformed child accepted");
            }

            if (buffer[0].placement.advanceX != 50 ||
                buffer[0].placement.offsetX != 5 ||
                buffer[0].placement.offsetY != 10 ||
                buffer[1].placement.offsetX != 7 ||
                buffer[1].placement.offsetY != 8)
            {
                return fail(
                    "case 11 transactional failure");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Mark-to-Mark apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  LTR attachment:            PASS\n"
            "  Class-selected anchor:     PASS\n"
            "  LookupFlag filtering:      PASS\n"
            "  Candidate boundary:        PASS\n"
            "  NULL Mark2 anchor:         PASS\n"
            "  Same ligature component:   PASS\n"
            "  Cross-component rejection: PASS\n"
            "  Ligature-base exception:   PASS\n"
            "  Logical RTL attachment:    PASS\n"
            "  Stacked propagation:       PASS\n"
            "  Transactional failure:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs