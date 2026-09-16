// test_opentype_gsub_ir_alternate.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gsub_alternate_view.h"
#include "opentype_gsub_lookup_apply.h"
#include "opentype_layout_view.h"
#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    static void appendGsubIRAlternateU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGsubIRAlternateU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubIRAlternateU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGsubIRAlternateU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGsubIRAlternateCoverage1(std::vector<uint8_t>& data, const uint16_t* glyphs, size_t count)
    {
        appendGsubIRAlternateU16(data, 1);
        appendGsubIRAlternateU16(data, static_cast<uint16_t>(count));

        for (size_t i = 0; i < count; ++i)
            appendGsubIRAlternateU16(data, glyphs[i]);
    }


    struct GsubIRAlternateSetSpec
    {
        const uint16_t* glyphs{ nullptr };
        uint16_t glyphCount{ 0 };
    };


    static std::vector<uint8_t> makeGsubIRAlternateSubtable(
        const uint16_t* inputs, const GsubIRAlternateSetSpec* sets, size_t count)
    {
        std::vector<uint8_t> data;

        appendGsubIRAlternateU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubIRAlternateU16(data, 0);

        appendGsubIRAlternateU16(data, static_cast<uint16_t>(count));

        const size_t setOffsetPatches = data.size();

        for (size_t i = 0; i < count; ++i)
            appendGsubIRAlternateU16(data, 0);

        for (size_t i = 0; i < count; ++i)
        {
            const size_t setOffset = data.size();

            patchGsubIRAlternateU16(
                data, setOffsetPatches + i * 2,
                static_cast<uint16_t>(setOffset));

            appendGsubIRAlternateU16(data, sets[i].glyphCount);

            for (uint16_t j = 0; j < sets[i].glyphCount; ++j)
                appendGsubIRAlternateU16(data, sets[i].glyphs[j]);
        }

        const size_t coverageOffset = data.size();

        patchGsubIRAlternateU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset));

        appendGsubIRAlternateCoverage1(data, inputs, count);
        return data;
    }


    static std::vector<uint8_t> makeGsubIRAlternateLookup(
        const std::vector<std::vector<uint8_t>>& subtables)
    {
        std::vector<uint8_t> data;

        appendGsubIRAlternateU16(data, 3);
        appendGsubIRAlternateU16(data, 0);
        appendGsubIRAlternateU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patches = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGsubIRAlternateU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();

            patchGsubIRAlternateU16(
                data, patches + i * 2,
                static_cast<uint16_t>(offset));

            data.insert(data.end(), subtables[i].begin(), subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGsubIRAlternateExtensionLookup(
        const std::vector<uint8_t>& alternateSubtable)
    {
        std::vector<uint8_t> data;

        appendGsubIRAlternateU16(data, 7);
        appendGsubIRAlternateU16(data, 0);
        appendGsubIRAlternateU16(data, 1);
        appendGsubIRAlternateU16(data, 8);

        const size_t extensionBase = data.size();

        appendGsubIRAlternateU16(data, 1);
        appendGsubIRAlternateU16(data, 3);

        const size_t extensionOffsetPatch = data.size();
        appendGsubIRAlternateU32(data, 0);

        const size_t alternateOffset = data.size();

        patchGsubIRAlternateU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(alternateOffset - extensionBase));

        data.insert(data.end(), alternateSubtable.begin(), alternateSubtable.end());
        return data;
    }


    static void appendGsubIRAlternateGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubIRAlternateBuffer(const uint32_t* glyphs, size_t count)
    {
        OpenTypeShapingBuffer buffer;

        for (size_t i = 0; i < count; ++i)
            appendGsubIRAlternateGlyph(buffer, glyphs[i], static_cast<uint32_t>(i));

        return buffer;
    }


    static bool gsubIRAlternateGlyphEqual(
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


    static bool gsubIRAlternateBuffersEqual(
        const OpenTypeShapingBuffer& a, const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gsubIRAlternateGlyphEqual(a[i], b[i]))
                return false;
        }

        return true;
    }


    static bool gsubIRAlternateGlyphIdsEqual(
        const OpenTypeShapingBuffer& buffer, const uint32_t* expected, size_t count) noexcept
    {
        if (buffer.size() != count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (buffer[i].glyphId != expected[i])
                return false;
        }

        return true;
    }


    static bool compileGsubIRAlternate(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId, const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        const OpenTypeGdefView gdef{};

        if (!compileOpenTypeGsubAlternateLookup(lookup, gdef, ir, lookupId))
            return false;

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GsubAlternate &&
            openTypeGsubIRAlternateLookupValid(ir, *compiled);
    }


    static bool testGsubIRAlternateResolver(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiled,
        uint16_t glyphId, const char* caseName)
    {
        uint16_t rawReplacement = 0;
        uint16_t irReplacement = 0;

        const OpenTypeGsubResolveResult rawResult =
            resolveOpenTypeGsubAlternateLookup(
                rawLookup, glyphId, rawReplacement);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRAlternateLookup(
                ir, compiled, glyphId, irReplacement);

        const bool sameResult =
            (rawResult == OpenTypeGsubResolveResult::Invalid &&
                irResult == OpenTypeShapingIRResult::Invalid) ||
            (rawResult == OpenTypeGsubResolveResult::NoMatch &&
                irResult == OpenTypeShapingIRResult::NoMatch) ||
            (rawResult == OpenTypeGsubResolveResult::Match &&
                irResult == OpenTypeShapingIRResult::Match);

        if (!sameResult ||
            (rawResult == OpenTypeGsubResolveResult::Match &&
                rawReplacement != irReplacement))
        {
            std::printf(
                "GSUB IR Alternate: FAIL\n"
                "  Case: %s\n"
                "  Glyph: %u\n"
                "  Raw result: %u\n"
                "  IR result:  %u\n"
                "  Raw replacement: %u\n"
                "  IR replacement:  %u\n",
                caseName,
                static_cast<unsigned>(glyphId),
                static_cast<unsigned>(rawResult),
                static_cast<unsigned>(irResult),
                static_cast<unsigned>(rawReplacement),
                static_cast<unsigned>(irReplacement));

            return false;
        }

        return true;
    }


    static bool testGsubIRAlternateExecution(
        const OpenTypeLayoutLookupView& rawLookup,
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        const OpenTypeShapingBuffer& input,
        const uint32_t* expected, size_t expectedCount,
        const char* caseName)
    {
        OpenTypeShapingBuffer rawBuffer = input;
        OpenTypeShapingBuffer irBuffer = input;

        const bool rawSuccess =
            applyOpenTypeGsubAlternateLookup(rawLookup, rawBuffer);

        const bool irSuccess =
            applyOpenTypeGsubIRLookup(ir, lookupId, irBuffer);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GSUB IR Alternate: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gsubIRAlternateBuffersEqual(rawBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Alternate: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR buffer mismatch\n",
                caseName);

            return false;
        }

        if (expected &&
            !gsubIRAlternateGlyphIdsEqual(irBuffer, expected, expectedCount))
        {
            std::printf(
                "GSUB IR Alternate: FAIL\n"
                "  Case: %s\n"
                "  Unexpected final glyph sequence\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGsubIRAlternate()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Alternate: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - native Type 3, complete AlternateSets retained.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10, 20 };
            const uint16_t alternates10[] = { 100, 101, 102 };
            const uint16_t alternates20[] = { 200, 201 };

            const GsubIRAlternateSetSpec sets[] =
            {
                { alternates10, 3 },
                { alternates20, 2 }
            };

            const std::vector<uint8_t> subtable =
                makeGsubIRAlternateSubtable(inputs, sets, 2);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRAlternateLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 1 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRAlternate(lookup, ir, lookupId, compiled))
                return fail("case 1 compilation");

            if (compiled->payloadCount != 1 ||
                compiled->payloadOffset >= ir.gsubAlternateSubtables.size())
            {
                return fail("case 1 compiled payload");
            }

            const OpenTypeShapingIRGsubAlternateSubtable& irSubtable =
                ir.gsubAlternateSubtables[compiled->payloadOffset];

            if (irSubtable.pairCount != 2 ||
                irSubtable.pairOffset + 2 > ir.gsubAlternatePairs.size())
            {
                return fail("case 1 pair count");
            }

            const OpenTypeShapingIRGsubAlternatePair& pair10 =
                ir.gsubAlternatePairs[irSubtable.pairOffset];

            const OpenTypeShapingIRGsubAlternatePair& pair20 =
                ir.gsubAlternatePairs[irSubtable.pairOffset + 1];

            if (pair10.input != 10 || pair20.input != 20 ||
                pair10.alternateSetIndex >= ir.gsubAlternateSets.size() ||
                pair20.alternateSetIndex >= ir.gsubAlternateSets.size())
            {
                return fail("case 1 pair mapping");
            }

            const OpenTypeShapingIRGsubAlternateSet& set10 =
                ir.gsubAlternateSets[pair10.alternateSetIndex];

            const OpenTypeShapingIRGsubAlternateSet& set20 =
                ir.gsubAlternateSets[pair20.alternateSetIndex];

            if (set10.glyphCount != 3 || set20.glyphCount != 2 ||
                set10.glyphOffset + 3 > ir.gsubAlternateGlyphs.size() ||
                set20.glyphOffset + 2 > ir.gsubAlternateGlyphs.size())
            {
                return fail("case 1 alternate set geometry");
            }

            if (ir.gsubAlternateGlyphs[set10.glyphOffset] != 100 ||
                ir.gsubAlternateGlyphs[set10.glyphOffset + 1] != 101 ||
                ir.gsubAlternateGlyphs[set10.glyphOffset + 2] != 102 ||
                ir.gsubAlternateGlyphs[set20.glyphOffset] != 200 ||
                ir.gsubAlternateGlyphs[set20.glyphOffset + 1] != 201)
            {
                return fail("case 1 alternate order");
            }

            if (!testGsubIRAlternateResolver(
                lookup, ir, *compiled, 10, "native resolver glyph 10") ||
                !testGsubIRAlternateResolver(
                    lookup, ir, *compiled, 20, "native resolver glyph 20") ||
                !testGsubIRAlternateResolver(
                    lookup, ir, *compiled, 30, "native resolver NoMatch"))
            {
                return false;
            }

            const uint32_t inputGlyphs[] = { 9, 10, 20, 10, 30 };
            const uint32_t expected[] = { 9, 100, 200, 100, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRAlternateBuffer(inputGlyphs, 5);

            if (!testGsubIRAlternateExecution(
                lookup, ir, lookupId, input,
                expected, 5, "native execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - subtable order: first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputGlyph[] = { 10 };
            const uint16_t alternateA[] = { 111 };
            const uint16_t alternateB[] = { 222 };

            const GsubIRAlternateSetSpec setA[] = { { alternateA, 1 } };
            const GsubIRAlternateSetSpec setB[] = { { alternateB, 1 } };

            const std::vector<uint8_t> subtableA =
                makeGsubIRAlternateSubtable(inputGlyph, setA, 1);

            const std::vector<uint8_t> subtableB =
                makeGsubIRAlternateSubtable(inputGlyph, setB, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRAlternateLookup({ subtableA, subtableB });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 2 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRAlternate(lookup, ir, lookupId, compiled))
                return fail("case 2 compilation");

            if (!testGsubIRAlternateResolver(
                lookup, ir, *compiled, 10, "subtable order"))
            {
                return false;
            }

            uint16_t replacement = 0;

            if (resolveOpenTypeGsubIRAlternateLookup(
                ir, *compiled, 10, replacement) !=
                OpenTypeShapingIRResult::Match ||
                replacement != 111)
            {
                return fail("case 2 first subtable did not win");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - empty AlternateSet falls through to later subtable.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputGlyph[] = { 10 };
            const uint16_t alternate[] = { 222 };

            const GsubIRAlternateSetSpec emptySet[] = { { nullptr, 0 } };
            const GsubIRAlternateSetSpec realSet[] = { { alternate, 1 } };

            const std::vector<uint8_t> emptySubtable =
                makeGsubIRAlternateSubtable(inputGlyph, emptySet, 1);

            const std::vector<uint8_t> realSubtable =
                makeGsubIRAlternateSubtable(inputGlyph, realSet, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRAlternateLookup({ emptySubtable, realSubtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 3 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRAlternate(lookup, ir, lookupId, compiled))
                return fail("case 3 compilation");

            uint16_t rawReplacement = 0;
            uint16_t irReplacement = 0;

            if (resolveOpenTypeGsubAlternateLookup(
                lookup, 10, rawReplacement) !=
                OpenTypeGsubResolveResult::Match ||
                resolveOpenTypeGsubIRAlternateLookup(
                    ir, *compiled, 10, irReplacement) !=
                OpenTypeShapingIRResult::Match ||
                rawReplacement != 222 ||
                irReplacement != 222)
            {
                return fail("case 3 empty-set fallthrough");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Extension Type 7 -> Type 3.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputGlyph[] = { 10 };
            const uint16_t alternates[] = { 300, 301, 302 };
            const GsubIRAlternateSetSpec sets[] = { { alternates, 3 } };

            const std::vector<uint8_t> subtable =
                makeGsubIRAlternateSubtable(inputGlyph, sets, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRAlternateExtensionLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 4 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGsubIREffectiveType(lookup, effectiveType) ||
                effectiveType != 3)
            {
                return fail("case 4 effective type");
            }

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRAlternate(lookup, ir, lookupId, compiled))
                return fail("case 4 compilation");

            if (!testGsubIRAlternateResolver(
                lookup, ir, *compiled, 10, "Type 7 -> Type 3 resolver"))
            {
                return false;
            }

            const uint32_t inputGlyphs[] = { 10, 11 };
            const uint32_t expected[] = { 300, 11 };

            const OpenTypeShapingBuffer input =
                makeGsubIRAlternateBuffer(inputGlyphs, 2);

            if (!testGsubIRAlternateExecution(
                lookup, ir, lookupId, input,
                expected, 2, "Type 7 -> Type 3 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - exact-position execution for contextual nesting.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputGlyph[] = { 20 };
            const uint16_t alternates[] = { 400, 401 };
            const GsubIRAlternateSetSpec sets[] = { { alternates, 2 } };

            const std::vector<uint8_t> subtable =
                makeGsubIRAlternateSubtable(inputGlyph, sets, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRAlternateLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupBytes.data(), lookupBytes.size()));

            if (!lookup)
                return fail("case 5 raw lookup");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRAlternate(lookup, ir, lookupId, compiled))
                return fail("case 5 compilation");

            const uint32_t inputGlyphs[] = { 10, 20, 30 };

            OpenTypeShapingBuffer rawBuffer =
                makeGsubIRAlternateBuffer(inputGlyphs, 3);

            OpenTypeShapingBuffer irBuffer = rawBuffer;

            OpenTypeGsubEditLog rawEdits;
            OpenTypeGsubEditLog irEdits;
            OpenTypeGsubApplyState state;

            const OpenTypeGsubApplyAtResult rawResult =
                applyOpenTypeGsubAlternateAt(
                    lookup, rawBuffer, 1, rawEdits);

            const OpenTypeGsubApplyAtResult irResult =
                applyOpenTypeGsubIRLookupAt(
                    ir, lookupId, irBuffer, 1, state, irEdits);

            if (rawResult != OpenTypeGsubApplyAtResult::Match ||
                irResult != OpenTypeGsubApplyAtResult::Match ||
                !gsubIRAlternateBuffersEqual(rawBuffer, irBuffer) ||
                rawEdits.size() != 1 ||
                irEdits.size() != 1 ||
                rawEdits[0].inputPositions.size() != 1 ||
                irEdits[0].inputPositions.size() != 1 ||
                rawEdits[0].inputPositions[0] != 1 ||
                irEdits[0].inputPositions[0] != 1 ||
                rawEdits[0].outputCount != 1 ||
                irEdits[0].outputCount != 1)
            {
                return fail("case 5 exact-position execution");
            }

            const uint32_t expected[] = { 10, 400, 30 };

            if (!gsubIRAlternateGlyphIdsEqual(irBuffer, expected, 3))
                return fail("case 5 exact-position result");

            ++passed;
        }


        std::printf(
            "GSUB IR Alternate: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Native Type 3:             PASS\n"
            "  AlternateSet retention:    PASS\n"
            "  Subtable order:            PASS\n"
            "  Empty-set fallthrough:     PASS\n"
            "  Type 7 -> Type 3:          PASS\n"
            "  Exact-position execution:  PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
