// test_opentype_gsub_ir_ligature.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "opentype_container.h"
#include "opentype_face_tables.h"
#include "opentype_gdef_view.h"
#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_gsub_lookup_apply.h"
#include "opentype_gsub_ligature_view.h"

#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    // ========================================================================
    // Basic comparison helpers
    // ========================================================================

    static bool openTypeGsubIRLigatureResultMatches(
        OpenTypeGsubResolveResult oldResult,
        OpenTypeShapingIRResult irResult) noexcept
    {
        switch (oldResult)
        {
        case OpenTypeGsubResolveResult::Invalid:
            return irResult == OpenTypeShapingIRResult::Invalid;

        case OpenTypeGsubResolveResult::NoMatch:
            return irResult == OpenTypeShapingIRResult::NoMatch;

        case OpenTypeGsubResolveResult::Match:
            return irResult == OpenTypeShapingIRResult::Match;

        default:
            return false;
        }
    }


    static bool openTypeGsubIRLigatureTestGlyphMatches(
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


    static bool openTypeGsubIRLigatureTestBufferMatches(
        const OpenTypeShapingBuffer& a,
        const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!openTypeGsubIRLigatureTestGlyphMatches(a[i], b[i]))
                return false;
        }

        return true;
    }


    // ========================================================================
    // LookupList extraction
    // ========================================================================

    static bool openTypeGsubIRLigatureTestLookupList(
        const ByteSpan& gsubData,
        OpenTypeLayoutLookupListView& result) noexcept
    {
        result = {};

        if (gsubData.size() < 10)
            return false;

        OpenTypeByteStream stream(gsubData);

        uint16_t majorVersion = 0;
        uint16_t minorVersion = 0;
        uint16_t scriptListOffset = 0;
        uint16_t featureListOffset = 0;
        uint16_t lookupListOffset = 0;

        if (!stream.readUInt16(majorVersion) ||
            !stream.readUInt16(minorVersion) ||
            !stream.readUInt16(scriptListOffset) ||
            !stream.readUInt16(featureListOffset) ||
            !stream.readUInt16(lookupListOffset))
        {
            return false;
        }

        if (majorVersion != 1)
            return false;

        if (scriptListOffset == 0 ||
            featureListOffset == 0 ||
            lookupListOffset == 0 ||
            scriptListOffset >= gsubData.size() ||
            featureListOffset >= gsubData.size() ||
            lookupListOffset >= gsubData.size())
        {
            return false;
        }

        result = OpenTypeLayoutLookupListView(
            gsubData.subSpan(lookupListOffset));

        return static_cast<bool>(result);
    }


    // ========================================================================
    // Candidate buffer
    //
    // Build the exact logical input sequence represented by one compiled
    // LigatureSubst candidate.
    //
    // Scalar offsets deliberately leave one scalar position between components.
    // This gives the filtered-traversal test somewhere sensible to place an
    // ignored glyph without changing participating component provenance.
    // ========================================================================

    static bool makeOpenTypeGsubIRLigatureTestBuffer(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubLigaturePair& pair,
        const OpenTypeShapingIRGsubLigature& ligature,
        OpenTypeShapingBuffer& buffer)
    {
        buffer.clear();

        if (!openTypeGsubIRLigatureRecordValid(ir, ligature) ||
            ligature.componentCount < 2)
        {
            return false;
        }

        for (uint16_t i = 0; i < ligature.componentCount; ++i)
        {
            OpenTypeShapingGlyph glyph{};

            if (i == 0)
            {
                glyph.glyphId = pair.input;
            }
            else
            {
                glyph.glyphId =
                    ir.gsubLigatureComponents[
                        ligature.componentOffset + i - 1];
            }

            glyph.scalarOffset = uint32_t(i) * 2u;
            glyph.scalarCount = 1;

            buffer.pushBack(glyph);
        }

        return true;
    }


    // ========================================================================
    // Resolver differential
    // ========================================================================

    static bool testOpenTypeGsubIRLigatureResolver(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiledLookup,
        const OpenTypeShapingBuffer& input,
        uint16_t lookupIndex,
        bool requireMatch)
    {
        const OpenTypeLookupGlyphFilter oldFilter(lookup, gdef);

        if (!oldFilter)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Raw LookupFlag filter is invalid\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }

        OpenTypeGsubLigatureMatch oldMatch;
        OpenTypeGsubIRLigatureMatch irMatch;

        const OpenTypeGsubResolveResult oldResult =
            resolveOpenTypeGsubLigatureLookup(
                lookup, oldFilter, input, 0, oldMatch);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRLigatureLookup(
                ir, compiledLookup, input, 0, irMatch);

        if (!openTypeGsubIRLigatureResultMatches(oldResult, irResult))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Resolver result mismatch\n"
                "  Old result: %u\n"
                "  IR result:  %u\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(oldResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (requireMatch &&
            oldResult != OpenTypeGsubResolveResult::Match)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Expected candidate did not match\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }

        if (oldResult != OpenTypeGsubResolveResult::Match)
            return true;

        if (oldMatch.ligatureGlyph != irMatch.ligatureGlyph ||
            oldMatch.positions != irMatch.positions)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Ligature match mismatch\n"
                "  Old glyph:     %u\n"
                "  IR glyph:      %u\n"
                "  Old positions: %zu\n"
                "  IR positions:  %zu\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(oldMatch.ligatureGlyph),
                static_cast<unsigned>(irMatch.ligatureGlyph),
                oldMatch.positions.size(),
                irMatch.positions.size());

            const size_t count =
                oldMatch.positions.size() < irMatch.positions.size()
                ? oldMatch.positions.size()
                : irMatch.positions.size();

            for (size_t i = 0; i < count; ++i)
            {
                if (oldMatch.positions[i] == irMatch.positions[i])
                    continue;

                std::printf(
                    "  Position %zu: old=%zu IR=%zu\n",
                    i,
                    oldMatch.positions[i],
                    irMatch.positions[i]);

                break;
            }

            return false;
        }

        return true;
    }


    // ========================================================================
    // Whole-lookup differential
    // ========================================================================

    static bool testOpenTypeGsubIRLigatureExecution(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId compiledLookupId,
        const OpenTypeShapingBuffer& input,
        uint16_t lookupIndex,
        size_t& outputGlyphs)
    {
        OpenTypeShapingBuffer oldBuffer = input;
        OpenTypeShapingBuffer irBuffer = input;

        if (!applyOpenTypeGsubLigatureLookup(
            lookup, gdef, oldBuffer))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Old executor failed\n"
                "  LookupFlag: 0x%04X\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(lookup.lookupFlag()));

            return false;
        }

        if (!applyOpenTypeGsubIRLookup(
            ir, compiledLookupId, irBuffer))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  IR executor failed\n"
                "  LookupFlag: 0x%04X\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(lookup.lookupFlag()));

            return false;
        }

        if (!openTypeGsubIRLigatureTestBufferMatches(
            oldBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Whole-buffer mismatch\n"
                "  Input glyphs: %zu\n"
                "  Old glyphs:   %zu\n"
                "  IR glyphs:    %zu\n",
                static_cast<unsigned>(lookupIndex),
                input.size(),
                oldBuffer.size(),
                irBuffer.size());

            const size_t count =
                oldBuffer.size() < irBuffer.size()
                ? oldBuffer.size()
                : irBuffer.size();

            for (size_t i = 0; i < count; ++i)
            {
                if (openTypeGsubIRLigatureTestGlyphMatches(
                    oldBuffer[i], irBuffer[i]))
                {
                    continue;
                }

                std::printf(
                    "  First mismatch: %zu\n"
                    "    old gid: %u\n"
                    "    IR gid:  %u\n"
                    "    old src: [%u,%u)\n"
                    "    IR src:  [%u,%u)\n"
                    "    old lig: id=%u comp=%u count=%u\n"
                    "    IR lig:  id=%u comp=%u count=%u\n",
                    i,
                    static_cast<unsigned>(oldBuffer[i].glyphId),
                    static_cast<unsigned>(irBuffer[i].glyphId),
                    static_cast<unsigned>(oldBuffer[i].scalarOffset),
                    static_cast<unsigned>(
                        oldBuffer[i].scalarOffset +
                        oldBuffer[i].scalarCount),
                    static_cast<unsigned>(irBuffer[i].scalarOffset),
                    static_cast<unsigned>(
                        irBuffer[i].scalarOffset +
                        irBuffer[i].scalarCount),
                    static_cast<unsigned>(oldBuffer[i].ligature.id),
                    static_cast<unsigned>(oldBuffer[i].ligature.component),
                    static_cast<unsigned>(oldBuffer[i].ligature.componentCount),
                    static_cast<unsigned>(irBuffer[i].ligature.id),
                    static_cast<unsigned>(irBuffer[i].ligature.component),
                    static_cast<unsigned>(irBuffer[i].ligature.componentCount));

                break;
            }

            return false;
        }

        outputGlyphs += oldBuffer.size();
        return true;
    }


    // ========================================================================
    // Find one glyph ignored by this lookup.
    //
    // This also directly compares raw GDEF filtering against compiled IR
    // filtering for the complete uint16 glyph domain until an ignored glyph
    // is found.
    //
    // found == false is valid. For example, RightToLeft alone does not cause
    // any glyph to be skipped.
    // ========================================================================

    static bool findOpenTypeGsubIRLigatureIgnoredGlyph(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiledLookup,
        uint16_t lookupIndex,
        uint16_t& result,
        bool& found)
    {
        result = 0;
        found = false;

        const OpenTypeLookupGlyphFilter oldFilter(lookup, gdef);

        if (!oldFilter)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            bool oldSkip = false;
            bool irSkip = false;

            if (!oldFilter.shouldSkip(glyphId, oldSkip))
            {
                std::printf(
                    "GSUB IR Ligature: FAIL\n"
                    "  Lookup: %u\n"
                    "  Raw filter failed for glyph %u\n",
                    static_cast<unsigned>(lookupIndex),
                    static_cast<unsigned>(glyphId));

                return false;
            }

            if (!openTypeShapingIRLookupShouldSkip(
                ir,
                compiledLookup.filter,
                glyphId,
                irSkip))
            {
                std::printf(
                    "GSUB IR Ligature: FAIL\n"
                    "  Lookup: %u\n"
                    "  IR filter failed for glyph %u\n",
                    static_cast<unsigned>(lookupIndex),
                    static_cast<unsigned>(glyphId));

                return false;
            }

            if (oldSkip != irSkip)
            {
                std::printf(
                    "GSUB IR Ligature: FAIL\n"
                    "  Lookup: %u\n"
                    "  Filter mismatch for glyph %u\n"
                    "  Old skip: %u\n"
                    "  IR skip:  %u\n",
                    static_cast<unsigned>(lookupIndex),
                    static_cast<unsigned>(glyphId),
                    static_cast<unsigned>(oldSkip),
                    static_cast<unsigned>(irSkip));

                return false;
            }

            if (!oldSkip)
                continue;

            result = static_cast<uint16_t>(glyphId);
            found = true;
            return true;
        }

        return true;
    }


    // ========================================================================
    // Filtered traversal differential
    //
    // Given a candidate which normally matches:
    //
    //     A B C
    //
    // insert a glyph ignored by LookupFlag:
    //
    //     A ignored B C
    //
    // The expected participating physical positions become:
    //
    //     { 0, 2, 3 }
    //
    // This directly proves that the compiled IR uses filtered physical
    // traversal rather than assuming contiguous components.
    // ========================================================================

    static bool testOpenTypeGsubIRLigatureFilteredTraversal(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiledLookup,
        OpenTypeShapingIRLookupId compiledLookupId,
        const OpenTypeShapingBuffer& baseInput,
        uint16_t ignoredGlyph,
        uint16_t lookupIndex,
        size_t& filteredTraversalCases,
        size_t& inputGlyphs,
        size_t& outputGlyphs)
    {
        if (baseInput.size() < 2)
            return false;

        OpenTypeShapingBuffer input = baseInput;

        OpenTypeShapingGlyph ignored{};

        ignored.glyphId = ignoredGlyph;
        ignored.scalarOffset = 1;
        ignored.scalarCount = 1;

        input.glyphs().insert(
            input.glyphs().begin() + 1,
            ignored);

        const OpenTypeLookupGlyphFilter oldFilter(lookup, gdef);

        if (!oldFilter)
            return false;

        OpenTypeGsubLigatureMatch oldMatch;
        OpenTypeGsubIRLigatureMatch irMatch;

        const OpenTypeGsubResolveResult oldResult =
            resolveOpenTypeGsubLigatureLookup(
                lookup, oldFilter, input, 0, oldMatch);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRLigatureLookup(
                ir, compiledLookup, input, 0, irMatch);

        if (oldResult != OpenTypeGsubResolveResult::Match ||
            irResult != OpenTypeShapingIRResult::Match)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Filtered traversal did not match\n"
                "  Ignored glyph: %u\n"
                "  Old result: %u\n"
                "  IR result:  %u\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(ignoredGlyph),
                static_cast<unsigned>(oldResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (oldMatch.ligatureGlyph != irMatch.ligatureGlyph ||
            oldMatch.positions != irMatch.positions)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Filtered traversal match mismatch\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }

        if (oldMatch.positions.size() < 2 ||
            oldMatch.positions[0] != 0 ||
            oldMatch.positions[1] != 2)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Ignored glyph was not skipped physically\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }

        ++filteredTraversalCases;
        inputGlyphs += input.size();

        return testOpenTypeGsubIRLigatureExecution(
            lookup,
            gdef,
            ir,
            compiledLookupId,
            input,
            lookupIndex,
            outputGlyphs);
    }


    // ========================================================================
    // One compiled ligature candidate
    // ========================================================================

    static bool testOpenTypeGsubIRLigatureCandidate(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& compiledLookup,
        OpenTypeShapingIRLookupId compiledLookupId,
        const OpenTypeShapingIRGsubLigaturePair& pair,
        const OpenTypeShapingIRGsubLigature& ligature,
        uint16_t lookupIndex,
        bool hasIgnoredGlyph,
        uint16_t ignoredGlyph,
        bool& filteredTraversalExercised,
        size_t& candidateCases,
        size_t& matchedCandidateCases,
        size_t& filteredTraversalCases,
        size_t& inputGlyphs,
        size_t& outputGlyphs)
    {
        OpenTypeShapingBuffer input;

        if (!makeOpenTypeGsubIRLigatureTestBuffer(
            ir, pair, ligature, input))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Unable to build candidate buffer\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }

        ++candidateCases;
        inputGlyphs += input.size();


        // ------------------------------------------------------------
        // Resolve first so we know whether this compiled candidate is actually
        // eligible under its own LookupFlag.
        //
        // A candidate whose secondary component is itself ignored can
        // legitimately produce NoMatch. Raw and IR must still agree.
        // ------------------------------------------------------------

        const OpenTypeLookupGlyphFilter oldFilter(lookup, gdef);

        if (!oldFilter)
            return false;

        OpenTypeGsubLigatureMatch oldMatch;
        OpenTypeGsubIRLigatureMatch irMatch;

        const OpenTypeGsubResolveResult oldResult =
            resolveOpenTypeGsubLigatureLookup(
                lookup, oldFilter, input, 0, oldMatch);

        const OpenTypeShapingIRResult irResult =
            resolveOpenTypeGsubIRLigatureLookup(
                ir, compiledLookup, input, 0, irMatch);

        if (!openTypeGsubIRLigatureResultMatches(oldResult, irResult))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Candidate resolver result mismatch\n"
                "  Old result: %u\n"
                "  IR result:  %u\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(oldResult),
                static_cast<unsigned>(irResult));

            return false;
        }

        if (oldResult == OpenTypeGsubResolveResult::Match)
        {
            ++matchedCandidateCases;

            if (oldMatch.ligatureGlyph != irMatch.ligatureGlyph ||
                oldMatch.positions != irMatch.positions)
            {
                std::printf(
                    "GSUB IR Ligature: FAIL\n"
                    "  Lookup: %u\n"
                    "  Candidate match mismatch\n",
                    static_cast<unsigned>(lookupIndex));

                return false;
            }
        }


        // ------------------------------------------------------------
        // Whole-lookup execution must agree whether this particular candidate
        // matched or not.
        // ------------------------------------------------------------

        if (!testOpenTypeGsubIRLigatureExecution(
            lookup,
            gdef,
            ir,
            compiledLookupId,
            input,
            lookupIndex,
            outputGlyphs))
        {
            return false;
        }


        // ------------------------------------------------------------
        // One direct skipped-glyph traversal case is enough per lookup.
        //
        // Only use a base candidate that itself matched normally.
        // ------------------------------------------------------------

        if (!filteredTraversalExercised &&
            hasIgnoredGlyph &&
            oldResult == OpenTypeGsubResolveResult::Match)
        {
            if (!testOpenTypeGsubIRLigatureFilteredTraversal(
                lookup,
                gdef,
                ir,
                compiledLookup,
                compiledLookupId,
                input,
                ignoredGlyph,
                lookupIndex,
                filteredTraversalCases,
                inputGlyphs,
                outputGlyphs))
            {
                return false;
            }

            filteredTraversalExercised = true;
        }

        return true;
    }


    // ========================================================================
    // One Type 4 lookup
    // ========================================================================

    static bool testOpenTypeGsubIRLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        uint16_t lookupIndex,
        size_t& candidateCases,
        size_t& matchedCandidateCases,
        size_t& filteredTraversalCases,
        size_t& inputGlyphs,
        size_t& outputGlyphs,
        size_t& subtables,
        size_t& compiledPairs,
        size_t& compiledLigatures,
        size_t& extensionLookups)
    {
        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 4)
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Invalid effective Type 4 lookup\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }

        if (lookup.lookupType() == 7)
            ++extensionLookups;


        // ------------------------------------------------------------
        // Compile.
        // ------------------------------------------------------------

        OpenTypeShapingIR ir;

        OpenTypeShapingIRLookupId compiledLookupId =
            kOpenTypeShapingIRInvalid;

        if (!compileOpenTypeGsubLigatureLookup(
            lookup, gdef, ir, compiledLookupId))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Compilation failed\n"
                "  LookupFlag: 0x%04X\n",
                static_cast<unsigned>(lookupIndex),
                static_cast<unsigned>(lookup.lookupFlag()));

            return false;
        }

        const OpenTypeShapingIRLookup* compiledLookup =
            ir.lookup(compiledLookupId);

        if (!compiledLookup ||
            compiledLookup->op != OpenTypeShapingIROp::GsubLigature ||
            !openTypeGsubIRLigatureLookupValid(ir, *compiledLookup))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Lookup: %u\n"
                "  Invalid compiled lookup\n",
                static_cast<unsigned>(lookupIndex));

            return false;
        }


        // ------------------------------------------------------------
        // Compare raw and compiled filtering and locate one glyph which can
        // exercise physical skipped-glyph traversal.
        // ------------------------------------------------------------

        uint16_t ignoredGlyph = 0;
        bool hasIgnoredGlyph = false;

        if (!findOpenTypeGsubIRLigatureIgnoredGlyph(
            lookup,
            gdef,
            ir,
            *compiledLookup,
            lookupIndex,
            ignoredGlyph,
            hasIgnoredGlyph))
        {
            return false;
        }

        bool filteredTraversalExercised = false;


        // ------------------------------------------------------------
        // Every compiled candidate.
        // ------------------------------------------------------------

        subtables += compiledLookup->payloadCount;

        const size_t firstSubtable =
            compiledLookup->payloadOffset;

        for (size_t subtableIndex = 0;
            subtableIndex < compiledLookup->payloadCount;
            ++subtableIndex)
        {
            const OpenTypeShapingIRGsubLigatureSubtable& subtable =
                ir.gsubLigatureSubtables[
                    firstSubtable + subtableIndex];

            if (!openTypeGsubIRLigatureSubtableValid(ir, subtable))
                return false;

            compiledPairs += subtable.pairCount;

            for (size_t pairIndex = 0;
                pairIndex < subtable.pairCount;
                ++pairIndex)
            {
                const OpenTypeShapingIRGsubLigaturePair& pair =
                    ir.gsubLigaturePairs[
                        subtable.pairOffset + pairIndex];

                compiledLigatures += pair.ligatureCount;

                for (size_t candidateIndex = 0;
                    candidateIndex < pair.ligatureCount;
                    ++candidateIndex)
                {
                    const OpenTypeShapingIRGsubLigature& ligature =
                        ir.gsubLigatures[
                            pair.ligatureOffset + candidateIndex];

                    if (!testOpenTypeGsubIRLigatureCandidate(
                        lookup,
                        gdef,
                        ir,
                        *compiledLookup,
                        compiledLookupId,
                        pair,
                        ligature,
                        lookupIndex,
                        hasIgnoredGlyph,
                        ignoredGlyph,
                        filteredTraversalExercised,
                        candidateCases,
                        matchedCandidateCases,
                        filteredTraversalCases,
                        inputGlyphs,
                        outputGlyphs))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRLigature
    //
    // Test every effective GSUB Type 4 lookup in every face, including
    // nonzero LookupFlag lookups.
    //
    // For lookups which actually ignore at least one glyph, one additional
    // non-contiguous component traversal is tested.
    // ========================================================================

    static bool testOpenTypeGsubIRLigature(const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Ligature: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        if (fontData.empty())
            return fail("empty font data");


        SharedMemBuff fontBuffer(fontData.size());

        if (!fontBuffer)
            return fail("unable to allocate font buffer");

        std::memcpy(
            fontBuffer.data(),
            fontData.begin(),
            fontData.size());

        OpenTypeContainer container(fontBuffer);

        if (!container.isValid())
            return fail("invalid OpenType container");


        // ------------------------------------------------------------
        // Statistics.
        // ------------------------------------------------------------

        size_t faceCount = 0;
        size_t gsubFaceCount = 0;
        size_t lookupCount = 0;

        size_t ligatureLookupCount = 0;
        size_t flaggedLookupCount = 0;
        size_t extensionLookups = 0;

        size_t subtables = 0;
        size_t compiledPairs = 0;
        size_t compiledLigatures = 0;

        size_t candidateCases = 0;
        size_t matchedCandidateCases = 0;
        size_t filteredTraversalCases = 0;

        size_t executionInputGlyphs = 0;
        size_t executionOutputGlyphs = 0;


        // ------------------------------------------------------------
        // Every face.
        // ------------------------------------------------------------

        FontFace face;

        while (container(face))
        {
            ++faceCount;

            const IProvideOpenTypeTables* tables =
                openTypeTableProvider(face);

            if (!tables)
                return fail("font face does not provide OpenType tables");

            const TableRecord* gsub =
                tables->getTable(OTAG("GSUB"));

            if (!gsub)
                continue;

            ++gsubFaceCount;


            // ------------------------------------------------------------
            // GDEF.
            //
            // Zero-filter lookups can operate without it. Flagged lookups
            // requiring classes or mark sets are validated by both compilers.
            // ------------------------------------------------------------

            const TableRecord* gdefTable =
                tables->getTable(OTAG("GDEF"));

            OpenTypeGdefView gdef{};

            if (gdefTable)
            {
                gdef = OpenTypeGdefView(gdefTable->data);

                if (!gdef)
                    return fail("invalid GDEF table");
            }


            // ------------------------------------------------------------
            // LookupList.
            // ------------------------------------------------------------

            OpenTypeLayoutLookupListView lookups;

            if (!openTypeGsubIRLigatureTestLookupList(
                gsub->data, lookups))
            {
                return fail("invalid GSUB LookupList");
            }

            lookupCount += lookups.size();


            // ------------------------------------------------------------
            // Every effective Type 4 lookup.
            // ------------------------------------------------------------

            for (uint16_t lookupIndex = 0;
                lookupIndex < lookups.size();
                ++lookupIndex)
            {
                const OpenTypeLayoutLookupView lookup =
                    lookups.lookup(lookupIndex);

                if (!lookup)
                    return fail("invalid GSUB lookup");

                uint16_t effectiveType = 0;

                if (!openTypeGsubIREffectiveType(
                    lookup, effectiveType))
                {
                    return fail(
                        "unable to resolve effective GSUB lookup type");
                }

                if (effectiveType != 4)
                    continue;

                if (lookup.lookupFlag() != 0)
                    ++flaggedLookupCount;

                ++ligatureLookupCount;

                if (!testOpenTypeGsubIRLigatureLookup(
                    lookup,
                    gdef,
                    lookupIndex,
                    candidateCases,
                    matchedCandidateCases,
                    filteredTraversalCases,
                    executionInputGlyphs,
                    executionOutputGlyphs,
                    subtables,
                    compiledPairs,
                    compiledLigatures,
                    extensionLookups))
                {
                    return false;
                }
            }
        }


        // ------------------------------------------------------------
        // Require real coverage.
        // ------------------------------------------------------------

        if (faceCount == 0)
            return fail("container produced no font faces");

        if (gsubFaceCount == 0)
            return fail("font contains no GSUB table");

        if (ligatureLookupCount == 0)
            return fail("font contains no supported GSUB Type 4 lookup");

        if (candidateCases == 0)
            return fail("no ligature candidates were exercised");


        // ------------------------------------------------------------
        // Summary.
        // ------------------------------------------------------------

        std::printf(
            "GSUB IR Ligature: PASS\n"
            "  Faces:                    %zu\n"
            "  Faces with GSUB:          %zu\n"
            "  GSUB lookups:             %zu\n"
            "  Type 4 lookups tested:    %zu\n"
            "  Flagged Type 4 tested:    %zu\n"
            "  Type 4 subtables:         %zu\n"
            "  First-glyph pairs:        %zu\n"
            "  Ligature candidates:      %zu\n"
            "  Candidate cases:          %zu\n"
            "  Candidate matches:        %zu\n"
            "  Filter traversal cases:   %zu\n"
            "  Extension Type 4 lookups: %zu\n"
            "  Execution input glyphs:   %zu\n"
            "  Execution output glyphs:  %zu\n",
            faceCount,
            gsubFaceCount,
            lookupCount,
            ligatureLookupCount,
            flaggedLookupCount,
            subtables,
            compiledPairs,
            compiledLigatures,
            candidateCases,
            matchedCandidateCases,
            filteredTraversalCases,
            extensionLookups,
            executionInputGlyphs,
            executionOutputGlyphs);

        return true;
    }


    // ========================================================================
    // Filename convenience overload.
    // ========================================================================

    static bool testOpenTypeGsubIRLigature(const char* fontFilename)
    {
        std::vector<uint8_t> fontBytes;

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "GSUB IR Ligature: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return testOpenTypeGsubIRLigature(
            ByteSpan(fontBytes.data(), fontBytes.size()));
    }

} // namespace waavs