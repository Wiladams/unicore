// test_opentype_gsub_ir_chain_context.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gsub_chain_context_match.h"
//#include "opentype_gsub_lookup_apply.h"

#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    // ========================================================================
    // Binary helpers
    // ========================================================================

    static void appendGsubIRChainU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGsubIRChainU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubIRChainU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGsubIRChainU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGsubIRChainCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, glyphId);
    }


    static void appendGsubIRChainClassDef2(std::vector<uint8_t>& data,
        const uint16_t* starts, const uint16_t* ends,
        const uint16_t* classes, size_t count)
    {
        appendGsubIRChainU16(data, 2);
        appendGsubIRChainU16(data, static_cast<uint16_t>(count));

        for (size_t i = 0; i < count; ++i)
        {
            appendGsubIRChainU16(data, starts[i]);
            appendGsubIRChainU16(data, ends[i]);
            appendGsubIRChainU16(data, classes[i]);
        }
    }


    // ========================================================================
    // GDEF
    //
    // Glyph 100 is a mark.
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainGdef()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 12);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 0);

        const uint16_t starts[] = { 100 };
        const uint16_t ends[] = { 100 };
        const uint16_t classes[] = { 3 };

        appendGsubIRChainClassDef2(data, starts, ends, classes, 1);

        return data;
    }


    // ========================================================================
    // Basic nested lookup builders
    // ========================================================================

    static void appendGsubIRChainSingleLookup(std::vector<uint8_t>& data,
        size_t lookupOffsetPatch, uint16_t inputGlyph, uint16_t outputGlyph)
    {
        patchGsubIRChainU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 8);

        const size_t substBase = data.size();

        appendGsubIRChainU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, outputGlyph);

        const size_t coverageOffset = data.size();

        patchGsubIRChainU16(data, coveragePatch,
            static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubIRChainCoverage1(data, inputGlyph);
    }


    static void appendGsubIRChainMultipleLookup(std::vector<uint8_t>& data,
        size_t lookupOffsetPatch, uint16_t inputGlyph,
        uint16_t a, uint16_t b, uint16_t c)
    {
        patchGsubIRChainU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubIRChainU16(data, 2);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 8);

        const size_t substBase = data.size();

        appendGsubIRChainU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t sequenceOffset = data.size();

        patchGsubIRChainU16(data, sequencePatch,
            static_cast<uint16_t>(sequenceOffset - substBase));

        appendGsubIRChainU16(data, 3);
        appendGsubIRChainU16(data, a);
        appendGsubIRChainU16(data, b);
        appendGsubIRChainU16(data, c);

        const size_t coverageOffset = data.size();

        patchGsubIRChainU16(data, coveragePatch,
            static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubIRChainCoverage1(data, inputGlyph);
    }


    static void appendGsubIRChainType6Lookup(std::vector<uint8_t>& data,
        size_t lookupOffsetPatch, const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag = 0)
    {
        patchGsubIRChainU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubIRChainU16(data, 6);
        appendGsubIRChainU16(data, lookupFlag);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
    }


    static void appendGsubIRChainExtensionType6Lookup(std::vector<uint8_t>& data,
        size_t lookupOffsetPatch, const std::vector<uint8_t>& chainSubtable)
    {
        patchGsubIRChainU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubIRChainU16(data, 7);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 8);

        const size_t extensionBase = data.size();

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 6);

        const size_t extensionOffsetPatch = data.size();
        appendGsubIRChainU32(data, 0);

        const size_t chainOffset = data.size();

        patchGsubIRChainU32(data, extensionOffsetPatch,
            static_cast<uint32_t>(chainOffset - extensionBase));

        data.insert(data.end(), chainSubtable.begin(), chainSubtable.end());
    }


    // ========================================================================
    // Type 5 helper used for nested Type 6 -> Type 5 execution.
    //
    // Context:
    //
    //     20 30
    //
    // Action:
    //
    //     sequenceIndex 0 -> lookup 2
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainNestedType5Subtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 3);
        appendGsubIRChainU16(data, 2);
        appendGsubIRChainU16(data, 1);

        const size_t coverage0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 2);

        const size_t coverage0 = data.size();
        patchGsubIRChainU16(data, coverage0Patch, static_cast<uint16_t>(coverage0));
        appendGsubIRChainCoverage1(data, 20);

        const size_t coverage1 = data.size();
        patchGsubIRChainU16(data, coverage1Patch, static_cast<uint16_t>(coverage1));
        appendGsubIRChainCoverage1(data, 30);

        return data;
    }


    static void appendGsubIRChainType5Lookup(std::vector<uint8_t>& data,
        size_t lookupOffsetPatch, const std::vector<uint8_t>& subtable)
    {
        patchGsubIRChainU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubIRChainU16(data, 5);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
    }


    // ========================================================================
    // Type 6 Format 1
    //
    //     backtrack: 5
    //     input:     10 20 30
    //     lookahead: 40
    //
    //     sequenceIndex 1 -> Lookup 1
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainFormat1Subtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t ruleSetBase = data.size();

        patchGsubIRChainU16(data, ruleSetPatch, static_cast<uint16_t>(ruleSetBase));

        appendGsubIRChainU16(data, 1);

        const size_t rulePatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t ruleBase = data.size();

        patchGsubIRChainU16(data, rulePatch,
            static_cast<uint16_t>(ruleBase - ruleSetBase));

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 5);

        appendGsubIRChainU16(data, 3);
        appendGsubIRChainU16(data, 20);
        appendGsubIRChainU16(data, 30);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 40);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 1);

        const size_t coverageOffset = data.size();

        patchGsubIRChainU16(data, coveragePatch,
            static_cast<uint16_t>(coverageOffset));

        appendGsubIRChainCoverage1(data, 10);

        return data;
    }


    // ========================================================================
    // Type 6 Format 2
    //
    //     backtrack class: 5 -> 1
    //
    //     input classes:
    //         10 -> 1
    //         20 -> 2
    //         30 -> 3
    //
    //     lookahead class:
    //         40 -> 1
    //
    //     sequenceIndex 2 -> Lookup 1
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainFormat2Subtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t backtrackClassDefPatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t inputClassDefPatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookaheadClassDefPatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 2);

        appendGsubIRChainU16(data, 0);

        const size_t classSetPatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t classSetBase = data.size();

        patchGsubIRChainU16(data, classSetPatch,
            static_cast<uint16_t>(classSetBase));

        appendGsubIRChainU16(data, 1);

        const size_t classRulePatch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t classRuleBase = data.size();

        patchGsubIRChainU16(data, classRulePatch,
            static_cast<uint16_t>(classRuleBase - classSetBase));

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 1);

        appendGsubIRChainU16(data, 3);
        appendGsubIRChainU16(data, 2);
        appendGsubIRChainU16(data, 3);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 1);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 2);
        appendGsubIRChainU16(data, 1);

        const size_t coverageOffset = data.size();
        patchGsubIRChainU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));
        appendGsubIRChainCoverage1(data, 10);

        const size_t backtrackClassDefOffset = data.size();
        patchGsubIRChainU16(data, backtrackClassDefPatch,
            static_cast<uint16_t>(backtrackClassDefOffset));

        {
            const uint16_t starts[] = { 5 };
            const uint16_t ends[] = { 5 };
            const uint16_t classes[] = { 1 };

            appendGsubIRChainClassDef2(data, starts, ends, classes, 1);
        }

        const size_t inputClassDefOffset = data.size();
        patchGsubIRChainU16(data, inputClassDefPatch,
            static_cast<uint16_t>(inputClassDefOffset));

        {
            const uint16_t starts[] = { 10, 20, 30 };
            const uint16_t ends[] = { 10, 20, 30 };
            const uint16_t classes[] = { 1, 2, 3 };

            appendGsubIRChainClassDef2(data, starts, ends, classes, 3);
        }

        const size_t lookaheadClassDefOffset = data.size();
        patchGsubIRChainU16(data, lookaheadClassDefPatch,
            static_cast<uint16_t>(lookaheadClassDefOffset));

        {
            const uint16_t starts[] = { 40 };
            const uint16_t ends[] = { 40 };
            const uint16_t classes[] = { 1 };

            appendGsubIRChainClassDef2(data, starts, ends, classes, 1);
        }

        return data;
    }


    // ========================================================================
    // Type 6 Format 3
    //
    //     backtrack: 5
    //     input:     10 20 30
    //     lookahead: 40
    //
    // Actions:
    //
    //     sequenceIndex 1 -> Lookup 1
    //     sequenceIndex 3 -> Lookup 2
    //
    // Lookup 1:
    //
    //     20 -> 200 201 202
    //
    // After the first edit the mutable input sequence is:
    //
    //     10 200 201 202 30
    //
    // Therefore later sequenceIndex 3 addresses glyph 202.
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainFormat3Subtable(bool twoActions = true)
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 3);

        appendGsubIRChainU16(data, 1);

        const size_t backtrackCoveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 3);

        const size_t inputCoverage0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t inputCoverage1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t inputCoverage2Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);

        const size_t lookaheadCoveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, twoActions ? 2 : 1);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 1);

        if (twoActions)
        {
            appendGsubIRChainU16(data, 3);
            appendGsubIRChainU16(data, 2);
        }

        const size_t backtrackCoverage = data.size();
        patchGsubIRChainU16(data, backtrackCoveragePatch,
            static_cast<uint16_t>(backtrackCoverage));
        appendGsubIRChainCoverage1(data, 5);

        const size_t inputCoverage0 = data.size();
        patchGsubIRChainU16(data, inputCoverage0Patch,
            static_cast<uint16_t>(inputCoverage0));
        appendGsubIRChainCoverage1(data, 10);

        const size_t inputCoverage1 = data.size();
        patchGsubIRChainU16(data, inputCoverage1Patch,
            static_cast<uint16_t>(inputCoverage1));
        appendGsubIRChainCoverage1(data, 20);

        const size_t inputCoverage2 = data.size();
        patchGsubIRChainU16(data, inputCoverage2Patch,
            static_cast<uint16_t>(inputCoverage2));
        appendGsubIRChainCoverage1(data, 30);

        const size_t lookaheadCoverage = data.size();
        patchGsubIRChainU16(data, lookaheadCoveragePatch,
            static_cast<uint16_t>(lookaheadCoverage));
        appendGsubIRChainCoverage1(data, 40);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRChainFormat3NoActionSubtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 3);

        appendGsubIRChainU16(data, 1);

        const size_t backtrackCoveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 3);

        const size_t inputCoverage0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t inputCoverage1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t inputCoverage2Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);

        const size_t lookaheadCoveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 0);

        const size_t backtrackCoverage = data.size();
        patchGsubIRChainU16(data, backtrackCoveragePatch,
            static_cast<uint16_t>(backtrackCoverage));
        appendGsubIRChainCoverage1(data, 5);

        const size_t inputCoverage0 = data.size();
        patchGsubIRChainU16(data, inputCoverage0Patch,
            static_cast<uint16_t>(inputCoverage0));
        appendGsubIRChainCoverage1(data, 10);

        const size_t inputCoverage1 = data.size();
        patchGsubIRChainU16(data, inputCoverage1Patch,
            static_cast<uint16_t>(inputCoverage1));
        appendGsubIRChainCoverage1(data, 20);

        const size_t inputCoverage2 = data.size();
        patchGsubIRChainU16(data, inputCoverage2Patch,
            static_cast<uint16_t>(inputCoverage2));
        appendGsubIRChainCoverage1(data, 30);

        const size_t lookaheadCoverage = data.size();
        patchGsubIRChainU16(data, lookaheadCoveragePatch,
            static_cast<uint16_t>(lookaheadCoverage));
        appendGsubIRChainCoverage1(data, 40);

        return data;
    }


    // ========================================================================
    // LookupList builders
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainFormat1LookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 2);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainType6Lookup(
            data, lookup0Patch,
            makeGsubIRChainFormat1Subtable());

        appendGsubIRChainSingleLookup(
            data, lookup1Patch, 20, 220);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRChainFormat2LookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 2);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainType6Lookup(
            data, lookup0Patch,
            makeGsubIRChainFormat2Subtable());

        appendGsubIRChainSingleLookup(
            data, lookup1Patch, 30, 330);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRChainFormat3LookupList(
        uint16_t finalInputGlyph = 202,
        bool extension = false)
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubIRChainU16(data, 0);

        if (extension)
        {
            appendGsubIRChainExtensionType6Lookup(
                data, lookup0Patch,
                makeGsubIRChainFormat3Subtable());
        }
        else
        {
            appendGsubIRChainType6Lookup(
                data, lookup0Patch,
                makeGsubIRChainFormat3Subtable());
        }

        appendGsubIRChainMultipleLookup(
            data, lookup1Patch,
            20, 200, 201, 202);

        appendGsubIRChainSingleLookup(
            data, lookup2Patch,
            finalInputGlyph, 250);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRChainFilteredLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 2);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainType6Lookup(
            data, lookup0Patch,
            makeGsubIRChainFormat3Subtable(false),
            0x0008u);

        appendGsubIRChainSingleLookup(
            data, lookup1Patch, 20, 220);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRChainNoActionLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 1);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainType6Lookup(
            data, lookup0Patch,
            makeGsubIRChainFormat3NoActionSubtable());

        return data;
    }


    // ========================================================================
    // Type 6 -> Type 5 nested lookup.
    //
    // Lookup 0:
    //
    //     Type 6:
    //       5 | 10 20 30 | 40
    //       sequenceIndex 1 -> Lookup 1
    //
    // Lookup 1:
    //
    //     Type 5:
    //       20 30
    //       sequenceIndex 0 -> Lookup 2
    //
    // Lookup 2:
    //
    //     20 -> 220
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainNestedType5LookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRChainU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainType6Lookup(
            data, lookup0Patch,
            makeGsubIRChainFormat3Subtable(false));

        appendGsubIRChainType5Lookup(
            data, lookup1Patch,
            makeGsubIRChainNestedType5Subtable());

        appendGsubIRChainSingleLookup(
            data, lookup2Patch, 20, 220);

        return data;
    }


    // ========================================================================
    // Recursive Type 6.
    //
    // Input glyph 10 invokes the same lookup at sequenceIndex 0.
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRChainRecursiveLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRChainU16(data, 1);

        const size_t lookup0Patch = data.size();
        appendGsubIRChainU16(data, 0);

        patchGsubIRChainU16(data, lookup0Patch,
            static_cast<uint16_t>(data.size()));

        appendGsubIRChainU16(data, 6);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 8);

        const size_t substBase = data.size();

        appendGsubIRChainU16(data, 3);

        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);

        const size_t inputCoveragePatch = data.size();
        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 0);

        appendGsubIRChainU16(data, 1);
        appendGsubIRChainU16(data, 0);
        appendGsubIRChainU16(data, 0);

        const size_t coverageOffset = data.size();

        patchGsubIRChainU16(data, inputCoveragePatch,
            static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubIRChainCoverage1(data, 10);

        return data;
    }


    // ========================================================================
    // Shaping-buffer helpers
    // ========================================================================

    static void appendGsubIRChainGlyph(
        OpenTypeShapingBuffer& buffer,
        uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};

        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubIRChainBuffer(
        const uint32_t* glyphs, size_t count)
    {
        OpenTypeShapingBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGsubIRChainGlyph(
                buffer, glyphs[i],
                static_cast<uint32_t>(i));
        }

        return buffer;
    }


    static bool gsubIRChainGlyphEqual(
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


    static bool gsubIRChainBuffersEqual(
        const OpenTypeShapingBuffer& a,
        const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gsubIRChainGlyphEqual(a[i], b[i]))
                return false;
        }

        return true;
    }


    static bool gsubIRChainGlyphIdsEqual(
        const OpenTypeShapingBuffer& buffer,
        const uint32_t* expected, size_t count) noexcept
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


    // ========================================================================
    // Raw Type 6 resolver
    // ========================================================================

    static OpenTypeGsubChainContextMatchResult resolveGsubIRChainRawLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGsubChainContextMatch& match) noexcept
    {
        match.clear();

        if (!lookup ||
            !openTypeGsubHasEffectiveLookupType(lookup, 6))
        {
            return OpenTypeGsubChainContextMatchResult::Invalid;
        }

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubChainContextMatchResult::Invalid;

        for (uint16_t subtableIndex = 0;
            subtableIndex < lookup.subtableCount();
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGsubEffectiveSubtable(
                    lookup, 6, subtableIndex);

            if (!data)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const OpenTypeGsubChainContextSubstView subst(data);

            if (!subst)
                return OpenTypeGsubChainContextMatchResult::Invalid;

            const OpenTypeGsubChainContextMatchResult result =
                matchOpenTypeGsubChainContextSubst(
                    subst, filter, buffer,
                    glyphIndex, match);

            if (result == OpenTypeGsubChainContextMatchResult::Invalid ||
                result == OpenTypeGsubChainContextMatchResult::Match)
            {
                return result;
            }
        }

        return OpenTypeGsubChainContextMatchResult::NoMatch;
    }


    static bool gsubIRChainResultMatches(
        OpenTypeGsubChainContextMatchResult rawResult,
        OpenTypeShapingIRResult irResult) noexcept
    {
        switch (rawResult)
        {
        case OpenTypeGsubChainContextMatchResult::Invalid:
            return irResult == OpenTypeShapingIRResult::Invalid;

        case OpenTypeGsubChainContextMatchResult::NoMatch:
            return irResult == OpenTypeShapingIRResult::NoMatch;

        case OpenTypeGsubChainContextMatchResult::Match:
            return irResult == OpenTypeShapingIRResult::Match;

        default:
            return false;
        }
    }


    static bool gsubIRChainOpMatchesType(
        OpenTypeShapingIROp op, uint16_t effectiveType) noexcept
    {
        switch (effectiveType)
        {
        case 1:
            return op == OpenTypeShapingIROp::GsubSingle;

        case 2:
            return op == OpenTypeShapingIROp::GsubMultiple;

        case 4:
            return op == OpenTypeShapingIROp::GsubLigature;

        case 5:
            return op == OpenTypeShapingIROp::GsubContext;

        case 6:
            return op == OpenTypeShapingIROp::GsubChainContext;

        default:
            return false;
        }
    }


    // ========================================================================
    // Compiler helper
    // ========================================================================

    static bool compileGsubIRChainLookup(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& compiledLookupId,
        const OpenTypeShapingIRLookup*& compiledLookup)
    {
        compiledLookupId = kOpenTypeShapingIRInvalid;
        compiledLookup = nullptr;

        if (!compileOpenTypeGsubChainContextLookup(
            lookups, lookupIndex, gdef,
            ir, compiledLookupId))
        {
            return false;
        }

        compiledLookup = ir.lookup(compiledLookupId);

        return compiledLookup &&
            compiledLookup->op ==
            OpenTypeShapingIROp::GsubChainContext &&
            openTypeGsubIRChainContextLookupValid(
                ir, *compiledLookup);
    }


    // ========================================================================
    // Differential matcher
    // ========================================================================

    static bool testGsubIRChainMatcher(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiledLookup,
        const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex,
        const size_t* expectedPositions,
        size_t expectedPositionCount,
        const char* caseName)
    {
        const OpenTypeLayoutLookupView rawLookup =
            lookups.lookup(lookupIndex);

        if (!rawLookup)
            return false;

        OpenTypeGsubChainContextMatch rawMatch;
        OpenTypeGsubIRChainContextMatch irMatch;

        const OpenTypeGsubChainContextMatchResult rawResult =
            resolveGsubIRChainRawLookup(
                rawLookup, gdef, buffer,
                glyphIndex, rawMatch);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRChainContextLookup(
                ir, compiledLookup, buffer,
                glyphIndex, irMatch);

        if (!gsubIRChainResultMatches(rawResult, irResult))
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Matcher result mismatch\n"
                "  Raw: %u\n"
                "  IR:  %u\n",
                caseName,
                static_cast<unsigned>(rawResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (rawResult != OpenTypeGsubChainContextMatchResult::Match)
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Expected matcher success\n",
                caseName);

            return false;
        }

        if (rawMatch.inputPositions != irMatch.inputPositions)
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Input positions differ\n",
                caseName);

            return false;
        }

        if (expectedPositions)
        {
            if (irMatch.inputPositions.size() != expectedPositionCount)
                return false;

            for (size_t i = 0; i < expectedPositionCount; ++i)
            {
                if (irMatch.inputPositions[i] != expectedPositions[i])
                    return false;
            }
        }

        if (rawMatch.lookups.size() != irMatch.lookupCount)
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Action count mismatch\n"
                "  Raw: %zu\n"
                "  IR:  %u\n",
                caseName,
                rawMatch.lookups.size(),
                static_cast<unsigned>(irMatch.lookupCount));

            return false;
        }

        for (size_t i = 0; i < rawMatch.lookups.size(); ++i)
        {
            const OpenTypeSequenceLookup& rawAction =
                rawMatch.lookups[i];

            const OpenTypeShapingIRSequenceLookup& irAction =
                ir.gsubChainContextLookups[
                    irMatch.lookupOffset + i];

            if (rawAction.sequenceIndex != irAction.sequenceIndex)
                return false;

            if (rawAction.lookupListIndex >= lookups.size())
                return false;

            const OpenTypeLayoutLookupView nestedRaw =
                lookups.lookup(rawAction.lookupListIndex);

            uint16_t effectiveType = 0;

            if (!nestedRaw ||
                !openTypeGsubIREffectiveType(
                    nestedRaw, effectiveType))
            {
                return false;
            }

            const OpenTypeShapingIRLookup* nestedIR =
                ir.lookup(irAction.lookup);

            if (!nestedIR ||
                !gsubIRChainOpMatchesType(
                    nestedIR->op, effectiveType))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // Matcher NoMatch differential
    // ========================================================================

    static bool testGsubIRChainMatcherNoMatch(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiledLookup,
        const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex,
        const char* caseName)
    {
        const OpenTypeLayoutLookupView rawLookup =
            lookups.lookup(lookupIndex);

        if (!rawLookup)
            return false;

        OpenTypeGsubChainContextMatch rawMatch;
        OpenTypeGsubIRChainContextMatch irMatch;

        const OpenTypeGsubChainContextMatchResult rawResult =
            resolveGsubIRChainRawLookup(
                rawLookup, gdef, buffer,
                glyphIndex, rawMatch);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRChainContextLookup(
                ir, compiledLookup, buffer,
                glyphIndex, irMatch);

        if (rawResult != OpenTypeGsubChainContextMatchResult::NoMatch ||
            irResult != OpenTypeShapingIRResult::NoMatch)
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Expected NoMatch\n",
                caseName);

            return false;
        }

        return rawMatch.inputPositions.empty() &&
            irMatch.inputPositions.empty();
    }


    // ========================================================================
    // Whole-lookup differential
    // ========================================================================

    static bool testGsubIRChainExecution(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId compiledLookupId,
        const OpenTypeShapingBuffer& input,
        const uint32_t* expected,
        size_t expectedCount,
        bool expectedSuccess,
        const char* caseName)
    {
        OpenTypeShapingBuffer rawBuffer = input;
        OpenTypeShapingBuffer irBuffer = input;

        const bool rawSuccess =
            applyOpenTypeGsubLookup(
                lookups, lookupIndex,
                gdef, rawBuffer);

        const bool irSuccess =
            applyOpenTypeGsubIRLookup(
                ir, compiledLookupId,
                irBuffer);

        if (rawSuccess != irSuccess ||
            rawSuccess != expectedSuccess)
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Execution status mismatch\n"
                "  Raw:      %u\n"
                "  IR:       %u\n"
                "  Expected: %u\n",
                caseName,
                static_cast<unsigned>(rawSuccess),
                static_cast<unsigned>(irSuccess),
                static_cast<unsigned>(expectedSuccess));

            return false;
        }

        if (!gsubIRChainBuffersEqual(rawBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Raw/IR buffer mismatch\n"
                "  Raw glyphs: %zu\n"
                "  IR glyphs:  %zu\n",
                caseName,
                rawBuffer.size(),
                irBuffer.size());

            return false;
        }

        if (expected &&
            !gsubIRChainGlyphIdsEqual(
                irBuffer, expected, expectedCount))
        {
            std::printf(
                "GSUB IR Chain Context: FAIL\n"
                "  Case: %s\n"
                "  Unexpected final glyph sequence\n",
                caseName);

            return false;
        }

        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRChainContext
    // ========================================================================

    static bool testOpenTypeGsubIRChainContext()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Chain Context: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Format 1
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat1LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 2)
                return fail("case 1 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 20, 30, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 5);

            const size_t expectedPositions[] =
            { 1, 2, 3 };

            if (!testGsubIRChainMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 1,
                expectedPositions, 3,
                "Format 1 matcher"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 5, 10, 220, 30, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "Format 1 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 2
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat2LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 2)
                return fail("case 2 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 20, 30, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 5);

            const size_t expectedPositions[] =
            { 1, 2, 3 };

            if (!testGsubIRChainMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 1,
                expectedPositions, 3,
                "Format 2 matcher"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 5, 10, 20, 330, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "Format 2 execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Format 3 dynamic sequenceIndex
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat3LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 3 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 20, 30, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 5);

            const size_t expectedPositions[] =
            { 1, 2, 3 };

            if (!testGsubIRChainMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 1,
                expectedPositions, 3,
                "Format 3 matcher"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 5, 10, 200, 201, 250, 30, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 7, true,
                "dynamic sequenceIndex"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Bad backtrack and bad lookahead
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat3LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            {
                const uint32_t glyphs[] =
                { 6, 10, 20, 30, 40 };

                const OpenTypeShapingBuffer input =
                    makeGsubIRChainBuffer(glyphs, 5);

                if (!testGsubIRChainMatcherNoMatch(
                    lookups, 0, gdef,
                    ir, *compiled, input, 1,
                    "bad backtrack"))
                {
                    return false;
                }
            }

            {
                const uint32_t glyphs[] =
                { 5, 10, 20, 30, 41 };

                const OpenTypeShapingBuffer input =
                    makeGsubIRChainBuffer(glyphs, 5);

                if (!testGsubIRChainMatcherNoMatch(
                    lookups, 0, gdef,
                    ir, *compiled, input, 1,
                    "bad lookahead"))
                {
                    return false;
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Nested NoMatch
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat3LookupList(999);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 20, 30, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 5);

            const uint32_t expected[] =
            { 5, 10, 200, 201, 202, 30, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 7, true,
                "nested NoMatch"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Resume after input, not lookahead
        //
        // Two contexts overlap at the first context's lookahead:
        //
        //     5 | 10 20 30 | 40
        //
        //                  30 | 40 ...
        //
        // The important invariant here is simply that Type 6 resumes after
        // the mapped input boundary rather than skipping through lookahead.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat3LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t inputGlyphs[] =
            {
                5, 10, 20, 30, 40,
                5, 10, 20, 30, 40
            };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 10);

            const uint32_t expected[] =
            {
                5, 10, 200, 201, 250, 30, 40,
                5, 10, 200, 201, 250, 30, 40
            };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 14, true,
                "resume boundary"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Extension Type 7 -> Type 6
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFormat3LookupList(
                    202, true);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 20, 30, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 5);

            const uint32_t expected[] =
            { 5, 10, 200, 201, 250, 30, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 7, true,
                "Type 7 -> Type 6"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - LookupFlag traversal through every chain region
        //
        // Physical buffer:
        //
        //     5 M 10 M 20 M 30 M 40
        //     0 1  2 3  4 5  6 7  8
        //
        // M is a GDEF mark ignored by IgnoreMarks.
        //
        // Input positions must be:
        //
        //     { 2, 4, 6 }
        //
        // This simultaneously requires filtered traversal through:
        //
        //     backtrack
        //     later input
        //     lookahead
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainFilteredLookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const std::vector<uint8_t> gdefBytes =
                makeGsubIRChainGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(
                    gdefBytes.data(),
                    gdefBytes.size()));

            if (!gdef)
                return fail("case 8 GDEF");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 100, 10, 100, 20, 100, 30, 100, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 9);

            const size_t expectedPositions[] =
            { 2, 4, 6 };

            if (!testGsubIRChainMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 2,
                expectedPositions, 3,
                "filtered traversal"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 5, 100, 10, 100, 220, 100, 30, 100, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 9, true,
                "filtered execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Type 6 -> Type 5
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainNestedType5LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 9 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 9 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 5, 10, 20, 30, 40 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(inputGlyphs, 5);

            const uint32_t expected[] =
            { 5, 10, 220, 30, 40 };

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "Type 6 -> Type 5"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 10 - Zero-action match
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainNoActionLookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 10 compilation");
            }

            const uint32_t inputGlyphs[] =
            {
                5, 10, 20, 30, 40,
                5, 10, 20, 30, 40
            };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(
                    inputGlyphs, 10);

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                inputGlyphs, 10, true,
                "zero-action match"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 11 - Recursive Type 6 protection and transactional rollback
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRChainRecursiveLookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 1)
                return fail("case 11 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRChainLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 11 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10 };

            const OpenTypeShapingBuffer input =
                makeGsubIRChainBuffer(
                    inputGlyphs, 1);

            if (!testGsubIRChainExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                inputGlyphs, 1, false,
                "recursive rollback"))
            {
                return false;
            }

            ++passed;
        }


        std::printf(
            "GSUB IR Chain Context: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1:                 PASS\n"
            "  Format 2:                 PASS\n"
            "  Format 3:                 PASS\n"
            "  Backtrack/lookahead:      PASS\n"
            "  Dynamic sequenceIndex:    PASS\n"
            "  Nested NoMatch:           PASS\n"
            "  Resume boundary:          PASS\n"
            "  Type 7 -> Type 6:         PASS\n"
            "  Filtered traversal:       PASS\n"
            "  Type 6 -> Type 5:         PASS\n"
            "  Zero-action match:        PASS\n"
            "  Recursive rollback:       PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs