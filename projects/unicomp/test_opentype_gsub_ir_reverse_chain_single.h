// test_opentype_gsub_ir_reverse_chain_single.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
//#include "opentype_gsub_lookup_apply.h"
#include "opentype_gsub_reverse_chain_single_match.h"
#include "opentype_layout_view.h"
#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    static void appendGsubIRReverseU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGsubIRReverseU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubIRReverseU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGsubIRReverseU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGsubIRReverseCoverage1(std::vector<uint8_t>& data, const uint16_t* glyphs, size_t count)
    {
        appendGsubIRReverseU16(data, 1);
        appendGsubIRReverseU16(data, static_cast<uint16_t>(count));

        for (size_t i = 0; i < count; ++i)
            appendGsubIRReverseU16(data, glyphs[i]);
    }


    struct GsubIRReverseCoverageSpec
    {
        const uint16_t* glyphs{ nullptr };
        uint16_t glyphCount{ 0 };
    };


    static std::vector<uint8_t> makeGsubIRReverseSubtable(
        const uint16_t* inputs, const uint16_t* outputs, size_t inputCount,
        const GsubIRReverseCoverageSpec* backtrack, size_t backtrackCount,
        const GsubIRReverseCoverageSpec* lookahead, size_t lookaheadCount)
    {
        std::vector<uint8_t> data;

        appendGsubIRReverseU16(data, 1);

        const size_t inputCoveragePatch = data.size();
        appendGsubIRReverseU16(data, 0);

        appendGsubIRReverseU16(data, static_cast<uint16_t>(backtrackCount));

        const size_t backtrackPatches = data.size();
        for (size_t i = 0; i < backtrackCount; ++i)
            appendGsubIRReverseU16(data, 0);

        appendGsubIRReverseU16(data, static_cast<uint16_t>(lookaheadCount));

        const size_t lookaheadPatches = data.size();
        for (size_t i = 0; i < lookaheadCount; ++i)
            appendGsubIRReverseU16(data, 0);

        appendGsubIRReverseU16(data, static_cast<uint16_t>(inputCount));

        for (size_t i = 0; i < inputCount; ++i)
            appendGsubIRReverseU16(data, outputs[i]);

        const size_t inputCoverageOffset = data.size();
        patchGsubIRReverseU16(data, inputCoveragePatch, static_cast<uint16_t>(inputCoverageOffset));
        appendGsubIRReverseCoverage1(data, inputs, inputCount);

        for (size_t i = 0; i < backtrackCount; ++i)
        {
            const size_t offset = data.size();
            patchGsubIRReverseU16(data, backtrackPatches + i * 2, static_cast<uint16_t>(offset));
            appendGsubIRReverseCoverage1(data, backtrack[i].glyphs, backtrack[i].glyphCount);
        }

        for (size_t i = 0; i < lookaheadCount; ++i)
        {
            const size_t offset = data.size();
            patchGsubIRReverseU16(data, lookaheadPatches + i * 2, static_cast<uint16_t>(offset));
            appendGsubIRReverseCoverage1(data, lookahead[i].glyphs, lookahead[i].glyphCount);
        }

        return data;
    }


    static std::vector<uint8_t> makeGsubIRReverseLookup(
        const std::vector<std::vector<uint8_t>>& subtables, uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGsubIRReverseU16(data, 8);
        appendGsubIRReverseU16(data, lookupFlag);
        appendGsubIRReverseU16(data, static_cast<uint16_t>(subtables.size()));

        const size_t patches = data.size();

        for (size_t i = 0; i < subtables.size(); ++i)
            appendGsubIRReverseU16(data, 0);

        for (size_t i = 0; i < subtables.size(); ++i)
        {
            const size_t offset = data.size();
            patchGsubIRReverseU16(data, patches + i * 2, static_cast<uint16_t>(offset));
            data.insert(data.end(), subtables[i].begin(), subtables[i].end());
        }

        return data;
    }


    static std::vector<uint8_t> makeGsubIRReverseExtensionLookup(const std::vector<uint8_t>& subtable)
    {
        std::vector<uint8_t> data;

        appendGsubIRReverseU16(data, 7);
        appendGsubIRReverseU16(data, 0);
        appendGsubIRReverseU16(data, 1);
        appendGsubIRReverseU16(data, 8);

        const size_t extensionBase = data.size();

        appendGsubIRReverseU16(data, 1);
        appendGsubIRReverseU16(data, 8);

        const size_t extensionOffsetPatch = data.size();
        appendGsubIRReverseU32(data, 0);

        const size_t subtableOffset = data.size();

        patchGsubIRReverseU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(subtableOffset - extensionBase));

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    static void appendGsubIRReverseClassDef2(
        std::vector<uint8_t>& data, const uint16_t* starts,
        const uint16_t* ends, const uint16_t* classes, size_t count)
    {
        appendGsubIRReverseU16(data, 2);
        appendGsubIRReverseU16(data, static_cast<uint16_t>(count));

        for (size_t i = 0; i < count; ++i)
        {
            appendGsubIRReverseU16(data, starts[i]);
            appendGsubIRReverseU16(data, ends[i]);
            appendGsubIRReverseU16(data, classes[i]);
        }
    }


    static std::vector<uint8_t> makeGsubIRReverseGdef()
    {
        std::vector<uint8_t> data;

        appendGsubIRReverseU16(data, 1);
        appendGsubIRReverseU16(data, 0);

        appendGsubIRReverseU16(data, 12);
        appendGsubIRReverseU16(data, 0);
        appendGsubIRReverseU16(data, 0);
        appendGsubIRReverseU16(data, 0);

        const uint16_t starts[] = { 100 };
        const uint16_t ends[] = { 100 };
        const uint16_t classes[] = { 3 };

        appendGsubIRReverseClassDef2(data, starts, ends, classes, 1);

        return data;
    }


    static void appendGsubIRReverseGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubIRReverseBuffer(const uint32_t* glyphs, size_t count)
    {
        OpenTypeShapingBuffer buffer;

        for (size_t i = 0; i < count; ++i)
            appendGsubIRReverseGlyph(buffer, glyphs[i], static_cast<uint32_t>(i));

        return buffer;
    }


    static bool gsubIRReverseGlyphEqual(
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


    static bool gsubIRReverseBuffersEqual(
        const OpenTypeShapingBuffer& a, const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gsubIRReverseGlyphEqual(a[i], b[i]))
                return false;
        }

        return true;
    }


    static bool gsubIRReverseGlyphIdsEqual(
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


    static bool compileGsubIRReverse(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& lookupId,
        const OpenTypeShapingIRLookup*& compiled)
    {
        lookupId = kOpenTypeShapingIRInvalid;
        compiled = nullptr;

        if (!compileOpenTypeGsubReverseChainSingleLookup(lookup, gdef, ir, lookupId))
            return false;

        compiled = ir.lookup(lookupId);

        return compiled &&
            compiled->op == OpenTypeShapingIROp::GsubReverseChainSingle &&
            openTypeGsubIRReverseChainSingleLookupValid(ir, *compiled);
    }


    static bool testGsubIRReverseExecution(
        const OpenTypeLayoutLookupView& rawLookup, const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId,
        const OpenTypeShapingBuffer& input, const uint32_t* expected,
        size_t expectedCount, const char* caseName)
    {
        OpenTypeShapingBuffer rawBuffer = input;
        OpenTypeShapingBuffer irBuffer = input;

        const bool rawSuccess =
            applyOpenTypeGsubReverseChainSingleLookup(
                rawLookup, gdef, rawBuffer);

        const bool irSuccess =
            applyOpenTypeGsubIRLookup(
                ir, lookupId, irBuffer);

        if (!rawSuccess || !irSuccess)
        {
            std::printf(
                "GSUB IR Reverse Chain Single: FAIL\n"
                "  Case: %s\n"
                "  Raw success: %u\n"
                "  IR success:  %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess));

            return false;
        }

        if (!gsubIRReverseBuffersEqual(rawBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Reverse Chain Single: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR buffer mismatch\n",
                caseName);

            return false;
        }

        if (expected &&
            !gsubIRReverseGlyphIdsEqual(
                irBuffer, expected, expectedCount))
        {
            std::printf(
                "GSUB IR Reverse Chain Single: FAIL\n"
                "  Case: %s\n"
                "  Unexpected final glyph sequence\n",
                caseName);

            return false;
        }

        return true;
    }


    static bool testOpenTypeGsubIRReverseChainSingle()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Reverse Chain Single: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - native Type 8 with backtrack and lookahead.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputs[] = { 110 };
            const uint16_t backGlyph[] = { 5 };
            const uint16_t lookGlyph[] = { 40 };

            const GsubIRReverseCoverageSpec backtrack[] =
            {
                { backGlyph, 1 }
            };

            const GsubIRReverseCoverageSpec lookahead[] =
            {
                { lookGlyph, 1 }
            };

            const std::vector<uint8_t> subtable =
                makeGsubIRReverseSubtable(
                    inputs, outputs, 1,
                    backtrack, 1,
                    lookahead, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseLookup({ subtable });

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

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 40 };

            const uint32_t expected[] =
            { 5, 110, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 3);

            if (!testGsubIRReverseExecution(
                lookup, gdef,
                ir, lookupId, input,
                expected, 3,
                "native Type 8"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - nearest-first backtrack and lookahead ordering.
        //
        //     4 5 | 10 | 40 41
        //
        // Backtrack index 0 must see 5.
        // Lookahead index 0 must see 40.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputs[] = { 120 };

            const uint16_t back0[] = { 5 };
            const uint16_t back1[] = { 4 };

            const uint16_t look0[] = { 40 };
            const uint16_t look1[] = { 41 };

            const GsubIRReverseCoverageSpec backtrack[] =
            {
                { back0, 1 },
                { back1, 1 }
            };

            const GsubIRReverseCoverageSpec lookahead[] =
            {
                { look0, 1 },
                { look1, 1 }
            };

            const std::vector<uint8_t> subtable =
                makeGsubIRReverseSubtable(
                    inputs, outputs, 1,
                    backtrack, 2,
                    lookahead, 2);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseLookup({ subtable });

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

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 4, 5, 10, 40, 41 };

            const uint32_t expected[] =
            { 4, 5, 120, 40, 41 };

            const OpenTypeShapingBuffer input =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 5);

            if (!testGsubIRReverseExecution(
                lookup, gdef,
                ir, lookupId, input,
                expected, 5,
                "nearest-first ordering"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - LookupFlag filtering in both directions.
        //
        //     5 M | 10 | M 40
        //     0 1    2    3 4
        //
        // Glyph 100 is a GDEF mark ignored by IgnoreMarks.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputs[] = { 130 };

            const uint16_t backGlyph[] = { 5 };
            const uint16_t lookGlyph[] = { 40 };

            const GsubIRReverseCoverageSpec backtrack[] =
            {
                { backGlyph, 1 }
            };

            const GsubIRReverseCoverageSpec lookahead[] =
            {
                { lookGlyph, 1 }
            };

            const std::vector<uint8_t> subtable =
                makeGsubIRReverseSubtable(
                    inputs, outputs, 1,
                    backtrack, 1,
                    lookahead, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseLookup(
                    { subtable }, 0x0008u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            const std::vector<uint8_t> gdefBytes =
                makeGsubIRReverseGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(
                    gdefBytes.data(),
                    gdefBytes.size()));

            if (!lookup || !gdef)
                return fail("case 3 raw lookup/GDEF");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 100, 10, 100, 40 };

            const uint32_t expected[] =
            { 5, 100, 130, 100, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 5);

            if (!testGsubIRReverseExecution(
                lookup, gdef,
                ir, lookupId, input,
                expected, 5,
                "filtered traversal"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - reverse scan semantics.
        //
        // Rule:
        //
        //     10 followed by 20 -> 20
        //
        // Input:
        //
        //     10 10 20
        //
        // Reverse execution changes the second 10 first:
        //
        //     10 20 20
        //
        // The first 10 then sees the newly-created 20:
        //
        //     20 20 20
        //
        // A forward scan would incorrectly leave:
        //
        //     10 20 20
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputs[] = { 20 };

            const uint16_t lookGlyph[] = { 20 };

            const GsubIRReverseCoverageSpec lookahead[] =
            {
                { lookGlyph, 1 }
            };

            const std::vector<uint8_t> subtable =
                makeGsubIRReverseSubtable(
                    inputs, outputs, 1,
                    nullptr, 0,
                    lookahead, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseLookup({ subtable });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 4 raw lookup");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 10, 20 };

            const uint32_t expected[] =
            { 20, 20, 20 };

            const OpenTypeShapingBuffer input =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 3);

            if (!testGsubIRReverseExecution(
                lookup, gdef,
                ir, lookupId, input,
                expected, 3,
                "reverse scan"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - subtable order: first matching subtable wins.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputA[] = { 111 };
            const uint16_t outputB[] = { 222 };

            const std::vector<uint8_t> subtableA =
                makeGsubIRReverseSubtable(
                    inputs, outputA, 1,
                    nullptr, 0,
                    nullptr, 0);

            const std::vector<uint8_t> subtableB =
                makeGsubIRReverseSubtable(
                    inputs, outputB, 1,
                    nullptr, 0,
                    nullptr, 0);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseLookup(
                    { subtableA, subtableB });

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

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const uint32_t inputGlyphs[] = { 10 };
            const uint32_t expected[] = { 111 };

            const OpenTypeShapingBuffer input =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 1);

            if (!testGsubIRReverseExecution(
                lookup, gdef,
                ir, lookupId, input,
                expected, 1,
                "subtable order"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Extension Type 7 -> Type 8.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputs[] = { 140 };

            const std::vector<uint8_t> subtable =
                makeGsubIRReverseSubtable(
                    inputs, outputs, 1,
                    nullptr, 0,
                    nullptr, 0);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseExtensionLookup(
                    subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(
                    lookupBytes.data(),
                    lookupBytes.size()));

            if (!lookup)
                return fail("case 6 raw lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGsubIREffectiveType(
                lookup, effectiveType) ||
                effectiveType != 8)
            {
                return fail("case 6 effective type");
            }

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 11 };

            const uint32_t expected[] =
            { 140, 11 };

            const OpenTypeShapingBuffer input =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 2);

            if (!testGsubIRReverseExecution(
                lookup, gdef,
                ir, lookupId, input,
                expected, 2,
                "Type 7 -> Type 8"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - exact-position execution for contextual nesting.
        //
        // Exact-position execution performs exactly one Type 8 match. It does
        // not own the reverse whole-buffer scan.
        // ====================================================================

        {
            ++cases;

            const uint16_t inputs[] = { 10 };
            const uint16_t outputs[] = { 150 };

            const uint16_t backGlyph[] = { 5 };
            const uint16_t lookGlyph[] = { 40 };

            const GsubIRReverseCoverageSpec backtrack[] =
            {
                { backGlyph, 1 }
            };

            const GsubIRReverseCoverageSpec lookahead[] =
            {
                { lookGlyph, 1 }
            };

            const std::vector<uint8_t> subtable =
                makeGsubIRReverseSubtable(
                    inputs, outputs, 1,
                    backtrack, 1,
                    lookahead, 1);

            const std::vector<uint8_t> lookupBytes =
                makeGsubIRReverseLookup({ subtable });

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

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRReverse(
                lookup, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 40 };

            OpenTypeShapingBuffer rawBuffer =
                makeGsubIRReverseBuffer(
                    inputGlyphs, 3);

            OpenTypeShapingBuffer irBuffer =
                rawBuffer;

            OpenTypeGsubEditLog rawEdits;
            OpenTypeGsubEditLog irEdits;
            OpenTypeGsubApplyState state;

            const OpenTypeGsubApplyAtResult rawResult =
                applyOpenTypeGsubReverseChainSingleAt(
                    lookup, gdef,
                    rawBuffer, 1, rawEdits);

            const OpenTypeGsubApplyAtResult irResult =
                applyOpenTypeGsubIRLookupAt(
                    ir, lookupId,
                    irBuffer, 1,
                    state, irEdits);

            if (rawResult != OpenTypeGsubApplyAtResult::Match ||
                irResult != OpenTypeGsubApplyAtResult::Match ||
                !gsubIRReverseBuffersEqual(rawBuffer, irBuffer) ||
                rawEdits.size() != 1 ||
                irEdits.size() != 1 ||
                rawEdits[0].inputPositions.size() != 1 ||
                irEdits[0].inputPositions.size() != 1 ||
                rawEdits[0].inputPositions[0] != 1 ||
                irEdits[0].inputPositions[0] != 1 ||
                rawEdits[0].outputCount != 1 ||
                irEdits[0].outputCount != 1)
            {
                return fail(
                    "case 7 exact-position execution");
            }

            const uint32_t expected[] =
            { 5, 150, 40 };

            if (!gsubIRReverseGlyphIdsEqual(
                irBuffer, expected, 3))
            {
                return fail(
                    "case 7 exact-position result");
            }

            ++passed;
        }


        std::printf(
            "GSUB IR Reverse Chain Single: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Native Type 8:             PASS\n"
            "  Backtrack/lookahead:       PASS\n"
            "  Nearest-first ordering:    PASS\n"
            "  Filtered traversal:        PASS\n"
            "  Reverse scan:              PASS\n"
            "  Subtable order:            PASS\n"
            "  Type 7 -> Type 8:          PASS\n"
            "  Exact-position execution:  PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
