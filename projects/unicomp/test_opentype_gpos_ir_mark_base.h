// test_opentype_gpos_ir_mark_base.h
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
#include "opentype_gpos_lookup_apply.h"
#include "opentype_layout_view.h"

namespace waavs
{
    static void appendGposIRMarkBaseU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRMarkBaseU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRMarkBaseS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRMarkBaseU16(data, static_cast<uint16_t>(value));
    }


    static void patchGposIRMarkBaseU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRMarkBaseU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRMarkBaseCoverage1(std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRMarkBaseU16(data, 1);
        appendGposIRMarkBaseU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRMarkBaseU16(data, glyph);
    }


    static void appendGposIRMarkBaseAnchor1(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposIRMarkBaseU16(data, 1);
        appendGposIRMarkBaseS16(data, x);
        appendGposIRMarkBaseS16(data, y);
    }


    struct GposIRMarkBaseMarkSpec
    {
        uint16_t glyph{ 0 };
        uint16_t markClass{ 0 };
        int16_t x{ 0 };
        int16_t y{ 0 };
    };


    struct GposIRMarkBaseBaseSpec
    {
        uint16_t glyph{ 0 };
        std::vector<uint8_t> hasAnchor{};
        std::vector<int16_t> x{};
        std::vector<int16_t> y{};
    };


    static std::vector<uint8_t> makeGposIRMarkBaseSubtable(
        const std::vector<GposIRMarkBaseMarkSpec>& marks,
        const std::vector<GposIRMarkBaseBaseSpec>& bases,
        uint16_t markClassCount)
    {
        if (markClassCount == 0)
            return {};

        for (const GposIRMarkBaseBaseSpec& base : bases)
        {
            if (base.hasAnchor.size() != markClassCount ||
                base.x.size() != markClassCount ||
                base.y.size() != markClassCount)
            {
                return {};
            }
        }

        std::vector<uint8_t> data;

        appendGposIRMarkBaseU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposIRMarkBaseU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposIRMarkBaseU16(data, 0);

        appendGposIRMarkBaseU16(data, markClassCount);

        const size_t markArrayPatch = data.size();
        appendGposIRMarkBaseU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposIRMarkBaseU16(data, 0);


        // MarkArray.

        const size_t markArrayOffset = data.size();
        patchGposIRMarkBaseU16(data, markArrayPatch, static_cast<uint16_t>(markArrayOffset));

        appendGposIRMarkBaseU16(data, static_cast<uint16_t>(marks.size()));

        const size_t markRecordBase = data.size();

        for (const GposIRMarkBaseMarkSpec& mark : marks)
        {
            appendGposIRMarkBaseU16(data, mark.markClass);
            appendGposIRMarkBaseU16(data, 0);
        }

        for (size_t i = 0; i < marks.size(); ++i)
        {
            const size_t anchorOffset = data.size() - markArrayOffset;

            patchGposIRMarkBaseU16(
                data, markRecordBase + i * 4 + 2,
                static_cast<uint16_t>(anchorOffset));

            appendGposIRMarkBaseAnchor1(data, marks[i].x, marks[i].y);
        }


        // BaseArray.

        const size_t baseArrayOffset = data.size();
        patchGposIRMarkBaseU16(data, baseArrayPatch, static_cast<uint16_t>(baseArrayOffset));

        appendGposIRMarkBaseU16(data, static_cast<uint16_t>(bases.size()));

        const size_t baseRecordBase = data.size();

        for (size_t baseIndex = 0; baseIndex < bases.size(); ++baseIndex)
        {
            for (uint16_t markClass = 0; markClass < markClassCount; ++markClass)
                appendGposIRMarkBaseU16(data, 0);
        }

        for (size_t baseIndex = 0; baseIndex < bases.size(); ++baseIndex)
        {
            const GposIRMarkBaseBaseSpec& base = bases[baseIndex];

            for (uint16_t markClass = 0; markClass < markClassCount; ++markClass)
            {
                if (!base.hasAnchor[markClass])
                    continue;

                const size_t anchorOffset = data.size() - baseArrayOffset;
                const size_t patchOffset =
                    baseRecordBase +
                    (baseIndex * markClassCount + markClass) * 2;

                patchGposIRMarkBaseU16(
                    data, patchOffset,
                    static_cast<uint16_t>(anchorOffset));

                appendGposIRMarkBaseAnchor1(
                    data, base.x[markClass], base.y[markClass]);
            }
        }


        // Coverages.

        const size_t markCoverageOffset = data.size();
        patchGposIRMarkBaseU16(
            data, markCoveragePatch,
            static_cast<uint16_t>(markCoverageOffset));

        std::vector<uint16_t> markGlyphs;
        markGlyphs.reserve(marks.size());

        for (const GposIRMarkBaseMarkSpec& mark : marks)
            markGlyphs.push_back(mark.glyph);

        appendGposIRMarkBaseCoverage1(data, markGlyphs);

        const size_t baseCoverageOffset = data.size();
        patchGposIRMarkBaseU16(
            data, baseCoveragePatch,
            static_cast<uint16_t>(baseCoverageOffset));

        std::vector<uint16_t> baseGlyphs;
        baseGlyphs.reserve(bases.size());

        for (const GposIRMarkBaseBaseSpec& base : bases)
            baseGlyphs.push_back(base.glyph);

        appendGposIRMarkBaseCoverage1(data, baseGlyphs);

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkBaseLookup(
        const std::vector<std::vector<uint8_t>>& subtables, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkBaseU16(data, 4);
        appendGposIRMarkBaseU16(data, lookupFlag);
        appendGposIRMarkBaseU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRMarkBaseU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();

            patchGposIRMarkBaseU16(
                data, patchBase + i * 2,
                static_cast<uint16_t>(offset));

            data.insert(data.end(), subtables[i].begin(), subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkBaseExtensionLookup(
        const std::vector<uint8_t>& markBaseSubtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkBaseU16(data, 9);
        appendGposIRMarkBaseU16(data, lookupFlag);
        appendGposIRMarkBaseU16(data, 1);
        appendGposIRMarkBaseU16(data, 8);

        const size_t extensionBase = data.size();

        appendGposIRMarkBaseU16(data, 1);
        appendGposIRMarkBaseU16(data, 4);

        const size_t extensionPatch = data.size();
        appendGposIRMarkBaseU32(data, 0);

        const size_t subtableOffset = data.size();

        patchGposIRMarkBaseU32(
            data, extensionPatch,
            static_cast<uint32_t>(subtableOffset - extensionBase));

        data.insert(data.end(), markBaseSubtable.begin(), markBaseSubtable.end());
        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkBaseGdef(
        const std::vector<uint16_t>& markGlyphs)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkBaseU16(data, 1);
        appendGposIRMarkBaseU16(data, 0);

        appendGposIRMarkBaseU16(data, 12);
        appendGposIRMarkBaseU16(data, 0);
        appendGposIRMarkBaseU16(data, 0);
        appendGposIRMarkBaseU16(data, 0);

        appendGposIRMarkBaseU16(data, 2);
        appendGposIRMarkBaseU16(data, static_cast<uint16_t>(markGlyphs.size()));

        for (uint16_t glyph : markGlyphs)
        {
            appendGposIRMarkBaseU16(data, glyph);
            appendGposIRMarkBaseU16(data, glyph);
            appendGposIRMarkBaseU16(data, 3);
        }

        return data;
    }


    static void appendGposIRMarkBaseGlyph(
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


    static ShapedGlyphBuffer makeGposIRMarkBaseBuffer(const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRMarkBaseGlyph(
                buffer, glyphs[i], static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i * 11),
                static_cast<int32_t>(i),
                static_cast<int32_t>(i * 5),
                -static_cast<int32_t>(i * 3));
        }

        return buffer;
    }


    static bool gposIRMarkBaseShapingEqual(
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


    static bool gposIRMarkBasePlacementEqual(
        const GlyphPlacement& a, const GlyphPlacement& b) noexcept
    {
        return
            a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool gposIRMarkBaseBuffersEqual(
        const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRMarkBaseShapingEqual(a[i].shaping, b[i].shaping) ||
                !gposIRMarkBasePlacementEqual(a[i].placement, b[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRMarkBaseAttachmentsEqual(
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


    static bool gposIRMarkBaseResultEqual(
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


    static bool compileGposIRMarkBase(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposMarkBaseLookup(
                lookup, gdef, ir, lookupId))
        {
            return false;
        }

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposMarkBase &&
            openTypeGposIRMarkBaseLookupValid(ir, *compiled);
    }


    static bool testGposIRMarkBaseResolveAt(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiled,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        const char* caseName)
    {
        OpenTypeGposMarkBaseMatch rawMatch;
        OpenTypeGposIRMarkBaseMatch irMatch;

        const OpenTypeGposResolveResult rawResult =
            resolveOpenTypeGposMarkBaseLookup(
                rawLookup, gdef, buffer,
                glyphIndex, rawMatch);

        const OpenTypeGposIRResult irResult =
            resolveOpenTypeGposIRMarkBaseLookup(
                ir, compiled, buffer,
                glyphIndex, irMatch);

        if (!gposIRMarkBaseResultEqual(rawResult, irResult))
        {
            std::printf(
                "GPOS IR MarkBase: FAIL\n"
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

        if (rawMatch.baseIndex != irMatch.baseIndex ||
            rawMatch.markIndex != irMatch.markIndex ||
            rawMatch.baseX != irMatch.base.x ||
            rawMatch.baseY != irMatch.base.y ||
            rawMatch.markX != irMatch.mark.x ||
            rawMatch.markY != irMatch.mark.y)
        {
            std::printf(
                "GPOS IR MarkBase: FAIL\n"
                "  Case: %s\n"
                "  Raw: base=%zu mark=%zu baseAnchor=(%d,%d) markAnchor=(%d,%d)\n"
                "  IR:  base=%zu mark=%zu baseAnchor=(%d,%d) markAnchor=(%d,%d)\n",
                caseName,
                rawMatch.baseIndex, rawMatch.markIndex,
                rawMatch.baseX, rawMatch.baseY,
                rawMatch.markX, rawMatch.markY,
                irMatch.baseIndex, irMatch.markIndex,
                irMatch.base.x, irMatch.base.y,
                irMatch.mark.x, irMatch.mark.y);

            return false;
        }

        return true;
    }


    static bool testGposIRMarkBaseSharedExecution(
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
            applyOpenTypeGposMarkBaseLookup(
                rawLookup, gdef,
                rawBuffer, rawAttachments);

        const bool irSuccess =
            applyOpenTypeGposIRLookup(
                ir, lookupId, irBuffer,
                runRightToLeft, irAttachments);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GPOS IR MarkBase: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gposIRMarkBaseBuffersEqual(rawBuffer, irBuffer) ||
            !gposIRMarkBaseAttachmentsEqual(rawAttachments, irAttachments))
        {
            std::printf(
                "GPOS IR MarkBase: FAIL\n"
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
                "GPOS IR MarkBase: FAIL\n"
                "  Case: %s\n"
                "  Attachment finalization failed\n",
                caseName);

            return false;
        }

        if (!gposIRMarkBaseBuffersEqual(rawFinal, irFinal))
        {
            std::printf(
                "GPOS IR MarkBase: FAIL\n"
                "  Case: %s\n"
                "  Finalized raw/IR mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRMarkBase()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR MarkBase: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - basic MarkToBase attachment.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 30, 40 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> bases =
            {
                { 10, { 1 }, { 300 }, { 500 } }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRMarkBaseSubtable(marks, bases, 1);

            if (subtable.empty())
                return fail("case 1 subtable construction");

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 2);

            if (!testGposIRMarkBaseResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "basic resolver") ||
                !testGposIRMarkBaseSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "basic execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - multiple mark classes and multiple bases.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 10, 20 },
                { 21, 1, 30, 40 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> bases =
            {
                { 10, { 1, 1 }, { 100, 200 }, { 300, 400 } },
                { 11, { 1, 1 }, { 500, 600 }, { 700, 800 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({
                    makeGposIRMarkBaseSubtable(marks, bases, 2)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            if (compiled->payloadCount != 1 ||
                ir.gposMarkRecords.size() != 2 ||
                ir.gposMarkBaseRecords.size() != 2 ||
                ir.gposMarkBaseAnchorRefs.size() != 4)
            {
                return fail("case 2 semantic representation");
            }

            const uint32_t glyphs[] = { 10, 21, 11, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 4);

            if (!testGposIRMarkBaseResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "class 1 resolver") ||
                !testGposIRMarkBaseResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 3, "class 0 resolver") ||
                !testGposIRMarkBaseSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "multi-class execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - intrinsic GDEF traversal skips preceding marks even when
        // LookupFlag IgnoreMarks is clear.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 20, 30 },
                { 21, 0, 25, 35 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> bases =
            {
                { 10, { 1 }, { 300 }, { 450 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({
                    makeGposIRMarkBaseSubtable(marks, bases, 1)
                    });

            const std::vector<uint8_t> gdefBytes =
                makeGposIRMarkBaseGdef({ 20, 21 });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefBytes.data(), gdefBytes.size()));

            if (!lookup || !gdef)
                return fail("case 3 raw views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            if (ir.gdefGlyphClasses.size() != kOpenTypeShapingIRGlyphDomainSize ||
                ir.gdefGlyphClasses[20] != 3 ||
                ir.gdefGlyphClasses[21] != 3)
            {
                return fail("case 3 GDEF class normalization");
            }

            const uint32_t glyphs[] = { 10, 20, 21 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 3);

            if (!testGposIRMarkBaseResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 2, "skip preceding mark") ||
                !testGposIRMarkBaseSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "intrinsic GDEF traversal"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - NULL BaseAnchor falls through to a later subtable.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 20, 25 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> firstBases =
            {
                { 10, { 0 }, { 0 }, { 0 } }
            };

            const std::vector<GposIRMarkBaseBaseSpec> secondBases =
            {
                { 10, { 1 }, { 333 }, { 444 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({
                    makeGposIRMarkBaseSubtable(marks, firstBases, 1),
                    makeGposIRMarkBaseSubtable(marks, secondBases, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 4 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            if (compiled->payloadCount != 2)
                return fail("case 4 subtable count");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 2);

            if (!testGposIRMarkBaseResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "NULL BaseAnchor fallthrough") ||
                !testGposIRMarkBaseSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "NULL BaseAnchor execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 25, 50 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> firstBases =
            {
                { 10, { 1 }, { 300 }, { 500 } }
            };

            const std::vector<GposIRMarkBaseBaseSpec> secondBases =
            {
                { 10, { 1 }, { 900 }, { 1200 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({
                    makeGposIRMarkBaseSubtable(marks, firstBases, 1),
                    makeGposIRMarkBaseSubtable(marks, secondBases, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer irBuffer =
                makeGposIRMarkBaseBuffer(glyphs, 2);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(irBuffer.size());

            if (!applyOpenTypeGposIRLookup(
                    ir, lookupId, irBuffer,
                    false, attachments))
            {
                return fail("case 5 IR execution");
            }

            if (irBuffer[1].placement.offsetX != 275 ||
                irBuffer[1].placement.offsetY != 450 ||
                attachments[1].type != OpenTypeGposAttachmentType::Mark ||
                attachments[1].parent != 0)
            {
                return fail("case 5 first subtable did not win");
            }

            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 2);

            if (!testGposIRMarkBaseSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "subtable order"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - uncovered base does not cause a farther base search.
        //
        // The first non-mark glyph is 11. Base 10 farther back is covered, but
        // Type 4 must stop at glyph 11 and report NoMatch.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 20, 30 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> bases =
            {
                { 10, { 1 }, { 300 }, { 400 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({
                    makeGposIRMarkBaseSubtable(marks, bases, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 6 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t glyphs[] = { 10, 11, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 3);

            OpenTypeGposMarkBaseMatch rawMatch;
            OpenTypeGposIRMarkBaseMatch irMatch;

            const OpenTypeGposResolveResult rawResult =
                resolveOpenTypeGposMarkBaseLookup(
                    lookup, gdef, input, 2, rawMatch);

            const OpenTypeGposIRResult irResult =
                resolveOpenTypeGposIRMarkBaseLookup(
                    ir, *compiled, input, 2, irMatch);

            if (rawResult != OpenTypeGposResolveResult::NoMatch ||
                irResult != OpenTypeGposIRResult::NoMatch)
            {
                return fail("case 6 farther-base search occurred");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - ExtensionPos Type 9 -> MarkBase Type 4.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 35, 45 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> bases =
            {
                { 10, { 1 }, { 310 }, { 470 } }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRMarkBaseSubtable(marks, bases, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseExtensionLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 7 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGposIREffectiveType(
                    lookup, effectiveType) ||
                effectiveType != 4)
            {
                return fail("case 7 effective type");
            }

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkBaseBuffer(glyphs, 2);

            if (!testGposIRMarkBaseResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 1, "Type 9 -> Type 4 resolver") ||
                !testGposIRMarkBaseSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "Type 9 -> Type 4 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - exact-position application, final resolve and provenance.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkBaseMarkSpec> marks =
            {
                { 20, 0, 40, 60 }
            };

            const std::vector<GposIRMarkBaseBaseSpec> bases =
            {
                { 10, { 1 }, { 300 }, { 500 } }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkBaseLookup({
                    makeGposIRMarkBaseSubtable(marks, bases, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 8 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRMarkBase(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            ShapedGlyphBuffer rawBuffer;
            appendGposIRMarkBaseGlyph(rawBuffer, 10, 4, 600, 2, 13, -7);
            appendGposIRMarkBaseGlyph(rawBuffer, 20, 8, 0, 0, -5, 9);

            rawBuffer[0].shaping.scalarCount = 2;
            rawBuffer[0].shaping.ligature.id = 71;
            rawBuffer[0].shaping.ligature.component = 1;
            rawBuffer[0].shaping.ligature.componentCount = 3;

            rawBuffer[1].shaping.scalarCount = 3;
            rawBuffer[1].shaping.ligature.id = 72;
            rawBuffer[1].shaping.ligature.component = 2;
            rawBuffer[1].shaping.ligature.componentCount = 4;

            const OpenTypeShapingGlyph baseShaping = rawBuffer[0].shaping;
            const OpenTypeShapingGlyph markShaping = rawBuffer[1].shaping;

            ShapedGlyphBuffer irBuffer = rawBuffer;

            OpenTypeGposAttachmentState rawAttachments;
            OpenTypeGposAttachmentState irAttachments;

            rawAttachments.reset(2);
            irAttachments.reset(2);

            const OpenTypeGposResolveResult rawResult =
                applyOpenTypeGposMarkBaseLookupAt(
                    lookup, gdef, rawBuffer,
                    1, rawAttachments);

            const OpenTypeGposIRResult irResult =
                applyOpenTypeGposIRMarkBaseLookupAt(
                    ir, *compiled, irBuffer,
                    1, irAttachments);

            if (rawResult != OpenTypeGposResolveResult::Match ||
                irResult != OpenTypeGposIRResult::Match ||
                !gposIRMarkBaseBuffersEqual(rawBuffer, irBuffer) ||
                !gposIRMarkBaseAttachmentsEqual(rawAttachments, irAttachments))
            {
                return fail("case 8 exact-position differential");
            }

            if (!gposIRMarkBaseShapingEqual(irBuffer[0].shaping, baseShaping) ||
                !gposIRMarkBaseShapingEqual(irBuffer[1].shaping, markShaping))
            {
                return fail("case 8 provenance changed");
            }

            if (irBuffer[0].placement.advanceX != 600 ||
                irBuffer[0].placement.advanceY != 2 ||
                irBuffer[0].placement.offsetX != 13 ||
                irBuffer[0].placement.offsetY != -7 ||
                irBuffer[1].placement.offsetX != 260 ||
                irBuffer[1].placement.offsetY != 440 ||
                irAttachments[1].type != OpenTypeGposAttachmentType::Mark ||
                irAttachments[1].parent != 0)
            {
                return fail("case 8 local attachment result");
            }

            ShapedGlyphBuffer rawFinal = rawBuffer;
            ShapedGlyphBuffer irFinal = irBuffer;

            if (!resolveOpenTypeGposAttachments(
                    rawFinal, rawAttachments, false) ||
                !resolveOpenTypeGposAttachments(
                    irFinal, irAttachments, false) ||
                !gposIRMarkBaseBuffersEqual(rawFinal, irFinal))
            {
                return fail("case 8 final attachment resolve");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR MarkBase: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Basic attachment:         PASS\n"
            "  Multiple mark classes:    PASS\n"
            "  Intrinsic GDEF traversal: PASS\n"
            "  NULL anchor fallthrough:  PASS\n"
            "  Subtable order:           PASS\n"
            "  First non-mark boundary:  PASS\n"
            "  Type 9 -> Type 4:         PASS\n"
            "  Exact-position apply:     PASS\n"
            "  Final attachment resolve: PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
