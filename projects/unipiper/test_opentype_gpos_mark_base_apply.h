// test_opentype_gpos_mark_base_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    static void appendGposMarkBaseApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposMarkBaseApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposMarkBaseApplyU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposMarkBaseApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposMarkBaseApplyAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposMarkBaseApplyU16(data, 1);
        appendGposMarkBaseApplyS16(data, x);
        appendGposMarkBaseApplyS16(data, y);
    }

    static void appendGposMarkBaseApplyCoverage(
        std::vector<uint8_t>& data,
        const uint16_t* glyphs, uint16_t count)
    {
        appendGposMarkBaseApplyU16(data, 1);
        appendGposMarkBaseApplyU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposMarkBaseApplyU16(data, glyphs[i]);
    }


    static std::vector<uint8_t> makeGposMarkBaseApplySubtable()
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseApplyU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposMarkBaseApplyU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposMarkBaseApplyU16(data, 0);

        appendGposMarkBaseApplyU16(data, 2);

        const size_t markArrayPatch = data.size();
        appendGposMarkBaseApplyU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposMarkBaseApplyU16(data, 0);


        // MarkCoverage: 100, 101.

        patchGposMarkBaseApplyU16(
            data, markCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 101 };
            appendGposMarkBaseApplyCoverage(data, glyphs, 2);
        }


        // BaseCoverage: 20, 30.

        patchGposMarkBaseApplyU16(
            data, baseCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 20, 30 };
            appendGposMarkBaseApplyCoverage(data, glyphs, 2);
        }


        // MarkArray.

        patchGposMarkBaseApplyU16(
            data, markArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposMarkBaseApplyU16(data, 2);

            // glyph 100 -> class 0.

            appendGposMarkBaseApplyU16(data, 0);
            const size_t mark0AnchorPatch = data.size();
            appendGposMarkBaseApplyU16(data, 0);

            // glyph 101 -> class 1.

            appendGposMarkBaseApplyU16(data, 1);
            const size_t mark1AnchorPatch = data.size();
            appendGposMarkBaseApplyU16(data, 0);

            patchGposMarkBaseApplyU16(
                data, mark0AnchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposMarkBaseApplyAnchor(data, 30, 40);

            patchGposMarkBaseApplyU16(
                data, mark1AnchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposMarkBaseApplyAnchor(data, 15, 25);
        }


        // BaseArray.

        patchGposMarkBaseApplyU16(
            data, baseArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposMarkBaseApplyU16(data, 2);

            const size_t base20Class0Patch = data.size();
            appendGposMarkBaseApplyU16(data, 0);

            const size_t base20Class1Patch = data.size();
            appendGposMarkBaseApplyU16(data, 0);

            const size_t base30Class0Patch = data.size();
            appendGposMarkBaseApplyU16(data, 0);

            // base 30 class 1 is intentionally NULL.

            appendGposMarkBaseApplyU16(data, 0);

            patchGposMarkBaseApplyU16(
                data, base20Class0Patch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposMarkBaseApplyAnchor(data, 300, 400);

            patchGposMarkBaseApplyU16(
                data, base20Class1Patch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposMarkBaseApplyAnchor(data, 500, 600);

            patchGposMarkBaseApplyU16(
                data, base30Class0Patch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposMarkBaseApplyAnchor(data, 700, 800);
        }

        return data;
    }


    static std::vector<uint8_t> makeGposMarkBaseApplyLookup(
        const std::vector<uint8_t>& subtable)
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseApplyU16(data, 4);
        appendGposMarkBaseApplyU16(data, 0);
        appendGposMarkBaseApplyU16(data, 1);
        appendGposMarkBaseApplyU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());

        return data;
    }


    // Glyphs 100..101 are Marks.

    static std::vector<uint8_t> makeGposMarkBaseApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposMarkBaseApplyU16(data, 1);
        appendGposMarkBaseApplyU16(data, 0);

        appendGposMarkBaseApplyU16(data, 12);
        appendGposMarkBaseApplyU16(data, 0);
        appendGposMarkBaseApplyU16(data, 0);
        appendGposMarkBaseApplyU16(data, 0);

        appendGposMarkBaseApplyU16(data, 2);
        appendGposMarkBaseApplyU16(data, 1);

        appendGposMarkBaseApplyU16(data, 100);
        appendGposMarkBaseApplyU16(data, 101);
        appendGposMarkBaseApplyU16(data, 3);

        return data;
    }


    static ShapedGlyph makeGposMarkBaseApplyGlyph(
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


    static bool testOpenTypeGposMarkBaseApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Mark-to-Base apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        const std::vector<uint8_t> subtable =
            makeGposMarkBaseApplySubtable();

        const std::vector<uint8_t> lookupData =
            makeGposMarkBaseApplyLookup(subtable);

        const std::vector<uint8_t> gdefData =
            makeGposMarkBaseApplyGdef();

        const OpenTypeLayoutLookupView lookup(
            ByteSpan(lookupData.data(), lookupData.size()));

        const OpenTypeGdefView gdef(
            ByteSpan(gdefData.data(), gdefData.size()));


        // ====================================================================
        // Case 1 - Basic LTR attachment.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600, 10, 20));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0));

            if (!applyOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 1 application");
            }

            if (buffer[0].placement.advanceX != 600 ||
                buffer[0].placement.offsetX != 10 ||
                buffer[0].placement.offsetY != 20)
            {
                return fail("case 1 base changed");
            }

            if (buffer[1].placement.advanceX != 0 ||
                buffer[1].placement.offsetX != -320 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 1 mark placement");
            }

            // Absolute anchor alignment.

            if (10 + 300 !=
                600 - 320 + 30)
            {
                return fail("case 1 X alignment");
            }

            if (20 + 400 !=
                380 + 40)
            {
                return fail("case 1 Y alignment");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Anchor attachment overrides earlier mark placement.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600, 10, 20));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0, 999, -999));

            if (!applyOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 2 application");
            }

            if (buffer[1].placement.offsetX != -320 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 2 placement not overridden");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Preceding marks are skipped.
        //
        //   base  mark  mark
        //    20    101   100
        //
        // Exact target 100 attaches to base 20, not mark 101.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(101, 0));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0));

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            if (applyOpenTypeGposMarkBaseLookupAt(
                lookup, gdef, buffer, 2,
                attachments) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 3 exact application");
            }

            if (attachments[2].type !=
                OpenTypeGposAttachmentType::Mark ||
                attachments[2].parent != 0)
            {
                return fail("case 3 wrong parent");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - First non-mark stops the search.
        //
        //   covered base | uncovered non-mark | mark
        //       20                 40           100
        //
        // 40 is not in BaseCoverage. Do not continue backward to 20.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(40, 500));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0));

            OpenTypeGposMarkBaseMatch match;

            if (resolveOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, 2, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 4 searched past non-mark");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - NULL class-selected BaseAnchor is NoMatch.
        //
        // Base 30 has no class-1 anchor.
        // Mark 101 belongs to class 1.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(30, 600));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(101, 0));

            OpenTypeGposMarkBaseMatch match;

            if (resolveOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, 1, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 5 NULL BaseAnchor");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Logical RTL pen compensation.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600, 10, 20));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0));

            if (!applyOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, true))
            {
                return fail("case 6 application");
            }

            if (buffer[1].placement.offsetX != 880 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 6 RTL placement");
            }

            // Mark origin is -600 in our logical RTL convention.

            if (10 + 300 !=
                -600 + 880 + 30)
            {
                return fail("case 6 RTL alignment");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Delayed parent propagation.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0));

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            if (!applyOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, attachments))
            {
                return fail("case 7 application");
            }

            // Only local anchor delta exists before finalization.

            if (buffer[1].placement.offsetX != 270 ||
                buffer[1].placement.offsetY != 360)
            {
                return fail("case 7 local attachment");
            }

            // Simulate a later GPOS lookup moving the base.

            buffer[0].placement.offsetX += 25;
            buffer[0].placement.offsetY -= 10;

            if (!resolveOpenTypeGposAttachments(
                buffer, attachments, false))
            {
                return fail("case 7 finalization");
            }

            if (buffer[1].placement.offsetX != -305 ||
                buffer[1].placement.offsetY != 350)
            {
                return fail("case 7 delayed propagation");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Whole lookup with stacked marks.
        // ====================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(20, 600));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(101, 0));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(100, 0));

            if (!applyOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 8 whole lookup");
            }

            // mark 101, class 1:
            //   local X = 500 - 15 = 485
            //   final X = 485 - 600 = -115

            if (buffer[1].placement.offsetX != -115 ||
                buffer[1].placement.offsetY != 575)
            {
                return fail("case 8 first mark");
            }

            // mark 100, class 0:
            //   local X = 300 - 30 = 270
            //   final X = 270 - 600 = -330

            if (buffer[2].placement.offsetX != -330 ||
                buffer[2].placement.offsetY != 360)
            {
                return fail("case 8 second mark");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Malformed Anchor is transactional.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> badSubtable =
                makeGposMarkBaseApplySubtable();

            const OpenTypeGposMarkBasePosView markBase(
                ByteSpan(
                    badSubtable.data(),
                    badSubtable.size()));

            if (!markBase)
                return fail("case 9 initial subtable");

            const uint16_t markArrayOffset =
                markBase.markArrayOffset();

            const OpenTypeGposMarkArrayView marks =
                markBase.markArray();

            if (!marks)
                return fail("case 9 initial MarkArray");

            uint16_t anchorOffset = 0;

            if (!marks.markAnchorOffset(0, anchorOffset))
                return fail("case 9 anchor offset");

            const size_t absoluteAnchorOffset =
                size_t(markArrayOffset) +
                size_t(anchorOffset);

            if (absoluteAnchorOffset + 2 >
                badSubtable.size())
            {
                return fail("case 9 synthetic bounds");
            }

            badSubtable[absoluteAnchorOffset] = 0;
            badSubtable[absoluteAnchorOffset + 1] = 4;

            const std::vector<uint8_t> badLookupData =
                makeGposMarkBaseApplyLookup(
                    badSubtable);

            const OpenTypeLayoutLookupView badLookup(
                ByteSpan(
                    badLookupData.data(),
                    badLookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(
                    20, 600, 10, 20));

            buffer.pushBack(
                makeGposMarkBaseApplyGlyph(
                    100, 0, 7, 8));

            if (applyOpenTypeGposMarkBaseLookup(
                badLookup, gdef, buffer, false))
            {
                return fail("case 9 malformed child accepted");
            }

            if (buffer[0].placement.advanceX != 600 ||
                buffer[0].placement.offsetX != 10 ||
                buffer[0].placement.offsetY != 20 ||
                buffer[1].placement.offsetX != 7 ||
                buffer[1].placement.offsetY != 8)
            {
                return fail("case 9 transactional failure");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Mark-to-Base apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  LTR attachment:            PASS\n"
            "  Placement override:        PASS\n"
            "  Stacked-mark search:       PASS\n"
            "  Non-mark search boundary:  PASS\n"
            "  NULL BaseAnchor:           PASS\n"
            "  Logical RTL attachment:    PASS\n"
            "  Delayed propagation:       PASS\n"
            "  Whole lookup:              PASS\n"
            "  Transactional failure:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs