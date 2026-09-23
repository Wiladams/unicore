// test_opentype_gpos_ir_pair.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_gpos_ir_executor.h"
//#include "opentype_gpos_lookup_apply.h"
#include "opentype_layout_view.h"

namespace waavs
{
    static void appendGposIRPairU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRPairU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGposIRPairU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRPairU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRPairS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRPairU16(data, static_cast<uint16_t>(value));
    }


    static void appendGposIRPairCoverage1(
        std::vector<uint8_t>& data, const std::vector<uint16_t>& glyphs)
    {
        appendGposIRPairU16(data, 1);
        appendGposIRPairU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyph : glyphs)
            appendGposIRPairU16(data, glyph);
    }


    struct GposIRPairValueSpec
    {
        int16_t xPlacement{ 0 };
        int16_t yPlacement{ 0 };
        int16_t xAdvance{ 0 };
        int16_t yAdvance{ 0 };
    };


    static void appendGposIRPairValueRecord(
        std::vector<uint8_t>& data, uint16_t valueFormat, const GposIRPairValueSpec& value)
    {
        if ((valueFormat & kOpenTypeGposValueXPlacement) != 0)
            appendGposIRPairS16(data, value.xPlacement);

        if ((valueFormat & kOpenTypeGposValueYPlacement) != 0)
            appendGposIRPairS16(data, value.yPlacement);

        if ((valueFormat & kOpenTypeGposValueXAdvance) != 0)
            appendGposIRPairS16(data, value.xAdvance);

        if ((valueFormat & kOpenTypeGposValueYAdvance) != 0)
            appendGposIRPairS16(data, value.yAdvance);
    }


    struct GposIRPairExplicitSpec
    {
        uint16_t first{ 0 };
        uint16_t second{ 0 };
        GposIRPairValueSpec firstValue{};
        GposIRPairValueSpec secondValue{};
    };


    static std::vector<uint8_t> makeGposIRPairFormat1(
        const std::vector<GposIRPairExplicitSpec>& sourcePairs,
        uint16_t valueFormat1, uint16_t valueFormat2)
    {
        std::vector<uint16_t> firstGlyphs;

        for (const GposIRPairExplicitSpec& pair : sourcePairs)
        {
            if (firstGlyphs.empty() || firstGlyphs.back() != pair.first)
                firstGlyphs.push_back(pair.first);
        }

        std::vector<uint8_t> data;

        appendGposIRPairU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposIRPairU16(data, 0);

        appendGposIRPairU16(data, valueFormat1);
        appendGposIRPairU16(data, valueFormat2);
        appendGposIRPairU16(data, static_cast<uint16_t>(firstGlyphs.size()));

        const size_t pairSetPatchBase = data.size();

        for (size_t i = 0; i < firstGlyphs.size(); ++i)
            appendGposIRPairU16(data, 0);

        for (size_t firstIndex = 0; firstIndex < firstGlyphs.size(); ++firstIndex)
        {
            const uint16_t firstGlyph = firstGlyphs[firstIndex];
            const size_t pairSetOffset = data.size();

            patchGposIRPairU16(
                data, pairSetPatchBase + firstIndex * 2,
                static_cast<uint16_t>(pairSetOffset));

            uint16_t pairCount = 0;

            for (const GposIRPairExplicitSpec& pair : sourcePairs)
            {
                if (pair.first == firstGlyph)
                    ++pairCount;
            }

            appendGposIRPairU16(data, pairCount);

            for (const GposIRPairExplicitSpec& pair : sourcePairs)
            {
                if (pair.first != firstGlyph)
                    continue;

                appendGposIRPairU16(data, pair.second);
                appendGposIRPairValueRecord(data, valueFormat1, pair.firstValue);
                appendGposIRPairValueRecord(data, valueFormat2, pair.secondValue);
            }
        }

        const size_t coverageOffset = data.size();
        patchGposIRPairU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRPairCoverage1(data, firstGlyphs);

        return data;
    }


    struct GposIRPairClassRangeSpec
    {
        uint16_t first{ 0 };
        uint16_t last{ 0 };
        uint16_t value{ 0 };
    };


    static void appendGposIRPairClassDef2(
        std::vector<uint8_t>& data, const std::vector<GposIRPairClassRangeSpec>& ranges)
    {
        appendGposIRPairU16(data, 2);
        appendGposIRPairU16(data, static_cast<uint16_t>(ranges.size()));

        for (const GposIRPairClassRangeSpec& range : ranges)
        {
            appendGposIRPairU16(data, range.first);
            appendGposIRPairU16(data, range.last);
            appendGposIRPairU16(data, range.value);
        }
    }


    struct GposIRPairClassValueSpec
    {
        GposIRPairValueSpec firstValue{};
        GposIRPairValueSpec secondValue{};
    };


    static std::vector<uint8_t> makeGposIRPairFormat2(
        const std::vector<uint16_t>& coverageGlyphs,
        const std::vector<GposIRPairClassRangeSpec>& firstClassRanges,
        const std::vector<GposIRPairClassRangeSpec>& secondClassRanges,
        uint16_t firstClassCount, uint16_t secondClassCount,
        const std::vector<GposIRPairClassValueSpec>& values,
        uint16_t valueFormat1, uint16_t valueFormat2)
    {
        std::vector<uint8_t> data;

        appendGposIRPairU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposIRPairU16(data, 0);

        appendGposIRPairU16(data, valueFormat1);
        appendGposIRPairU16(data, valueFormat2);

        const size_t classDef1Patch = data.size();
        appendGposIRPairU16(data, 0);

        const size_t classDef2Patch = data.size();
        appendGposIRPairU16(data, 0);

        appendGposIRPairU16(data, firstClassCount);
        appendGposIRPairU16(data, secondClassCount);

        const size_t expectedValues =
            size_t(firstClassCount) * size_t(secondClassCount);

        if (values.size() != expectedValues)
            return {};

        for (const GposIRPairClassValueSpec& value : values)
        {
            appendGposIRPairValueRecord(data, valueFormat1, value.firstValue);
            appendGposIRPairValueRecord(data, valueFormat2, value.secondValue);
        }

        const size_t coverageOffset = data.size();
        patchGposIRPairU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGposIRPairCoverage1(data, coverageGlyphs);

        const size_t classDef1Offset = data.size();
        patchGposIRPairU16(data, classDef1Patch, static_cast<uint16_t>(classDef1Offset));
        appendGposIRPairClassDef2(data, firstClassRanges);

        const size_t classDef2Offset = data.size();
        patchGposIRPairU16(data, classDef2Patch, static_cast<uint16_t>(classDef2Offset));
        appendGposIRPairClassDef2(data, secondClassRanges);

        return data;
    }


    static std::vector<uint8_t> makeGposIRPairLookup(
        const std::vector<std::vector<uint8_t>>& subtables, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRPairU16(data, 2);
        appendGposIRPairU16(data, lookupFlag);
        appendGposIRPairU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patchBase = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRPairU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();
            patchGposIRPairU16(data, patchBase + i * 2, static_cast<uint16_t>(offset));
            data.insert(data.end(), subtables[i].begin(), subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRPairExtensionLookup(
        const std::vector<uint8_t>& pairSubtable)
    {
        std::vector<uint8_t> data;

        appendGposIRPairU16(data, 9);
        appendGposIRPairU16(data, 0);
        appendGposIRPairU16(data, 1);
        appendGposIRPairU16(data, 8);

        const size_t extensionBase = data.size();

        appendGposIRPairU16(data, 1);
        appendGposIRPairU16(data, 2);

        const size_t extensionOffsetPatch = data.size();
        appendGposIRPairU32(data, 0);

        const size_t pairOffset = data.size();

        patchGposIRPairU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(pairOffset - extensionBase));

        data.insert(data.end(), pairSubtable.begin(), pairSubtable.end());
        return data;
    }


    static std::vector<uint8_t> makeGposIRPairMarkGdef(uint16_t markGlyph)
    {
        std::vector<uint8_t> data;

        appendGposIRPairU16(data, 1);
        appendGposIRPairU16(data, 0);

        appendGposIRPairU16(data, 12);
        appendGposIRPairU16(data, 0);
        appendGposIRPairU16(data, 0);
        appendGposIRPairU16(data, 0);

        appendGposIRPairU16(data, 2);
        appendGposIRPairU16(data, 1);
        appendGposIRPairU16(data, markGlyph);
        appendGposIRPairU16(data, markGlyph);
        appendGposIRPairU16(data, 3);

        return data;
    }


    static void appendGposIRPairGlyph(
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


    static ShapedGlyphBuffer makeGposIRPairBuffer(
        const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRPairGlyph(
                buffer, glyphs[i], static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i),
                static_cast<int32_t>(i),
                static_cast<int32_t>(i * 2),
                -static_cast<int32_t>(i));
        }

        return buffer;
    }


    static bool gposIRPairShapingEqual(
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


    static bool gposIRPairPlacementEqual(
        const GlyphPlacement& a, const GlyphPlacement& b) noexcept
    {
        return
            a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool gposIRPairBuffersEqual(
        const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRPairShapingEqual(a[i].shaping, b[i].shaping) ||
                !gposIRPairPlacementEqual(a[i].placement, b[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    static bool compileGposIRPair(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGposPairLookup(lookup, gdef, ir, lookupId))
            return false;

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposPair &&
            openTypeGposIRPairLookupValid(ir, *compiled);
    }


    static bool testGposIRPairExecution(
        const OpenTypeLayoutLookupView& rawLookup, const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        const ShapedGlyphBuffer& input, const char* caseName)
    {
        ShapedGlyphBuffer rawBuffer = input;
        ShapedGlyphBuffer irBuffer = input;

        const bool rawSuccess =
            applyOpenTypeGposPairLookup(rawLookup, gdef, rawBuffer);

        const bool irSuccess =
            applyOpenTypeGposIRLookup(ir, lookupId, irBuffer);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GPOS IR Pair: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gposIRPairBuffersEqual(rawBuffer, irBuffer))
        {
            std::printf(
                "GPOS IR Pair: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR buffer mismatch\n",
                caseName);

            for (size_t i = 0; i < rawBuffer.size() && i < irBuffer.size(); ++i)
            {
                const GlyphPlacement& a = rawBuffer[i].placement;
                const GlyphPlacement& b = irBuffer[i].placement;

                if (!gposIRPairPlacementEqual(a, b))
                {
                    std::printf(
                        "  Glyph index: %zu\n"
                        "  Raw: adv=(%d,%d) off=(%d,%d)\n"
                        "  IR:  adv=(%d,%d) off=(%d,%d)\n",
                        i,
                        a.advanceX, a.advanceY, a.offsetX, a.offsetY,
                        b.advanceX, b.advanceY, b.offsetX, b.offsetY);
                }
            }

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRPair()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR Pair: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - PairPos Format 1 explicit pairs.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueXAdvance;

            const uint16_t valueFormat2 =
                kOpenTypeGposValueYPlacement;

            const std::vector<GposIRPairExplicitSpec> pairs =
            {
                { 10, 20, { 3, 0, -25, 0 }, { 0, 7, 0, 0 } },
                { 10, 30, { 5, 0, -35, 0 }, { 0, 9, 0, 0 } },
                { 20, 30, { 8, 0, -45, 0 }, { 0, 11, 0, 0 } }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat1(
                    pairs, valueFormat1, valueFormat2);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            if (compiled->payloadCount != 1 ||
                compiled->payloadOffset >= ir.gposPairSubtables.size())
            {
                return fail("case 1 payload");
            }

            const OpenTypeShapingIRGposPairSubtable& irSubtable =
                ir.gposPairSubtables[compiled->payloadOffset];

            if (irSubtable.kind != OpenTypeShapingIRGposPairKind::Explicit ||
                irSubtable.secondParticipates != 1 ||
                irSubtable.payloadIndex >= ir.gposPairExplicitSubtables.size())
            {
                return fail("case 1 explicit representation");
            }

            const OpenTypeShapingIRGposPairExplicitSubtable& explicitSubtable =
                ir.gposPairExplicitSubtables[irSubtable.payloadIndex];

            if (explicitSubtable.pairCount != 3)
                return fail("case 1 explicit pair count");

            const uint32_t glyphs[] = { 10, 20, 30 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 3);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "Format 1 explicit"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - PairPos Format 2 class pair.
        //
        // First Coverage remains semantically significant. Class zero matrix
        // entries are deliberately non-zero, so a non-covered glyph must still
        // produce NoMatch.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXAdvance;

            const uint16_t valueFormat2 =
                kOpenTypeGposValueXPlacement;

            const std::vector<uint16_t> coverage =
            {
                10, 11
            };

            const std::vector<GposIRPairClassRangeSpec> firstClasses =
            {
                { 10, 11, 1 }
            };

            const std::vector<GposIRPairClassRangeSpec> secondClasses =
            {
                { 20, 20, 1 },
                { 21, 21, 2 }
            };

            std::vector<GposIRPairClassValueSpec> values(2 * 3);

            // class1=0 row: deliberately non-zero.
            values[0 * 3 + 0].firstValue.xAdvance = -101;
            values[0 * 3 + 1].firstValue.xAdvance = -102;
            values[0 * 3 + 2].firstValue.xAdvance = -103;

            // class1=1 row.
            values[1 * 3 + 0].firstValue.xAdvance = -10;
            values[1 * 3 + 1].firstValue.xAdvance = -20;
            values[1 * 3 + 2].firstValue.xAdvance = -30;

            values[1 * 3 + 1].secondValue.xPlacement = 4;
            values[1 * 3 + 2].secondValue.xPlacement = 6;

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat2(
                    coverage,
                    firstClasses,
                    secondClasses,
                    2, 3, values,
                    valueFormat1, valueFormat2);

            if (subtable.empty())
                return fail("case 2 subtable construction");

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            const OpenTypeShapingIRGposPairSubtable& irSubtable =
                ir.gposPairSubtables[compiled->payloadOffset];

            if (irSubtable.kind != OpenTypeShapingIRGposPairKind::Class ||
                irSubtable.secondParticipates != 1 ||
                irSubtable.payloadIndex >= ir.gposPairClassSubtables.size())
            {
                return fail("case 2 class representation");
            }

            const OpenTypeShapingIRGposPairClassSubtable& classSubtable =
                ir.gposPairClassSubtables[irSubtable.payloadIndex];

            if (classSubtable.firstClassCount != 2 ||
                classSubtable.secondClassCount != 3 ||
                !openTypeGposIRGlyphSetValid(ir, classSubtable.firstCoverage) ||
                !openTypeGposIRGlyphClassMapValid(ir, classSubtable.firstClassMap) ||
                !openTypeGposIRGlyphClassMapValid(ir, classSubtable.secondClassMap))
            {
                return fail("case 2 normalized class representation");
            }

            const uint32_t glyphs[] = { 10, 20, 11, 21, 12, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 6);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "Format 2 class"))
            {
                return false;
            }

            // Explicitly prove the non-covered class-zero first glyph does not
            // match even though its class-matrix entry is non-zero.
            ShapedGlyphBuffer rawBuffer =
                makeGposIRPairBuffer(glyphs + 4, 2);

            ShapedGlyphBuffer irBuffer = rawBuffer;
            size_t rawResume = 0;
            size_t irResume = 0;

            const OpenTypeGposResolveResult rawResult =
                applyOpenTypeGposPairLookupAt(
                    lookup, gdef, rawBuffer, 0, &rawResume);

            const OpenTypeGposIRResult irResult =
                applyOpenTypeGposIRPairLookupAt(
                    ir, *compiled, irBuffer, 0, &irResume);

            if (rawResult != OpenTypeGposResolveResult::NoMatch ||
                irResult != OpenTypeGposIRResult::NoMatch)
            {
                return fail("case 2 first Coverage not enforced");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - LookupFlag filtered second-glyph traversal.
        //
        // glyph 100 is GDEF Mark and IgnoreMarks is active. Pair 10,20 must
        // match across the physically intervening mark.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXAdvance;

            const std::vector<GposIRPairExplicitSpec> pairs =
            {
                { 10, 20, { 0, 0, -40, 0 }, {} }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat1(
                    pairs, valueFormat1, 0);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup(
                    { subtable },
                    kOpenTypeLookupFlagIgnoreMarks);

            const std::vector<uint8_t> gdefBytes =
                makeGposIRPairMarkGdef(100);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefBytes.data(), gdefBytes.size()));

            if (!lookup || !gdef)
                return fail("case 3 raw views");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            if ((compiled->filter.flags & OpenTypeShapingIRIgnoreMarks) == 0 ||
                ir.gdefGlyphClasses.size() != kOpenTypeShapingIRGlyphDomainSize)
            {
                return fail("case 3 filter normalization");
            }

            const uint32_t glyphs[] = { 10, 100, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 3);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "filtered second-glyph traversal"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - valueFormat2 == 0 resumes AT the second glyph.
        //
        // This permits both 10,20 and then 20,30 to match in one whole lookup.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXAdvance;

            const std::vector<GposIRPairExplicitSpec> pairs =
            {
                { 10, 20, { 0, 0, -10, 0 }, {} },
                { 20, 30, { 0, 0, -20, 0 }, {} }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat1(
                    pairs, valueFormat1, 0);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 4 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            const OpenTypeShapingIRGposPairSubtable& irSubtable =
                ir.gposPairSubtables[compiled->payloadOffset];

            if (irSubtable.secondParticipates != 0)
                return fail("case 4 secondParticipates");

            const uint32_t glyphs[] = { 10, 20, 30 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 3);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "resume at second glyph"))
            {
                return false;
            }

            ShapedGlyphBuffer rawBuffer = input;
            ShapedGlyphBuffer irBuffer = input;

            size_t rawResume = 0;
            size_t irResume = 0;

            if (applyOpenTypeGposPairLookupAt(
                    lookup, gdef, rawBuffer, 0, &rawResume) !=
                    OpenTypeGposResolveResult::Match ||
                applyOpenTypeGposIRPairLookupAt(
                    ir, *compiled, irBuffer, 0, &irResume) !=
                    OpenTypeGposIRResult::Match ||
                rawResume != 1 ||
                irResume != 1)
            {
                return fail("case 4 exact resume index");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - valueFormat2 != 0 resumes AFTER the second glyph.
        //
        // The second record is present but numerically zero. This proves that
        // presence, not adjustment.empty(), controls scan semantics.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXAdvance;

            const uint16_t valueFormat2 =
                kOpenTypeGposValueXPlacement;

            const std::vector<GposIRPairExplicitSpec> pairs =
            {
                { 10, 20, { 0, 0, -10, 0 }, { 0, 0, 0, 0 } },
                { 20, 30, { 0, 0, -20, 0 }, { 0, 0, 0, 0 } }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat1(
                    pairs, valueFormat1, valueFormat2);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const OpenTypeShapingIRGposPairSubtable& irSubtable =
                ir.gposPairSubtables[compiled->payloadOffset];

            if (irSubtable.secondParticipates != 1)
                return fail("case 5 secondParticipates");

            const uint32_t glyphs[] = { 10, 20, 30 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 3);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "resume after second glyph"))
            {
                return false;
            }

            ShapedGlyphBuffer rawBuffer = input;
            ShapedGlyphBuffer irBuffer = input;

            size_t rawResume = 0;
            size_t irResume = 0;

            if (applyOpenTypeGposPairLookupAt(
                    lookup, gdef, rawBuffer, 0, &rawResume) !=
                    OpenTypeGposResolveResult::Match ||
                applyOpenTypeGposIRPairLookupAt(
                    ir, *compiled, irBuffer, 0, &irResume) !=
                    OpenTypeGposIRResult::Match ||
                rawResume != 2 ||
                irResume != 2)
            {
                return fail("case 5 exact resume index");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXAdvance;

            const std::vector<GposIRPairExplicitSpec> firstPairs =
            {
                { 10, 20, { 0, 0, -10, 0 }, {} }
            };

            const std::vector<GposIRPairExplicitSpec> secondPairs =
            {
                { 10, 20, { 0, 0, -99, 0 }, {} }
            };

            const std::vector<uint8_t> firstSubtable =
                makeGposIRPairFormat1(
                    firstPairs, valueFormat1, 0);

            const std::vector<uint8_t> secondSubtable =
                makeGposIRPairFormat1(
                    secondPairs, valueFormat1, 0);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup({
                    firstSubtable,
                    secondSubtable
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 6 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            if (compiled->payloadCount != 2)
                return fail("case 6 subtable count");

            const uint32_t glyphs[] = { 10, 20 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 2);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "subtable order"))
            {
                return false;
            }

            ShapedGlyphBuffer irBuffer = input;

            if (!applyOpenTypeGposIRLookup(
                    ir, lookupId, irBuffer))
            {
                return fail("case 6 IR execution");
            }

            if (irBuffer[0].placement.advanceX !=
                input[0].placement.advanceX - 10)
            {
                return fail("case 6 first subtable did not win");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - ExtensionPos Type 9 -> PairPos Type 2.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueXAdvance;

            const std::vector<GposIRPairExplicitSpec> pairs =
            {
                { 10, 20, { 6, 0, -17, 0 }, {} }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat1(
                    pairs, valueFormat1, 0);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairExtensionLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 7 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGposIREffectiveType(
                    lookup, effectiveType) ||
                effectiveType != 2)
            {
                return fail("case 7 effective type");
            }

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t glyphs[] = { 10, 20, 30 };

            const ShapedGlyphBuffer input =
                makeGposIRPairBuffer(glyphs, 3);

            if (!testGposIRPairExecution(
                    lookup, gdef, ir, lookupId, input,
                    "Type 9 -> Type 2"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - exact-position pair execution and provenance preservation.
        // ====================================================================

        {
            ++cases;

            const uint16_t valueFormat1 =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueYPlacement |
                kOpenTypeGposValueXAdvance |
                kOpenTypeGposValueYAdvance;

            const uint16_t valueFormat2 =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueYAdvance;

            const std::vector<GposIRPairExplicitSpec> pairs =
            {
                {
                    10, 20,
                    { 7, -3, -20, 4 },
                    { -5, 0, 0, 6 }
                }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRPairFormat1(
                    pairs, valueFormat1, valueFormat2);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRPairLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 8 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;
            const OpenTypeGdefView gdef{};

            if (!compileGposIRPair(
                    lookup, gdef, ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            ShapedGlyphBuffer rawBuffer;
            appendGposIRPairGlyph(rawBuffer, 10, 4, 500, 1, 11, -9);
            appendGposIRPairGlyph(rawBuffer, 20, 8, 600, 2, -3, 7);

            rawBuffer[0].shaping.scalarCount = 2;
            rawBuffer[0].shaping.ligature.id = 71;
            rawBuffer[0].shaping.ligature.component = 1;
            rawBuffer[0].shaping.ligature.componentCount = 3;

            rawBuffer[1].shaping.scalarCount = 4;
            rawBuffer[1].shaping.ligature.id = 72;
            rawBuffer[1].shaping.ligature.component = 2;
            rawBuffer[1].shaping.ligature.componentCount = 3;

            ShapedGlyphBuffer irBuffer = rawBuffer;

            const OpenTypeShapingGlyph firstShaping =
                rawBuffer[0].shaping;

            const OpenTypeShapingGlyph secondShaping =
                rawBuffer[1].shaping;

            size_t rawResume = 0;
            size_t irResume = 0;

            const OpenTypeGposResolveResult rawResult =
                applyOpenTypeGposPairLookupAt(
                    lookup, gdef, rawBuffer, 0, &rawResume);

            const OpenTypeGposIRResult irResult =
                applyOpenTypeGposIRPairLookupAt(
                    ir, *compiled, irBuffer, 0, &irResume);

            if (rawResult != OpenTypeGposResolveResult::Match ||
                irResult != OpenTypeGposIRResult::Match ||
                rawResume != irResume ||
                rawResume != 2 ||
                !gposIRPairBuffersEqual(rawBuffer, irBuffer))
            {
                return fail("case 8 exact-position differential");
            }

            if (!gposIRPairShapingEqual(
                    irBuffer[0].shaping, firstShaping) ||
                !gposIRPairShapingEqual(
                    irBuffer[1].shaping, secondShaping))
            {
                return fail("case 8 provenance changed");
            }

            if (irBuffer[0].placement.offsetX != 18 ||
                irBuffer[0].placement.offsetY != -12 ||
                irBuffer[0].placement.advanceX != 480 ||
                irBuffer[0].placement.advanceY != 5 ||
                irBuffer[1].placement.offsetX != -8 ||
                irBuffer[1].placement.offsetY != 7 ||
                irBuffer[1].placement.advanceX != 600 ||
                irBuffer[1].placement.advanceY != 8)
            {
                return fail("case 8 additive placement result");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR Pair: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 explicit:         PASS\n"
            "  Format 2 class:            PASS\n"
            "  First Coverage:            PASS\n"
            "  Filtered second traversal: PASS\n"
            "  Resume at second:          PASS\n"
            "  Resume after second:       PASS\n"
            "  Zero second adjustment:    PASS\n"
            "  Subtable order:            PASS\n"
            "  Type 9 -> Type 2:          PASS\n"
            "  Exact-position apply:      PASS\n"
            "  Provenance preserved:      PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
