// test_opentype_gpos_ir_single.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_gpos_ir_executor.h"
//#include "opentype_gpos_lookup_apply.h"
#include "opentype_layout_view.h"

namespace waavs
{
    static void appendGposIRSingleU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGposIRSingleU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGposIRSingleU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGposIRSingleU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGposIRSingleS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposIRSingleU16(data, static_cast<uint16_t>(value));
    }


    static void appendGposIRSingleCoverage1(std::vector<uint8_t>& data, const uint16_t* glyphs, size_t count)
    {
        appendGposIRSingleU16(data, 1);
        appendGposIRSingleU16(data, static_cast<uint16_t>(count));

        for (size_t i = 0; i < count; ++i)
            appendGposIRSingleU16(data, glyphs[i]);
    }


    struct GposIRSingleValueSpec
    {
        int16_t xPlacement{ 0 };
        int16_t yPlacement{ 0 };
        int16_t xAdvance{ 0 };
        int16_t yAdvance{ 0 };
    };


    static void appendGposIRSingleValueRecord(
        std::vector<uint8_t>& data, uint16_t valueFormat, const GposIRSingleValueSpec& value)
    {
        if ((valueFormat & kOpenTypeGposValueXPlacement) != 0)
            appendGposIRSingleS16(data, value.xPlacement);

        if ((valueFormat & kOpenTypeGposValueYPlacement) != 0)
            appendGposIRSingleS16(data, value.yPlacement);

        if ((valueFormat & kOpenTypeGposValueXAdvance) != 0)
            appendGposIRSingleS16(data, value.xAdvance);

        if ((valueFormat & kOpenTypeGposValueYAdvance) != 0)
            appendGposIRSingleS16(data, value.yAdvance);
    }


    static std::vector<uint8_t> makeGposIRSingleFormat1(
        const uint16_t* glyphs, size_t glyphCount,
        uint16_t valueFormat, const GposIRSingleValueSpec& value)
    {
        std::vector<uint8_t> data;

        appendGposIRSingleU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposIRSingleU16(data, 0);

        appendGposIRSingleU16(data, valueFormat);
        appendGposIRSingleValueRecord(data, valueFormat, value);

        const size_t coverageOffset = data.size();
        patchGposIRSingleU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGposIRSingleCoverage1(data, glyphs, glyphCount);
        return data;
    }


    static std::vector<uint8_t> makeGposIRSingleFormat2(
        const uint16_t* glyphs, const GposIRSingleValueSpec* values,
        size_t glyphCount, uint16_t valueFormat)
    {
        std::vector<uint8_t> data;

        appendGposIRSingleU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposIRSingleU16(data, 0);

        appendGposIRSingleU16(data, valueFormat);
        appendGposIRSingleU16(data, static_cast<uint16_t>(glyphCount));

        for (size_t i = 0; i < glyphCount; ++i)
            appendGposIRSingleValueRecord(data, valueFormat, values[i]);

        const size_t coverageOffset = data.size();
        patchGposIRSingleU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGposIRSingleCoverage1(data, glyphs, glyphCount);
        return data;
    }


    static std::vector<uint8_t> makeGposIRSingleLookup(
        const std::vector<std::vector<uint8_t>>& subtables, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposIRSingleU16(data, 1);
        appendGposIRSingleU16(data, lookupFlag);
        appendGposIRSingleU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patches = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGposIRSingleU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();
            patchGposIRSingleU16(data, patches + i * 2, static_cast<uint16_t>(offset));
            data.insert(data.end(), subtables[i].begin(), subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGposIRSingleExtensionLookup(
        const std::vector<uint8_t>& singleSubtable)
    {
        std::vector<uint8_t> data;

        appendGposIRSingleU16(data, 9);
        appendGposIRSingleU16(data, 0);
        appendGposIRSingleU16(data, 1);
        appendGposIRSingleU16(data, 8);

        const size_t extensionBase = data.size();

        appendGposIRSingleU16(data, 1);
        appendGposIRSingleU16(data, 1);

        const size_t extensionOffsetPatch = data.size();
        appendGposIRSingleU32(data, 0);

        const size_t singleOffset = data.size();

        patchGposIRSingleU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(singleOffset - extensionBase));

        data.insert(data.end(), singleSubtable.begin(), singleSubtable.end());
        return data;
    }


    static void appendGposIRSingleGlyph(
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


    static ShapedGlyphBuffer makeGposIRSingleBuffer(const uint32_t* glyphs, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGposIRSingleGlyph(
                buffer, glyphs[i], static_cast<uint32_t>(i),
                500 + static_cast<int32_t>(i),
                static_cast<int32_t>(i),
                static_cast<int32_t>(i * 2),
                -static_cast<int32_t>(i));
        }

        return buffer;
    }


    static bool gposIRSingleShapingEqual(
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


    static bool gposIRSinglePlacementEqual(
        const GlyphPlacement& a, const GlyphPlacement& b) noexcept
    {
        return
            a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool gposIRSingleBuffersEqual(
        const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gposIRSingleShapingEqual(a[i].shaping, b[i].shaping) ||
                !gposIRSinglePlacementEqual(a[i].placement, b[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    static bool gposIRSingleAdjustmentEqual(
        const OpenTypeGposValueRecord& raw,
        const OpenTypeShapingIRPositionAdjustment& ir) noexcept
    {
        return
            static_cast<int32_t>(raw.xPlacement) == ir.offsetX &&
            static_cast<int32_t>(raw.yPlacement) == ir.offsetY &&
            static_cast<int32_t>(raw.xAdvance) == ir.advanceX &&
            static_cast<int32_t>(raw.yAdvance) == ir.advanceY;
    }


    static bool compileGposIRSingle(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId, const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        const OpenTypeGdefView gdef{};

        if (!compileOpenTypeGposSingleLookup(lookup, gdef, ir, lookupId))
            return false;

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GposSingle &&
            openTypeGposIRSingleLookupValid(ir, *compiled);
    }


    static bool testGposIRSingleResolver(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& compiled,
        uint16_t glyphId, const char* caseName)
    {
        OpenTypeGposValueRecord rawValue{};
        OpenTypeShapingIRPositionAdjustment irValue{};

        const OpenTypeGposResolveResult rawResult =
            resolveOpenTypeGposSingleLookup(rawLookup, glyphId, rawValue);

        const OpenTypeGposIRResult irResult =
            resolveOpenTypeGposIRSingleLookup(ir, compiled, glyphId, irValue);

        const bool sameResult =
            (rawResult == OpenTypeGposResolveResult::Invalid &&
                irResult == OpenTypeGposIRResult::Invalid) ||
            (rawResult == OpenTypeGposResolveResult::NoMatch &&
                irResult == OpenTypeGposIRResult::NoMatch) ||
            (rawResult == OpenTypeGposResolveResult::Match &&
                irResult == OpenTypeGposIRResult::Match);

        if (!sameResult ||
            (rawResult == OpenTypeGposResolveResult::Match &&
                !gposIRSingleAdjustmentEqual(rawValue, irValue)))
        {
            std::printf(
                "GPOS IR Single: FAIL\n"
                "  Case: %s\n"
                "  Glyph: %u\n"
                "  Raw result: %u\n"
                "  IR result:  %u\n",
                caseName,
                static_cast<unsigned>(glyphId),
                static_cast<unsigned>(rawResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        return true;
    }


    static bool applyRawGposIRSingleLookup(
        const OpenTypeLayoutLookupView& lookup, ShapedGlyphBuffer& buffer)
    {
        ShapedGlyphBuffer working = buffer;

        for (size_t i = 0; i < working.size(); ++i)
        {
            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(lookup, working, i);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        return true;
    }


    static bool testGposIRSingleExecution(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        const ShapedGlyphBuffer& input, const char* caseName)
    {
        ShapedGlyphBuffer rawBuffer = input;
        ShapedGlyphBuffer irBuffer = input;

        const bool rawSuccess =
            applyRawGposIRSingleLookup(rawLookup, rawBuffer);

        const bool irSuccess =
            applyOpenTypeGposIRLookup(ir, lookupId, irBuffer);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GPOS IR Single: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gposIRSingleBuffersEqual(rawBuffer, irBuffer))
        {
            std::printf(
                "GPOS IR Single: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR buffer mismatch\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGposIRSingle()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GPOS IR Single: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - SinglePos Format 1.
        //
        // One semantic adjustment applies to every covered glyph.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20, 30 };

            const uint16_t valueFormat =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueYPlacement |
                kOpenTypeGposValueXAdvance |
                kOpenTypeGposValueYAdvance;

            const GposIRSingleValueSpec value{
                7, -3, -20, 4
            };

            const std::vector<uint8_t> subtable =
                makeGposIRSingleFormat1(
                    glyphs, 3, valueFormat, value);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRSingleLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRSingle(lookup, ir, lookupId, compiled))
                return fail("case 1 compilation");

            if (compiled->payloadCount != 1 ||
                compiled->payloadOffset >= ir.gposSingleSubtables.size())
            {
                return fail("case 1 compiled payload");
            }

            const OpenTypeShapingIRGposSingleSubtable& irSubtable =
                ir.gposSingleSubtables[compiled->payloadOffset];

            if (irSubtable.pairCount != 3 ||
                irSubtable.pairOffset + irSubtable.pairCount > ir.gposSinglePairs.size())
            {
                return fail("case 1 pair count");
            }

            if (ir.gposSinglePairs[irSubtable.pairOffset].glyph != 10 ||
                ir.gposSinglePairs[irSubtable.pairOffset + 1].glyph != 20 ||
                ir.gposSinglePairs[irSubtable.pairOffset + 2].glyph != 30)
            {
                return fail("case 1 pair ordering");
            }

            if (!testGposIRSingleResolver(
                    lookup, ir, *compiled, 10, "Format 1 glyph 10") ||
                !testGposIRSingleResolver(
                    lookup, ir, *compiled, 20, "Format 1 glyph 20") ||
                !testGposIRSingleResolver(
                    lookup, ir, *compiled, 40, "Format 1 NoMatch"))
            {
                return false;
            }

            const uint32_t inputGlyphs[] = { 5, 10, 20, 30, 40 };

            const ShapedGlyphBuffer input =
                makeGposIRSingleBuffer(inputGlyphs, 5);

            if (!testGposIRSingleExecution(
                lookup, ir, lookupId, input, "Format 1 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - SinglePos Format 2.
        //
        // Coverage index selects distinct adjustments.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20, 30 };

            const uint16_t valueFormat =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueXAdvance;

            const GposIRSingleValueSpec values[] =
            {
                { 1, 0, -10, 0 },
                { 2, 0, -20, 0 },
                { 3, 0, -30, 0 }
            };

            const std::vector<uint8_t> subtable =
                makeGposIRSingleFormat2(
                    glyphs, values, 3, valueFormat);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRSingleLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRSingle(lookup, ir, lookupId, compiled))
                return fail("case 2 compilation");

            if (!testGposIRSingleResolver(
                    lookup, ir, *compiled, 10, "Format 2 glyph 10") ||
                !testGposIRSingleResolver(
                    lookup, ir, *compiled, 20, "Format 2 glyph 20") ||
                !testGposIRSingleResolver(
                    lookup, ir, *compiled, 30, "Format 2 glyph 30"))
            {
                return false;
            }

            const OpenTypeShapingIRGposSingleSubtable& irSubtable =
                ir.gposSingleSubtables[compiled->payloadOffset];

            if (irSubtable.pairCount != 3)
                return fail("case 2 pair count");

            const OpenTypeShapingIRPositionAdjustment& a =
                ir.gposSinglePairs[irSubtable.pairOffset].adjustment;

            const OpenTypeShapingIRPositionAdjustment& b =
                ir.gposSinglePairs[irSubtable.pairOffset + 1].adjustment;

            const OpenTypeShapingIRPositionAdjustment& c =
                ir.gposSinglePairs[irSubtable.pairOffset + 2].adjustment;

            if (a.offsetX != 1 || a.advanceX != -10 ||
                b.offsetX != 2 || b.advanceX != -20 ||
                c.offsetX != 3 || c.advanceX != -30)
            {
                return fail("case 2 coverage-index mapping");
            }

            const uint32_t inputGlyphs[] = { 30, 20, 10, 99 };

            const ShapedGlyphBuffer input =
                makeGposIRSingleBuffer(inputGlyphs, 4);

            if (!testGposIRSingleExecution(
                lookup, ir, lookupId, input, "Format 2 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - first matching subtable wins.
        //
        // The first subtable contains a real match with ValueFormat 0. Its
        // semantic adjustment is numerically empty, but it still wins.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyph[] = { 10 };

            const GposIRSingleValueSpec zero{};
            const GposIRSingleValueSpec second{ 50, 0, 0, 0 };

            const std::vector<uint8_t> firstSubtable =
                makeGposIRSingleFormat1(
                    glyph, 1, 0, zero);

            const std::vector<uint8_t> secondSubtable =
                makeGposIRSingleFormat1(
                    glyph, 1,
                    kOpenTypeGposValueXPlacement,
                    second);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRSingleLookup({
                    firstSubtable,
                    secondSubtable
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 3 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRSingle(lookup, ir, lookupId, compiled))
                return fail("case 3 compilation");

            OpenTypeGposValueRecord rawValue{};
            OpenTypeShapingIRPositionAdjustment irValue{};

            if (resolveOpenTypeGposSingleLookup(
                    lookup, 10, rawValue) !=
                    OpenTypeGposResolveResult::Match ||
                resolveOpenTypeGposIRSingleLookup(
                    ir, *compiled, 10, irValue) !=
                    OpenTypeGposIRResult::Match)
            {
                return fail("case 3 resolver");
            }

            if (rawValue.valueFormat != 0 || !irValue.empty())
                return fail("case 3 first matching zero adjustment did not win");

            const uint32_t inputGlyphs[] = { 10 };

            const ShapedGlyphBuffer input =
                makeGposIRSingleBuffer(inputGlyphs, 1);

            if (!testGposIRSingleExecution(
                lookup, ir, lookupId, input, "subtable order"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - ExtensionPos Type 9 -> SinglePos Type 1.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposIRSingleValueSpec value{
                -6, 5, 11, -2
            };

            const uint16_t valueFormat =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueYPlacement |
                kOpenTypeGposValueXAdvance |
                kOpenTypeGposValueYAdvance;

            const std::vector<uint8_t> subtable =
                makeGposIRSingleFormat1(
                    glyphs, 2, valueFormat, value);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRSingleExtensionLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 4 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGposIREffectiveType(
                    lookup, effectiveType) ||
                effectiveType != 1)
            {
                return fail("case 4 effective type");
            }

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRSingle(lookup, ir, lookupId, compiled))
                return fail("case 4 compilation");

            if (!testGposIRSingleResolver(
                    lookup, ir, *compiled, 10, "Type 9 -> Type 1 resolver"))
            {
                return false;
            }

            const uint32_t inputGlyphs[] = { 10, 20, 30 };

            const ShapedGlyphBuffer input =
                makeGposIRSingleBuffer(inputGlyphs, 3);

            if (!testGposIRSingleExecution(
                lookup, ir, lookupId, input, "Type 9 -> Type 1 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - exact-position mutation is additive and leaves shaping
        // identity/provenance untouched.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyph[] = { 20 };

            const GposIRSingleValueSpec value{
                7, -8, -30, 6
            };

            const uint16_t valueFormat =
                kOpenTypeGposValueXPlacement |
                kOpenTypeGposValueYPlacement |
                kOpenTypeGposValueXAdvance |
                kOpenTypeGposValueYAdvance;

            const std::vector<uint8_t> subtable =
                makeGposIRSingleFormat1(
                    glyph, 1, valueFormat, value);

            const std::vector<uint8_t> lookupBytes =
                makeGposIRSingleLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGposIRSingle(lookup, ir, lookupId, compiled))
                return fail("case 5 compilation");

            ShapedGlyphBuffer rawBuffer;
            appendGposIRSingleGlyph(rawBuffer, 10, 0, 400, 0, 1, 2);
            appendGposIRSingleGlyph(rawBuffer, 20, 4, 500, 3, 11, -9);
            appendGposIRSingleGlyph(rawBuffer, 30, 8, 600, 0, -3, 7);

            rawBuffer[1].shaping.scalarCount = 3;
            rawBuffer[1].shaping.ligature.id = 77;
            rawBuffer[1].shaping.ligature.component = 2;
            rawBuffer[1].shaping.ligature.componentCount = 4;

            ShapedGlyphBuffer irBuffer = rawBuffer;
            const OpenTypeShapingGlyph originalShaping = rawBuffer[1].shaping;

            const OpenTypeGposResolveResult rawResult =
                applyOpenTypeGposSingleLookupAt(
                    lookup, rawBuffer, 1);

            const OpenTypeGposIRResult irResult =
                applyOpenTypeGposIRSingleLookupAt(
                    ir, *compiled, irBuffer, 1);

            if (rawResult != OpenTypeGposResolveResult::Match ||
                irResult != OpenTypeGposIRResult::Match ||
                !gposIRSingleBuffersEqual(rawBuffer, irBuffer))
            {
                return fail("case 5 exact-position differential");
            }

            if (!gposIRSingleShapingEqual(
                    irBuffer[1].shaping, originalShaping))
            {
                return fail("case 5 shaping identity/provenance changed");
            }

            if (irBuffer[1].placement.offsetX != 18 ||
                irBuffer[1].placement.offsetY != -17 ||
                irBuffer[1].placement.advanceX != 470 ||
                irBuffer[1].placement.advanceY != 9)
            {
                return fail("case 5 additive placement result");
            }

            if (!gposIRSinglePlacementEqual(
                    irBuffer[0].placement,
                    rawBuffer[0].placement) ||
                !gposIRSinglePlacementEqual(
                    irBuffer[2].placement,
                    rawBuffer[2].placement))
            {
                return fail("case 5 unrelated glyph changed");
            }

            ++passed;
        }


        std::printf(
            "GPOS IR Single: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1:                 PASS\n"
            "  Format 2:                 PASS\n"
            "  Coverage-index mapping:   PASS\n"
            "  Subtable order:           PASS\n"
            "  Zero adjustment match:    PASS\n"
            "  Type 9 -> Type 1:         PASS\n"
            "  Exact-position apply:     PASS\n"
            "  Provenance preserved:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
