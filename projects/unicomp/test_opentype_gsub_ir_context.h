// test_opentype_gsub_ir_context.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gsub_context_match.h"
#include "opentype_gsub_lookup_apply.h"

#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    // ========================================================================
    // Binary helpers
    // ========================================================================

    static void appendGsubIRContextU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGsubIRContextU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubIRContextU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGsubIRContextU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGsubIRContextCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, glyphId);
    }


    static void appendGsubIRContextClassDef2(
        std::vector<uint8_t>& data,
        const uint16_t* starts,
        const uint16_t* ends,
        const uint16_t* classes,
        size_t count)
    {
        appendGsubIRContextU16(data, 2);
        appendGsubIRContextU16(data, static_cast<uint16_t>(count));

        for (size_t i = 0; i < count; ++i)
        {
            appendGsubIRContextU16(data, starts[i]);
            appendGsubIRContextU16(data, ends[i]);
            appendGsubIRContextU16(data, classes[i]);
        }
    }


    // ========================================================================
    // GDEF
    //
    // Glyph 100 is a mark. Everything else is class 0.
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRContextGdef()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 12);
        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 0);

        const uint16_t starts[] = { 100 };
        const uint16_t ends[] = { 100 };
        const uint16_t classes[] = { 3 };

        appendGsubIRContextClassDef2(data, starts, ends, classes, 1);

        return data;
    }


    // ========================================================================
    // Lookup builders
    // ========================================================================

    static void appendGsubIRContextSingleLookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        uint16_t inputGlyph, uint16_t outputGlyph)
    {
        patchGsubIRContextU16(
            data, lookupOffsetPatch,
            static_cast<uint16_t>(data.size()));

        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 8);

        const size_t substBase = data.size();

        appendGsubIRContextU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, outputGlyph);

        const size_t coverageOffset = data.size();

        patchGsubIRContextU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubIRContextCoverage1(data, inputGlyph);
    }


    static void appendGsubIRContextMultipleLookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        uint16_t inputGlyph, uint16_t a, uint16_t b, uint16_t c)
    {
        patchGsubIRContextU16(
            data, lookupOffsetPatch,
            static_cast<uint16_t>(data.size()));

        appendGsubIRContextU16(data, 2);
        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 8);

        const size_t substBase = data.size();

        appendGsubIRContextU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t sequenceOffset = data.size();

        patchGsubIRContextU16(
            data, sequencePatch,
            static_cast<uint16_t>(sequenceOffset - substBase));

        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, a);
        appendGsubIRContextU16(data, b);
        appendGsubIRContextU16(data, c);

        const size_t coverageOffset = data.size();

        patchGsubIRContextU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubIRContextCoverage1(data, inputGlyph);
    }


    static void appendGsubIRContextType5Lookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag = 0)
    {
        patchGsubIRContextU16(
            data, lookupOffsetPatch,
            static_cast<uint16_t>(data.size()));

        appendGsubIRContextU16(data, 5);
        appendGsubIRContextU16(data, lookupFlag);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
    }


    static void appendGsubIRContextExtensionType5Lookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        const std::vector<uint8_t>& contextSubtable)
    {
        patchGsubIRContextU16(
            data, lookupOffsetPatch,
            static_cast<uint16_t>(data.size()));

        appendGsubIRContextU16(data, 7);
        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 8);

        const size_t extensionBase = data.size();

        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 5);

        const size_t extensionOffsetPatch = data.size();
        appendGsubIRContextU32(data, 0);

        const size_t contextOffset = data.size();

        patchGsubIRContextU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(contextOffset - extensionBase));

        data.insert(
            data.end(),
            contextSubtable.begin(),
            contextSubtable.end());
    }


    // ========================================================================
    // Type 5 Format 1
    //
    // Context:
    //
    //     10 20 30
    //
    // Action:
    //
    //     sequenceIndex 1 -> Lookup 1
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRContextFormat1Subtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t ruleSetBase = data.size();

        patchGsubIRContextU16(
            data, ruleSetPatch,
            static_cast<uint16_t>(ruleSetBase));

        appendGsubIRContextU16(data, 1);

        const size_t rulePatch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t ruleBase = data.size();

        patchGsubIRContextU16(
            data, rulePatch,
            static_cast<uint16_t>(ruleBase - ruleSetBase));

        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, 1);

        appendGsubIRContextU16(data, 20);
        appendGsubIRContextU16(data, 30);

        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 1);

        const size_t coverageOffset = data.size();

        patchGsubIRContextU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset));

        appendGsubIRContextCoverage1(data, 10);

        return data;
    }


    // ========================================================================
    // Type 5 Format 2
    //
    // Coverage:
    //
    //     10
    //
    // Classes:
    //
    //     10 -> 1
    //     20 -> 2
    //     30 -> 3
    //
    // Rule:
    //
    //     class 1, class 2, class 3
    //
    // Action:
    //
    //     sequenceIndex 2 -> Lookup 1
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRContextFormat2Subtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t classDefPatch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 2);

        appendGsubIRContextU16(data, 0);

        const size_t classSetPatch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t classSetBase = data.size();

        patchGsubIRContextU16(
            data, classSetPatch,
            static_cast<uint16_t>(classSetBase));

        appendGsubIRContextU16(data, 1);

        const size_t classRulePatch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t classRuleBase = data.size();

        patchGsubIRContextU16(
            data, classRulePatch,
            static_cast<uint16_t>(classRuleBase - classSetBase));

        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, 1);

        appendGsubIRContextU16(data, 2);
        appendGsubIRContextU16(data, 3);

        appendGsubIRContextU16(data, 2);
        appendGsubIRContextU16(data, 1);

        const size_t coverageOffset = data.size();

        patchGsubIRContextU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset));

        appendGsubIRContextCoverage1(data, 10);

        const size_t classDefOffset = data.size();

        patchGsubIRContextU16(
            data, classDefPatch,
            static_cast<uint16_t>(classDefOffset));

        const uint16_t starts[] = { 10, 20, 30 };
        const uint16_t ends[] = { 10, 20, 30 };
        const uint16_t classes[] = { 1, 2, 3 };

        appendGsubIRContextClassDef2(
            data, starts, ends, classes, 3);

        return data;
    }


    // ========================================================================
    // Type 5 Format 3
    //
    // Context:
    //
    //     10 20 30
    //
    // Actions:
    //
    //     sequenceIndex 1 -> Lookup 1
    //     sequenceIndex 3 -> Lookup 2
    //
    // Lookup 1 expands:
    //
    //     20 -> 200 201 202
    //
    // The second action therefore targets current sequence position 3, glyph
    // 202, rather than original input position 3.
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRContextFormat3Subtable(
        bool twoActions = true)
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, twoActions ? 2 : 1);

        const size_t coverage0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 1);

        if (twoActions)
        {
            appendGsubIRContextU16(data, 3);
            appendGsubIRContextU16(data, 2);
        }

        const size_t coverage0 = data.size();

        patchGsubIRContextU16(
            data, coverage0Patch,
            static_cast<uint16_t>(coverage0));

        appendGsubIRContextCoverage1(data, 10);

        const size_t coverage1 = data.size();

        patchGsubIRContextU16(
            data, coverage1Patch,
            static_cast<uint16_t>(coverage1));

        appendGsubIRContextCoverage1(data, 20);

        const size_t coverage2 = data.size();

        patchGsubIRContextU16(
            data, coverage2Patch,
            static_cast<uint16_t>(coverage2));

        appendGsubIRContextCoverage1(data, 30);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRContextFormat3NoActionSubtable()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, 0);

        const size_t coverage0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t coverage0 = data.size();

        patchGsubIRContextU16(
            data, coverage0Patch,
            static_cast<uint16_t>(coverage0));

        appendGsubIRContextCoverage1(data, 10);

        const size_t coverage1 = data.size();

        patchGsubIRContextU16(
            data, coverage1Patch,
            static_cast<uint16_t>(coverage1));

        appendGsubIRContextCoverage1(data, 20);

        const size_t coverage2 = data.size();

        patchGsubIRContextU16(
            data, coverage2Patch,
            static_cast<uint16_t>(coverage2));

        appendGsubIRContextCoverage1(data, 30);

        return data;
    }


    // ========================================================================
    // LookupList builders
    // ========================================================================

    static std::vector<uint8_t> makeGsubIRContextFormat1LookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 2);

        const size_t lookup0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextType5Lookup(
            data, lookup0Patch,
            makeGsubIRContextFormat1Subtable());

        appendGsubIRContextSingleLookup(
            data, lookup1Patch, 20, 220);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRContextFormat2LookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 2);

        const size_t lookup0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextType5Lookup(
            data, lookup0Patch,
            makeGsubIRContextFormat2Subtable());

        appendGsubIRContextSingleLookup(
            data, lookup1Patch, 30, 330);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRContextFormat3LookupList(
        uint16_t finalInputGlyph = 202,
        bool extension = false)
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubIRContextU16(data, 0);

        if (extension)
        {
            appendGsubIRContextExtensionType5Lookup(
                data, lookup0Patch,
                makeGsubIRContextFormat3Subtable());
        }
        else
        {
            appendGsubIRContextType5Lookup(
                data, lookup0Patch,
                makeGsubIRContextFormat3Subtable());
        }

        appendGsubIRContextMultipleLookup(
            data, lookup1Patch, 20, 200, 201, 202);

        appendGsubIRContextSingleLookup(
            data, lookup2Patch, finalInputGlyph, 250);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRContextFilteredLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 2);

        const size_t lookup0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextType5Lookup(
            data, lookup0Patch,
            makeGsubIRContextFormat3Subtable(false),
            0x0008u);

        appendGsubIRContextSingleLookup(
            data, lookup1Patch, 20, 220);

        return data;
    }


    static std::vector<uint8_t> makeGsubIRContextNoActionLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 1);

        const size_t lookup0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextType5Lookup(
            data, lookup0Patch,
            makeGsubIRContextFormat3NoActionSubtable());

        return data;
    }


    static std::vector<uint8_t> makeGsubIRContextRecursiveLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubIRContextU16(data, 1);

        const size_t lookup0Patch = data.size();
        appendGsubIRContextU16(data, 0);

        patchGsubIRContextU16(
            data, lookup0Patch,
            static_cast<uint16_t>(data.size()));

        appendGsubIRContextU16(data, 5);
        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 8);

        const size_t substBase = data.size();

        appendGsubIRContextU16(data, 3);
        appendGsubIRContextU16(data, 1);
        appendGsubIRContextU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubIRContextU16(data, 0);

        appendGsubIRContextU16(data, 0);
        appendGsubIRContextU16(data, 0);

        const size_t coverageOffset = data.size();

        patchGsubIRContextU16(
            data, coveragePatch,
            static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubIRContextCoverage1(data, 10);

        return data;
    }


    // ========================================================================
    // Shaping-buffer helpers
    // ========================================================================

    static void appendGsubIRContextGlyph(
        OpenTypeShapingBuffer& buffer,
        uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};

        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubIRContextBuffer(
        const uint32_t* glyphs, size_t count)
    {
        OpenTypeShapingBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            appendGsubIRContextGlyph(
                buffer, glyphs[i],
                static_cast<uint32_t>(i));
        }

        return buffer;
    }


    static bool gsubIRContextGlyphEqual(
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


    static bool gsubIRContextBuffersEqual(
        const OpenTypeShapingBuffer& a,
        const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!gsubIRContextGlyphEqual(a[i], b[i]))
                return false;
        }

        return true;
    }


    static bool gsubIRContextGlyphIdsEqual(
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
    // Raw Type 5 lookup resolver
    // ========================================================================

    static OpenTypeGsubContextMatchResult resolveGsubIRContextRawLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGsubContextMatch& match) noexcept
    {
        match.clear();

        if (!lookup ||
            !openTypeGsubHasEffectiveLookupType(lookup, 5))
        {
            return OpenTypeGsubContextMatchResult::Invalid;
        }

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubContextMatchResult::Invalid;

        for (uint16_t subtableIndex = 0;
            subtableIndex < lookup.subtableCount();
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGsubEffectiveSubtable(
                    lookup, 5, subtableIndex);

            if (!data)
                return OpenTypeGsubContextMatchResult::Invalid;

            const OpenTypeGsubContextSubstView subst(data);

            if (!subst)
                return OpenTypeGsubContextMatchResult::Invalid;

            const OpenTypeGsubContextMatchResult result =
                matchOpenTypeGsubContextSubst(
                    subst, filter, buffer, glyphIndex, match);

            if (result == OpenTypeGsubContextMatchResult::Invalid ||
                result == OpenTypeGsubContextMatchResult::Match)
            {
                return result;
            }
        }

        return OpenTypeGsubContextMatchResult::NoMatch;
    }


    static bool gsubIRContextResultMatches(
        OpenTypeGsubContextMatchResult rawResult,
        OpenTypeShapingIRResult irResult) noexcept
    {
        switch (rawResult)
        {
        case OpenTypeGsubContextMatchResult::Invalid:
            return irResult == OpenTypeShapingIRResult::Invalid;

        case OpenTypeGsubContextMatchResult::NoMatch:
            return irResult == OpenTypeShapingIRResult::NoMatch;

        case OpenTypeGsubContextMatchResult::Match:
            return irResult == OpenTypeShapingIRResult::Match;

        default:
            return false;
        }
    }


    static bool gsubIRContextOpMatchesType(
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

        default:
            return false;
        }
    }


    // ========================================================================
    // compileGsubIRContextLookup
    // ========================================================================

    static bool compileGsubIRContextLookup(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& compiledLookupId,
        const OpenTypeShapingIRLookup*& compiledLookup)
    {
        compiledLookupId = kOpenTypeShapingIRInvalid;
        compiledLookup = nullptr;

        if (!compileOpenTypeGsubContextLookup(
            lookups, lookupIndex, gdef,
            ir, compiledLookupId))
        {
            return false;
        }

        compiledLookup = ir.lookup(compiledLookupId);

        return compiledLookup &&
            compiledLookup->op ==
            OpenTypeShapingIROp::GsubContext &&
            openTypeGsubIRContextLookupValid(
                ir, *compiledLookup);
    }


    // ========================================================================
    // Differential matcher
    // ========================================================================

    static bool testGsubIRContextMatcher(
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

        OpenTypeGsubContextMatch rawMatch;
        OpenTypeGsubIRContextMatch irMatch;

        const OpenTypeGsubContextMatchResult rawResult =
            resolveGsubIRContextRawLookup(
                rawLookup, gdef, buffer,
                glyphIndex, rawMatch);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRContextLookup(
                ir, compiledLookup, buffer,
                glyphIndex, irMatch);

        if (!gsubIRContextResultMatches(
            rawResult, irResult))
        {
            std::printf(
                "GSUB IR Context: FAIL\n"
                "  Case: %s\n"
                "  Matcher result mismatch\n"
                "  Raw: %u\n"
                "  IR:  %u\n",
                caseName,
                static_cast<unsigned>(rawResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (rawResult !=
            OpenTypeGsubContextMatchResult::Match)
        {
            std::printf(
                "GSUB IR Context: FAIL\n"
                "  Case: %s\n"
                "  Expected matcher success\n",
                caseName);

            return false;
        }

        if (rawMatch.positions != irMatch.positions)
        {
            std::printf(
                "GSUB IR Context: FAIL\n"
                "  Case: %s\n"
                "  Match positions differ\n",
                caseName);

            return false;
        }

        if (expectedPositions)
        {
            if (irMatch.positions.size() !=
                expectedPositionCount)
            {
                return false;
            }

            for (size_t i = 0;
                i < expectedPositionCount;
                ++i)
            {
                if (irMatch.positions[i] !=
                    expectedPositions[i])
                {
                    return false;
                }
            }
        }

        if (rawMatch.lookups.size() !=
            irMatch.lookupCount)
        {
            std::printf(
                "GSUB IR Context: FAIL\n"
                "  Case: %s\n"
                "  Action count mismatch\n"
                "  Raw: %zu\n"
                "  IR:  %u\n",
                caseName,
                rawMatch.lookups.size(),
                static_cast<unsigned>(
                    irMatch.lookupCount));

            return false;
        }

        for (size_t i = 0;
            i < rawMatch.lookups.size();
            ++i)
        {
            const OpenTypeSequenceLookup& rawAction =
                rawMatch.lookups[i];

            const OpenTypeShapingIRSequenceLookup& irAction =
                ir.gsubContextLookups[
                    irMatch.lookupOffset + i];

            if (rawAction.sequenceIndex !=
                irAction.sequenceIndex)
            {
                return false;
            }

            if (rawAction.lookupListIndex >=
                lookups.size())
            {
                return false;
            }

            const OpenTypeLayoutLookupView nestedRaw =
                lookups.lookup(
                    rawAction.lookupListIndex);

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
                !gsubIRContextOpMatchesType(
                    nestedIR->op, effectiveType))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // Whole-lookup differential
    // ========================================================================

    static bool testGsubIRContextExecution(
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
                "GSUB IR Context: FAIL\n"
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

        if (!gsubIRContextBuffersEqual(
            rawBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Context: FAIL\n"
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
            !gsubIRContextGlyphIdsEqual(
                irBuffer, expected, expectedCount))
        {
            std::printf(
                "GSUB IR Context: FAIL\n"
                "  Case: %s\n"
                "  Unexpected final glyph sequence\n",
                caseName);

            return false;
        }

        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRContext
    // ========================================================================

    static bool testOpenTypeGsubIRContext()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Context: FAIL\n"
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
                makeGsubIRContextFormat1LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 2)
                return fail("case 1 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 1 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 20, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 3);

            const size_t expectedPositions[] =
            { 0, 1, 2 };

            if (!testGsubIRContextMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 0,
                expectedPositions, 3,
                "Format 1 matcher"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 10, 220, 30 };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 3, true,
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
                makeGsubIRContextFormat2LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 2)
                return fail("case 2 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 2 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 20, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 3);

            const size_t expectedPositions[] =
            { 0, 1, 2 };

            if (!testGsubIRContextMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 0,
                expectedPositions, 3,
                "Format 2 matcher"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 10, 20, 330 };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 3, true,
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
                makeGsubIRContextFormat3LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 3 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 3 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 20, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 3);

            const size_t expectedPositions[] =
            { 0, 1, 2 };

            if (!testGsubIRContextMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 0,
                expectedPositions, 3,
                "Format 3 matcher"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 10, 200, 201, 250, 30 };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "dynamic sequenceIndex"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Nested NoMatch
        //
        // The first nested MultipleSubst still executes. The second action does
        // not match and therefore does nothing.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRContextFormat3LookupList(999);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 4 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 20, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 3);

            const uint32_t expected[] =
            { 10, 200, 201, 202, 30 };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "nested NoMatch"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Resume boundary
        //
        // Two adjacent matching contexts must both execute.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRContextFormat3LookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 5 compilation");
            }

            const uint32_t inputGlyphs[] =
            {
                10, 20, 30,
                10, 20, 30
            };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 6);

            const uint32_t expected[] =
            {
                10, 200, 201, 250, 30,
                10, 200, 201, 250, 30
            };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 10, true,
                "resume boundary"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Extension Type 7 -> Type 5
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRContextFormat3LookupList(
                    202, true);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 6 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 20, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 3);

            const uint32_t expected[] =
            { 10, 200, 201, 250, 30 };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "Type 7 -> Type 5"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - LookupFlag filtered traversal
        //
        // Physical:
        //
        //     10 M 20 M 30
        //      0 1  2 3  4
        //
        // M is glyph 100, GDEF mark class 3, skipped by IgnoreMarks.
        //
        // Matched positions must be:
        //
        //     { 0, 2, 4 }
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRContextFilteredLookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const std::vector<uint8_t> gdefBytes =
                makeGsubIRContextGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(
                    gdefBytes.data(),
                    gdefBytes.size()));

            if (!gdef)
                return fail("case 7 GDEF");

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 7 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10, 100, 20, 100, 30 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 5);

            const size_t expectedPositions[] =
            { 0, 2, 4 };

            if (!testGsubIRContextMatcher(
                lookups, 0, gdef,
                ir, *compiled, input, 0,
                expectedPositions, 3,
                "filtered traversal"))
            {
                return false;
            }

            const uint32_t expected[] =
            { 10, 100, 220, 100, 30 };

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                expected, 5, true,
                "filtered execution"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Zero-action match
        //
        // A context with no SequenceLookup records still counts as a match and
        // must resume after the matched input sequence.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRContextNoActionLookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 8 compilation");
            }

            const uint32_t inputGlyphs[] =
            {
                10, 20, 30,
                10, 20, 30
            };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 6);

            if (!testGsubIRContextExecution(
                lookups, 0, gdef,
                ir, lookupId, input,
                inputGlyphs, 6, true,
                "zero-action match"))
            {
                return false;
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Recursive Type 5 protection and transactional rollback
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubIRContextRecursiveLookupList();

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 1)
                return fail("case 9 LookupList");

            const OpenTypeGdefView gdef{};

            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            const OpenTypeShapingIRLookup* compiled = nullptr;

            if (!compileGsubIRContextLookup(
                lookups, 0, gdef,
                ir, lookupId, compiled))
            {
                return fail("case 9 compilation");
            }

            const uint32_t inputGlyphs[] =
            { 10 };

            const OpenTypeShapingBuffer input =
                makeGsubIRContextBuffer(
                    inputGlyphs, 1);

            if (!testGsubIRContextExecution(
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
            "GSUB IR Context: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1:                 PASS\n"
            "  Format 2:                 PASS\n"
            "  Format 3:                 PASS\n"
            "  Dynamic sequenceIndex:    PASS\n"
            "  Nested NoMatch:           PASS\n"
            "  Resume boundary:          PASS\n"
            "  Type 7 -> Type 5:         PASS\n"
            "  Filtered traversal:       PASS\n"
            "  Zero-action match:        PASS\n"
            "  Recursive rollback:       PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
