// test_opentype_gpos_ir_mark_mark.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gpos_attachment_state.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_gpos_ir_executor.h"
//#include "opentype_gpos_lookup_apply.h"
#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"

namespace waavs
{
    static void appendGposIRMarkMarkU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRMarkMarkU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRMarkMarkS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRMarkMarkU16(data, static_cast<uint16_t>(value));
    }


    static void patchGposIRMarkMarkU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRMarkMarkU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRMarkMarkCoverage1(std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRMarkMarkU16(data, 1);
        appendGposIRMarkMarkU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRMarkMarkU16(data, glyph);
    }


    static void appendGposIRMarkMarkAnchor1(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposIRMarkMarkU16(data, 1);
        appendGposIRMarkMarkS16(data, x);
        appendGposIRMarkMarkS16(data, y);
    }


    struct GposIRMarkMarkMark1Spec
    {
        uint16_t glyph{ 0 };
        uint16_t markClass{ 0 };
        int16_t x{ 0 };
        int16_t y{ 0 };
    };


    struct GposIRMarkMarkMark2Spec
    {
        uint16_t glyph{ 0 };
        std::vector<uint8_t> hasAnchor{};
        std::vector<int16_t> x{};
        std::vector<int16_t> y{};
    };


    struct GposIRMarkMarkGdefClassSpec
    {
        uint16_t glyph{ 0 };
        uint16_t glyphClass{ 0 };
    };


    static std::vector<uint8_t> makeGposIRMarkMarkSubtable(
        const std::vector<GposIRMarkMarkMark1Spec>& mark1,
        const std::vector<GposIRMarkMarkMark2Spec>& mark2,
        uint16_t markClassCount)
    {
        if (markClassCount == 0)
            return {};

        for (const GposIRMarkMarkMark2Spec& record : mark2)
        {
            if (record.hasAnchor.size() != markClassCount ||
                record.x.size() != markClassCount ||
                record.y.size() != markClassCount)
            {
                return {};
            }
        }

        std::vector<uint8_t> data;

        appendGposIRMarkMarkU16(data, 1);

        const size_t mark1CoveragePatch = data.size();
        appendGposIRMarkMarkU16(data, 0);

        const size_t mark2CoveragePatch = data.size();
        appendGposIRMarkMarkU16(data, 0);

        appendGposIRMarkMarkU16(data, markClassCount);

        const size_t mark1ArrayPatch = data.size();
        appendGposIRMarkMarkU16(data, 0);

        const size_t mark2ArrayPatch = data.size();
        appendGposIRMarkMarkU16(data, 0);


        // Mark1Array.

        const size_t mark1ArrayOffset = data.size();
        patchGposIRMarkMarkU16(data, mark1ArrayPatch, static_cast<uint16_t>(mark1ArrayOffset));

        appendGposIRMarkMarkU16(data, static_cast<uint16_t>(mark1.size()));

        const size_t mark1RecordBase = data.size();

        for (const GposIRMarkMarkMark1Spec& record : mark1)
        {
            appendGposIRMarkMarkU16(data, record.markClass);
            appendGposIRMarkMarkU16(data, 0);
        }

        for (size_t i = 0; i < mark1.size(); ++i)
        {
            const size_t anchorOffset = data.size() - mark1ArrayOffset;

            patchGposIRMarkMarkU16(
                data, mark1RecordBase + i * 4 + 2,
                static_cast<uint16_t>(anchorOffset));

            appendGposIRMarkMarkAnchor1(data, mark1[i].x, mark1[i].y);
        }


        // Mark2Array.

        const size_t mark2ArrayOffset = data.size();
        patchGposIRMarkMarkU16(data, mark2ArrayPatch, static_cast<uint16_t>(mark2ArrayOffset));

        appendGposIRMarkMarkU16(data, static_cast<uint16_t>(mark2.size()));

        const size_t mark2RecordBase = data.size();

        for (size_t i = 0; i < mark2.size(); ++i)
        {
            for (uint16_t markClass = 0; markClass < markClassCount; ++markClass)
                appendGposIRMarkMarkU16(data, 0);
        }

        for (size_t mark2Index = 0; mark2Index < mark2.size(); ++mark2Index)
        {
            const GposIRMarkMarkMark2Spec& record = mark2[mark2Index];

            for (uint16_t markClass = 0; markClass < markClassCount; ++markClass)
            {
                if (!record.hasAnchor[markClass])
                    continue;

                const size_t anchorOffset = data.size() - mark2ArrayOffset;
                const size_t patchOffset =
                    mark2RecordBase +
                    (mark2Index * markClassCount + markClass) * 2;

                patchGposIRMarkMarkU16(
                    data, patchOffset,
                    static_cast<uint16_t>(anchorOffset));

                appendGposIRMarkMarkAnchor1(
                    data, record.x[markClass], record.y[markClass]);
            }
        }


        // Coverages.

        const size_t mark1CoverageOffset = data.size();
        patchGposIRMarkMarkU16(
            data, mark1CoveragePatch,
            static_cast<uint16_t>(mark1CoverageOffset));

        std::vector<uint16_t> mark1Glyphs;
        mark1Glyphs.reserve(mark1.size());

        for (const GposIRMarkMarkMark1Spec& record : mark1)
            mark1Glyphs.push_back(record.glyph);

        appendGposIRMarkMarkCoverage1(data, mark1Glyphs);

        const size_t mark2CoverageOffset = data.size();
        patchGposIRMarkMarkU16(
            data, mark2CoveragePatch,
            static_cast<uint16_t>(mark2CoverageOffset));

        std::vector<uint16_t> mark2Glyphs;
        mark2Glyphs.reserve(mark2.size());

        for (const GposIRMarkMarkMark2Spec& record : mark2)
            mark2Glyphs.push_back(record.glyph);

        appendGposIRMarkMarkCoverage1(data, mark2Glyphs);

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkMarkLookup(
        const std::vector<std::vector<uint8_t>>& subtables, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkMarkU16(data, 6);
        appendGposIRMarkMarkU16(data, lookupFlag);
        appendGposIRMarkMarkU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRMarkMarkU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();

            patchGposIRMarkMarkU16(
                data, patchBase + i * 2,
                static_cast<uint16_t>(offset));

            data.insert(data.end(), subtables[i].begin(), subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkMarkExtensionLookup(
        const std::vector<uint8_t>& subtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkMarkU16(data, 9);
        appendGposIRMarkMarkU16(data, lookupFlag);
        appendGposIRMarkMarkU16(data, 1);
        appendGposIRMarkMarkU16(data, 8);

        const size_t extensionBase = data.size();

        appendGposIRMarkMarkU16(data, 1);
        appendGposIRMarkMarkU16(data, 6);

        const size_t extensionPatch = data.size();
        appendGposIRMarkMarkU32(data, 0);

        const size_t subtableOffset = data.size();

        patchGposIRMarkMarkU32(
            data, extensionPatch,
            static_cast<uint32_t>(subtableOffset - extensionBase));

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkMarkGdef(
        const std::vector<GposIRMarkMarkGdefClassSpec>& classes)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkMarkU16(data, 1);
        appendGposIRMarkMarkU16(data, 0);

        appendGposIRMarkMarkU16(data, 12);
        appendGposIRMarkMarkU16(data, 0);
        appendGposIRMarkMarkU16(data, 0);
        appendGposIRMarkMarkU16(data, 0);

        appendGposIRMarkMarkU16(data, 2);
        appendGposIRMarkMarkU16(data, static_cast<uint16_t>(classes.size()));

        for (const GposIRMarkMarkGdefClassSpec& entry : classes)
        {
            appendGposIRMarkMarkU16(data, entry.glyph);
            appendGposIRMarkMarkU16(data, entry.glyph);
            appendGposIRMarkMarkU16(data, entry.glyphClass);
        }

        return data;
    }


    static void appendGposIRMarkMarkGlyph(
        ShapedGlyphBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset,
        int32_t advanceX = 500, int32_t advanceY = 0,
        int32_t offsetX = 0, int32_t offsetY = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = scalarOffset;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;
        glyph.placement.advanceY = advanceY;
        glyph.placement.offsetX = offsetX;
        glyph.placement.offsetY = offsetY;

        buffer.pushBack(glyph);
    }


    static ShapedGlyphBuffer makeGposIRMarkMarkBuffer(const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRMarkMarkGlyph(
                buffer, glyphs[i], static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i * 17),
                static_cast<int32_t>(i),
                static_cast<int32_t>(i * 4),
                -static_cast<int32_t>(i * 3));
        }

        return buffer;
    }


    static bool gposIRMarkMarkShapingEqual(
        const OpenTypeShapingGlyph& a, const OpenTypeShapingGlyph& b) noexcept
    {
        return
            a.glyphId == b.glyphId &&
            a.scalarOffset == b.scalarOffset &&
            a.scalarCount == b.scalarCount &&
            a.ligature.id == b.ligature.id &&
            a.ligature.component == b.ligature.component &&
            a.ligature.componentCount == b.ligature.componentCount;
    }


    static bool gposIRMarkMarkPlacementEqual(
        const GlyphPlacement& a, const GlyphPlacement& b) noexcept
    {
        return
            a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool gposIRMarkMarkBuffersEqual(
        const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRMarkMarkShapingEqual(a[i].shaping, b[i].shaping) ||
                !gposIRMarkMarkPlacementEqual(a[i].placement, b[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRMarkMarkAttachmentsEqual(
        const OpenTypeGposAttachmentState& a,
        const OpenTypeGposAttachmentState& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (a[i].type != b[i].type ||
                a[i].parent != b[i].parent)
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRMarkMarkResultEqual(
        OpenTypeGposResolveResult raw,
        OpenTypeGposIRResult ir) noexcept
    {
        if (raw == OpenTypeGposResolveResult::Invalid)
            return ir == OpenTypeGposIRResult::Invalid;

        if (raw == OpenTypeGposResolveResult::NoMatch)
            return ir == OpenTypeGposIRResult::NoMatch;

        if (raw == OpenTypeGposResolveResult::Match)
            return ir == OpenTypeGposIRResult::Match;

        return false;
    }


    static bool compileGposIRMarkMark(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposMarkMarkLookup(
                lookup, gdef, ir, lookupId))
        {
            return false;
        }

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposMarkMark &&
            openTypeGposIRMarkMarkLookupValid(ir, *compiled);
    }


    static bool testGposIRMarkMarkResolveAt(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiled,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        const char* caseName)
    {
        const OpenTypeLookupGlyphFilter filter(rawLookup, gdef);

        if (!filter)
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Raw filter invalid\n",
                caseName);

            return false;
        }

        OpenTypeGposMarkMarkMatch rawMatch;
        OpenTypeGposIRMarkMarkMatch irMatch;

        const OpenTypeGposResolveResult rawResult =
            resolveOpenTypeGposMarkMarkLookup(
                rawLookup, filter,
                buffer, glyphIndex,
                rawMatch);

        const OpenTypeGposIRResult irResult =
            resolveOpenTypeGposIRMarkMarkLookup(
                ir, compiled,
                buffer, glyphIndex,
                irMatch);

        if (!gposIRMarkMarkResultEqual(rawResult, irResult))
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Raw result: %u\n"
                "  IR result:  %u\n",
                caseName,
                static_cast<unsigned>(rawResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (rawResult != OpenTypeGposResolveResult::Match)
            return true;

        if (rawMatch.mark1Index != irMatch.mark1Index ||
            rawMatch.mark2Index != irMatch.mark2Index ||
            rawMatch.mark1X != irMatch.mark1.x ||
            rawMatch.mark1Y != irMatch.mark1.y ||
            rawMatch.mark2X != irMatch.mark2.x ||
            rawMatch.mark2Y != irMatch.mark2.y)
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Raw: mark1=%zu mark2=%zu mark1Anchor=(%d,%d) mark2Anchor=(%d,%d)\n"
                "  IR:  mark1=%zu mark2=%zu mark1Anchor=(%d,%d) mark2Anchor=(%d,%d)\n",
                caseName,
                rawMatch.mark1Index, rawMatch.mark2Index,
                rawMatch.mark1X, rawMatch.mark1Y,
                rawMatch.mark2X, rawMatch.mark2Y,
                irMatch.mark1Index, irMatch.mark2Index,
                irMatch.mark1.x, irMatch.mark1.y,
                irMatch.mark2.x, irMatch.mark2.y);

            return false;
        }

        return true;
    }


    static bool testGposIRMarkMarkSharedExecution(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId lookupId,
        const ShapedGlyphBuffer& input,
        bool runRightToLeft,
        const char* caseName)
    {
        ShapedGlyphBuffer rawBuffer = input;
        ShapedGlyphBuffer irBuffer = input;

        OpenTypeGposAttachmentState rawAttachments;
        OpenTypeGposAttachmentState irAttachments;

        rawAttachments.reset(rawBuffer.size());
        irAttachments.reset(irBuffer.size());

        const bool rawSuccess =
            applyOpenTypeGposMarkMarkLookup(
                rawLookup, gdef,
                rawBuffer, rawAttachments);

        const bool irSuccess =
            applyOpenTypeGposIRLookup(
                ir, lookupId,
                irBuffer, runRightToLeft,
                irAttachments);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gposIRMarkMarkBuffersEqual(rawBuffer, irBuffer) ||
            !gposIRMarkMarkAttachmentsEqual(rawAttachments, irAttachments))
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Shared-state raw/IR mismatch\n",
                caseName);

            return false;
        }

        ShapedGlyphBuffer rawFinal = rawBuffer;
        ShapedGlyphBuffer irFinal = irBuffer;

        if (!resolveOpenTypeGposAttachments(
                rawFinal, rawAttachments, runRightToLeft) ||
            !resolveOpenTypeGposAttachments(
                irFinal, irAttachments, runRightToLeft))
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Attachment finalization failed\n",
                caseName);

            return false;
        }

        if (!gposIRMarkMarkBuffersEqual(rawFinal, irFinal))
        {
            std::printf(
                "GPOS IR MarkMark: FAIL\n"
                "  Case: %s\n"
                "  Finalized raw/IR mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRMarkMark()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR MarkMark: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - basic Mark1 -> Mark2 attachment.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkMarkMark1Spec> mark1 =
            {
                { 20, 0, 30, 40 }
            };

            const std::vector<GposIRMarkMarkMark2Spec> mark2 =
            {
                { 10, { 1 }, { 300 }, { 500 } }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRMarkMarkSubtable(mark1, mark2, 1);

            if (subtable.empty())
                return fail("case 1 subtable construction");

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "basic resolver") ||
                !testGposIRMarkMarkSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "basic execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - multiple Mark1 classes and Mark2 rows.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkMarkMark1Spec> mark1 =
            {
                { 20, 0, 10, 20 },
                { 21, 1, 30, 40 }
            };

            const std::vector<GposIRMarkMarkMark2Spec> mark2 =
            {
                { 10, { 1, 1 }, { 100, 200 }, { 300, 400 } },
                { 11, { 1, 1 }, { 500, 600 }, { 700, 800 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(mark1, mark2, 2)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            if (compiled->payloadCount != 1 ||
                ir.gposMarkRecords.size() != 2 ||
                ir.gposMark2Records.size() != 2 ||
                ir.gposMark2AnchorRefs.size() != 4)
            {
                return fail("case 2 semantic representation");
            }

            const uint32_t glyphsA[] = { 10, 21 };
            const ShapedGlyphBuffer inputA =
                makeGposIRMarkMarkBuffer(glyphsA, 2);

            const uint32_t glyphsB[] = { 11, 20 };
            const ShapedGlyphBuffer inputB =
                makeGposIRMarkMarkBuffer(glyphsB, 2);

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    inputA, 1, "class 1 resolver") ||
                !testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    inputB, 1, "class 0 resolver"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - normal LookupFlag filtering is used while walking backward.
        //
        // Glyph 11 is GDEF base class 1 and IgnoreBaseGlyphs is set, so Mark2
        // glyph 10 is found behind it.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkMarkMark1Spec> mark1 =
            {
                { 20, 0, 20, 30 }
            };

            const std::vector<GposIRMarkMarkMark2Spec> mark2 =
            {
                { 10, { 1 }, { 300 }, { 450 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(mark1, mark2, 1)
                    }, 0x0002);

            const std::vector<uint8_t> gdefBytes =
                makeGposIRMarkMarkGdef({
                    { 10, 3 },
                    { 11, 1 },
                    { 20, 3 }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefBytes.data(), gdefBytes.size()));

            if (!lookup || !gdef)
                return fail("case 3 raw views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            const uint32_t glyphs[] = { 10, 11, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 3);

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 2, "filtered backward traversal") ||
                !testGposIRMarkMarkSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "filtered execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - same non-zero ligature association and same component.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        { { 20, 0, 20, 30 } },
                        { { 10, { 1 }, { 300 }, { 450 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 4 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            input[0].shaping.ligature.id = 71;
            input[0].shaping.ligature.component = 2;
            input[0].shaping.ligature.componentCount = 3;

            input[1].shaping.ligature.id = 71;
            input[1].shaping.ligature.component = 2;
            input[1].shaping.ligature.componentCount = 3;

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "same component"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - same ligature association but different components rejects.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        { { 20, 0, 20, 30 } },
                        { { 10, { 1 }, { 300 }, { 450 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            input[0].shaping.ligature.id = 72;
            input[0].shaping.ligature.component = 1;
            input[0].shaping.ligature.componentCount = 3;

            input[1].shaping.ligature.id = 72;
            input[1].shaping.ligature.component = 2;
            input[1].shaping.ligature.componentCount = 3;

            OpenTypeGposMarkMarkMatch rawMatch;
            OpenTypeGposIRMarkMarkMatch irMatch;
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposResolveResult rawResult =
                resolveOpenTypeGposMarkMarkLookup(
                    lookup, filter, input, 1, rawMatch);

            const OpenTypeGposIRResult irResult =
                resolveOpenTypeGposIRMarkMarkLookup(
                    ir, *compiled, input, 1, irMatch);

            if (rawResult != OpenTypeGposResolveResult::NoMatch ||
                irResult != OpenTypeGposIRResult::NoMatch)
            {
                return fail("case 5 incompatible components");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - different ligature IDs are allowed when one glyph is the
        // ligature base representation (component zero).
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        { { 20, 0, 20, 30 } },
                        { { 10, { 1 }, { 300 }, { 450 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 6 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            input[0].shaping.ligature.id = 80;
            input[0].shaping.ligature.component = 0;
            input[0].shaping.ligature.componentCount = 2;

            input[1].shaping.ligature.id = 81;
            input[1].shaping.ligature.component = 2;
            input[1].shaping.ligature.componentCount = 2;

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "ligature base exception"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - NULL Mark2 anchor falls through to a later subtable.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkMarkMark1Spec> mark1 =
            {
                { 20, 0, 20, 25 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        mark1,
                        { { 10, { 0 }, { 0 }, { 0 } } },
                        1),
                    makeGposIRMarkMarkSubtable(
                        mark1,
                        { { 10, { 1 }, { 333 }, { 444 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 7 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "NULL anchor fallthrough") ||
                !testGposIRMarkMarkSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "NULL anchor execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkMarkMark1Spec> mark1 =
            {
                { 20, 0, 25, 50 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        mark1,
                        { { 10, { 1 }, { 300 }, { 500 } } },
                        1),
                    makeGposIRMarkMarkSubtable(
                        mark1,
                        { { 10, { 1 }, { 900 }, { 1200 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 8 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer irBuffer =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(irBuffer.size());

            if (!applyOpenTypeGposIRLookup(
                    ir, lookupId, irBuffer,
                    false, attachments))
            {
                return fail("case 8 IR execution");
            }

            if (irBuffer[1].placement.offsetX != 275 ||
                irBuffer[1].placement.offsetY != 450 ||
                attachments[1].type != OpenTypeGposAttachmentType::Mark ||
                attachments[1].parent != 0)
            {
                return fail("case 8 first subtable did not win");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - preceding eligible glyph is the only Mark2 candidate.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        { { 20, 0, 20, 30 } },
                        { { 10, { 1 }, { 300 }, { 400 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 9 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 9 compilation");
            }

            const uint32_t glyphs[] = { 10, 11, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 3);

            OpenTypeGposMarkMarkMatch rawMatch;
            OpenTypeGposIRMarkMarkMatch irMatch;
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            const OpenTypeGposResolveResult rawResult =
                resolveOpenTypeGposMarkMarkLookup(
                    lookup, filter, input, 2, rawMatch);

            const OpenTypeGposIRResult irResult =
                resolveOpenTypeGposIRMarkMarkLookup(
                    ir, *compiled, input, 2, irMatch);

            if (rawResult != OpenTypeGposResolveResult::NoMatch ||
                irResult != OpenTypeGposIRResult::NoMatch)
            {
                return fail("case 9 farther Mark2 search occurred");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10 - ExtensionPos Type 9 -> Type 6.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGposIRMarkMarkSubtable(
                    { { 20, 0, 35, 45 } },
                    { { 10, { 1 }, { 310 }, { 470 } } },
                    1);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkExtensionLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 10 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGposIREffectiveType(
                    lookup, effectiveType) ||
                effectiveType != 6)
            {
                return fail("case 10 effective type");
            }

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 10 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkMarkBuffer(glyphs, 2);

            if (!testGposIRMarkMarkResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "Type 9 -> Type 6 resolver") ||
                !testGposIRMarkMarkSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "Type 9 -> Type 6 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 11 - exact-position apply, final resolve and provenance.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkMarkLookup({
                    makeGposIRMarkMarkSubtable(
                        { { 20, 0, 40, 60 } },
                        { { 10, { 1 }, { 300 }, { 500 } } },
                        1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 11 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkMark(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 11 compilation");
            }

            ShapedGlyphBuffer rawBuffer;

            appendGposIRMarkMarkGlyph(
                rawBuffer, 10, 4,
                0, 0, 13, -7);

            appendGposIRMarkMarkGlyph(
                rawBuffer, 20, 8,
                0, 0, -5, 9);

            rawBuffer[0].shaping.scalarCount = 2;
            rawBuffer[0].shaping.ligature.id = 91;
            rawBuffer[0].shaping.ligature.component = 2;
            rawBuffer[0].shaping.ligature.componentCount = 3;

            rawBuffer[1].shaping.scalarCount = 3;
            rawBuffer[1].shaping.ligature.id = 91;
            rawBuffer[1].shaping.ligature.component = 2;
            rawBuffer[1].shaping.ligature.componentCount = 3;

            const OpenTypeShapingGlyph mark2Shaping =
                rawBuffer[0].shaping;

            const OpenTypeShapingGlyph mark1Shaping =
                rawBuffer[1].shaping;

            ShapedGlyphBuffer irBuffer = rawBuffer;

            OpenTypeGposAttachmentState rawAttachments;
            OpenTypeGposAttachmentState irAttachments;

            rawAttachments.reset(2);
            irAttachments.reset(2);

            const OpenTypeGposResolveResult rawResult =
                applyOpenTypeGposMarkMarkLookupAt(
                    lookup, gdef,
                    rawBuffer, 1,
                    rawAttachments);

            const OpenTypeGposIRResult irResult =
                applyOpenTypeGposIRMarkMarkLookupAt(
                    ir, *compiled,
                    irBuffer, 1,
                    irAttachments);

            if (rawResult != OpenTypeGposResolveResult::Match ||
                irResult != OpenTypeGposIRResult::Match ||
                !gposIRMarkMarkBuffersEqual(rawBuffer, irBuffer) ||
                !gposIRMarkMarkAttachmentsEqual(rawAttachments, irAttachments))
            {
                return fail("case 11 exact-position differential");
            }

            if (!gposIRMarkMarkShapingEqual(
                    irBuffer[0].shaping, mark2Shaping) ||
                !gposIRMarkMarkShapingEqual(
                    irBuffer[1].shaping, mark1Shaping))
            {
                return fail("case 11 provenance changed");
            }

            if (irBuffer[0].placement.offsetX != 13 ||
                irBuffer[0].placement.offsetY != -7 ||
                irBuffer[1].placement.offsetX != 260 ||
                irBuffer[1].placement.offsetY != 440 ||
                irAttachments[1].type != OpenTypeGposAttachmentType::Mark ||
                irAttachments[1].parent != 0)
            {
                return fail("case 11 local attachment result");
            }

            ShapedGlyphBuffer rawFinal = rawBuffer;
            ShapedGlyphBuffer irFinal = irBuffer;

            if (!resolveOpenTypeGposAttachments(
                    rawFinal, rawAttachments, false) ||
                !resolveOpenTypeGposAttachments(
                    irFinal, irAttachments, false) ||
                !gposIRMarkMarkBuffersEqual(rawFinal, irFinal))
            {
                return fail("case 11 final attachment resolve");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR MarkMark: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Basic attachment:         PASS\n"
            "  Multiple mark classes:    PASS\n"
            "  Filtered backward walk:   PASS\n"
            "  Same component:           PASS\n"
            "  Different component:      PASS\n"
            "  Ligature-base exception:  PASS\n"
            "  NULL anchor fallthrough:  PASS\n"
            "  Subtable order:           PASS\n"
            "  First eligible boundary:  PASS\n"
            "  Type 9 -> Type 6:         PASS\n"
            "  Exact-position apply:     PASS\n"
            "  Final attachment resolve: PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
