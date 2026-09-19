// test_opentype_gpos_ir_chain_context.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gpos_apply_state.h"
#include "opentype_gpos_attachment_state.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_gpos_ir_executor.h"
#include "opentype_gpos_lookup_apply.h"
#include "opentype_layout_view.h"

namespace waavs
{
    static void appendGposIRChainU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRChainU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRChainS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRChainU16(data, static_cast<uint16_t>(value));
    }


    static void patchGposIRChainU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRChainU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRChainCoverage1(std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRChainU16(data, 1);
        appendGposIRChainU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRChainU16(data, glyph);
    }


    struct GposIRChainActionSpec
    {
        uint16_t sequenceIndex{ 0 };
        uint16_t lookupIndex{ 0 };
    };


    static std::vector<uint8_t> makeGposIRChainSingleLookup(
        uint16_t glyph, int16_t xPlacement, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> subtable;

        appendGposIRChainU16(subtable, 1);

        const size_t coveragePatch = subtable.size();
        appendGposIRChainU16(subtable, 0);

        appendGposIRChainU16(subtable, 0x0001);
        appendGposIRChainS16(subtable, xPlacement);

        const size_t coverageOffset = subtable.size();
        patchGposIRChainU16(subtable, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRChainCoverage1(subtable, { glyph });

        std::vector<uint8_t> lookup;
        appendGposIRChainU16(lookup, 1);
        appendGposIRChainU16(lookup, lookupFlag);
        appendGposIRChainU16(lookup, 1);
        appendGposIRChainU16(lookup, 8);
        lookup.insert(lookup.end(), subtable.begin(), subtable.end());
        return lookup;
    }


    static std::vector<uint8_t> makeGposIRChainPairLookup(
        uint16_t firstGlyph, uint16_t secondGlyph, int16_t firstXPlacement)
    {
        std::vector<uint8_t> subtable;

        appendGposIRChainU16(subtable, 1);

        const size_t coveragePatch = subtable.size();
        appendGposIRChainU16(subtable, 0);

        appendGposIRChainU16(subtable, 0x0001);
        appendGposIRChainU16(subtable, 0x0000);
        appendGposIRChainU16(subtable, 1);

        const size_t pairSetPatch = subtable.size();
        appendGposIRChainU16(subtable, 0);

        const size_t pairSetOffset = subtable.size();
        patchGposIRChainU16(subtable, pairSetPatch, static_cast<uint16_t>(pairSetOffset));

        appendGposIRChainU16(subtable, 1);
        appendGposIRChainU16(subtable, secondGlyph);
        appendGposIRChainS16(subtable, firstXPlacement);

        const size_t coverageOffset = subtable.size();
        patchGposIRChainU16(subtable, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRChainCoverage1(subtable, { firstGlyph });

        std::vector<uint8_t> lookup;
        appendGposIRChainU16(lookup, 2);
        appendGposIRChainU16(lookup, 0);
        appendGposIRChainU16(lookup, 1);
        appendGposIRChainU16(lookup, 8);
        lookup.insert(lookup.end(), subtable.begin(), subtable.end());
        return lookup;
    }


    static std::vector<uint8_t> makeGposIRChainContextFormat1Subtable(
        const std::vector<uint16_t>& backtrackGlyphs,
        const std::vector<uint16_t>& inputGlyphs,
        const std::vector<uint16_t>& lookaheadGlyphs,
        const std::vector<GposIRChainActionSpec>& actions)
    {
        if (inputGlyphs.empty())
            return {};

        std::vector<uint8_t> data;

        appendGposIRChainU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposIRChainU16(data, 0);

        appendGposIRChainU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGposIRChainU16(data, 0);

        const size_t ruleSetOffset = data.size();
        patchGposIRChainU16(data, ruleSetPatch, static_cast<uint16_t>(ruleSetOffset));

        appendGposIRChainU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposIRChainU16(data, 0);

        const size_t ruleOffset = data.size() - ruleSetOffset;
        patchGposIRChainU16(data, rulePatch, static_cast<uint16_t>(ruleOffset));

        appendGposIRChainU16(data, static_cast<uint16_t>(backtrackGlyphs.size()));

        for (uint16_t glyph : backtrackGlyphs)
            appendGposIRChainU16(data, glyph);

        appendGposIRChainU16(data, static_cast<uint16_t>(inputGlyphs.size()));

        for (size_t i = 1; i < inputGlyphs.size(); ++i)
            appendGposIRChainU16(data, inputGlyphs[i]);

        appendGposIRChainU16(data, static_cast<uint16_t>(lookaheadGlyphs.size()));

        for (uint16_t glyph : lookaheadGlyphs)
            appendGposIRChainU16(data, glyph);

        appendGposIRChainU16(data, static_cast<uint16_t>(actions.size()));

        for (const GposIRChainActionSpec& action : actions)
        {
            appendGposIRChainU16(data, action.sequenceIndex);
            appendGposIRChainU16(data, action.lookupIndex);
        }

        const size_t coverageOffset = data.size();
        patchGposIRChainU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRChainCoverage1(data, { inputGlyphs[0] });

        return data;
    }


    static void appendGposIRChainClassDef1(
        std::vector<uint8_t>& data, uint16_t startGlyph,
        const std::vector<uint16_t>& classes)
    {
        appendGposIRChainU16(data, 1);
        appendGposIRChainU16(data, startGlyph);
        appendGposIRChainU16(data, static_cast<uint16_t>(classes.size()));

        for (uint16_t value : classes)
            appendGposIRChainU16(data, value);
    }


    static std::vector<uint8_t> makeGposIRChainContextFormat2Subtable(
        uint16_t backtrackGlyph, uint16_t backtrackClass,
        uint16_t firstInputGlyph, uint16_t firstInputClass,
        uint16_t secondInputGlyph, uint16_t secondInputClass,
        uint16_t lookaheadGlyph, uint16_t lookaheadClass,
        const std::vector<GposIRChainActionSpec>& actions)
    {
        std::vector<uint8_t> data;

        appendGposIRChainU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposIRChainU16(data, 0);

        const size_t backtrackClassDefPatch = data.size();
        appendGposIRChainU16(data, 0);

        const size_t inputClassDefPatch = data.size();
        appendGposIRChainU16(data, 0);

        const size_t lookaheadClassDefPatch = data.size();
        appendGposIRChainU16(data, 0);

        const uint16_t classSetCount =
            static_cast<uint16_t>(firstInputClass + 1);

        appendGposIRChainU16(data, classSetCount);

        const size_t classSetPatchBase = data.size();

        for (uint16_t i = 0; i < classSetCount; ++i)
            appendGposIRChainU16(data, 0);

        const size_t classSetOffset = data.size();

        patchGposIRChainU16(
            data,
            classSetPatchBase + size_t(firstInputClass) * 2,
            static_cast<uint16_t>(classSetOffset));

        appendGposIRChainU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposIRChainU16(data, 0);

        const size_t ruleOffset = data.size() - classSetOffset;
        patchGposIRChainU16(data, rulePatch, static_cast<uint16_t>(ruleOffset));

        appendGposIRChainU16(data, 1);
        appendGposIRChainU16(data, backtrackClass);

        appendGposIRChainU16(data, 2);
        appendGposIRChainU16(data, secondInputClass);

        appendGposIRChainU16(data, 1);
        appendGposIRChainU16(data, lookaheadClass);

        appendGposIRChainU16(data, static_cast<uint16_t>(actions.size()));

        for (const GposIRChainActionSpec& action : actions)
        {
            appendGposIRChainU16(data, action.sequenceIndex);
            appendGposIRChainU16(data, action.lookupIndex);
        }

        const size_t coverageOffset = data.size();
        patchGposIRChainU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRChainCoverage1(data, { firstInputGlyph });

        {
            const uint16_t first =
                backtrackGlyph;

            const size_t classDefOffset = data.size();
            patchGposIRChainU16(
                data, backtrackClassDefPatch,
                static_cast<uint16_t>(classDefOffset));

            appendGposIRChainClassDef1(
                data, first, { backtrackClass });
        }

        {
            const uint16_t first =
                firstInputGlyph < secondInputGlyph ? firstInputGlyph : secondInputGlyph;

            const uint16_t last =
                firstInputGlyph > secondInputGlyph ? firstInputGlyph : secondInputGlyph;

            std::vector<uint16_t> classes(size_t(last - first) + 1, 0);
            classes[firstInputGlyph - first] = firstInputClass;
            classes[secondInputGlyph - first] = secondInputClass;

            const size_t classDefOffset = data.size();
            patchGposIRChainU16(
                data, inputClassDefPatch,
                static_cast<uint16_t>(classDefOffset));

            appendGposIRChainClassDef1(data, first, classes);
        }

        {
            const size_t classDefOffset = data.size();
            patchGposIRChainU16(
                data, lookaheadClassDefPatch,
                static_cast<uint16_t>(classDefOffset));

            appendGposIRChainClassDef1(
                data, lookaheadGlyph, { lookaheadClass });
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRChainContextFormat3Subtable(
        const std::vector<uint16_t>& backtrackGlyphs,
        const std::vector<uint16_t>& inputGlyphs,
        const std::vector<uint16_t>& lookaheadGlyphs,
        const std::vector<GposIRChainActionSpec>& actions)
    {
        if (inputGlyphs.empty())
            return {};

        std::vector<uint8_t> data;

        appendGposIRChainU16(data, 3);

        appendGposIRChainU16(data, static_cast<uint16_t>(backtrackGlyphs.size()));

        const size_t backtrackPatchBase = data.size();

        for (size_t i = 0; i < backtrackGlyphs.size(); ++i)
            appendGposIRChainU16(data, 0);

        appendGposIRChainU16(data, static_cast<uint16_t>(inputGlyphs.size()));

        const size_t inputPatchBase = data.size();

        for (size_t i = 0; i < inputGlyphs.size(); ++i)
            appendGposIRChainU16(data, 0);

        appendGposIRChainU16(data, static_cast<uint16_t>(lookaheadGlyphs.size()));

        const size_t lookaheadPatchBase = data.size();

        for (size_t i = 0; i < lookaheadGlyphs.size(); ++i)
            appendGposIRChainU16(data, 0);

        appendGposIRChainU16(data, static_cast<uint16_t>(actions.size()));

        for (const GposIRChainActionSpec& action : actions)
        {
            appendGposIRChainU16(data, action.sequenceIndex);
            appendGposIRChainU16(data, action.lookupIndex);
        }

        for (size_t i = 0; i < backtrackGlyphs.size(); ++i)
        {
            const size_t coverageOffset = data.size();

            patchGposIRChainU16(
                data, backtrackPatchBase + i * 2,
                static_cast<uint16_t>(coverageOffset));

            appendGposIRChainCoverage1(data, { backtrackGlyphs[i] });
        }

        for (size_t i = 0; i < inputGlyphs.size(); ++i)
        {
            const size_t coverageOffset = data.size();

            patchGposIRChainU16(
                data, inputPatchBase + i * 2,
                static_cast<uint16_t>(coverageOffset));

            appendGposIRChainCoverage1(data, { inputGlyphs[i] });
        }

        for (size_t i = 0; i < lookaheadGlyphs.size(); ++i)
        {
            const size_t coverageOffset = data.size();

            patchGposIRChainU16(
                data, lookaheadPatchBase + i * 2,
                static_cast<uint16_t>(coverageOffset));

            appendGposIRChainCoverage1(data, { lookaheadGlyphs[i] });
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRChainContextLookup(
        const std::vector<std::vector<uint8_t>>& subtables,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> lookup;

        appendGposIRChainU16(lookup, 8);
        appendGposIRChainU16(lookup, lookupFlag);
        appendGposIRChainU16(lookup, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = lookup.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRChainU16(lookup, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = lookup.size();
            patchGposIRChainU16(
                lookup, patchBase + i * 2,
                static_cast<uint16_t>(offset));

            lookup.insert(lookup.end(), subtables[i].begin(), subtables[i].end());
        }

        return lookup;
    }


    static std::vector<uint8_t> makeGposIRChainContextExtensionLookup(
        const std::vector<uint8_t>& chainSubtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> lookup;

        appendGposIRChainU16(lookup, 9);
        appendGposIRChainU16(lookup, lookupFlag);
        appendGposIRChainU16(lookup, 1);
        appendGposIRChainU16(lookup, 8);

        const size_t extensionBase = lookup.size();

        appendGposIRChainU16(lookup, 1);
        appendGposIRChainU16(lookup, 8);

        const size_t offsetPatch = lookup.size();
        appendGposIRChainU32(lookup, 0);

        const size_t subtableOffset = lookup.size();

        patchGposIRChainU32(
            lookup, offsetPatch,
            static_cast<uint32_t>(subtableOffset - extensionBase));

        lookup.insert(lookup.end(), chainSubtable.begin(), chainSubtable.end());
        return lookup;
    }


    static std::vector<uint8_t> makeGposIRChainContextLookupList(
        const std::vector<std::vector<uint8_t>>& lookups)
    {
        std::vector<uint8_t> data;

        appendGposIRChainU16(data, static_cast<uint16_t>(lookups.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < lookups.size(); ++i)
            appendGposIRChainU16(data, 0);

        for (size_t i = 0; i < lookups.size(); ++i)
        {
            const size_t offset = data.size();
            patchGposIRChainU16(
                data, patchBase + i * 2,
                static_cast<uint16_t>(offset));

            data.insert(data.end(), lookups[i].begin(), lookups[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRChainGdefBases(
        const std::vector<uint16_t>& glyphs)
    {
        if (glyphs.empty())
            return {};

        std::vector<uint8_t> data;

        appendGposIRChainU16(data, 1);
        appendGposIRChainU16(data, 0);
        appendGposIRChainU16(data, 12);
        appendGposIRChainU16(data, 0);
        appendGposIRChainU16(data, 0);
        appendGposIRChainU16(data, 0);

        appendGposIRChainU16(data, 2);
        appendGposIRChainU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
        {
            appendGposIRChainU16(data, glyph);
            appendGposIRChainU16(data, glyph);
            appendGposIRChainU16(data, 1);
        }

        return data;
    }


    static void appendGposIRChainGlyph(
        ShapedGlyphBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset,
        int32_t advanceX = 500, int32_t offsetX = 0)
    {
        ShapedGlyph glyph{};
        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = scalarOffset;
        glyph.shaping.scalarCount = 1;
        glyph.placement.advanceX = advanceX;
        glyph.placement.offsetX = offsetX;
        buffer.pushBack(glyph);
    }


    static ShapedGlyphBuffer makeGposIRChainBuffer(const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRChainGlyph(
                buffer, glyphs[i], static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i * 13));
        }

        return buffer;
    }


    static bool gposIRChainShapingEqual(
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


    static bool gposIRChainBuffersEqual(
        const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRChainShapingEqual(a[i].shaping, b[i].shaping) ||
                a[i].placement.advanceX != b[i].placement.advanceX ||
                a[i].placement.advanceY != b[i].placement.advanceY ||
                a[i].placement.offsetX != b[i].placement.offsetX ||
                a[i].placement.offsetY != b[i].placement.offsetY)
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRChainAttachmentsEqual(
        const OpenTypeGposAttachmentState& a,
        const OpenTypeGposAttachmentState& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (a[i].type != b[i].type || a[i].parent != b[i].parent)
                return false;
        }

        return true;
    }


    static bool compileGposIRChainContext(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposChainContextLookup(
            lookups, lookupIndex, gdef, ir, lookupId))
        {
            return false;
        }

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposChainContext &&
            openTypeGposIRChainContextLookupValid(ir, *compiled);
    }


    static bool runGposIRChainDifferential(
        const OpenTypeLayoutLookupListView& lookups, uint16_t rawLookupIndex,
        const OpenTypeGdefView& gdef, const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiled,
        const ShapedGlyphBuffer& input, bool runRightToLeft,
        const char* caseName)
    {
        ShapedGlyphBuffer rawBuffer = input;
        ShapedGlyphBuffer irBuffer = input;

        OpenTypeGposAttachmentState rawAttachments;
        OpenTypeGposAttachmentState irAttachments;

        rawAttachments.reset(rawBuffer.size());
        irAttachments.reset(irBuffer.size());

        OpenTypeGposApplyState rawState;
        OpenTypeGposApplyState irState;

        const bool rawSuccess =
            applyOpenTypeGposChainContextLookup(
                lookups, rawLookupIndex, gdef,
                rawBuffer, rawAttachments,
                rawState, runRightToLeft);

        const bool irSuccess =
            applyOpenTypeGposIRChainContextLookup(
                ir, compiled,
                irBuffer, runRightToLeft,
                irAttachments, irState);

        if (rawSuccess != irSuccess)
        {
            std::printf(
                "GPOS IR ChainContext: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!rawSuccess)
            return true;

        if (!gposIRChainBuffersEqual(rawBuffer, irBuffer) ||
            !gposIRChainAttachmentsEqual(rawAttachments, irAttachments))
        {
            std::printf(
                "GPOS IR ChainContext: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR shared-state mismatch\n",
                caseName);

            return false;
        }

        ShapedGlyphBuffer rawFinal = rawBuffer;
        ShapedGlyphBuffer irFinal = irBuffer;

        if (!resolveOpenTypeGposAttachments(rawFinal, rawAttachments, runRightToLeft) ||
            !resolveOpenTypeGposAttachments(irFinal, irAttachments, runRightToLeft) ||
            !gposIRChainBuffersEqual(rawFinal, irFinal))
        {
            std::printf(
                "GPOS IR ChainContext: FAIL\n"
                "  Case: %s\n"
                "  Finalized raw/IR mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRChainContext()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR ChainContext: FAIL\n"
                    "  %s\n",
                    message);
                return false;
            };


        // ====================================================================
        // Case 1 - Format 1, full backtrack/input/lookahead match.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 75);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 1 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 1 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Format 1"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 2 class normalization.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat2Subtable(
                        5, 1,
                        10, 2,
                        20, 3,
                        30, 4,
                        { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 90);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 2 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 2 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Format 2 classes"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Format 3 coverage normalization.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat3Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 0, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(10, 55);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 3 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 3 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Format 3 coverage"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - filtered backtrack/input/lookahead traversal.
        //
        // Glyphs 6, 11, and 21 are GDEF bases and the root ignores bases.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 1, 1 } })
                    }, 0x0002);

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 80);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const std::vector<uint8_t> gdefBytes =
                makeGposIRChainGdefBases({ 6, 11, 21 });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefBytes.data(), gdefBytes.size()));

            if (!lookups || !gdef)
                return fail("case 4 source views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 4 compilation");

            const uint32_t glyphs[] = { 5, 6, 10, 11, 20, 21, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 7);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "filtered chain traversal"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - backtrack/lookahead are not part of nested workspace.
        //
        // PairPos wants 20 followed by lookahead glyph 30. Since Type 8 nested
        // positioning is confined to input [10,20], the PairPos must NoMatch.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRChainPairLookup(20, 30, 140);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 5 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 5 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "input range confinement"))
            {
                return false;
            }

            ShapedGlyphBuffer check = input;
            OpenTypeGposAttachmentState attachments;
            attachments.reset(check.size());
            OpenTypeGposApplyState state;

            if (!applyOpenTypeGposIRChainContextLookup(
                ir, *compiled, check, false, attachments, state) ||
                check[2].placement.offsetX != input[2].placement.offsetX ||
                check[3].placement.offsetX != input[3].placement.offsetX)
            {
                return fail("case 5 nested lookup escaped input range");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - nested NoMatch is legal; later action still executes.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 0, 1 }, { 1, 2 } })
                    });

            const std::vector<uint8_t> noMatch =
                makeGposIRChainSingleLookup(99, 1000);

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 65);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, noMatch, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 6 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 6 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "nested NoMatch"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - action ordering.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 1, 1 }, { 1, 2 } })
                    });

            const std::vector<uint8_t> first =
                makeGposIRChainSingleLookup(20, 30);

            const std::vector<uint8_t> second =
                makeGposIRChainSingleLookup(20, 45);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, first, second });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 7 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 7 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "action ordering"))
            {
                return false;
            }

            ShapedGlyphBuffer check = input;

            if (!applyOpenTypeGposIRChainContextLookup(ir, *compiled, check, false) ||
                check[2].placement.offsetX != 75)
            {
                return fail("case 7 actions did not accumulate");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - subtable order: first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> firstSubtable =
                makeGposIRChainContextFormat1Subtable(
                    { 5 }, { 10, 20 }, { 30 },
                    { { 1, 1 } });

            const std::vector<uint8_t> secondSubtable =
                makeGposIRChainContextFormat1Subtable(
                    { 5 }, { 10, 20 }, { 30 },
                    { { 1, 2 } });

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    firstSubtable,
                    secondSubtable
                    });

            const std::vector<uint8_t> first =
                makeGposIRChainSingleLookup(20, 40);

            const std::vector<uint8_t> second =
                makeGposIRChainSingleLookup(20, 900);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, first, second });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 8 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 8 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "subtable order"))
            {
                return false;
            }

            ShapedGlyphBuffer check = input;

            if (!applyOpenTypeGposIRChainContextLookup(ir, *compiled, check, false) ||
                check[2].placement.offsetX != 40)
            {
                return fail("case 8 first matching subtable did not win");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - ExtensionPos Type 9 -> Type 8.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> chainSubtable =
                makeGposIRChainContextFormat3Subtable(
                    { 5 }, { 10, 20 }, { 30 },
                    { { 1, 1 } });

            const std::vector<uint8_t> root =
                makeGposIRChainContextExtensionLookup(chainSubtable);

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 95);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 9 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 9 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Type 9 -> Type 8"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 10 - oversized sequenceIndex is ignored.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 7, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 500);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 10 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 10 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "oversized sequenceIndex"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 11 - self-recursive Type 8 is bounded.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        {}, { 10 }, {},
                        { { 0, 0 } })
                    });

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 11 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 11 recursive compilation");

            if (ir.gposChainContextLookups.empty() ||
                ir.gposChainContextLookups[0].lookup != lookupId)
            {
                return fail("case 11 stable self-reference");
            }

            const uint32_t glyphs[] = { 10 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 1);

            ShapedGlyphBuffer rawBuffer = input;
            ShapedGlyphBuffer irBuffer = input;

            OpenTypeGposAttachmentState rawAttachments;
            OpenTypeGposAttachmentState irAttachments;
            rawAttachments.reset(1);
            irAttachments.reset(1);

            OpenTypeGposApplyState rawState;
            OpenTypeGposApplyState irState;

            const bool rawSuccess =
                applyOpenTypeGposChainContextLookup(
                    lookups, 0, gdef, rawBuffer,
                    rawAttachments, rawState, false);

            const bool irSuccess =
                applyOpenTypeGposIRChainContextLookup(
                    ir, *compiled, irBuffer,
                    false, irAttachments, irState);

            if (rawSuccess || irSuccess ||
                !gposIRChainBuffersEqual(rawBuffer, input) ||
                !gposIRChainBuffersEqual(irBuffer, input))
            {
                return fail("case 11 recursion protection");
            }

            ++passed;
        }


        // ====================================================================
        // Case 12 - mixed Type 8 -> Type 7 dependency.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 0, 1 } })
                    });

            const std::vector<uint8_t> nestedContext =
                []()
                {
                    std::vector<uint8_t> data;

                    appendGposIRChainU16(data, 3);
                    appendGposIRChainU16(data, 1);
                    appendGposIRChainU16(data, 1);

                    const size_t coveragePatch = data.size();
                    appendGposIRChainU16(data, 0);

                    appendGposIRChainU16(data, 0);
                    appendGposIRChainU16(data, 2);

                    const size_t coverageOffset = data.size();
                    patchGposIRChainU16(
                        data, coveragePatch,
                        static_cast<uint16_t>(coverageOffset));

                    appendGposIRChainCoverage1(data, { 10 });

                    std::vector<uint8_t> lookup;
                    appendGposIRChainU16(lookup, 7);
                    appendGposIRChainU16(lookup, 0);
                    appendGposIRChainU16(lookup, 1);
                    appendGposIRChainU16(lookup, 8);
                    lookup.insert(lookup.end(), data.begin(), data.end());
                    return lookup;
                }();

            const std::vector<uint8_t> single =
                makeGposIRChainSingleLookup(10, 120);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({
                    root, nestedContext, single
                    });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 12 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 12 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            if (!runGposIRChainDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Type 8 -> Type 7"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 13 - shaping identity/provenance remains unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRChainContextLookup({
                    makeGposIRChainContextFormat1Subtable(
                        { 5 }, { 10, 20 }, { 30 },
                        { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRChainSingleLookup(20, 70);

            const std::vector<uint8_t> listBytes =
                makeGposIRChainContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 13 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRChainContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 13 compilation");

            const uint32_t glyphs[] = { 5, 10, 20, 30 };
            ShapedGlyphBuffer input = makeGposIRChainBuffer(glyphs, 4);

            input[1].shaping.scalarOffset = 7;
            input[1].shaping.scalarCount = 2;
            input[1].shaping.ligature.id = 42;
            input[1].shaping.ligature.component = 0;
            input[1].shaping.ligature.componentCount = 2;

            input[2].shaping.scalarOffset = 9;
            input[2].shaping.scalarCount = 3;
            input[2].shaping.ligature.id = 42;
            input[2].shaping.ligature.component = 2;
            input[2].shaping.ligature.componentCount = 2;

            const OpenTypeShapingGlyph before0 = input[0].shaping;
            const OpenTypeShapingGlyph before1 = input[1].shaping;
            const OpenTypeShapingGlyph before2 = input[2].shaping;
            const OpenTypeShapingGlyph before3 = input[3].shaping;

            ShapedGlyphBuffer check = input;

            if (!applyOpenTypeGposIRChainContextLookup(ir, *compiled, check, false) ||
                !gposIRChainShapingEqual(check[0].shaping, before0) ||
                !gposIRChainShapingEqual(check[1].shaping, before1) ||
                !gposIRChainShapingEqual(check[2].shaping, before2) ||
                !gposIRChainShapingEqual(check[3].shaping, before3))
            {
                return fail("case 13 provenance changed");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR ChainContext: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1:                 PASS\n"
            "  Format 2:                 PASS\n"
            "  Format 3:                 PASS\n"
            "  Filtered chain traversal: PASS\n"
            "  Input range:              PASS\n"
            "  Nested NoMatch:           PASS\n"
            "  Action ordering:          PASS\n"
            "  Subtable order:           PASS\n"
            "  Type 9 -> Type 8:         PASS\n"
            "  Oversized sequenceIndex:  PASS\n"
            "  Recursion protection:     PASS\n"
            "  Type 8 -> Type 7:         PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
