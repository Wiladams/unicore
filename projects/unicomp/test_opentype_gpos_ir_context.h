// test_opentype_gpos_ir_context.h
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
//#include "opentype_gpos_lookup_apply.h"
#include "opentype_layout_view.h"

namespace waavs
{
    static void appendGposIRContextU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRContextU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRContextS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRContextU16(data, static_cast<uint16_t>(value));
    }


    static void patchGposIRContextU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRContextU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRContextCoverage1(std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRContextU16(data, 1);
        appendGposIRContextU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRContextU16(data, glyph);
    }


    struct GposIRContextActionSpec
    {
        uint16_t sequenceIndex{ 0 };
        uint16_t lookupIndex{ 0 };
    };


    static std::vector<uint8_t> makeGposIRContextSingleLookup(
        uint16_t glyph, int16_t xPlacement, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> subtable;

        appendGposIRContextU16(subtable, 1);

        const size_t coveragePatch = subtable.size();
        appendGposIRContextU16(subtable, 0);

        appendGposIRContextU16(subtable, 0x0001);
        appendGposIRContextS16(subtable, xPlacement);

        const size_t coverageOffset = subtable.size();
        patchGposIRContextU16(subtable, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRContextCoverage1(subtable, { glyph });

        std::vector<uint8_t> lookup;
        appendGposIRContextU16(lookup, 1);
        appendGposIRContextU16(lookup, lookupFlag);
        appendGposIRContextU16(lookup, 1);
        appendGposIRContextU16(lookup, 8);
        lookup.insert(lookup.end(), subtable.begin(), subtable.end());
        return lookup;
    }


    static std::vector<uint8_t> makeGposIRContextPairLookup(
        uint16_t firstGlyph, uint16_t secondGlyph, int16_t firstXPlacement)
    {
        std::vector<uint8_t> subtable;

        appendGposIRContextU16(subtable, 1);

        const size_t coveragePatch = subtable.size();
        appendGposIRContextU16(subtable, 0);

        appendGposIRContextU16(subtable, 0x0001);
        appendGposIRContextU16(subtable, 0x0000);
        appendGposIRContextU16(subtable, 1);

        const size_t pairSetPatch = subtable.size();
        appendGposIRContextU16(subtable, 0);

        const size_t pairSetOffset = subtable.size();
        patchGposIRContextU16(subtable, pairSetPatch, static_cast<uint16_t>(pairSetOffset));

        appendGposIRContextU16(subtable, 1);
        appendGposIRContextU16(subtable, secondGlyph);
        appendGposIRContextS16(subtable, firstXPlacement);

        const size_t coverageOffset = subtable.size();
        patchGposIRContextU16(subtable, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRContextCoverage1(subtable, { firstGlyph });

        std::vector<uint8_t> lookup;
        appendGposIRContextU16(lookup, 2);
        appendGposIRContextU16(lookup, 0);
        appendGposIRContextU16(lookup, 1);
        appendGposIRContextU16(lookup, 8);
        lookup.insert(lookup.end(), subtable.begin(), subtable.end());
        return lookup;
    }


    static std::vector<uint8_t> makeGposIRContextFormat1Subtable(
        const std::vector<uint16_t>& inputGlyphs,
        const std::vector<GposIRContextActionSpec>& actions)
    {
        if (inputGlyphs.empty())
            return {};

        std::vector<uint8_t> data;

        appendGposIRContextU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposIRContextU16(data, 0);

        appendGposIRContextU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGposIRContextU16(data, 0);

        const size_t ruleSetOffset = data.size();
        patchGposIRContextU16(data, ruleSetPatch, static_cast<uint16_t>(ruleSetOffset));

        appendGposIRContextU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposIRContextU16(data, 0);

        const size_t ruleOffset = data.size() - ruleSetOffset;
        patchGposIRContextU16(data, rulePatch, static_cast<uint16_t>(ruleOffset));

        appendGposIRContextU16(data, static_cast<uint16_t>(inputGlyphs.size()));
        appendGposIRContextU16(data, static_cast<uint16_t>(actions.size()));

        for (size_t i = 1; i < inputGlyphs.size(); ++i)
            appendGposIRContextU16(data, inputGlyphs[i]);

        for (const GposIRContextActionSpec& action : actions)
        {
            appendGposIRContextU16(data, action.sequenceIndex);
            appendGposIRContextU16(data, action.lookupIndex);
        }

        const size_t coverageOffset = data.size();
        patchGposIRContextU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRContextCoverage1(data, { inputGlyphs[0] });

        return data;
    }


    static void appendGposIRContextClassDef1(
        std::vector<uint8_t>& data, uint16_t startGlyph,
        const std::vector<uint16_t>& classes)
    {
        appendGposIRContextU16(data, 1);
        appendGposIRContextU16(data, startGlyph);
        appendGposIRContextU16(data, static_cast<uint16_t>(classes.size()));

        for (uint16_t value : classes)
            appendGposIRContextU16(data, value);
    }


    static std::vector<uint8_t> makeGposIRContextFormat2Subtable(
        uint16_t firstGlyph, uint16_t firstClass,
        uint16_t secondClass,
        const std::vector<GposIRContextActionSpec>& actions)
    {
        std::vector<uint8_t> data;

        appendGposIRContextU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposIRContextU16(data, 0);

        const size_t classDefPatch = data.size();
        appendGposIRContextU16(data, 0);

        const uint16_t classSetCount =
            static_cast<uint16_t>((firstClass > secondClass ? firstClass : secondClass) + 1);

        appendGposIRContextU16(data, classSetCount);

        const size_t classSetPatchBase = data.size();

        for (uint16_t i = 0; i < classSetCount; ++i)
            appendGposIRContextU16(data, 0);

        const size_t classSetOffset = data.size();

        patchGposIRContextU16(
            data,
            classSetPatchBase + size_t(firstClass) * 2,
            static_cast<uint16_t>(classSetOffset));

        appendGposIRContextU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposIRContextU16(data, 0);

        const size_t ruleOffset = data.size() - classSetOffset;
        patchGposIRContextU16(data, rulePatch, static_cast<uint16_t>(ruleOffset));

        appendGposIRContextU16(data, 2);
        appendGposIRContextU16(data, static_cast<uint16_t>(actions.size()));
        appendGposIRContextU16(data, secondClass);

        for (const GposIRContextActionSpec& action : actions)
        {
            appendGposIRContextU16(data, action.sequenceIndex);
            appendGposIRContextU16(data, action.lookupIndex);
        }

        const size_t coverageOffset = data.size();
        patchGposIRContextU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRContextCoverage1(data, { firstGlyph });

        const size_t classDefOffset = data.size();
        patchGposIRContextU16(data, classDefPatch, static_cast<uint16_t>(classDefOffset));

        const uint16_t startGlyph = firstGlyph < 20 ? firstGlyph : 20;
        const uint16_t lastGlyph = firstGlyph > 20 ? firstGlyph : 20;
        std::vector<uint16_t> classes(size_t(lastGlyph - startGlyph) + 1, 0);

        classes[firstGlyph - startGlyph] = firstClass;
        classes[20 - startGlyph] = secondClass;

        appendGposIRContextClassDef1(data, startGlyph, classes);
        return data;
    }


    static std::vector<uint8_t> makeGposIRContextFormat3Subtable(
        const std::vector<uint16_t>& inputGlyphs,
        const std::vector<GposIRContextActionSpec>& actions)
    {
        if (inputGlyphs.empty())
            return {};

        std::vector<uint8_t> data;

        appendGposIRContextU16(data, 3);
        appendGposIRContextU16(data, static_cast<uint16_t>(inputGlyphs.size()));
        appendGposIRContextU16(data, static_cast<uint16_t>(actions.size()));

        const size_t coveragePatchBase = data.size();

        for (size_t i = 0; i < inputGlyphs.size(); ++i)
            appendGposIRContextU16(data, 0);

        for (const GposIRContextActionSpec& action : actions)
        {
            appendGposIRContextU16(data, action.sequenceIndex);
            appendGposIRContextU16(data, action.lookupIndex);
        }

        for (size_t i = 0; i < inputGlyphs.size(); ++i)
        {
            const size_t coverageOffset = data.size();

            patchGposIRContextU16(
                data, coveragePatchBase + i * 2,
                static_cast<uint16_t>(coverageOffset));

            appendGposIRContextCoverage1(data, { inputGlyphs[i] });
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRContextLookup(
        const std::vector<std::vector<uint8_t>>& subtables,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> lookup;

        appendGposIRContextU16(lookup, 7);
        appendGposIRContextU16(lookup, lookupFlag);
        appendGposIRContextU16(lookup, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = lookup.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRContextU16(lookup, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = lookup.size();
            patchGposIRContextU16(lookup, patchBase + i * 2, static_cast<uint16_t>(offset));
            lookup.insert(lookup.end(), subtables[i].begin(), subtables[i].end());
        }

        return lookup;
    }


    static std::vector<uint8_t> makeGposIRContextExtensionLookup(
        const std::vector<uint8_t>& contextSubtable, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> lookup;

        appendGposIRContextU16(lookup, 9);
        appendGposIRContextU16(lookup, lookupFlag);
        appendGposIRContextU16(lookup, 1);
        appendGposIRContextU16(lookup, 8);

        const size_t extensionBase = lookup.size();

        appendGposIRContextU16(lookup, 1);
        appendGposIRContextU16(lookup, 7);

        const size_t offsetPatch = lookup.size();
        appendGposIRContextU32(lookup, 0);

        const size_t subtableOffset = lookup.size();

        patchGposIRContextU32(
            lookup, offsetPatch,
            static_cast<uint32_t>(subtableOffset - extensionBase));

        lookup.insert(lookup.end(), contextSubtable.begin(), contextSubtable.end());
        return lookup;
    }


    static std::vector<uint8_t> makeGposIRContextLookupList(
        const std::vector<std::vector<uint8_t>>& lookups)
    {
        std::vector<uint8_t> data;

        appendGposIRContextU16(data, static_cast<uint16_t>(lookups.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < lookups.size(); ++i)
            appendGposIRContextU16(data, 0);

        for (size_t i = 0; i < lookups.size(); ++i)
        {
            const size_t offset = data.size();
            patchGposIRContextU16(data, patchBase + i * 2, static_cast<uint16_t>(offset));
            data.insert(data.end(), lookups[i].begin(), lookups[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRContextGdefBase(uint16_t baseGlyph)
    {
        std::vector<uint8_t> data;

        appendGposIRContextU16(data, 1);
        appendGposIRContextU16(data, 0);
        appendGposIRContextU16(data, 12);
        appendGposIRContextU16(data, 0);
        appendGposIRContextU16(data, 0);
        appendGposIRContextU16(data, 0);

        appendGposIRContextU16(data, 2);
        appendGposIRContextU16(data, 1);
        appendGposIRContextU16(data, baseGlyph);
        appendGposIRContextU16(data, baseGlyph);
        appendGposIRContextU16(data, 1);

        return data;
    }


    static void appendGposIRContextGlyph(
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


    static ShapedGlyphBuffer makeGposIRContextBuffer(const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
            appendGposIRContextGlyph(buffer, glyphs[i], static_cast<uint32_t>(i), 500 + static_cast<int32_t>(i * 11));

        return buffer;
    }


    static bool gposIRContextShapingEqual(
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


    static bool gposIRContextBuffersEqual(
        const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRContextShapingEqual(a[i].shaping, b[i].shaping) ||
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


    static bool gposIRContextAttachmentsEqual(
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


    static bool compileGposIRContext(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposContextLookup(
            lookups, lookupIndex, gdef, ir, lookupId))
        {
            return false;
        }

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposContext &&
            openTypeGposIRContextLookupValid(ir, *compiled);
    }


    static bool runGposIRContextDifferential(
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
            applyOpenTypeGposContextLookup(
                lookups, rawLookupIndex, gdef,
                rawBuffer, rawAttachments,
                rawState, runRightToLeft);

        const bool irSuccess =
            applyOpenTypeGposIRContextLookup(
                ir, compiled,
                irBuffer, runRightToLeft,
                irAttachments, irState);

        if (rawSuccess != irSuccess)
        {
            std::printf(
                "GPOS IR Context: FAIL\n"
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

        if (!gposIRContextBuffersEqual(rawBuffer, irBuffer) ||
            !gposIRContextAttachmentsEqual(rawAttachments, irAttachments))
        {
            std::printf(
                "GPOS IR Context: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR shared-state mismatch\n",
                caseName);

            return false;
        }

        ShapedGlyphBuffer rawFinal = rawBuffer;
        ShapedGlyphBuffer irFinal = irBuffer;

        if (!resolveOpenTypeGposAttachments(rawFinal, rawAttachments, runRightToLeft) ||
            !resolveOpenTypeGposAttachments(irFinal, irAttachments, runRightToLeft) ||
            !gposIRContextBuffersEqual(rawFinal, irFinal))
        {
            std::printf(
                "GPOS IR Context: FAIL\n"
                "  Case: %s\n"
                "  Finalized raw/IR mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRContext()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR Context: FAIL\n"
                    "  %s\n",
                    message);
                return false;
            };


        // ====================================================================
        // Case 1 - Format 1, nested SinglePos.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 75);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 1 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 1 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Format 1 + Single"))
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
                makeGposIRContextLookup({
                    makeGposIRContextFormat2Subtable(
                        10, 1, 2, { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 90);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 2 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 2 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
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
                makeGposIRContextLookup({
                    makeGposIRContextFormat3Subtable(
                        { 10, 20 }, { { 0, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(10, 55);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 3 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 3 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Format 3 coverage"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - parent LookupFlag filtering during context matching.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 1, 1 } })
                    }, 0x0002);

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 80);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const std::vector<uint8_t> gdefBytes =
                makeGposIRContextGdefBase(11);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefBytes.data(), gdefBytes.size()));

            if (!lookups || !gdef)
                return fail("case 4 source views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 4 compilation");

            const uint32_t glyphs[] = { 10, 11, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 3);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "filtered context match"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - nested PairPos is confined to matched physical input span.
        //
        // Context covers [10,20]. Pair lookup wants 20 followed by 30. Glyph
        // 30 is outside the contextual range, so nested PairPos must NoMatch.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRContextPairLookup(20, 30, 140);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 5 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 5 compilation");

            const uint32_t glyphs[] = { 10, 20, 30 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 3);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "physical range confinement"))
            {
                return false;
            }

            ShapedGlyphBuffer check = input;
            OpenTypeGposAttachmentState attachments;
            attachments.reset(check.size());
            OpenTypeGposApplyState state;

            if (!applyOpenTypeGposIRContextLookup(
                ir, *compiled, check, false, attachments, state) ||
                check[1].placement.offsetX != input[1].placement.offsetX ||
                check[2].placement.offsetX != input[2].placement.offsetX)
            {
                return fail("case 5 nested Pair escaped context range");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - nested NoMatch is legal; later action still executes.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 0, 1 }, { 1, 2 } })
                    });

            const std::vector<uint8_t> noMatch =
                makeGposIRContextSingleLookup(99, 1000);

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 65);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, noMatch, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 6 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 6 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "nested NoMatch"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - multiple nested actions execute in design order.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 1, 1 }, { 1, 2 } })
                    });

            const std::vector<uint8_t> first =
                makeGposIRContextSingleLookup(20, 30);

            const std::vector<uint8_t> second =
                makeGposIRContextSingleLookup(20, 45);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, first, second });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 7 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 7 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "multiple actions"))
            {
                return false;
            }

            ShapedGlyphBuffer check = input;

            if (!applyOpenTypeGposIRContextLookup(ir, *compiled, check, false) ||
                check[1].placement.offsetX != 75)
            {
                return fail("case 7 actions did not accumulate");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - ExtensionPos Type 9 -> Type 7.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> contextSubtable =
                makeGposIRContextFormat3Subtable(
                    { 10, 20 }, { { 1, 1 } });

            const std::vector<uint8_t> root =
                makeGposIRContextExtensionLookup(contextSubtable);

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 95);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 8 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 8 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "Type 9 -> Type 7"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - oversized sequenceIndex is ignored.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 7, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 500);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 9 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 9 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            if (!runGposIRContextDifferential(
                lookups, 0, gdef, ir, *compiled,
                input, false, "oversized sequenceIndex"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 10 - self-recursive contextual lookup is bounded.
        //
        // Compilation must preserve the self-reference with a stable IR ID.
        // Runtime recursion protection must reject infinite recursion
        // transactionally on both raw and IR paths.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10 }, { { 0, 0 } })
                    });

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 10 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 10 recursive compilation");

            if (compiled->payloadCount != 1 ||
                ir.gposContextRules.empty() ||
                ir.gposContextLookups.empty() ||
                ir.gposContextLookups[0].lookup != lookupId)
            {
                return fail("case 10 stable self-reference");
            }

            const uint32_t glyphs[] = { 10 };
            const ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 1);

            ShapedGlyphBuffer rawBuffer = input;
            ShapedGlyphBuffer irBuffer = input;

            OpenTypeGposAttachmentState rawAttachments;
            OpenTypeGposAttachmentState irAttachments;
            rawAttachments.reset(1);
            irAttachments.reset(1);

            OpenTypeGposApplyState rawState;
            OpenTypeGposApplyState irState;

            const bool rawSuccess =
                applyOpenTypeGposContextLookup(
                    lookups, 0, gdef, rawBuffer,
                    rawAttachments, rawState, false);

            const bool irSuccess =
                applyOpenTypeGposIRContextLookup(
                    ir, *compiled, irBuffer,
                    false, irAttachments, irState);

            if (rawSuccess || irSuccess ||
                !gposIRContextBuffersEqual(rawBuffer, input) ||
                !gposIRContextBuffersEqual(irBuffer, input))
            {
                return fail("case 10 recursion protection");
            }

            ++passed;
        }


        // ====================================================================
        // Case 11 - shaping identity/provenance remains unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> root =
                makeGposIRContextLookup({
                    makeGposIRContextFormat1Subtable(
                        { 10, 20 }, { { 1, 1 } })
                    });

            const std::vector<uint8_t> nested =
                makeGposIRContextSingleLookup(20, 70);

            const std::vector<uint8_t> listBytes =
                makeGposIRContextLookupList({ root, nested });

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(listBytes.data(), listBytes.size()));

            if (!lookups)
                return fail("case 11 lookup list");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId;
            const OpenTypeShapingIRLookup* compiled;

            if (!compileGposIRContext(lookups, 0, gdef, ir, lookupId, compiled))
                return fail("case 11 compilation");

            const uint32_t glyphs[] = { 10, 20 };
            ShapedGlyphBuffer input = makeGposIRContextBuffer(glyphs, 2);

            input[0].shaping.scalarOffset = 7;
            input[0].shaping.scalarCount = 2;
            input[0].shaping.ligature.id = 42;
            input[0].shaping.ligature.component = 0;
            input[0].shaping.ligature.componentCount = 2;

            input[1].shaping.scalarOffset = 9;
            input[1].shaping.scalarCount = 3;
            input[1].shaping.ligature.id = 42;
            input[1].shaping.ligature.component = 2;
            input[1].shaping.ligature.componentCount = 2;

            const OpenTypeShapingGlyph before0 = input[0].shaping;
            const OpenTypeShapingGlyph before1 = input[1].shaping;

            ShapedGlyphBuffer check = input;

            if (!applyOpenTypeGposIRContextLookup(ir, *compiled, check, false) ||
                !gposIRContextShapingEqual(check[0].shaping, before0) ||
                !gposIRContextShapingEqual(check[1].shaping, before1))
            {
                return fail("case 11 provenance changed");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR Context: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1:                 PASS\n"
            "  Format 2:                 PASS\n"
            "  Format 3:                 PASS\n"
            "  Filtered context match:   PASS\n"
            "  Physical range:           PASS\n"
            "  Nested NoMatch:           PASS\n"
            "  Action ordering:          PASS\n"
            "  Type 9 -> Type 7:         PASS\n"
            "  Oversized sequenceIndex:  PASS\n"
            "  Recursion protection:     PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
