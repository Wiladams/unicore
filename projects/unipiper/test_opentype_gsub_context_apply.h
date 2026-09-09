// test_opentype_gsub_context_apply.h
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
    // Binary construction helpers.
    // ====================================================================

    static void appendGsubContextApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGsubContextApplyU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubContextApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void patchGsubContextApplyU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGsubContextApplyCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, glyphId);
    }


    // ====================================================================
    // Type 5 Format 3 subtable.
    //
    // Context:
    //
    //   10 20 30
    //
    // Actions:
    //
    //   sequenceIndex 1 -> Lookup 1
    //   sequenceIndex 3 -> Lookup 2
    //
    // Lookup 1 expands:
    //
    //   20 -> 200 201 202
    //
    // Therefore the second action resolves index 3 against:
    //
    //   10 200 201 202 30
    //
    // and targets glyph 202.
    // ====================================================================

    static void appendGsubContextApplyType5Subtable(std::vector<uint8_t>& data)
    {
        const size_t base = data.size();

        appendGsubContextApplyU16(data, 3);
        appendGsubContextApplyU16(data, 3);
        appendGsubContextApplyU16(data, 2);

        const size_t coverage0Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGsubContextApplyU16(data, 0);


        // SequenceLookup 0:
        // sequenceIndex = 1
        // lookupListIndex = 1

        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 1);


        // SequenceLookup 1:
        //
        // This deliberately exceeds the original highest sequenceIndex of 2.
        // It becomes valid after Lookup 1 expands position 1.

        appendGsubContextApplyU16(data, 3);
        appendGsubContextApplyU16(data, 2);


        const size_t coverage0 = data.size();
        patchGsubContextApplyU16(data, coverage0Patch, static_cast<uint16_t>(coverage0 - base));
        appendGsubContextApplyCoverage(data, 10);

        const size_t coverage1 = data.size();
        patchGsubContextApplyU16(data, coverage1Patch, static_cast<uint16_t>(coverage1 - base));
        appendGsubContextApplyCoverage(data, 20);

        const size_t coverage2 = data.size();
        patchGsubContextApplyU16(data, coverage2Patch, static_cast<uint16_t>(coverage2 - base));
        appendGsubContextApplyCoverage(data, 30);
    }


    static void appendGsubContextApplyType5Lookup(std::vector<uint8_t>& data, size_t lookupOffsetPatch)
    {
        patchGsubContextApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        // LookupType 5
        // LookupFlag 0
        // SubTableCount 1
        // Subtable immediately follows 8-byte Lookup header.

        appendGsubContextApplyU16(data, 5);
        appendGsubContextApplyU16(data, 0);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 8);

        appendGsubContextApplyType5Subtable(data);
    }


    // ====================================================================
    // Lookup 1 - MultipleSubst.
    //
    //   20 -> 200 201 202
    // ====================================================================

    static void appendGsubContextApplyMultipleLookup(std::vector<uint8_t>& data, size_t lookupOffsetPatch)
    {
        patchGsubContextApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubContextApplyU16(data, 2);
        appendGsubContextApplyU16(data, 0);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 8);

        const size_t substBase = data.size();

        appendGsubContextApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubContextApplyU16(data, 0);

        appendGsubContextApplyU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubContextApplyU16(data, 0);


        // Sequence table.

        const size_t sequenceOffset = data.size();
        patchGsubContextApplyU16(data, sequencePatch, static_cast<uint16_t>(sequenceOffset - substBase));

        appendGsubContextApplyU16(data, 3);
        appendGsubContextApplyU16(data, 200);
        appendGsubContextApplyU16(data, 201);
        appendGsubContextApplyU16(data, 202);


        // Coverage.

        const size_t coverageOffset = data.size();
        patchGsubContextApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubContextApplyCoverage(data, 20);
    }


    // ====================================================================
    // Lookup 2 - SingleSubst Format 2.
    //
    //   inputGlyph -> 250
    //
    // inputGlyph is normally 202. Some tests deliberately use another
    // Coverage glyph to force a nested NoMatch.
    // ====================================================================

    static void appendGsubContextApplySingleLookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch, uint16_t inputGlyph)
    {
        patchGsubContextApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 0);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 8);

        const size_t substBase = data.size();

        appendGsubContextApplyU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubContextApplyU16(data, 0);

        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 250);

        const size_t coverageOffset = data.size();
        patchGsubContextApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubContextApplyCoverage(data, inputGlyph);
    }


    // ====================================================================
    // LookupList:
    //
    //   0 -> Type 5 ContextSubst
    //   1 -> Type 2 MultipleSubst
    //   2 -> Type 1 SingleSubst
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextApplyLookupList(uint16_t finalInputGlyph = 202)
    {
        std::vector<uint8_t> data;

        appendGsubContextApplyU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        appendGsubContextApplyType5Lookup(data, lookup0Patch);
        appendGsubContextApplyMultipleLookup(data, lookup1Patch);
        appendGsubContextApplySingleLookup(data, lookup2Patch, finalInputGlyph);

        return data;
    }


    // ====================================================================
    // Extension Type 7 -> Type 5.
    // ====================================================================

    static void appendGsubContextApplyExtensionType5Lookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch)
    {
        patchGsubContextApplyU16(data, lookupOffsetPatch, static_cast<uint16_t>(data.size()));

        appendGsubContextApplyU16(data, 7);
        appendGsubContextApplyU16(data, 0);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 8);

        const size_t extensionBase = data.size();

        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 5);

        const size_t extensionOffsetPatch = data.size();
        appendGsubContextApplyU32(data, 0);

        const size_t contextOffset = data.size();

        patchGsubContextApplyU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(contextOffset - extensionBase));

        appendGsubContextApplyType5Subtable(data);
    }


    static std::vector<uint8_t> makeGsubContextApplyExtensionLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubContextApplyU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendGsubContextApplyU16(data, 0);

        appendGsubContextApplyExtensionType5Lookup(data, lookup0Patch);
        appendGsubContextApplyMultipleLookup(data, lookup1Patch);
        appendGsubContextApplySingleLookup(data, lookup2Patch, 202);

        return data;
    }


    // ====================================================================
    // Recursive Type 5.
    //
    // Lookup 0:
    //
    //   context glyph 10
    //   SequenceLookup { 0, 0 }
    //
    // The lookup invokes itself forever unless the nesting guard works.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextApplyRecursiveLookupList()
    {
        std::vector<uint8_t> data;

        appendGsubContextApplyU16(data, 1);

        const size_t lookupPatch = data.size();
        appendGsubContextApplyU16(data, 0);

        patchGsubContextApplyU16(data, lookupPatch, static_cast<uint16_t>(data.size()));

        appendGsubContextApplyU16(data, 5);
        appendGsubContextApplyU16(data, 0);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 8);

        const size_t substBase = data.size();

        appendGsubContextApplyU16(data, 3);
        appendGsubContextApplyU16(data, 1);
        appendGsubContextApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubContextApplyU16(data, 0);

        appendGsubContextApplyU16(data, 0);
        appendGsubContextApplyU16(data, 0);

        const size_t coverageOffset = data.size();
        patchGsubContextApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset - substBase));

        appendGsubContextApplyCoverage(data, 10);

        return data;
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static void appendGsubContextApplyGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static bool gsubContextApplyGlyphsEqual(
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


    static bool testOpenTypeGsubContextApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB ContextSubst apply: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1 - MultipleSubst at one exact physical position.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 1 LookupList");

            OpenTypeShapingBuffer buffer;

            appendGsubContextApplyGlyph(buffer, 10, 0);
            appendGsubContextApplyGlyph(buffer, 20, 1);
            appendGsubContextApplyGlyph(buffer, 30, 2);

            OpenTypeGsubApplyState state;
            OpenTypeGsubEditLog edits;
            const OpenTypeGdefView gdef{};

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubLookupAt(lookups, 1, gdef, buffer, 1, state, edits);

            const uint32_t expected[] = { 10, 200, 201, 202, 30 };

            if (result != OpenTypeGsubApplyAtResult::Match)
                return fail("case 1 result");

            if (!gsubContextApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 1 glyphs");

            if (edits.size() != 1 ||
                edits[0].inputPositions.size() != 1 ||
                edits[0].inputPositions[0] != 1 ||
                edits[0].outputCount != 3)
            {
                return fail("case 1 edit");
            }

            if (buffer[1].scalarOffset != 1 ||
                buffer[2].scalarOffset != 1 ||
                buffer[3].scalarOffset != 1)
            {
                return fail("case 1 provenance");
            }

            if (state.nestingDepth != 0)
                return fail("case 1 nesting state");

            ++passed;
        }


        // ====================================================================
        // Case 2 - SingleSubst at one exact physical position.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubContextApplyGlyph(buffer, 202, 7);

            OpenTypeGsubApplyState state;
            OpenTypeGsubEditLog edits;
            const OpenTypeGdefView gdef{};

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubLookupAt(lookups, 2, gdef, buffer, 0, state, edits);

            if (result != OpenTypeGsubApplyAtResult::Match ||
                buffer.size() != 1 ||
                buffer[0].glyphId != 250)
            {
                return fail("case 2 substitution");
            }

            if (buffer[0].scalarOffset != 7 || buffer[0].scalarCount != 1)
                return fail("case 2 provenance");

            if (edits.size() != 1 ||
                edits[0].inputPositions.size() != 1 ||
                edits[0].inputPositions[0] != 0 ||
                edits[0].outputCount != 1)
            {
                return fail("case 2 edit");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Type 5 at-position execution.
        //
        // This is the central dynamic sequenceIndex test.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubContextApplyGlyph(buffer, 10, 0);
            appendGsubContextApplyGlyph(buffer, 20, 1);
            appendGsubContextApplyGlyph(buffer, 30, 2);

            OpenTypeGsubApplyState state;
            OpenTypeGsubEditLog edits;
            const OpenTypeGdefView gdef{};

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubLookupAt(lookups, 0, gdef, buffer, 0, state, edits);

            const uint32_t expected[] = { 10, 200, 201, 250, 30 };

            if (result != OpenTypeGsubApplyAtResult::Match)
                return fail("case 3 result");

            if (!gsubContextApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 3 dynamic sequenceIndex");

            if (edits.size() != 2)
                return fail("case 3 edit count");

            if (edits[0].inputPositions.size() != 1 ||
                edits[0].inputPositions[0] != 1 ||
                edits[0].outputCount != 3)
            {
                return fail("case 3 expansion edit");
            }

            // Coordinates for this edit are relative to the buffer AFTER
            // the preceding expansion.

            if (edits[1].inputPositions.size() != 1 ||
                edits[1].inputPositions[0] != 3 ||
                edits[1].outputCount != 1)
            {
                return fail("case 3 later edit coordinates");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Whole LookupList dispatcher.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubContextApplyGlyph(buffer, 10, 0);
            appendGsubContextApplyGlyph(buffer, 20, 1);
            appendGsubContextApplyGlyph(buffer, 30, 2);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 4 dispatcher");

            const uint32_t expected[] = { 10, 200, 201, 250, 30 };

            if (!gsubContextApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 4 result");

            if (buffer[1].scalarOffset != 1 ||
                buffer[2].scalarOffset != 1 ||
                buffer[3].scalarOffset != 1)
            {
                return fail("case 4 provenance");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Nested lookup NoMatch does not invalidate outer context.
        //
        // Lookup 2 covers glyph 999 instead of 202.
        //
        // The first nested expansion still occurs. The second nested lookup
        // simply does nothing.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyLookupList(999);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubContextApplyGlyph(buffer, 10, 0);
            appendGsubContextApplyGlyph(buffer, 20, 1);
            appendGsubContextApplyGlyph(buffer, 30, 2);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 5 outer context rejected nested NoMatch");

            const uint32_t expected[] = { 10, 200, 201, 202, 30 };

            if (!gsubContextApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 5 result");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Resume boundary.
        //
        // Two adjacent matching contexts must both execute.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubContextApplyGlyph(buffer, 10, 0);
            appendGsubContextApplyGlyph(buffer, 20, 1);
            appendGsubContextApplyGlyph(buffer, 30, 2);

            appendGsubContextApplyGlyph(buffer, 10, 3);
            appendGsubContextApplyGlyph(buffer, 20, 4);
            appendGsubContextApplyGlyph(buffer, 30, 5);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 6 dispatcher");

            const uint32_t expected[] =
            {
                10, 200, 201, 250, 30,
                10, 200, 201, 250, 30
            };

            if (!gsubContextApplyGlyphsEqual(buffer, expected, 10))
                return fail("case 6 resume boundary");

            ++passed;
        }


        // ====================================================================
        // Case 7 - Extension Type 7 -> Type 5.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyExtensionLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubContextApplyGlyph(buffer, 10, 0);
            appendGsubContextApplyGlyph(buffer, 20, 1);
            appendGsubContextApplyGlyph(buffer, 30, 2);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 7 Type 7 dispatcher");

            const uint32_t expected[] = { 10, 200, 201, 250, 30 };

            if (!gsubContextApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 7 Type 7 -> Type 5");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Recursive Type 5 protection and transactional rollback.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextApplyRecursiveLookupList();
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubContextApplyGlyph(buffer, 10, 0);

            const OpenTypeGdefView gdef{};

            if (applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 8 recursive lookup accepted");

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 10 ||
                buffer[0].scalarOffset != 0 ||
                buffer[0].scalarCount != 1)
            {
                return fail("case 8 rollback");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ContextSubst apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Multiple at-position:      PASS\n"
            "  Single at-position:        PASS\n"
            "  Dynamic sequenceIndex:     PASS\n"
            "  Type 5 dispatcher:         PASS\n"
            "  Nested NoMatch:            PASS\n"
            "  Resume boundary:           PASS\n"
            "  Type 7 -> Type 5:          PASS\n"
            "  Recursive rollback:        PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs