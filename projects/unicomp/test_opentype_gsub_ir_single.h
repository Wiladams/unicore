// test_opentype_gsub_ir_single.h
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
#include "opentype_gsub_single_view.h"

#include "opentype_shaping_ir.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"


namespace waavs
{
    // ========================================================================
    // openTypeGsubIRSingleResultMatches
    // ========================================================================

    static bool openTypeGsubIRSingleResultMatches(
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
    // openTypeGsubIRSingleGlyphMatches
    //
    // SingleSubst may alter only glyphId.
    //
    // The differential test nevertheless compares the entire shaping record
    // so provenance changes are caught.
    // ========================================================================

    static bool openTypeGsubIRSingleGlyphMatches(
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
    // openTypeGsubIRSingleBufferMatches
    // ========================================================================

    static bool openTypeGsubIRSingleBufferMatches(
        const OpenTypeShapingBuffer& a,
        const OpenTypeShapingBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!openTypeGsubIRSingleGlyphMatches(a[i], b[i]))
                return false;
        }

        return true;
    }


    // ========================================================================
    // openTypeGsubIRTestLookupList
    //
    // Extract LookupList directly from the GSUB header.
    //
    // GSUB 1.x begins:
    //
    //     uint16 majorVersion
    //     uint16 minorVersion
    //     Offset16 scriptListOffset
    //     Offset16 featureListOffset
    //     Offset16 lookupListOffset
    //
    // FeatureVariations in GSUB 1.1 is irrelevant to this test.
    // ========================================================================

    static bool openTypeGsubIRTestLookupList(
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
    // makeOpenTypeGsubIRSingleExecutionBuffer
    //
    // Build a runtime buffer containing every input glyph represented by the
    // compiled lookup, plus several likely non-matching boundary glyphs.
    //
    // Provenance is deliberately populated so the differential executor test
    // proves that SingleSubst leaves it untouched.
    // ========================================================================

    static bool makeOpenTypeGsubIRSingleExecutionBuffer(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRLookup& lookup,
        OpenTypeShapingBuffer& result)
    {
        result.clear();

        if (!openTypeGsubIRSingleLookupValid(ir, lookup))
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


        // Every compiled input glyph.

        const size_t firstSubtable = lookup.payloadOffset;

        for (size_t i = 0; i < lookup.payloadCount; ++i)
        {
            const OpenTypeShapingIRGsubSingleSubtable& subtable =
                ir.gsubSingleSubtables[firstSubtable + i];

            if (!openTypeGsubIRSingleSubtableValid(ir, subtable))
                return false;

            for (size_t pairIndex = 0; pairIndex < subtable.pairCount; ++pairIndex)
            {
                const OpenTypeShapingIRGsubSinglePair& pair =
                    ir.gsubSinglePairs[subtable.pairOffset + pairIndex];

                appendGlyph(pair.input);
            }
        }

        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRSingleLookup
    //
    // Differential proof for one effective GSUB Type 1 lookup.
    //
    // Part 1:
    //
    //     old resolver == IR resolver
    //
    // for the complete uint16 glyph domain.
    //
    // Part 2:
    //
    //     old whole-lookup executor == IR whole-lookup executor
    //
    // including source and ligature provenance.
    // ========================================================================

    static bool testOpenTypeGsubIRSingleLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        uint16_t sourceLookupIndex,
        size_t& glyphComparisons,
        size_t& executionGlyphs,
        size_t& format1Subtables,
        size_t& format2Subtables,
        size_t& extensionLookups)
    {
        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(lookup, effectiveType) ||
            effectiveType != 1)
        {
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

        if (!compileOpenTypeGsubSingleLookup(
            lookup, gdef, ir, compiledLookupId))
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Lookup: %u\n"
                "  Compilation failed\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        const OpenTypeShapingIRLookup* compiledLookup =
            ir.lookup(compiledLookupId);

        if (!compiledLookup ||
            compiledLookup->op != OpenTypeShapingIROp::GsubSingle)
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Lookup: %u\n"
                "  Invalid compiled lookup\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }


        // ------------------------------------------------------------
        // Count source formats for diagnostics.
        // ------------------------------------------------------------

        for (uint16_t i = 0; i < lookup.subtableCount(); ++i)
        {
            const ByteSpan data =
                openTypeGsubIREffectiveSubtable(
                    lookup, 1, i);

            if (!data)
                return false;

            const OpenTypeGsubSingleSubstView single(data);

            if (!single)
                return false;

            if (single.format() == 1)
                ++format1Subtables;
            else if (single.format() == 2)
                ++format2Subtables;
            else
                return false;
        }


        // ------------------------------------------------------------
        // Exhaustive resolver comparison.
        //
        // This is deliberately independent of the compiled pair list.
        // If the compiler accidentally omits or invents an input glyph, this
        // scan will expose it.
        // ------------------------------------------------------------

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t oldReplacement = 0;
            uint16_t irReplacement = 0;

            const OpenTypeGsubResolveResult oldResult =
                resolveOpenTypeGsubSingleLookup(
                    lookup, glyphId, oldReplacement);

            const OpenTypeShapingIRResult irResult =
                resolveOpenTypeGsubIRSingleLookup(
                    ir,
                    *compiledLookup,
                    static_cast<uint16_t>(glyphId),
                    irReplacement);

            ++glyphComparisons;

            if (!openTypeGsubIRSingleResultMatches(
                oldResult, irResult))
            {
                std::printf(
                    "GSUB IR Single: FAIL\n"
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

            if (oldResult == OpenTypeGsubResolveResult::Match &&
                oldReplacement != irReplacement)
            {
                std::printf(
                    "GSUB IR Single: FAIL\n"
                    "  Lookup:       %u\n"
                    "  Glyph:        %u\n"
                    "  Old output:   %u\n"
                    "  IR output:    %u\n",
                    static_cast<unsigned>(sourceLookupIndex),
                    static_cast<unsigned>(glyphId),
                    static_cast<unsigned>(oldReplacement),
                    static_cast<unsigned>(irReplacement));

                return false;
            }
        }


        // ------------------------------------------------------------
        // Whole-lookup execution comparison.
        // ------------------------------------------------------------

        OpenTypeShapingBuffer input;

        if (!makeOpenTypeGsubIRSingleExecutionBuffer(
            ir, *compiledLookup, input))
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Lookup: %u\n"
                "  Unable to build execution buffer\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        OpenTypeShapingBuffer oldBuffer = input;
        OpenTypeShapingBuffer irBuffer = input;

        if (!applyOpenTypeGsubSingleLookup(
            lookup, gdef, oldBuffer))
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Lookup: %u\n"
                "  Old executor failed\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        if (!applyOpenTypeGsubIRLookup(
            ir, compiledLookupId, irBuffer))
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Lookup: %u\n"
                "  IR executor failed\n",
                static_cast<unsigned>(sourceLookupIndex));

            return false;
        }

        if (!openTypeGsubIRSingleBufferMatches(
            oldBuffer, irBuffer))
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Lookup: %u\n"
                "  Whole-buffer result mismatch\n"
                "  Old glyphs: %zu\n"
                "  IR glyphs:  %zu\n",
                static_cast<unsigned>(sourceLookupIndex),
                oldBuffer.size(),
                irBuffer.size());

            const size_t count =
                oldBuffer.size() < irBuffer.size()
                ? oldBuffer.size()
                : irBuffer.size();

            for (size_t i = 0; i < count; ++i)
            {
                if (openTypeGsubIRSingleGlyphMatches(
                    oldBuffer[i], irBuffer[i]))
                {
                    continue;
                }

                std::printf(
                    "  First mismatch: %zu\n"
                    "    input gid: %u\n"
                    "    old gid:   %u\n"
                    "    IR gid:    %u\n"
                    "    old src:   [%u,%u)\n"
                    "    IR src:    [%u,%u)\n",
                    i,
                    static_cast<unsigned>(input[i].glyphId),
                    static_cast<unsigned>(oldBuffer[i].glyphId),
                    static_cast<unsigned>(irBuffer[i].glyphId),
                    static_cast<unsigned>(oldBuffer[i].scalarOffset),
                    static_cast<unsigned>(
                        oldBuffer[i].scalarOffset +
                        oldBuffer[i].scalarCount),
                    static_cast<unsigned>(irBuffer[i].scalarOffset),
                    static_cast<unsigned>(
                        irBuffer[i].scalarOffset +
                        irBuffer[i].scalarCount));

                break;
            }

            return false;
        }

        executionGlyphs += input.size();
        return true;
    }


    // ========================================================================
    // testOpenTypeGsubIRSingle
    //
    // Test every effective GSUB Type 1 lookup in every face of the
    // supplied OpenType container.
    // ========================================================================

    static bool testOpenTypeGsubIRSingle(const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "GSUB IR Single: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        if (fontData.empty())
            return fail("empty font data");


        // ------------------------------------------------------------
        // Retain the font bytes through OpenTypeContainer.
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
        size_t singleLookupCount = 0;
        size_t flaggedLookupCount = 0;

        size_t format1Subtables = 0;
        size_t format2Subtables = 0;
        size_t extensionLookups = 0;

        size_t glyphComparisons = 0;
        size_t executionGlyphs = 0;


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
            
            const TableRecord* gdefTable =
                tables->getTable(OTAG("GDEF"));

            OpenTypeGdefView gdef{};

            if (gdefTable)
            {
                gdef = OpenTypeGdefView(gdefTable->data);

                if (!gdef)
                    return fail("invalid GDEF table");
            }




            OpenTypeLayoutLookupListView lookups;

            if (!openTypeGsubIRTestLookupList(
                gsub->data, lookups))
            {
                return fail("invalid GSUB LookupList");
            }

            lookupCount += lookups.size();

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

                if (effectiveType != 1)
                    continue;

                if (lookup.lookupFlag() != 0)
                    ++flaggedLookupCount;

                ++singleLookupCount;

                if (!testOpenTypeGsubIRSingleLookup(
                    lookup,
                    gdef,
                    lookupIndex,
                    glyphComparisons,
                    executionGlyphs,
                    format1Subtables,
                    format2Subtables,
                    extensionLookups))
                {
                    return false;
                }
            }
        }


        // ------------------------------------------------------------
        // This test must actually exercise the new operation.
        // ------------------------------------------------------------

        if (faceCount == 0)
            return fail("container produced no font faces");

        if (gsubFaceCount == 0)
            return fail("font contains no GSUB table");

        if (singleLookupCount == 0)
            return fail("font contains no supported GSUB Type 1 lookup");


        std::printf(
            "GSUB IR Single: PASS\n"
            "  Faces:                    %zu\n"
            "  Faces with GSUB:          %zu\n"
            "  GSUB lookups:             %zu\n"
            "  Type 1 lookups tested:    %zu\n"
            "  Flagged Type 1 tested:    %zu\n"
            "  Format 1 subtables:       %zu\n"
            "  Format 2 subtables:       %zu\n"
            "  Extension Type 1 lookups: %zu\n"
            "  Glyph resolver checks:    %zu\n"
            "  Execution glyphs:         %zu\n",
            faceCount,
            gsubFaceCount,
            lookupCount,
            singleLookupCount,
            flaggedLookupCount,
            format1Subtables,
            format2Subtables,
            extensionLookups,
            glyphComparisons,
            executionGlyphs);

        return true;
    }


    // ========================================================================
    // Filename convenience overload.
    // ========================================================================

    static bool testOpenTypeGsubIRSingle(const char* fontFilename)
    {
        std::vector<uint8_t> fontBytes;

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "GSUB IR Single: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return testOpenTypeGsubIRSingle(
            ByteSpan(fontBytes.data(), fontBytes.size()));
    }

} // namespace waavs