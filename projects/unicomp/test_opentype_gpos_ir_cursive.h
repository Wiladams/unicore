// test_opentype_gpos_ir_cursive.h
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
    static void appendGposIRCursiveU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRCursiveU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRCursiveS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRCursiveU16(data, static_cast<uint16_t>(value));
    }


    static void patchGposIRCursiveU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRCursiveU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRCursiveCoverage1(
        std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRCursiveU16(data, 1);
        appendGposIRCursiveU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRCursiveU16(data, glyph);
    }


    static void appendGposIRCursiveAnchor1(
        std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposIRCursiveU16(data, 1);
        appendGposIRCursiveS16(data, x);
        appendGposIRCursiveS16(data, y);
    }


    struct GposIRCursiveRecordSpec
    {
        uint16_t glyph{ 0 };
        bool hasEntry{ false };
        bool hasExit{ false };
        int16_t entryX{ 0 };
        int16_t entryY{ 0 };
        int16_t exitX{ 0 };
        int16_t exitY{ 0 };
    };


    static std::vector<uint8_t> makeGposIRCursiveSubtable(
        const std::vector<GposIRCursiveRecordSpec>& records)
    {
        std::vector<uint8_t> data;

        appendGposIRCursiveU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposIRCursiveU16(data, 0);

        appendGposIRCursiveU16(data, static_cast<uint16_t>(records.size()));

        const size_t recordBase = data.size();

        for (size_t i = 0; i < records.size(); ++i)
        {
            appendGposIRCursiveU16(data, 0);
            appendGposIRCursiveU16(data, 0);
        }

        for (size_t i = 0; i < records.size(); ++i)
        {
            const GposIRCursiveRecordSpec& record = records[i];

            if (record.hasEntry)
            {
                const size_t offset = data.size();

                patchGposIRCursiveU16(
                    data, recordBase + i * 4,
                    static_cast<uint16_t>(offset));

                appendGposIRCursiveAnchor1(
                    data, record.entryX, record.entryY);
            }

            if (record.hasExit)
            {
                const size_t offset = data.size();

                patchGposIRCursiveU16(
                    data, recordBase + i * 4 + 2,
                    static_cast<uint16_t>(offset));

                appendGposIRCursiveAnchor1(
                    data, record.exitX, record.exitY);
            }
        }

        const size_t coverageOffset = data.size();

        patchGposIRCursiveU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset));

        std::vector<uint16_t> glyphs;
        glyphs.reserve(records.size());

        for (const GposIRCursiveRecordSpec& record : records)
            glyphs.push_back(record.glyph);

        appendGposIRCursiveCoverage1(data, glyphs);
        return data;
    }


    static std::vector<uint8_t> makeGposIRCursiveLookup(
        const std::vector<std::vector<uint8_t>>& subtables,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRCursiveU16(data, 3);
        appendGposIRCursiveU16(data, lookupFlag);
        appendGposIRCursiveU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRCursiveU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();

            patchGposIRCursiveU16(
                data, patchBase + i * 2,
                static_cast<uint16_t>(offset));

            data.insert(
                data.end(),
                subtables[i].begin(),
                subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRCursiveExtensionLookup(
        const std::vector<uint8_t>& cursiveSubtable,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRCursiveU16(data, 9);
        appendGposIRCursiveU16(data, lookupFlag);
        appendGposIRCursiveU16(data, 1);
        appendGposIRCursiveU16(data, 8);

        const size_t extensionBase = data.size();

        appendGposIRCursiveU16(data, 1);
        appendGposIRCursiveU16(data, 3);

        const size_t extensionPatch = data.size();
        appendGposIRCursiveU32(data, 0);

        const size_t cursiveOffset = data.size();

        patchGposIRCursiveU32(
            data, extensionPatch,
            static_cast<uint32_t>(cursiveOffset - extensionBase));

        data.insert(
            data.end(),
            cursiveSubtable.begin(),
            cursiveSubtable.end());

        return data;
    }


    static std::vector<uint8_t> makeGposIRCursiveMarkGdef(uint16_t markGlyph)
    {
        std::vector<uint8_t> data;

        appendGposIRCursiveU16(data, 1);
        appendGposIRCursiveU16(data, 0);

        appendGposIRCursiveU16(data, 12);
        appendGposIRCursiveU16(data, 0);
        appendGposIRCursiveU16(data, 0);
        appendGposIRCursiveU16(data, 0);

        appendGposIRCursiveU16(data, 2);
        appendGposIRCursiveU16(data, 1);

        appendGposIRCursiveU16(data, markGlyph);
        appendGposIRCursiveU16(data, markGlyph);
        appendGposIRCursiveU16(data, 3);

        return data;
    }


    static void appendGposIRCursiveGlyph(
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


    static ShapedGlyphBuffer makeGposIRCursiveBuffer(
        const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRCursiveGlyph(
                buffer, glyphs[i], static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i * 7),
                static_cast<int32_t>(i),
                static_cast<int32_t>(i * 3),
                -static_cast<int32_t>(i * 2));
        }

        return buffer;
    }


    static bool gposIRCursiveShapingEqual(
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


    static bool gposIRCursivePlacementEqual(
        const GlyphPlacement& a,
        const GlyphPlacement& b) noexcept
    {
        return
            a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool gposIRCursiveBuffersEqual(
        const ShapedGlyphBuffer& a,
        const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRCursiveShapingEqual(a[i].shaping, b[i].shaping) ||
                !gposIRCursivePlacementEqual(a[i].placement, b[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRCursiveAttachmentsEqual(
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


    static bool compileGposIRCursive(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposCursiveLookup(
            lookup, gdef, ir, lookupId))
        {
            return false;
        }

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposCursive &&
            openTypeGposIRCursiveLookupValid(ir, *compiled);
    }


    static bool gposIRCursiveResultEqual(
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


    static bool testGposIRCursiveResolveAt(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiled,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        const char* caseName)
    {
        const OpenTypeLookupGlyphFilter rawFilter(
            rawLookup, gdef);

        if (!rawFilter)
            return false;

        OpenTypeGposCursiveMatch rawMatch;
        OpenTypeGposIRCursiveMatch irMatch;

        const OpenTypeGposResolveResult rawResult =
            resolveOpenTypeGposCursiveLookup(
                rawLookup, rawFilter,
                buffer, glyphIndex, rawMatch);

        const OpenTypeGposIRResult irResult =
            resolveOpenTypeGposIRCursiveLookup(
                ir, compiled,
                buffer, glyphIndex, irMatch);

        if (!gposIRCursiveResultEqual(rawResult, irResult))
        {
            std::printf(
                "GPOS IR Cursive: FAIL\n"
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

        if (rawMatch.firstIndex != irMatch.firstIndex ||
            rawMatch.secondIndex != irMatch.secondIndex ||
            rawMatch.exitX != irMatch.exit.x ||
            rawMatch.exitY != irMatch.exit.y ||
            rawMatch.entryX != irMatch.entry.x ||
            rawMatch.entryY != irMatch.entry.y)
        {
            std::printf(
                "GPOS IR Cursive: FAIL\n"
                "  Case: %s\n"
                "  Raw match: [%zu,%zu] exit=(%d,%d) entry=(%d,%d)\n"
                "  IR match:  [%zu,%zu] exit=(%d,%d) entry=(%d,%d)\n",
                caseName,
                rawMatch.firstIndex, rawMatch.secondIndex,
                rawMatch.exitX, rawMatch.exitY,
                rawMatch.entryX, rawMatch.entryY,
                irMatch.firstIndex, irMatch.secondIndex,
                irMatch.exit.x, irMatch.exit.y,
                irMatch.entry.x, irMatch.entry.y);

            return false;
        }

        return true;
    }


    static bool testGposIRCursiveSharedExecution(
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
            applyOpenTypeGposCursiveLookup(
                rawLookup, gdef,
                rawBuffer, runRightToLeft,
                rawAttachments);

        const bool irSuccess =
            applyOpenTypeGposIRLookup(
                ir, lookupId,
                irBuffer, runRightToLeft,
                irAttachments);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GPOS IR Cursive: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gposIRCursiveBuffersEqual(
                rawBuffer, irBuffer) ||
            !gposIRCursiveAttachmentsEqual(
                rawAttachments, irAttachments))
        {
            std::printf(
                "GPOS IR Cursive: FAIL\n"
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
                "GPOS IR Cursive: FAIL\n"
                "  Case: %s\n"
                "  Attachment finalization failed\n",
                caseName);

            return false;
        }

        if (!gposIRCursiveBuffersEqual(
                rawFinal, irFinal))
        {
            std::printf(
                "GPOS IR Cursive: FAIL\n"
                "  Case: %s\n"
                "  Finalized raw/IR mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRCursive()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR Cursive: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - basic LTR cursive attachment.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> records =
            {
                { 10, false, true, 0, 0, 450, 20 },
                { 20, true, false, 100, -10, 0, 0 }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRCursiveSubtable(records);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRCursiveBuffer(glyphs, 2);

            if (!testGposIRCursiveResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 0, "basic LTR resolver") ||
                !testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "basic LTR execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - run RTL changes the main-line advance equation.
        //
        // LookupFlag remains clear, so attachment parent/child direction is
        // unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> records =
            {
                { 10, false, true, 0, 0, 420, 15 },
                { 20, true, false, 80, -7, 0, 0 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup({
                    makeGposIRCursiveSubtable(records)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRCursiveBuffer(glyphs, 2);

            if (!testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, true, "RTL run direction"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - LookupFlag RightToLeft changes cross-stream attachment
        // direction but does not reverse traversal.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> records =
            {
                { 10, false, true, 0, 0, 430, 21 },
                { 20, true, false, 90, -9, 0, 0 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup(
                    { makeGposIRCursiveSubtable(records) },
                    kOpenTypeLookupFlagRightToLeft);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 3 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            if ((compiled->filter.flags &
                OpenTypeShapingIRRightToLeft) == 0)
            {
                return fail("case 3 RightToLeft normalization");
            }

            const uint32_t glyphs[] = { 10, 20 };

            ShapedGlyphBuffer rawBuffer =
                makeGposIRCursiveBuffer(glyphs, 2);

            ShapedGlyphBuffer irBuffer = rawBuffer;

            OpenTypeGposAttachmentState rawAttachments;
            OpenTypeGposAttachmentState irAttachments;

            rawAttachments.reset(2);
            irAttachments.reset(2);

            if (!applyOpenTypeGposCursiveLookup(
                    lookup, gdef, rawBuffer,
                    false, rawAttachments) ||
                !applyOpenTypeGposIRLookup(
                    ir, lookupId, irBuffer,
                    false, irAttachments))
            {
                return fail("case 3 execution");
            }

            if (!gposIRCursiveBuffersEqual(
                    rawBuffer, irBuffer) ||
                !gposIRCursiveAttachmentsEqual(
                    rawAttachments, irAttachments))
            {
                return fail("case 3 raw/IR mismatch");
            }

            if (irAttachments[0].type !=
                    OpenTypeGposAttachmentType::Cursive ||
                irAttachments[0].parent != 1 ||
                irAttachments[1].attached())
            {
                return fail("case 3 attachment direction");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - IgnoreMarks filters only the following participating glyph.
        // Current glyph remains exact.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> records =
            {
                { 10, false, true, 0, 0, 440, 18 },
                { 20, true, false, 95, -12, 0, 0 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup(
                    { makeGposIRCursiveSubtable(records) },
                    kOpenTypeLookupFlagIgnoreMarks);

            const std::vector<uint8_t> gdefBytes =
                makeGposIRCursiveMarkGdef(100);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefBytes.data(), gdefBytes.size()));

            if (!lookup || !gdef)
                return fail("case 4 raw views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            const uint32_t glyphs[] = { 10, 100, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRCursiveBuffer(glyphs, 3);

            if (!testGposIRCursiveResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 0, "filtered traversal resolver") ||
                !testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "filtered traversal execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - NULL anchors produce NoMatch and allow a later subtable.
        //
        // The first subtable covers both glyphs but the first glyph has no
        // exit anchor. The second subtable supplies a valid pair.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> firstRecords =
            {
                { 10, false, false, 0, 0, 0, 0 },
                { 20, true, false, 70, 5, 0, 0 }
            };

            const std::vector<GposIRCursiveRecordSpec> secondRecords =
            {
                { 10, false, true, 0, 0, 410, 13 },
                { 20, true, false, 75, -4, 0, 0 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup({
                    makeGposIRCursiveSubtable(firstRecords),
                    makeGposIRCursiveSubtable(secondRecords)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            if (compiled->payloadCount != 2)
                return fail("case 5 subtable count");

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRCursiveBuffer(glyphs, 2);

            if (!testGposIRCursiveResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 0, "NULL-anchor fallthrough") ||
                !testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "NULL-anchor fallthrough execution"))
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

            const std::vector<GposIRCursiveRecordSpec> firstRecords =
            {
                { 10, false, true, 0, 0, 400, 10 },
                { 20, true, false, 100, 0, 0, 0 }
            };

            const std::vector<GposIRCursiveRecordSpec> secondRecords =
            {
                { 10, false, true, 0, 0, 700, 40 },
                { 20, true, false, 20, -30, 0, 0 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup({
                    makeGposIRCursiveSubtable(firstRecords),
                    makeGposIRCursiveSubtable(secondRecords)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 6 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRCursiveBuffer(glyphs, 2);

            if (!testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "subtable order"))
            {
                return false;
            }

            ShapedGlyphBuffer irBuffer = input;
            OpenTypeGposAttachmentState attachments;
            attachments.reset(2);

            if (!applyOpenTypeGposIRLookup(
                    ir, lookupId, irBuffer,
                    false, attachments))
            {
                return fail("case 6 IR execution");
            }

            const int32_t expectedAdvance =
                input[0].placement.offsetX +
                400 -
                input[1].placement.offsetX -
                100;

            if (irBuffer[0].placement.advanceX !=
                expectedAdvance)
            {
                return fail("case 6 first subtable did not win");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - ExtensionPos Type 9 -> CursivePos Type 3.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> records =
            {
                { 10, false, true, 0, 0, 460, 16 },
                { 20, true, false, 110, -6, 0, 0 }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRCursiveSubtable(records);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveExtensionLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 7 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGposIREffectiveType(
                    lookup, effectiveType) ||
                effectiveType != 3)
            {
                return fail("case 7 effective type");
            }

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRCursiveBuffer(glyphs, 2);

            if (!testGposIRCursiveResolveAt(
                    lookup, gdef, ir, *compiled,
                    input, 0, "Type 9 -> Type 3 resolver") ||
                !testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "Type 9 -> Type 3 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - three-glyph cursive chain and provenance preservation.
        // ====================================================================

        {
            ++cases;

            const std::vector<GposIRCursiveRecordSpec> records =
            {
                { 10, false, true, 0, 0, 420, 12 },
                { 20, true, true, 90, -8, 430, 19 },
                { 30, true, false, 95, -11, 0, 0 }
            };

            const std::vector<uint8_t> lookupBytes =
                makeGposIRCursiveLookup({
                    makeGposIRCursiveSubtable(records)
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 8 raw lookup");

            const OpenTypeGdefView gdef{};
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRCursive(
                    lookup, gdef,
                    ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            ShapedGlyphBuffer input;

            appendGposIRCursiveGlyph(
                input, 10, 4,
                500, 0, 7, -3);

            appendGposIRCursiveGlyph(
                input, 20, 8,
                510, 0, -2, 5);

            appendGposIRCursiveGlyph(
                input, 30, 12,
                520, 0, 4, -6);

            input[0].shaping.scalarCount = 2;
            input[0].shaping.ligature.id = 71;
            input[0].shaping.ligature.component = 1;
            input[0].shaping.ligature.componentCount = 3;

            input[1].shaping.scalarCount = 3;
            input[1].shaping.ligature.id = 72;
            input[1].shaping.ligature.component = 2;
            input[1].shaping.ligature.componentCount = 4;

            input[2].shaping.scalarCount = 4;
            input[2].shaping.ligature.id = 73;
            input[2].shaping.ligature.component = 3;
            input[2].shaping.ligature.componentCount = 5;

            const OpenTypeShapingGlyph shaping0 =
                input[0].shaping;

            const OpenTypeShapingGlyph shaping1 =
                input[1].shaping;

            const OpenTypeShapingGlyph shaping2 =
                input[2].shaping;

            if (!testGposIRCursiveSharedExecution(
                    lookup, gdef, ir, lookupId,
                    input, false, "three-glyph chain"))
            {
                return false;
            }

            ShapedGlyphBuffer irBuffer = input;
            OpenTypeGposAttachmentState attachments;
            attachments.reset(irBuffer.size());

            if (!applyOpenTypeGposIRLookup(
                    ir, lookupId, irBuffer,
                    false, attachments))
            {
                return fail("case 8 IR shared execution");
            }

            if (attachments[1].type !=
                    OpenTypeGposAttachmentType::Cursive ||
                attachments[1].parent != 0 ||
                attachments[2].type !=
                    OpenTypeGposAttachmentType::Cursive ||
                attachments[2].parent != 1)
            {
                return fail("case 8 attachment chain");
            }

            if (!gposIRCursiveShapingEqual(
                    irBuffer[0].shaping, shaping0) ||
                !gposIRCursiveShapingEqual(
                    irBuffer[1].shaping, shaping1) ||
                !gposIRCursiveShapingEqual(
                    irBuffer[2].shaping, shaping2))
            {
                return fail("case 8 provenance changed");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR Cursive: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Basic LTR:                PASS\n"
            "  RTL run direction:        PASS\n"
            "  RightToLeft lookup flag:  PASS\n"
            "  Filtered traversal:       PASS\n"
            "  NULL-anchor fallthrough:  PASS\n"
            "  Subtable order:           PASS\n"
            "  Type 9 -> Type 3:         PASS\n"
            "  Attachment chain:         PASS\n"
            "  Final attachment resolve: PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
