// test_opentype_gsub_chain_context_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGsubChainApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubChainApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGsubChainApplyU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubChainApplyU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }



    static void appendGsubChainApplyCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, glyphId);
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 0);

        appendGsubChainApplyU16(data, 12);
        appendGsubChainApplyU16(data, 0);
        appendGsubChainApplyU16(data, 0);
        appendGsubChainApplyU16(data, 0);

        appendGsubChainApplyU16(data, 2);
        appendGsubChainApplyU16(data, 1);

        appendGsubChainApplyU16(data, 100);
        appendGsubChainApplyU16(data, 102);
        appendGsubChainApplyU16(data, 3);

        return data;
    }


    // ====================================================================
    // Type 6 Format 3 main subtable.
    //
    // Chain:
    //
    //   9 | 10 20 30 | 40
    //
    // Actions:
    //
    //   sequenceIndex 1 -> Lookup 1
    //   sequenceIndex 3 -> Lookup 2
    //
    // Lookup 1:
    //
    //   20 -> 200 201 202
    //
    // Current input sequence becomes:
    //
    //   10 200 201 202 30
    //
    // Therefore sequenceIndex 3 then targets 202.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyMainSubtable()
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 3);


        // Backtrack.

        appendGsubChainApplyU16(data, 1);

        const size_t backtrackPatch = data.size();
        appendGsubChainApplyU16(data, 0);


        // Input.

        appendGsubChainApplyU16(data, 3);

        const size_t input0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t input1Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t input2Patch = data.size();
        appendGsubChainApplyU16(data, 0);


        // Lookahead.

        appendGsubChainApplyU16(data, 1);

        const size_t lookaheadPatch = data.size();
        appendGsubChainApplyU16(data, 0);


        // SequenceLookup records.

        appendGsubChainApplyU16(data, 2);

        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 1);

        // Deliberately beyond the original highest sequenceIndex of 2.
        // Lookup 1 expands the sequence enough to make index 3 valid.

        appendGsubChainApplyU16(data, 3);
        appendGsubChainApplyU16(data, 2);


        auto appendCoverage = [&](size_t patch, uint16_t glyphId)
            {
                const size_t offset = data.size();

                patchGsubChainApplyU16(data, patch, static_cast<uint16_t>(offset));
                appendGsubChainApplyCoverage(data, glyphId);
            };


        appendCoverage(backtrackPatch, 9);

        appendCoverage(input0Patch, 10);
        appendCoverage(input1Patch, 20);
        appendCoverage(input2Patch, 30);

        appendCoverage(lookaheadPatch, 40);

        return data;
    }


    // ====================================================================
    // Type 6 Format 3 input-only subtable.
    //
    // Used for:
    //
    //   zero-action matches
    //   lookahead resume
    //   nested Type 6
    //   recursion testing
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyInputOnlySubtable(
        uint16_t glyphId, bool withAction = false, uint16_t lookupListIndex = 0)
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 3);

        // Backtrack count.

        appendGsubChainApplyU16(data, 0);


        // Input count and Coverage offset.

        appendGsubChainApplyU16(data, 1);

        const size_t inputPatch = data.size();
        appendGsubChainApplyU16(data, 0);


        // Lookahead count.

        appendGsubChainApplyU16(data, 0);


        // SequenceLookup count.

        appendGsubChainApplyU16(data, withAction ? 1 : 0);

        if (withAction)
        {
            appendGsubChainApplyU16(data, 0);
            appendGsubChainApplyU16(data, lookupListIndex);
        }


        const size_t coverageOffset = data.size();
        patchGsubChainApplyU16(data, inputPatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainApplyCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Append one Type 6 Lookup containing one or two subtables.
    // ====================================================================

    static void appendGsubChainApplyType6Lookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        const std::vector<uint8_t>& subtable0, uint16_t lookupFlag = 0,
        const std::vector<uint8_t>* subtable1 = nullptr)
    {
        patchGsubChainApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        const size_t lookupBase = data.size();
        const uint16_t subtableCount = subtable1 ? 2 : 1;

        appendGsubChainApplyU16(data, 6);
        appendGsubChainApplyU16(data, lookupFlag);
        appendGsubChainApplyU16(data, subtableCount);

        const size_t subtable0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        size_t subtable1Patch = 0;

        if (subtable1)
        {
            subtable1Patch = data.size();
            appendGsubChainApplyU16(data, 0);
        }


        const size_t subtable0Offset = data.size() - lookupBase;
        patchGsubChainApplyU16(data, subtable0Patch, static_cast<uint16_t>(subtable0Offset));

        data.insert(data.end(), subtable0.begin(), subtable0.end());


        if (subtable1)
        {
            const size_t subtable1Offset = data.size() - lookupBase;
            patchGsubChainApplyU16(data, subtable1Patch, static_cast<uint16_t>(subtable1Offset));

            data.insert(data.end(), subtable1->begin(), subtable1->end());
        }
    }

    // ====================================================================
// Extension Type 7 -> Type 6.
//
// The LookupFlag belongs to the outer Type 7 Lookup. The effective
// ChainContextSubst receives that same filtering policy.
// ====================================================================

    static void appendGsubChainApplyExtensionType6Lookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        const std::vector<uint8_t>& subtable, uint16_t lookupFlag = 0)
    {
        patchGsubChainApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        // LookupType 7
        // LookupFlag
        // SubTableCount 1
        // ExtensionSubst immediately follows the Lookup header.

        appendGsubChainApplyU16(data, 7);
        appendGsubChainApplyU16(data, lookupFlag);
        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 8);

        const size_t extensionBase = data.size();

        // ExtensionSubst Format 1.
        //
        // extensionLookupType = 6

        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 6);

        const size_t extensionOffsetPatch = data.size();
        appendGsubChainApplyU32(data, 0);

        const size_t chainOffset = data.size();

        patchGsubChainApplyU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(chainOffset - extensionBase));

        data.insert(data.end(), subtable.begin(), subtable.end());
    }


    // ====================================================================
    // Lookup 1 - MultipleSubst:
    //
    //   20 -> 200 201 202
    // ====================================================================

    static void appendGsubChainApplyMultipleLookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch)
    {
        patchGsubChainApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubChainApplyU16(data, 2);
        appendGsubChainApplyU16(data, 0);
        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 8);

        const size_t substBase = data.size();

        appendGsubChainApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubChainApplyU16(data, 0);

        appendGsubChainApplyU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubChainApplyU16(data, 0);


        const size_t sequenceOffset = data.size();
        patchGsubChainApplyU16(
            data, sequencePatch, static_cast<uint16_t>(sequenceOffset - substBase));

        appendGsubChainApplyU16(data, 3);
        appendGsubChainApplyU16(data, 200);
        appendGsubChainApplyU16(data, 201);
        appendGsubChainApplyU16(data, 202);


        const size_t coverageOffset = data.size();
        patchGsubChainApplyU16(
            data, coveragePatch, static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubChainApplyCoverage(data, 20);
    }


    // ====================================================================
    // SingleSubst Format 2.
    // ====================================================================

    static void appendGsubChainApplySingleLookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        uint16_t inputGlyph, uint16_t outputGlyph)
    {
        patchGsubChainApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 0);
        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, 8);

        const size_t substBase = data.size();

        appendGsubChainApplyU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubChainApplyU16(data, 0);

        appendGsubChainApplyU16(data, 1);
        appendGsubChainApplyU16(data, outputGlyph);

        const size_t coverageOffset = data.size();
        patchGsubChainApplyU16(
            data, coveragePatch, static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubChainApplyCoverage(data, inputGlyph);
    }


    // ====================================================================
    // Main LookupList.
    //
    //   Lookup 0 -> Type 6 ChainContextSubst
    //   Lookup 1 -> MultipleSubst 20 -> 200 201 202
    //   Lookup 2 -> SingleSubst finalInputGlyph -> 250
    //
    // When resumeLookahead is true:
    //
    //   Lookup 0 gets a second Type 6 subtable:
    //
    //       input 40 -> SequenceLookup { 0, 3 }
    //
    //   Lookup 3 -> SingleSubst 40 -> 400
    //
    // This proves that lookahead remains eligible after the first chain.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyLookupList(
        uint16_t finalInputGlyph = 202, uint16_t chainLookupFlag = 0,
        bool resumeLookahead = false)
    {
        std::vector<uint8_t> data;

        const uint16_t lookupCount = resumeLookahead ? 4 : 3;

        appendGsubChainApplyU16(data, lookupCount);

        const size_t lookup0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        size_t lookup3Patch = 0;

        if (resumeLookahead)
        {
            lookup3Patch = data.size();
            appendGsubChainApplyU16(data, 0);
        }


        const std::vector<uint8_t> mainSubtable =
            makeGsubChainApplyMainSubtable();

        if (resumeLookahead)
        {
            const std::vector<uint8_t> lookaheadSubtable =
                makeGsubChainApplyInputOnlySubtable(40, true, 3);

            appendGsubChainApplyType6Lookup(
                data, lookup0Patch, mainSubtable,
                chainLookupFlag, &lookaheadSubtable);
        }
        else
        {
            appendGsubChainApplyType6Lookup(
                data, lookup0Patch, mainSubtable, chainLookupFlag);
        }

        appendGsubChainApplyMultipleLookup(data, lookup1Patch);
        appendGsubChainApplySingleLookup(data, lookup2Patch, finalInputGlyph, 250);

        if (resumeLookahead)
            appendGsubChainApplySingleLookup(data, lookup3Patch, 40, 400);

        return data;
    }

    static std::vector<uint8_t> makeGsubChainApplyExtensionLookupList(
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const std::vector<uint8_t> mainSubtable =
            makeGsubChainApplyMainSubtable();

        appendGsubChainApplyExtensionType6Lookup(
            data, lookup0Patch, mainSubtable, lookupFlag);

        appendGsubChainApplyMultipleLookup(data, lookup1Patch);
        appendGsubChainApplySingleLookup(data, lookup2Patch, 202, 250);

        return data;
    }



    // ====================================================================
    // Zero-action Type 6 LookupList.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyZeroActionLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 1);

        const size_t lookup0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const std::vector<uint8_t> subtable =
            makeGsubChainApplyInputOnlySubtable(10);

        appendGsubChainApplyType6Lookup(data, lookup0Patch, subtable);

        return data;
    }


    // ====================================================================
    // Successful nested Type 6.
    //
    // Lookup 0:
    //
    //   Type 6 input 10 -> Lookup 1
    //
    // Lookup 1:
    //
    //   Type 6 input 10 -> Lookup 2
    //
    // Lookup 2:
    //
    //   SingleSubst 10 -> 11
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyNestedType6LookupList()
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubChainApplyU16(data, 0);


        const std::vector<uint8_t> outerSubtable =
            makeGsubChainApplyInputOnlySubtable(10, true, 1);

        const std::vector<uint8_t> innerSubtable =
            makeGsubChainApplyInputOnlySubtable(10, true, 2);

        appendGsubChainApplyType6Lookup(data, lookup0Patch, outerSubtable);
        appendGsubChainApplyType6Lookup(data, lookup1Patch, innerSubtable);
        appendGsubChainApplySingleLookup(data, lookup2Patch, 10, 11);

        return data;
    }


    // ====================================================================
    // Recursive Type 6.
    //
    // Lookup 0:
    //
    //   Type 6 input 10
    //   SequenceLookup { 0, 0 }
    //
    // Nesting protection must reject this and the outer transactional Lookup
    // must leave the original buffer unchanged.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainApplyRecursiveLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubChainApplyU16(data, 1);

        const size_t lookup0Patch = data.size();
        appendGsubChainApplyU16(data, 0);

        const std::vector<uint8_t> recursiveSubtable =
            makeGsubChainApplyInputOnlySubtable(10, true, 0);

        appendGsubChainApplyType6Lookup(data, lookup0Patch, recursiveSubtable);

        return data;
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static void appendGsubChainApplyGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubChainApplyBuffer()
    {
        OpenTypeShapingBuffer buffer;

        appendGsubChainApplyGlyph(buffer, 9, 0);
        appendGsubChainApplyGlyph(buffer, 10, 1);
        appendGsubChainApplyGlyph(buffer, 20, 2);
        appendGsubChainApplyGlyph(buffer, 30, 3);
        appendGsubChainApplyGlyph(buffer, 40, 4);

        return buffer;
    }


    // Physical:
    //
    //   9 M 10 M 20 M 30 M 40
    //
    // With IgnoreMarks the logical chain is still:
    //
    //   9 | 10 20 30 | 40

    static OpenTypeShapingBuffer makeGsubChainApplyFilteredBuffer()
    {
        OpenTypeShapingBuffer buffer;

        appendGsubChainApplyGlyph(buffer, 9, 0);
        appendGsubChainApplyGlyph(buffer, 100, 1);

        appendGsubChainApplyGlyph(buffer, 10, 2);
        appendGsubChainApplyGlyph(buffer, 101, 3);

        appendGsubChainApplyGlyph(buffer, 20, 4);
        appendGsubChainApplyGlyph(buffer, 102, 5);

        appendGsubChainApplyGlyph(buffer, 30, 6);
        appendGsubChainApplyGlyph(buffer, 100, 7);

        appendGsubChainApplyGlyph(buffer, 40, 8);

        return buffer;
    }


    static bool gsubChainApplyGlyphsEqual(
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


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGsubChainContextApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB ChainContextSubst apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Type 6 exact-position execution.
        //
        // This is the central dynamic sequenceIndex test.
        //
        // Before:
        //
        //   9 | 10 20 30 | 40
        //
        // Lookup 1:
        //
        //   20 -> 200 201 202
        //
        // Lookup 2 then resolves sequenceIndex 3 against:
        //
        //   10 200 201 202 30
        //
        // producing:
        //
        //   9 | 10 200 201 250 30 | 40
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 1 LookupList");

            OpenTypeShapingBuffer buffer = makeGsubChainApplyBuffer();

            OpenTypeGsubApplyState state;
            OpenTypeGsubEditLog edits;
            const OpenTypeGdefView gdef{};

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubLookupAt(lookups, 0, gdef, buffer, 1, state, edits);

            const uint32_t expected[] =
            {
                9,
                10, 200, 201, 250, 30,
                40
            };

            if (result != OpenTypeGsubApplyAtResult::Match)
                return fail("case 1 result");

            if (!gsubChainApplyGlyphsEqual(buffer, expected, 7))
                return fail("case 1 dynamic sequenceIndex");

            if (edits.size() != 2)
                return fail("case 1 edit count");

            if (edits[0].inputPositions.size() != 1 ||
                edits[0].inputPositions[0] != 2 ||
                edits[0].outputCount != 3)
            {
                return fail("case 1 expansion edit");
            }

            if (edits[1].inputPositions.size() != 1 ||
                edits[1].inputPositions[0] != 4 ||
                edits[1].outputCount != 1)
            {
                return fail("case 1 later edit coordinates");
            }

            if (buffer[2].scalarOffset != 2 ||
                buffer[3].scalarOffset != 2 ||
                buffer[4].scalarOffset != 2)
            {
                return fail("case 1 provenance");
            }

            if (state.nestingDepth != 0)
                return fail("case 1 nesting state");

            ++passed;
        }


        // ====================================================================
        // Case 2 - IgnoreMarks through execution.
        //
        // Only matched input members enter SequenceState. Ignored marks remain
        // physical non-members while generated MultipleSubst outputs become
        // current input members.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyLookupList(202, 0x0008u);

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const std::vector<uint8_t> gdefData = makeGsubChainApplyGdef();
            const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

            if (!gdef)
                return fail("case 2 GDEF");

            OpenTypeShapingBuffer buffer = makeGsubChainApplyFilteredBuffer();

            OpenTypeGsubApplyState state;
            OpenTypeGsubEditLog edits;

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubLookupAt(lookups, 0, gdef, buffer, 2, state, edits);

            const uint32_t expected[] =
            {
                9, 100,
                10, 101,
                200, 201, 250, 102,
                30, 100,
                40
            };

            if (result != OpenTypeGsubApplyAtResult::Match)
                return fail("case 2 result");

            if (!gsubChainApplyGlyphsEqual(buffer, expected, 11))
                return fail("case 2 filtered execution");

            if (edits.size() != 2 ||
                edits[0].inputPositions.size() != 1 ||
                edits[0].inputPositions[0] != 4 ||
                edits[0].outputCount != 3 ||
                edits[1].inputPositions.size() != 1 ||
                edits[1].inputPositions[0] != 6 ||
                edits[1].outputCount != 1)
            {
                return fail("case 2 edit coordinates");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Whole LookupList Type 6 dispatcher.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer = makeGsubChainApplyBuffer();
            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 3 dispatcher");

            const uint32_t expected[] =
            {
                9,
                10, 200, 201, 250, 30,
                40
            };

            if (!gsubChainApplyGlyphsEqual(buffer, expected, 7))
                return fail("case 3 result");

            ++passed;
        }


        // ====================================================================
        // Case 4 - Backtrack and lookahead gate execution.
        //
        // Neither context-only region may itself be substituted, but either
        // one failing must prevent the input actions from running.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));
            const OpenTypeGdefView gdef{};


            // Bad backtrack.

            {
                OpenTypeShapingBuffer buffer;

                appendGsubChainApplyGlyph(buffer, 8, 0);
                appendGsubChainApplyGlyph(buffer, 10, 1);
                appendGsubChainApplyGlyph(buffer, 20, 2);
                appendGsubChainApplyGlyph(buffer, 30, 3);
                appendGsubChainApplyGlyph(buffer, 40, 4);

                OpenTypeGsubApplyState state;
                OpenTypeGsubEditLog edits;

                const OpenTypeGsubApplyAtResult result =
                    applyOpenTypeGsubLookupAt(
                        lookups, 0, gdef, buffer, 1, state, edits);

                const uint32_t expected[] = { 8, 10, 20, 30, 40 };

                if (result != OpenTypeGsubApplyAtResult::NoMatch)
                    return fail("case 4 bad backtrack result");

                if (!edits.empty() ||
                    !gsubChainApplyGlyphsEqual(buffer, expected, 5))
                {
                    return fail("case 4 bad backtrack mutation");
                }
            }


            // Bad lookahead.

            {
                OpenTypeShapingBuffer buffer;

                appendGsubChainApplyGlyph(buffer, 9, 0);
                appendGsubChainApplyGlyph(buffer, 10, 1);
                appendGsubChainApplyGlyph(buffer, 20, 2);
                appendGsubChainApplyGlyph(buffer, 30, 3);
                appendGsubChainApplyGlyph(buffer, 41, 4);

                OpenTypeGsubApplyState state;
                OpenTypeGsubEditLog edits;

                const OpenTypeGsubApplyAtResult result =
                    applyOpenTypeGsubLookupAt(
                        lookups, 0, gdef, buffer, 1, state, edits);

                const uint32_t expected[] = { 9, 10, 20, 30, 41 };

                if (result != OpenTypeGsubApplyAtResult::NoMatch)
                    return fail("case 4 bad lookahead result");

                if (!edits.empty() ||
                    !gsubChainApplyGlyphsEqual(buffer, expected, 5))
                {
                    return fail("case 4 bad lookahead mutation");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Nested NoMatch.
        //
        // Lookup 2 covers 999 instead of 202.
        //
        // Lookup 1 still expands 20. Lookup 2 simply returns NoMatch; the
        // successful outer Type 6 match remains valid.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyLookupList(999);

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer = makeGsubChainApplyBuffer();
            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 5 outer chain rejected nested NoMatch");

            const uint32_t expected[] =
            {
                9,
                10, 200, 201, 202, 30,
                40
            };

            if (!gsubChainApplyGlyphsEqual(buffer, expected, 7))
                return fail("case 5 result");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Lookahead resume boundary.
        //
        // Lookup 0 contains two Type 6 subtables.
        //
        // First:
        //
        //   9 | 10 20 30 | 40
        //
        // After it executes:
        //
        //   9 | 10 200 201 250 30 | 40
        //
        // The resume position must be the current one-past INPUT boundary,
        // which is exactly the physical position of 40.
        //
        // The second Type 6 subtable then matches:
        //
        //   input 40
        //
        // and applies Lookup 3:
        //
        //   40 -> 400
        //
        // If the outer executor incorrectly resumes after lookahead, 40 will
        // remain unchanged and this case will fail.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyLookupList(202, 0, true);

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 4)
                return fail("case 6 LookupList");

            OpenTypeShapingBuffer buffer = makeGsubChainApplyBuffer();
            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 6 dispatcher");

            const uint32_t expected[] =
            {
                9,
                10, 200, 201, 250, 30,
                400
            };

            if (!gsubChainApplyGlyphsEqual(buffer, expected, 7))
                return fail("case 6 lookahead resume");

            ++passed;
        }


        // ====================================================================
        // Case 7 - Zero-action Match.
        //
        // A successful Type 6 match with no SequenceLookup records must still
        // advance the outer scanner and complete successfully.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyZeroActionLookupList();

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubChainApplyGlyph(buffer, 10, 0);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 7 zero-action match rejected");

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 10 ||
                buffer[0].scalarOffset != 0 ||
                buffer[0].scalarCount != 1)
            {
                return fail("case 7 buffer changed");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Successful nested Type 6.
        //
        // This specifically exercises effective Type 6 in
        // applyOpenTypeGsubLookupAt().
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyNestedType6LookupList();

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubChainApplyGlyph(buffer, 10, 7);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 8 nested Type 6 rejected");

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 11 ||
                buffer[0].scalarOffset != 7 ||
                buffer[0].scalarCount != 1)
            {
                return fail("case 8 nested Type 6 result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Recursive Type 6 protection and transactional rollback.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyRecursiveLookupList();

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubChainApplyGlyph(buffer, 10, 0);

            const OpenTypeGdefView gdef{};

            if (applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 9 recursive lookup accepted");

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 10 ||
                buffer[0].scalarOffset != 0 ||
                buffer[0].scalarCount != 1)
            {
                return fail("case 9 rollback");
            }

            ++passed;
        }

        // ====================================================================
        // Case 10 - Extension Type 7 -> Type 6.
        //
        // The outer Type 7 Lookup has IgnoreMarks. This proves that its
        // LookupFlag governs matching by the effective Type 6 subtable.
        //
        // Physical:
        //
        //   9 M 10 M 20 M 30 M 40
        //
        // Effective logical chain:
        //
        //   9 | 10 20 30 | 40
        //
        // Result:
        //
        //   9 M 10 M 200 201 250 M 30 M 40
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGsubChainApplyExtensionLookupList(0x0008u);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 10 LookupList");

            const OpenTypeLayoutLookupView lookup = lookups.lookup(0);

            if (!lookup || lookup.lookupType() != 7)
                return fail("case 10 extension lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGsubEffectiveLookupType(lookup, effectiveType) ||
                effectiveType != 6)
            {
                return fail("case 10 effective type");
            }

            const std::vector<uint8_t> gdefData = makeGsubChainApplyGdef();
            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            if (!gdef)
                return fail("case 10 GDEF");

            OpenTypeShapingBuffer buffer =
                makeGsubChainApplyFilteredBuffer();

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 10 Type 7 dispatcher");

            const uint32_t expected[] =
            {
                9, 100,
                10, 101,
                200, 201, 250, 102,
                30, 100,
                40
            };

            if (!gsubChainApplyGlyphsEqual(buffer, expected, 11))
                return fail("case 10 Type 7 -> Type 6");

            ++passed;
        }


        std::printf(
            "OpenType GSUB ChainContextSubst apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Dynamic sequenceIndex:     PASS\n"
            "  Filtered execution:        PASS\n"
            "  Type 6 dispatcher:         PASS\n"
            "  Context gating:            PASS\n"
            "  Nested NoMatch:            PASS\n"
            "  Lookahead resume:          PASS\n"
            "  Zero-action match:         PASS\n"
            "  Nested Type 6:             PASS\n"
            "  Recursive rollback:        PASS\n"
            "  Type 7 -> Type 6:          PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs