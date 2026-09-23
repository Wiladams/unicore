// test_opentype_gsub_ir_multiple.h
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
//#include "opentype_gsub_lookup_apply.h"
#include "opentype_gsub_multiple_view.h"

#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    // ========================================================================
    // openTypeGsubIRMultipleResultMatches
    // ========================================================================

    static bool openTypeGsubIRMultipleResultMatches(
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


    // ========================================================================
    // openTypeGsubIRMultipleGlyphMatches
    //
    // MultipleSubst duplicates the complete source shaping record into every
    // output glyph, changing only glyphId.
    // ========================================================================

    static bool openTypeGsubIRMultipleGlyphMatches(
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


    // ========================================================================
    // openTypeGsubIRMultipleBufferMatches
    // ========================================================================

    static bool openTypeGsubIRMultipleBufferMatches(
        const OpenTypeShapingBuffer& a,
        const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!openTypeGsubIRMultipleGlyphMatches(a[i], b[i]))
                return false;
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRMultipleSequenceMatches
    //
    // Compare one raw MultipleSubst Sequence against its compiled IR sequence.
    // ========================================================================

    static bool openTypeGsubIRMultipleSequenceMatches(
        const OpenTypeGsubMultipleSequenceView& oldSequence,
        const OpenTypeShapingIR& ir, uint32_t sequenceIndex)
    {
        if (!oldSequence ||
            sequenceIndex >= ir.gsubMultipleSequences.size())
        {
            return false;
        }

        const OpenTypeShapingIRGlyphSequence& irSequence =
            ir.gsubMultipleSequences[sequenceIndex];

        if (!openTypeGsubIRGlyphSequenceValid(ir, irSequence))
            return false;

        const uint16_t oldCount = oldSequence.glyphCount();

        if (oldCount != irSequence.glyphCount)
            return false;

        for (uint16_t i = 0; i < oldCount; ++i)
        {
            uint16_t oldGlyph = 0;

            if (!oldSequence.glyphId(i, oldGlyph))
                return false;

            const uint16_t irGlyph =
                ir.gsubMultipleGlyphs[irSequence.glyphOffset + i];

            if (oldGlyph != irGlyph)
                return false;
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRMultipleTestLookupList
    //
    // Extract LookupList directly from the GSUB header.
    // ========================================================================

    static bool openTypeGsubIRMultipleTestLookupList(
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
    // makeOpenTypeGsubIRMultipleExecutionBuffer
    //
    // Build a runtime buffer containing every compiled MultipleSubst input
    // glyph plus several boundary/non-match probes.
    //
    // Provenance is deliberately nontrivial so 1 -> N copying is checked.
    // ========================================================================

    static bool makeOpenTypeGsubIRMultipleExecutionBuffer(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& result)
    {
        result.clear();

        if (!openTypeGsubIRMultipleLookupValid(ir, lookup))
            return false;

        size_t glyphIndex = 0;

        auto appendGlyph =
            [&](uint16_t glyphId)
            {
                OpenTypeShapingGlyph glyph{};

                glyph.glyphId = glyphId;
                glyph.scalarOffset =
                    static_cast<uint32_t>(glyphIndex * 3);
                glyph.scalarCount =
                    static_cast<uint32_t>((glyphIndex % 3) + 1);

                if ((glyphIndex & 1u) == 0)
                {
                    glyph.ligature.id =
                        static_cast<uint32_t>(glyphIndex + 1);
                    glyph.ligature.component = 0;
                    glyph.ligature.componentCount = 2;
                }
                else
                {
                    glyph.ligature.id =
                        static_cast<uint32_t>(glyphIndex);
                    glyph.ligature.component = 1;
                    glyph.ligature.componentCount = 0;
                }

                result.pushBack(glyph);
                ++glyphIndex;
            };


        // Boundary/non-match probes.

        appendGlyph(0x0000);
        appendGlyph(0x0001);
        appendGlyph(0x7FFF);
        appendGlyph(0xFFFE);
        appendGlyph(0xFFFF);


        // Every compiled MultipleSubst input glyph.

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGsubMultipleSubtable& subtable =
                ir.gsubMultipleSubtables[firstSubtable + i];

            if (!openTypeGsubIRMultipleSubtableValid(ir, subtable))
                return false;

            for (size_t pairIndex = 0;
                pairIndex < subtable.pairCount;
                ++pairIndex)
            {
                const OpenTypeShapingIRGsubMultiplePair& pair =
                    ir.gsubMultiplePairs[
                        subtable.pairOffset + pairIndex];

                appendGlyph(pair.input);
            }
        }

        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRMultipleLookup
    //
    // Differential proof for one effective GSUB Type 2 lookup.
    //
    // Part 1:
    //
    //     raw resolver == IR resolver
    //
    // across the complete uint16 glyph domain.
    //
    // Part 2:
    //
    //     raw whole-lookup executor == IR whole-lookup executor
    //
    // including 1 -> N topology and all provenance.
    // ========================================================================

    static bool testOpenTypeGsubIRMultipleLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        uint16_t sourceLookupIndex,
        size_t& glyphComparisons,
        size_t& sequenceComparisons,
        size_t& outputGlyphComparisons,
        size_t& executionInputGlyphs,
        size_t& executionOutputGlyphs,
        size_t& subtableCount,
        size_t& extensionLookups)
    {
        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(lookup, effectiveType) ||
            effectiveType != 2)
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  Invalid effective Type 2 lookup\n",
                static_cast<unsigned>(sourceLookupIndex));

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

        if (!compileOpenTypeGsubMultipleLookup(
            lookup, gdef, ir, compiledLookupId))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  Compilation failed\n"
                "  LookupFlag: 0x%04X\n",
                static_cast<unsigned>(sourceLookupIndex),
                static_cast<unsigned>(lookup.lookupFlag()));

            return false;
        }

        const OpenTypeShapingIRLookup* compiledLookup =
            ir.lookup(compiledLookupId);

        if (!compiledLookup ||
            compiledLookup->op != OpenTypeShapingIROp::GsubMultiple ||
            !openTypeGsubIRMultipleLookupValid(ir, *compiledLookup))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  Invalid compiled lookup\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        subtableCount += lookup.subtableCount();


        // ------------------------------------------------------------
        // Exhaustive resolver comparison.
        //
        // Type 2 has one input glyph. LookupFlag filtering therefore has no
        // secondary input position to traverse. Resolution itself remains a
        // direct comparison of the current glyph against the substitution.
        // ------------------------------------------------------------

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            OpenTypeGsubMultipleSequenceView oldSequence;
            uint32_t irSequenceIndex = kOpenTypeShapingIRInvalid;

            const OpenTypeGsubResolveResult oldResult =
                resolveOpenTypeGsubMultipleLookup(
                    lookup, glyphId, oldSequence);

            const OpenTypeShapingIRResult irResult =
                resolveOpenTypeGsubIRMultipleLookup(
                    ir,
                    *compiledLookup,
                    static_cast<uint16_t>(glyphId),
                    irSequenceIndex);

            ++glyphComparisons;

            if (!openTypeGsubIRMultipleResultMatches(
                oldResult, irResult))
            {
                std::printf(
                    "GSUB IR Multiple: FAIL\n"
                    "  Lookup: %u\n"
                    "  Glyph:  %u\n"
                    "  Resolver result mismatch\n"
                    "  Old:    %u\n"
                    "  IR:     %u\n",
                    static_cast<unsigned>(sourceLookupIndex),
                    static_cast<unsigned>(glyphId),
                    static_cast<unsigned>(oldResult),
                    static_cast<unsigned>(irResult));

                return false;
            }

            if (oldResult != OpenTypeGsubResolveResult::Match)
                continue;

            ++sequenceComparisons;

            if (!openTypeGsubIRMultipleSequenceMatches(
                oldSequence, ir, irSequenceIndex))
            {
                std::printf(
                    "GSUB IR Multiple: FAIL\n"
                    "  Lookup: %u\n"
                    "  Glyph:  %u\n"
                    "  Replacement sequence mismatch\n",
                    static_cast<unsigned>(sourceLookupIndex),
                    static_cast<unsigned>(glyphId));

                return false;
            }

            outputGlyphComparisons += oldSequence.glyphCount();
        }


        // ------------------------------------------------------------
        // Whole-lookup execution comparison.
        // ------------------------------------------------------------

        OpenTypeShapingBuffer input;

        if (!makeOpenTypeGsubIRMultipleExecutionBuffer(
            ir, *compiledLookup, input))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  Unable to build execution buffer\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        OpenTypeShapingBuffer oldBuffer = input;
        OpenTypeShapingBuffer irBuffer = input;

        executionInputGlyphs += input.size();

        if (!applyOpenTypeGsubMultipleLookup(
            lookup, gdef, oldBuffer))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  Old executor failed\n"
                "  LookupFlag: 0x%04X\n",
                static_cast<unsigned>(sourceLookupIndex),
                static_cast<unsigned>(lookup.lookupFlag()));

            return false;
        }

        if (!applyOpenTypeGsubIRLookup(
            ir, compiledLookupId, irBuffer))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  IR executor failed\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        executionOutputGlyphs += oldBuffer.size();

        if (!openTypeGsubIRMultipleBufferMatches(
            oldBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Lookup: %u\n"
                "  Whole-buffer result mismatch\n"
                "  Input glyphs: %zu\n"
                "  Old glyphs:   %zu\n"
                "  IR glyphs:    %zu\n",
                static_cast<unsigned>(sourceLookupIndex),
                input.size(),
                oldBuffer.size(),
                irBuffer.size());

            const size_t count =
                oldBuffer.size() < irBuffer.size()
                ? oldBuffer.size()
                : irBuffer.size();

            for (size_t i = 0; i < count; ++i)
            {
                if (openTypeGsubIRMultipleGlyphMatches(
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

        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRMultiple
    //
    // Test every effective GSUB Type 2 lookup in every face of the supplied
    // OpenType container, including flagged lookups supported by compiled GDEF.
    // ========================================================================

    static bool testOpenTypeGsubIRMultiple(const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Multiple: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        if (fontData.empty())
            return fail("empty font data");


        // ------------------------------------------------------------
        // Retain font bytes through OpenTypeContainer.
        // ------------------------------------------------------------

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

        size_t multipleLookupCount = 0;
        size_t flaggedLookupCount = 0;
        size_t multipleSubtables = 0;
        size_t extensionLookups = 0;

        size_t glyphComparisons = 0;
        size_t sequenceComparisons = 0;
        size_t outputGlyphComparisons = 0;

        size_t executionInputGlyphs = 0;
        size_t executionOutputGlyphs = 0;


        // ------------------------------------------------------------
        // Every face in TTF/OTF/TTC container.
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
            // GDEF is optional globally, but flagged lookups may require
            // specific GDEF data. The compiler/raw filter will enforce that.
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
            // GSUB LookupList.
            // ------------------------------------------------------------

            OpenTypeLayoutLookupListView lookups;

            if (!openTypeGsubIRMultipleTestLookupList(
                gsub->data, lookups))
            {
                return fail("invalid GSUB LookupList");
            }

            lookupCount += lookups.size();


            // ------------------------------------------------------------
            // Every effective Type 2 lookup.
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

                if (effectiveType != 2)
                    continue;

                if (lookup.lookupFlag() != 0)
                    ++flaggedLookupCount;

                ++multipleLookupCount;

                if (!testOpenTypeGsubIRMultipleLookup(
                    lookup,
                    gdef,
                    lookupIndex,
                    glyphComparisons,
                    sequenceComparisons,
                    outputGlyphComparisons,
                    executionInputGlyphs,
                    executionOutputGlyphs,
                    multipleSubtables,
                    extensionLookups))
                {
                    return false;
                }
            }
        }


        // ------------------------------------------------------------
        // Require actual coverage.
        // ------------------------------------------------------------

        if (faceCount == 0)
            return fail("container produced no font faces");

        if (gsubFaceCount == 0)
            return fail("font contains no GSUB table");

        if (multipleLookupCount == 0)
            return fail("font contains no supported GSUB Type 2 lookup");


        // ------------------------------------------------------------
        // Summary.
        // ------------------------------------------------------------

        std::printf(
            "GSUB IR Multiple: PASS\n"
            "  Faces:                    %zu\n"
            "  Faces with GSUB:          %zu\n"
            "  GSUB lookups:             %zu\n"
            "  Type 2 lookups tested:    %zu\n"
            "  Flagged Type 2 tested:    %zu\n"
            "  Type 2 subtables:         %zu\n"
            "  Extension Type 2 lookups: %zu\n"
            "  Glyph resolver checks:    %zu\n"
            "  Matched sequences:        %zu\n"
            "  Sequence glyph checks:    %zu\n"
            "  Execution input glyphs:   %zu\n"
            "  Execution output glyphs:  %zu\n",
            faceCount,
            gsubFaceCount,
            lookupCount,
            multipleLookupCount,
            flaggedLookupCount,
            multipleSubtables,
            extensionLookups,
            glyphComparisons,
            sequenceComparisons,
            outputGlyphComparisons,
            executionInputGlyphs,
            executionOutputGlyphs);

        return true;
    }


    // ========================================================================
    // Filename convenience overload.
    // ========================================================================

    static bool testOpenTypeGsubIRMultiple(const char* fontFilename)
    {
        std::vector<uint8_t> fontBytes;

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "GSUB IR Multiple: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return testOpenTypeGsubIRMultiple(
            ByteSpan(fontBytes.data(), fontBytes.size()));
    }

} // namespace waavs