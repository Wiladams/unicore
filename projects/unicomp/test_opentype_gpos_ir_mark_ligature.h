// test_opentype_gpos_ir_mark_ligature.h
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

namespace waavs
{
    static void appendGposIRMarkLigatureU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRMarkLigatureU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRMarkLigatureS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRMarkLigatureU16(data, static_cast<uint16_t>(value));
    }


    static void patchGposIRMarkLigatureU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRMarkLigatureU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRMarkLigatureCoverage1(std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRMarkLigatureU16(data, 1);
        appendGposIRMarkLigatureU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRMarkLigatureU16(data, glyph);
    }


    static void appendGposIRMarkLigatureAnchor1(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposIRMarkLigatureU16(data, 1);
        appendGposIRMarkLigatureS16(data, x);
        appendGposIRMarkLigatureS16(data, y);
    }


    struct GposIRMarkLigatureMarkSpec
    {
        uint16_t glyph{ 0 };
        uint16_t markClass{ 0 };
        int16_t x{ 0 };
        int16_t y{ 0 };
    };


    struct GposIRMarkLigatureLigatureSpec
    {
        uint16_t glyph{ 0 };
        uint16_t componentCount{ 0 };

        // Flattened row-major [component][markClass].
        std::vector<uint8_t> hasAnchor{};
        std::vector<int16_t> x{};
        std::vector<int16_t> y{};
    };


    static std::vector<uint8_t> makeGposIRMarkLigatureSubtable(
        const std::vector<GposIRMarkLigatureMarkSpec>& marks,
        const std::vector<GposIRMarkLigatureLigatureSpec>& ligatures,
        uint16_t markClassCount)
    {
        if (markClassCount == 0)
            return {};

        for (const GposIRMarkLigatureLigatureSpec& ligature : ligatures)
        {
            if (ligature.componentCount == 0)
                return {};

            const size_t count =
                size_t(ligature.componentCount) * markClassCount;

            if (ligature.hasAnchor.size() != count ||
                ligature.x.size() != count ||
                ligature.y.size() != count)
            {
                return {};
            }
        }

        std::vector<uint8_t> data;

        appendGposIRMarkLigatureU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposIRMarkLigatureU16(data, 0);

        const size_t ligatureCoveragePatch = data.size();
        appendGposIRMarkLigatureU16(data, 0);

        appendGposIRMarkLigatureU16(data, markClassCount);

        const size_t markArrayPatch = data.size();
        appendGposIRMarkLigatureU16(data, 0);

        const size_t ligatureArrayPatch = data.size();
        appendGposIRMarkLigatureU16(data, 0);


        // MarkArray.

        const size_t markArrayOffset = data.size();
        patchGposIRMarkLigatureU16(
            data, markArrayPatch,
            static_cast<uint16_t>(markArrayOffset));

        appendGposIRMarkLigatureU16(
            data, static_cast<uint16_t>(marks.size()));

        const size_t markRecordBase = data.size();

        for (const GposIRMarkLigatureMarkSpec& mark : marks)
        {
            appendGposIRMarkLigatureU16(data, mark.markClass);
            appendGposIRMarkLigatureU16(data, 0);
        }

        for (size_t i = 0; i < marks.size(); ++i)
        {
            const size_t anchorOffset =
                data.size() - markArrayOffset;

            patchGposIRMarkLigatureU16(
                data, markRecordBase + i * 4 + 2,
                static_cast<uint16_t>(anchorOffset));

            appendGposIRMarkLigatureAnchor1(
                data, marks[i].x, marks[i].y);
        }


        // LigatureArray.

        const size_t ligatureArrayOffset = data.size();
        patchGposIRMarkLigatureU16(
            data, ligatureArrayPatch,
            static_cast<uint16_t>(ligatureArrayOffset));

        appendGposIRMarkLigatureU16(
            data, static_cast<uint16_t>(ligatures.size()));

        const size_t ligatureOffsetPatchBase = data.size();

        for (size_t i = 0; i < ligatures.size(); ++i)
            appendGposIRMarkLigatureU16(data, 0);

        for (size_t ligatureIndex = 0;
            ligatureIndex < ligatures.size();
            ++ligatureIndex)
        {
            const GposIRMarkLigatureLigatureSpec& ligature =
                ligatures[ligatureIndex];

            const size_t attachOffset =
                data.size() - ligatureArrayOffset;

            patchGposIRMarkLigatureU16(
                data,
                ligatureOffsetPatchBase + ligatureIndex * 2,
                static_cast<uint16_t>(attachOffset));

            const size_t attachBase = data.size();

            appendGposIRMarkLigatureU16(
                data, ligature.componentCount);

            const size_t componentRecordBase = data.size();

            for (uint16_t component = 0;
                component < ligature.componentCount;
                ++component)
            {
                for (uint16_t markClass = 0;
                    markClass < markClassCount;
                    ++markClass)
                {
                    appendGposIRMarkLigatureU16(data, 0);
                }
            }

            for (uint16_t component = 0;
                component < ligature.componentCount;
                ++component)
            {
                for (uint16_t markClass = 0;
                    markClass < markClassCount;
                    ++markClass)
                {
                    const size_t flat =
                        size_t(component) * markClassCount +
                        markClass;

                    if (!ligature.hasAnchor[flat])
                        continue;

                    const size_t anchorOffset =
                        data.size() - attachBase;

                    const size_t patchOffset =
                        componentRecordBase +
                        flat * 2;

                    patchGposIRMarkLigatureU16(
                        data, patchOffset,
                        static_cast<uint16_t>(anchorOffset));

                    appendGposIRMarkLigatureAnchor1(
                        data,
                        ligature.x[flat],
                        ligature.y[flat]);
                }
            }
        }


        // Coverages.

        const size_t markCoverageOffset = data.size();

        patchGposIRMarkLigatureU16(
            data, markCoveragePatch,
            static_cast<uint16_t>(markCoverageOffset));

        std::vector<uint16_t> markGlyphs;
        markGlyphs.reserve(marks.size());

        for (const GposIRMarkLigatureMarkSpec& mark : marks)
            markGlyphs.push_back(mark.glyph);

        appendGposIRMarkLigatureCoverage1(
            data, markGlyphs);

        const size_t ligatureCoverageOffset = data.size();

        patchGposIRMarkLigatureU16(
            data, ligatureCoveragePatch,
            static_cast<uint16_t>(ligatureCoverageOffset));

        std::vector<uint16_t> ligatureGlyphs;
        ligatureGlyphs.reserve(ligatures.size());

        for (const GposIRMarkLigatureLigatureSpec& ligature : ligatures)
            ligatureGlyphs.push_back(ligature.glyph);

        appendGposIRMarkLigatureCoverage1(
            data, ligatureGlyphs);

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkLigatureLookup(
        const std::vector<std::vector<uint8_t>>& subtables,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkLigatureU16(data, 5);
        appendGposIRMarkLigatureU16(data, lookupFlag);
        appendGposIRMarkLigatureU16(
            data, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRMarkLigatureU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();

            patchGposIRMarkLigatureU16(
                data, patchBase + i * 2,
                static_cast<uint16_t>(offset));

            data.insert(
                data.end(),
                subtables[i].begin(),
                subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkLigatureExtensionLookup(
        const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkLigatureU16(data, 9);
        appendGposIRMarkLigatureU16(data, lookupFlag);
        appendGposIRMarkLigatureU16(data, 1);
        appendGposIRMarkLigatureU16(data, 8);

        const size_t extensionBase = data.size();

        appendGposIRMarkLigatureU16(data, 1);
        appendGposIRMarkLigatureU16(data, 5);

        const size_t extensionPatch = data.size();
        appendGposIRMarkLigatureU32(data, 0);

        const size_t subtableOffset = data.size();

        patchGposIRMarkLigatureU32(
            data, extensionPatch,
            static_cast<uint32_t>(
                subtableOffset - extensionBase));

        data.insert(
            data.end(),
            subtable.begin(),
            subtable.end());

        return data;
    }


    static std::vector<uint8_t> makeGposIRMarkLigatureGdef(
        const std::vector<uint16_t>& markGlyphs)
    {
        std::vector<uint8_t> data;

        appendGposIRMarkLigatureU16(data, 1);
        appendGposIRMarkLigatureU16(data, 0);

        appendGposIRMarkLigatureU16(data, 12);
        appendGposIRMarkLigatureU16(data, 0);
        appendGposIRMarkLigatureU16(data, 0);
        appendGposIRMarkLigatureU16(data, 0);

        appendGposIRMarkLigatureU16(data, 2);
        appendGposIRMarkLigatureU16(
            data, static_cast<uint16_t>(markGlyphs.size()));

        for (uint16_t glyph : markGlyphs)
        {
            appendGposIRMarkLigatureU16(data, glyph);
            appendGposIRMarkLigatureU16(data, glyph);
            appendGposIRMarkLigatureU16(data, 3);
        }

        return data;
    }


    static void appendGposIRMarkLigatureGlyph(
        ShapedGlyphBuffer& buffer,
        uint32_t glyphId,
        uint32_t scalarOffset,
        int32_t advanceX = 500,
        int32_t advanceY = 0,
        int32_t offsetX = 0,
        int32_t offsetY = 0)
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


    static ShapedGlyphBuffer makeGposIRMarkLigatureBuffer(
        const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRMarkLigatureGlyph(
                buffer, glyphs[i],
                static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i * 13),
                static_cast<int32_t>(i),
                static_cast<int32_t>(i * 4),
                -static_cast<int32_t>(i * 3));
        }

        return buffer;
    }


    static bool gposIRMarkLigatureShapingEqual(
        const OpenTypeShapingGlyph& a,
        const OpenTypeShapingGlyph& b) noexcept
    {
        return
            a.glyphId == b.glyphId &&
            a.scalarOffset == b.scalarOffset &&
            a.scalarCount == b.scalarCount &&
            a.ligature.id == b.ligature.id &&
            a.ligature.component == b.ligature.component &&
            a.ligature.componentCount == b.ligature.componentCount;
    }


    static bool gposIRMarkLigaturePlacementEqual(
        const GlyphPlacement& a,
        const GlyphPlacement& b) noexcept
    {
        return
            a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool gposIRMarkLigatureBuffersEqual(
        const ShapedGlyphBuffer& a,
        const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRMarkLigatureShapingEqual(
                    a[i].shaping, b[i].shaping) ||
                !gposIRMarkLigaturePlacementEqual(
                    a[i].placement, b[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRMarkLigatureAttachmentsEqual(
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


    static bool gposIRMarkLigatureResultEqual(
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


    static bool compileGposIRMarkLigature(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposMarkLigatureLookup(
                lookup, gdef, ir, lookupId))
        {
            return false;
        }

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op ==
                OpenTypeShapingIROp::GposMarkLigature &&
            openTypeGposIRMarkLigatureLookupValid(
                ir, *compiled);
    }


    static bool testGposIRMarkLigatureResolveAt(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiled,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        const char* caseName)
    {
        OpenTypeGposMarkLigatureMatch rawMatch;
        OpenTypeGposIRMarkLigatureMatch irMatch;

        const OpenTypeGposResolveResult rawResult =
            resolveOpenTypeGposMarkLigatureLookup(
                rawLookup, gdef,
                buffer, glyphIndex,
                rawMatch);

        const OpenTypeGposIRResult irResult =
            resolveOpenTypeGposIRMarkLigatureLookup(
                ir, compiled,
                buffer, glyphIndex,
                irMatch);

        if (!gposIRMarkLigatureResultEqual(
                rawResult, irResult))
        {
            std::printf(
                "GPOS IR MarkLigature: FAIL\n"
                "  Case: %s\n"
                "  Raw result: %u\n"
                "  IR result:  %u\n",
                caseName,
                static_cast<unsigned>(rawResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (rawResult !=
            OpenTypeGposResolveResult::Match)
        {
            return true;
        }

        if (rawMatch.ligatureIndex !=
                irMatch.ligatureIndex ||
            rawMatch.markIndex !=
                irMatch.markIndex ||
            rawMatch.componentIndex !=
                irMatch.componentIndex ||
            rawMatch.ligatureX !=
                irMatch.ligature.x ||
            rawMatch.ligatureY !=
                irMatch.ligature.y ||
            rawMatch.markX !=
                irMatch.mark.x ||
            rawMatch.markY !=
                irMatch.mark.y)
        {
            std::printf(
                "GPOS IR MarkLigature: FAIL\n"
                "  Case: %s\n"
                "  Raw: lig=%zu mark=%zu comp=%u ligAnchor=(%d,%d) markAnchor=(%d,%d)\n"
                "  IR:  lig=%zu mark=%zu comp=%u ligAnchor=(%d,%d) markAnchor=(%d,%d)\n",
                caseName,
                rawMatch.ligatureIndex,
                rawMatch.markIndex,
                static_cast<unsigned>(
                    rawMatch.componentIndex),
                rawMatch.ligatureX,
                rawMatch.ligatureY,
                rawMatch.markX,
                rawMatch.markY,
                irMatch.ligatureIndex,
                irMatch.markIndex,
                static_cast<unsigned>(
                    irMatch.componentIndex),
                irMatch.ligature.x,
                irMatch.ligature.y,
                irMatch.mark.x,
                irMatch.mark.y);

            return false;
        }

        return true;
    }


    static bool testGposIRMarkLigatureSharedExecution(
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
            applyOpenTypeGposMarkLigatureLookup(
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
                "GPOS IR MarkLigature: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gposIRMarkLigatureBuffersEqual(
                rawBuffer, irBuffer) ||
            !gposIRMarkLigatureAttachmentsEqual(
                rawAttachments, irAttachments))
        {
            std::printf(
                "GPOS IR MarkLigature: FAIL\n"
                "  Case: %s\n"
                "  Shared-state raw/IR mismatch\n",
                caseName);

            return false;
        }

        ShapedGlyphBuffer rawFinal = rawBuffer;
        ShapedGlyphBuffer irFinal = irBuffer;

        if (!resolveOpenTypeGposAttachments(
                rawFinal, rawAttachments,
                runRightToLeft) ||
            !resolveOpenTypeGposAttachments(
                irFinal, irAttachments,
                runRightToLeft))
        {
            std::printf(
                "GPOS IR MarkLigature: FAIL\n"
                "  Case: %s\n"
                "  Attachment finalization failed\n",
                caseName);

            return false;
        }

        if (!gposIRMarkLigatureBuffersEqual(
                rawFinal, irFinal))
        {
            std::printf(
                "GPOS IR MarkLigature: FAIL\n"
                "  Case: %s\n"
                "  Finalized raw/IR mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRMarkLigature()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR MarkLigature: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - basic attachment with default final component.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 30, 40 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 2,
                    { 1, 1 },
                    { 100, 300 },
                    { 200, 500 }
                }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRMarkLigatureSubtable(
                    marks, ligatures, 1);

            if (subtable.empty())
                return fail("case 1 subtable construction");

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 2);

            if (!testGposIRMarkLigatureResolveAt(
                    lookup, gdef,
                    ir, *compiled,
                    input, 1,
                    "basic resolver") ||
                !testGposIRMarkLigatureSharedExecution(
                    lookup, gdef,
                    ir, lookupId,
                    input, false,
                    "basic execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - matching ligature provenance selects a specific component.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 20, 30 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 3,
                    { 1, 1, 1 },
                    { 100, 200, 300 },
                    { 400, 500, 600 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, ligatures, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            ShapedGlyphBuffer input;

            appendGposIRMarkLigatureGlyph(
                input, 10, 0);

            appendGposIRMarkLigatureGlyph(
                input, 20, 1);

            input[0].shaping.ligature.id = 77;
            input[0].shaping.ligature.component = 0;
            input[0].shaping.ligature.componentCount = 3;

            input[1].shaping.ligature.id = 77;
            input[1].shaping.ligature.component = 2;
            input[1].shaping.ligature.componentCount = 3;

            if (!testGposIRMarkLigatureResolveAt(
                    lookup, gdef,
                    ir, *compiled,
                    input, 1,
                    "provenance component"))
            {
                return false;
            }

            OpenTypeGposIRMarkLigatureMatch irMatch;

            if (resolveOpenTypeGposIRMarkLigatureLookup(
                    ir, *compiled,
                    input, 1,
                    irMatch) !=
                    OpenTypeGposIRResult::Match ||
                irMatch.componentIndex != 1)
            {
                return fail("case 2 component selection");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - oversized provenance component clamps to final component.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 15, 25 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 2,
                    { 1, 1 },
                    { 100, 250 },
                    { 300, 450 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, ligatures, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 3 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            ShapedGlyphBuffer input;

            appendGposIRMarkLigatureGlyph(
                input, 10, 0);

            appendGposIRMarkLigatureGlyph(
                input, 20, 1);

            input[0].shaping.ligature.id = 88;
            input[0].shaping.ligature.componentCount = 2;

            input[1].shaping.ligature.id = 88;
            input[1].shaping.ligature.component = 7;
            input[1].shaping.ligature.componentCount = 7;

            if (!testGposIRMarkLigatureResolveAt(
                    lookup, gdef,
                    ir, *compiled,
                    input, 1,
                    "component clamp"))
            {
                return false;
            }

            OpenTypeGposIRMarkLigatureMatch irMatch;

            if (resolveOpenTypeGposIRMarkLigatureLookup(
                    ir, *compiled,
                    input, 1,
                    irMatch) !=
                    OpenTypeGposIRResult::Match ||
                irMatch.componentIndex != 1)
            {
                return fail("case 3 component clamp result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - intrinsic GDEF traversal skips preceding marks.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 20, 30 },
                { 21, 0, 25, 35 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 2,
                    { 1, 1 },
                    { 100, 300 },
                    { 200, 500 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, ligatures, 1)
                    });

            const std::vector<uint8_t> gdefBytes =
                makeGposIRMarkLigatureGdef(
                    { 20, 21 });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(
                    gdefBytes.data(),
                    gdefBytes.size()));

            if (!lookup || !gdef)
                return fail("case 4 raw views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            if (ir.gdefGlyphClasses.size() !=
                    kOpenTypeShapingIRGlyphDomainSize ||
                ir.gdefGlyphClasses[20] != 3 ||
                ir.gdefGlyphClasses[21] != 3)
            {
                return fail("case 4 GDEF normalization");
            }

            const uint32_t glyphs[] =
            {
                10, 20, 21
            };

            const ShapedGlyphBuffer input =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 3);

            if (!testGposIRMarkLigatureResolveAt(
                    lookup, gdef,
                    ir, *compiled,
                    input, 2,
                    "skip preceding mark") ||
                !testGposIRMarkLigatureSharedExecution(
                    lookup, gdef,
                    ir, lookupId,
                    input, false,
                    "intrinsic GDEF traversal"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - NULL component/class anchor falls through.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 20, 25 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> firstLigatures =
            {
                {
                    10, 2,
                    { 1, 0 },
                    { 100, 0 },
                    { 200, 0 }
                }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> secondLigatures =
            {
                {
                    10, 2,
                    { 1, 1 },
                    { 110, 333 },
                    { 210, 444 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, firstLigatures, 1),
                    makeGposIRMarkLigatureSubtable(
                        marks, secondLigatures, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 2);

            if (!testGposIRMarkLigatureResolveAt(
                    lookup, gdef,
                    ir, *compiled,
                    input, 1,
                    "NULL anchor fallthrough") ||
                !testGposIRMarkLigatureSharedExecution(
                    lookup, gdef,
                    ir, lookupId,
                    input, false,
                    "NULL anchor execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 25, 50 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> firstLigatures =
            {
                {
                    10, 1,
                    { 1 },
                    { 300 },
                    { 500 }
                }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> secondLigatures =
            {
                {
                    10, 1,
                    { 1 },
                    { 900 },
                    { 1200 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, firstLigatures, 1),
                    makeGposIRMarkLigatureSubtable(
                        marks, secondLigatures, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 6 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer irBuffer =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 2);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(irBuffer.size());

            if (!applyOpenTypeGposIRLookup(
                    ir, lookupId,
                    irBuffer, false,
                    attachments))
            {
                return fail("case 6 IR execution");
            }

            if (irBuffer[1].placement.offsetX != 275 ||
                irBuffer[1].placement.offsetY != 450 ||
                attachments[1].type !=
                    OpenTypeGposAttachmentType::Mark ||
                attachments[1].parent != 0)
            {
                return fail("case 6 first subtable");
            }

            const ShapedGlyphBuffer input =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 2);

            if (!testGposIRMarkLigatureSharedExecution(
                    lookup, gdef,
                    ir, lookupId,
                    input, false,
                    "subtable order"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - first non-mark is the only ligature candidate.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 20, 30 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 1,
                    { 1 },
                    { 300 },
                    { 400 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, ligatures, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 7 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t glyphs[] =
            {
                10, 11, 20
            };

            const ShapedGlyphBuffer input =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 3);

            OpenTypeGposMarkLigatureMatch rawMatch;
            OpenTypeGposIRMarkLigatureMatch irMatch;

            const OpenTypeGposResolveResult rawResult =
                resolveOpenTypeGposMarkLigatureLookup(
                    lookup, gdef,
                    input, 2,
                    rawMatch);

            const OpenTypeGposIRResult irResult =
                resolveOpenTypeGposIRMarkLigatureLookup(
                    ir, *compiled,
                    input, 2,
                    irMatch);

            if (rawResult !=
                    OpenTypeGposResolveResult::NoMatch ||
                irResult !=
                    OpenTypeGposIRResult::NoMatch)
            {
                return fail("case 7 farther search occurred");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - ExtensionPos Type 9 -> Type 5.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 35, 45 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 1,
                    { 1 },
                    { 310 },
                    { 470 }
                }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRMarkLigatureSubtable(
                    marks, ligatures, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureExtensionLookup(
                    subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 8 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGposIREffectiveType(
                    lookup, effectiveType) ||
                effectiveType != 5)
            {
                return fail("case 8 effective type");
            }

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRMarkLigatureBuffer(
                    glyphs, 2);

            if (!testGposIRMarkLigatureResolveAt(
                    lookup, gdef,
                    ir, *compiled,
                    input, 1,
                    "Type 9 -> Type 5 resolver") ||
                !testGposIRMarkLigatureSharedExecution(
                    lookup, gdef,
                    ir, lookupId,
                    input, false,
                    "Type 9 -> Type 5 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - exact-position apply, final resolve and provenance.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRMarkLigatureMarkSpec> marks =
            {
                { 20, 0, 40, 60 }
            };

            const std::vector<GposIRMarkLigatureLigatureSpec> ligatures =
            {
                {
                    10, 2,
                    { 1, 1 },
                    { 150, 300 },
                    { 250, 500 }
                }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRMarkLigatureLookup({
                    makeGposIRMarkLigatureSubtable(
                        marks, ligatures, 1)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 9 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled =
                nullptr;

            if (!compileGposIRMarkLigature(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 9 compilation");
            }

            ShapedGlyphBuffer rawBuffer;

            appendGposIRMarkLigatureGlyph(
                rawBuffer, 10, 4,
                600, 2, 13, -7);

            appendGposIRMarkLigatureGlyph(
                rawBuffer, 20, 8,
                0, 0, -5, 9);

            rawBuffer[0].shaping.scalarCount = 2;
            rawBuffer[0].shaping.ligature.id = 71;
            rawBuffer[0].shaping.ligature.component = 0;
            rawBuffer[0].shaping.ligature.componentCount = 2;

            rawBuffer[1].shaping.scalarCount = 3;
            rawBuffer[1].shaping.ligature.id = 71;
            rawBuffer[1].shaping.ligature.component = 2;
            rawBuffer[1].shaping.ligature.componentCount = 2;

            const OpenTypeShapingGlyph ligatureShaping =
                rawBuffer[0].shaping;

            const OpenTypeShapingGlyph markShaping =
                rawBuffer[1].shaping;

            ShapedGlyphBuffer irBuffer = rawBuffer;

            OpenTypeGposAttachmentState rawAttachments;
            OpenTypeGposAttachmentState irAttachments;

            rawAttachments.reset(2);
            irAttachments.reset(2);

            const OpenTypeGposResolveResult rawResult =
                applyOpenTypeGposMarkLigatureLookupAt(
                    lookup, gdef,
                    rawBuffer, 1,
                    rawAttachments);

            const OpenTypeGposIRResult irResult =
                applyOpenTypeGposIRMarkLigatureLookupAt(
                    ir, *compiled,
                    irBuffer, 1,
                    irAttachments);

            if (rawResult !=
                    OpenTypeGposResolveResult::Match ||
                irResult !=
                    OpenTypeGposIRResult::Match ||
                !gposIRMarkLigatureBuffersEqual(
                    rawBuffer, irBuffer) ||
                !gposIRMarkLigatureAttachmentsEqual(
                    rawAttachments, irAttachments))
            {
                return fail(
                    "case 9 exact-position differential");
            }

            if (!gposIRMarkLigatureShapingEqual(
                    irBuffer[0].shaping,
                    ligatureShaping) ||
                !gposIRMarkLigatureShapingEqual(
                    irBuffer[1].shaping,
                    markShaping))
            {
                return fail(
                    "case 9 provenance changed");
            }

            if (irBuffer[0].placement.advanceX != 600 ||
                irBuffer[0].placement.advanceY != 2 ||
                irBuffer[0].placement.offsetX != 13 ||
                irBuffer[0].placement.offsetY != -7 ||
                irBuffer[1].placement.offsetX != 260 ||
                irBuffer[1].placement.offsetY != 440 ||
                irAttachments[1].type !=
                    OpenTypeGposAttachmentType::Mark ||
                irAttachments[1].parent != 0)
            {
                return fail(
                    "case 9 local attachment result");
            }

            ShapedGlyphBuffer rawFinal = rawBuffer;
            ShapedGlyphBuffer irFinal = irBuffer;

            if (!resolveOpenTypeGposAttachments(
                    rawFinal,
                    rawAttachments,
                    false) ||
                !resolveOpenTypeGposAttachments(
                    irFinal,
                    irAttachments,
                    false) ||
                !gposIRMarkLigatureBuffersEqual(
                    rawFinal, irFinal))
            {
                return fail(
                    "case 9 final attachment resolve");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR MarkLigature: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Basic attachment:         PASS\n"
            "  Provenance component:     PASS\n"
            "  Component clamp:          PASS\n"
            "  Intrinsic GDEF traversal: PASS\n"
            "  NULL anchor fallthrough:  PASS\n"
            "  Subtable order:           PASS\n"
            "  First non-mark boundary:  PASS\n"
            "  Type 9 -> Type 5:         PASS\n"
            "  Exact-position apply:     PASS\n"
            "  Final attachment resolve: PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
