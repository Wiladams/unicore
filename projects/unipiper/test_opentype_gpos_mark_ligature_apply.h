// test_opentype_gpos_mark_ligature_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    static void appendGposMarkLigApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposMarkLigApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposMarkLigApplyU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposMarkLigApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposMarkLigApplyAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposMarkLigApplyU16(data, 1);
        appendGposMarkLigApplyS16(data, x);
        appendGposMarkLigApplyS16(data, y);
    }

    static void appendGposMarkLigApplyCoverage(
        std::vector<uint8_t>& data, const uint16_t* glyphs, uint16_t count)
    {
        appendGposMarkLigApplyU16(data, 1);
        appendGposMarkLigApplyU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposMarkLigApplyU16(data, glyphs[i]);
    }


    // ====================================================================
    // Synthetic Type 5.
    //
    // Marks:
    //
    //   100 -> class 0 -> (20,30)
    //   101 -> class 1 -> (30,40)
    //
    // Ligature 500, three components:
    //
    //   component 0:
    //       class 0 -> (100,200)
    //       class 1 -> NULL
    //
    //   component 1:
    //       class 0 -> NULL
    //       class 1 -> (500,600)
    //
    //   component 2:
    //       class 0 -> (700,800)
    //       class 1 -> (900,1000)
    // ====================================================================

    static std::vector<uint8_t> makeGposMarkLigApplySubtable()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigApplyU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposMarkLigApplyU16(data, 0);

        const size_t ligatureCoveragePatch = data.size();
        appendGposMarkLigApplyU16(data, 0);

        appendGposMarkLigApplyU16(data, 2);

        const size_t markArrayPatch = data.size();
        appendGposMarkLigApplyU16(data, 0);

        const size_t ligatureArrayPatch = data.size();
        appendGposMarkLigApplyU16(data, 0);


        // MarkCoverage.

        patchGposMarkLigApplyU16(
            data, markCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 100, 101 };
            appendGposMarkLigApplyCoverage(data, glyphs, 2);
        }


        // LigatureCoverage.

        patchGposMarkLigApplyU16(
            data, ligatureCoveragePatch,
            static_cast<uint16_t>(data.size()));

        {
            const uint16_t glyphs[] = { 500 };
            appendGposMarkLigApplyCoverage(data, glyphs, 1);
        }


        // MarkArray.

        patchGposMarkLigApplyU16(
            data, markArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposMarkLigApplyU16(data, 2);

            appendGposMarkLigApplyU16(data, 0);
            const size_t mark0Patch = data.size();
            appendGposMarkLigApplyU16(data, 0);

            appendGposMarkLigApplyU16(data, 1);
            const size_t mark1Patch = data.size();
            appendGposMarkLigApplyU16(data, 0);

            patchGposMarkLigApplyU16(
                data, mark0Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkLigApplyAnchor(data, 20, 30);

            patchGposMarkLigApplyU16(
                data, mark1Patch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            appendGposMarkLigApplyAnchor(data, 30, 40);
        }


        // LigatureArray.

        patchGposMarkLigApplyU16(
            data, ligatureArrayPatch,
            static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposMarkLigApplyU16(data, 1);

            const size_t attachPatch = data.size();
            appendGposMarkLigApplyU16(data, 0);

            patchGposMarkLigApplyU16(
                data, attachPatch,
                static_cast<uint16_t>(
                    data.size() - arrayBegin));

            const size_t attachBegin = data.size();

            appendGposMarkLigApplyU16(data, 3);


            // Component 0.

            const size_t c0Class0Patch = data.size();
            appendGposMarkLigApplyU16(data, 0);

            appendGposMarkLigApplyU16(data, 0);


            // Component 1.

            appendGposMarkLigApplyU16(data, 0);

            const size_t c1Class1Patch = data.size();
            appendGposMarkLigApplyU16(data, 0);


            // Component 2.

            const size_t c2Class0Patch = data.size();
            appendGposMarkLigApplyU16(data, 0);

            const size_t c2Class1Patch = data.size();
            appendGposMarkLigApplyU16(data, 0);


            patchGposMarkLigApplyU16(
                data, c0Class0Patch,
                static_cast<uint16_t>(
                    data.size() - attachBegin));

            appendGposMarkLigApplyAnchor(data, 100, 200);


            patchGposMarkLigApplyU16(
                data, c1Class1Patch,
                static_cast<uint16_t>(
                    data.size() - attachBegin));

            appendGposMarkLigApplyAnchor(data, 500, 600);


            patchGposMarkLigApplyU16(
                data, c2Class0Patch,
                static_cast<uint16_t>(
                    data.size() - attachBegin));

            appendGposMarkLigApplyAnchor(data, 700, 800);


            patchGposMarkLigApplyU16(
                data, c2Class1Patch,
                static_cast<uint16_t>(
                    data.size() - attachBegin));

            appendGposMarkLigApplyAnchor(data, 900, 1000);
        }

        return data;
    }


    static std::vector<uint8_t> makeGposMarkLigApplyLookup(
        const std::vector<uint8_t>& subtable)
    {
        std::vector<uint8_t> data;

        appendGposMarkLigApplyU16(data, 5);
        appendGposMarkLigApplyU16(data, 0);
        appendGposMarkLigApplyU16(data, 1);
        appendGposMarkLigApplyU16(data, 8);

        data.insert(
            data.end(),
            subtable.begin(),
            subtable.end());

        return data;
    }


    // Glyphs 100..101 are GDEF marks.

    static std::vector<uint8_t> makeGposMarkLigApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposMarkLigApplyU16(data, 1);
        appendGposMarkLigApplyU16(data, 0);

        appendGposMarkLigApplyU16(data, 12);
        appendGposMarkLigApplyU16(data, 0);
        appendGposMarkLigApplyU16(data, 0);
        appendGposMarkLigApplyU16(data, 0);

        appendGposMarkLigApplyU16(data, 2);
        appendGposMarkLigApplyU16(data, 1);

        appendGposMarkLigApplyU16(data, 100);
        appendGposMarkLigApplyU16(data, 101);
        appendGposMarkLigApplyU16(data, 3);

        return data;
    }


    static ShapedGlyph makeGposMarkLigApplyGlyph(
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


    static ShapedGlyph makeGposMarkLigApplyLigature(
        uint32_t glyphId, int32_t advanceX,
        uint32_t ligatureId, uint16_t componentCount,
        int32_t offsetX = 0, int32_t offsetY = 0)
    {
        ShapedGlyph glyph =
            makeGposMarkLigApplyGlyph(
                glyphId, advanceX,
                offsetX, offsetY);

        glyph.shaping.ligature.id = ligatureId;
        glyph.shaping.ligature.component = 0;
        glyph.shaping.ligature.componentCount =
            componentCount;

        return glyph;
    }


    static ShapedGlyph makeGposMarkLigApplyMark(
        uint32_t glyphId,
        uint32_t ligatureId,
        uint16_t component)
    {
        ShapedGlyph glyph =
            makeGposMarkLigApplyGlyph(
                glyphId, 0);

        glyph.shaping.ligature.id = ligatureId;
        glyph.shaping.ligature.component = component;

        return glyph;
    }


    static bool testOpenTypeGposMarkLigatureApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Mark-to-Ligature apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        const std::vector<uint8_t> subtable =
            makeGposMarkLigApplySubtable();

        const std::vector<uint8_t> lookupData =
            makeGposMarkLigApplyLookup(subtable);

        const std::vector<uint8_t> gdefData =
            makeGposMarkLigApplyGdef();

        const OpenTypeLayoutLookupView lookup(
            ByteSpan(
                lookupData.data(),
                lookupData.size()));

        const OpenTypeGdefView gdef(
            ByteSpan(
                gdefData.data(),
                gdefData.size()));


        // ================================================================
        // Case 1 - Exact component association.
        //
        // mark component 2 -> ComponentRecord 1.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3, 10, 20));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 2));

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposMarkLigatureMatch match;

            if (resolveOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, 1, match) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 1 resolve");
            }

            if (match.ligatureIndex != 0 ||
                match.componentIndex != 1 ||
                match.ligatureX != 500 ||
                match.ligatureY != 600)
            {
                return fail("case 1 component selection");
            }

            if (!applyOpenTypeGposMarkLigatureMatch(
                buffer, attachments, match))
            {
                return fail("case 1 apply");
            }

            if (buffer[1].placement.offsetX != 470 ||
                buffer[1].placement.offsetY != 560)
            {
                return fail("case 1 local placement");
            }

            if (!resolveOpenTypeGposAttachments(
                buffer, attachments, false))
            {
                return fail("case 1 finalization");
            }

            if (buffer[1].placement.offsetX != -220 ||
                buffer[1].placement.offsetY != 580)
            {
                return fail("case 1 final placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Mismatched ligature ID uses final component.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3, 10, 20));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 88, 1));

            if (!applyOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 2 apply");
            }

            // Final component class 1:
            //
            // 900,1000 - 30,40 = 870,960
            //
            // LTR pen compensation:
            //
            // 870 + 10 - 700 = 180
            // 960 + 20       = 980

            if (buffer[1].placement.offsetX != 180 ||
                buffer[1].placement.offsetY != 980)
            {
                return fail("case 2 fallback component");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Oversized associated component clamps to final.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 9));

            OpenTypeGposMarkLigatureMatch match;

            if (resolveOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, 1, match) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 3 resolve");
            }

            if (match.componentIndex != 2 ||
                match.ligatureX != 900 ||
                match.ligatureY != 1000)
            {
                return fail("case 3 component clamp");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Preceding marks are skipped.
        //
        //   500 100 101
        //
        // Exact target 101 attaches to 500.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    100, 77, 1));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 2));

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            if (applyOpenTypeGposMarkLigatureLookupAt(
                lookup, gdef, buffer, 2,
                attachments) !=
                OpenTypeGposResolveResult::Match)
            {
                return fail("case 4 exact application");
            }

            if (attachments[2].type !=
                OpenTypeGposAttachmentType::Mark ||
                attachments[2].parent != 0)
            {
                return fail("case 4 wrong parent");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - First non-mark stops backward search.
        //
        //   500 40 101
        //
        // Glyph 40 is not in LigatureCoverage.
        // Do not continue backward to 500.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3));

            buffer.pushBack(
                makeGposMarkLigApplyGlyph(
                    40, 500));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 2));

            OpenTypeGposMarkLigatureMatch match;

            if (resolveOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, 2, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 5 searched past non-mark");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - NULL selected component/class anchor.
        //
        // mark 101 -> class 1
        // association component 1 -> ComponentRecord 0
        // ComponentRecord 0 class 1 is NULL.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 1));

            OpenTypeGposMarkLigatureMatch match;

            if (resolveOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, 1, match) !=
                OpenTypeGposResolveResult::NoMatch)
            {
                return fail("case 6 NULL anchor");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Logical RTL attachment.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3, 10, 20));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 2));

            if (!applyOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, true))
            {
                return fail("case 7 apply");
            }

            // local = 470,560
            //
            // RTL pen compensation:
            //
            // X = 470 + 10 + 700 = 1180
            // Y = 560 + 20       = 580

            if (buffer[1].placement.offsetX != 1180 ||
                buffer[1].placement.offsetY != 580)
            {
                return fail("case 7 RTL placement");
            }

            if (10 + 500 !=
                -700 + 1180 + 30)
            {
                return fail("case 7 RTL alignment");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Delayed parent propagation.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 2));

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            if (!applyOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer,
                attachments))
            {
                return fail("case 8 application");
            }

            if (buffer[1].placement.offsetX != 470 ||
                buffer[1].placement.offsetY != 560)
            {
                return fail("case 8 local attachment");
            }

            buffer[0].placement.offsetX += 25;
            buffer[0].placement.offsetY -= 10;

            if (!resolveOpenTypeGposAttachments(
                buffer, attachments, false))
            {
                return fail("case 8 finalization");
            }

            if (buffer[1].placement.offsetX != -205 ||
                buffer[1].placement.offsetY != 550)
            {
                return fail("case 8 delayed propagation");
            }

            ++passed;
        }


        // ================================================================
        // Case 9 - Whole lookup with two marks on different components.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    100, 77, 1));

            buffer.pushBack(
                makeGposMarkLigApplyMark(
                    101, 77, 2));

            if (!applyOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 9 whole lookup");
            }

            // mark 100:
            // component 0 class 0 = 100,200
            // local = 80,170
            // final X = 80 - 700 = -620

            if (buffer[1].placement.offsetX != -620 ||
                buffer[1].placement.offsetY != 170)
            {
                return fail("case 9 first mark");
            }

            // mark 101:
            // component 1 class 1 = 500,600
            // local = 470,560
            // final X = 470 - 700 = -230

            if (buffer[2].placement.offsetX != -230 ||
                buffer[2].placement.offsetY != 560)
            {
                return fail("case 9 second mark");
            }

            ++passed;
        }


        // ================================================================
        // Case 10 - Malformed mark Anchor is transactional.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> badSubtable =
                makeGposMarkLigApplySubtable();

            const OpenTypeGposMarkLigaturePosView initial(
                ByteSpan(
                    badSubtable.data(),
                    badSubtable.size()));

            if (!initial)
                return fail("case 10 initial subtable");

            const uint16_t markArrayOffset =
                initial.markArrayOffset();

            const OpenTypeGposMarkArrayView marks =
                initial.markArray();

            if (!marks)
                return fail("case 10 MarkArray");

            uint16_t anchorOffset = 0;

            if (!marks.markAnchorOffset(
                1, anchorOffset))
            {
                return fail("case 10 anchor offset");
            }

            const size_t absoluteOffset =
                size_t(markArrayOffset) +
                size_t(anchorOffset);

            if (absoluteOffset + 2 >
                badSubtable.size())
            {
                return fail("case 10 synthetic bounds");
            }

            badSubtable[absoluteOffset] = 0;
            badSubtable[absoluteOffset + 1] = 4;

            const std::vector<uint8_t> badLookupData =
                makeGposMarkLigApplyLookup(
                    badSubtable);

            const OpenTypeLayoutLookupView badLookup(
                ByteSpan(
                    badLookupData.data(),
                    badLookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposMarkLigApplyLigature(
                    500, 700, 77, 3, 10, 20));

            ShapedGlyph mark =
                makeGposMarkLigApplyMark(
                    101, 77, 2);

            mark.placement.offsetX = 7;
            mark.placement.offsetY = 8;

            buffer.pushBack(mark);

            if (applyOpenTypeGposMarkLigatureLookup(
                badLookup, gdef,
                buffer, false))
            {
                return fail("case 10 malformed child accepted");
            }

            if (buffer[0].placement.advanceX != 700 ||
                buffer[0].placement.offsetX != 10 ||
                buffer[0].placement.offsetY != 20 ||
                buffer[1].placement.offsetX != 7 ||
                buffer[1].placement.offsetY != 8)
            {
                return fail("case 10 transactional failure");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Mark-to-Ligature apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Component association:     PASS\n"
            "  Final-component fallback:  PASS\n"
            "  Component clamping:        PASS\n"
            "  Stacked-mark search:       PASS\n"
            "  Non-mark search boundary:  PASS\n"
            "  NULL component anchor:     PASS\n"
            "  Logical RTL attachment:    PASS\n"
            "  Delayed propagation:       PASS\n"
            "  Whole lookup:              PASS\n"
            "  Transactional failure:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs