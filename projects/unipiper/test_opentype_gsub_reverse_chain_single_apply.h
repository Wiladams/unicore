// test_opentype_gsub_reverse_chain_single_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGsubReverseApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubReverseApplyU16(
        std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGsubReverseApplyCoverage(
        std::vector<uint8_t>& data, std::initializer_list<uint16_t> glyphs)
    {
        appendGsubReverseApplyU16(data, 1);
        appendGsubReverseApplyU16(data, static_cast<uint16_t>(glyphs.size()));

        for (uint16_t glyphId : glyphs)
            appendGsubReverseApplyU16(data, glyphId);
    }


    // ====================================================================
    // Type 8 Format 1 builder.
    //
    // backtrack and lookahead values each create one singleton Coverage.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseApplySubtable(
        std::initializer_list<uint16_t> inputs,
        std::initializer_list<uint16_t> substitutes,
        std::initializer_list<uint16_t> backtrack,
        std::initializer_list<uint16_t> lookahead)
    {
        std::vector<uint8_t> data;

        appendGsubReverseApplyU16(data, 1);

        const size_t inputCoveragePatch = data.size();
        appendGsubReverseApplyU16(data, 0);


        // Backtrack Coverages.

        appendGsubReverseApplyU16(
            data, static_cast<uint16_t>(backtrack.size()));

        std::vector<size_t> backtrackPatches;
        backtrackPatches.reserve(backtrack.size());

        for (size_t i = 0; i < backtrack.size(); ++i)
        {
            backtrackPatches.push_back(data.size());
            appendGsubReverseApplyU16(data, 0);
        }


        // Lookahead Coverages.

        appendGsubReverseApplyU16(
            data, static_cast<uint16_t>(lookahead.size()));

        std::vector<size_t> lookaheadPatches;
        lookaheadPatches.reserve(lookahead.size());

        for (size_t i = 0; i < lookahead.size(); ++i)
        {
            lookaheadPatches.push_back(data.size());
            appendGsubReverseApplyU16(data, 0);
        }


        // Substitute array.

        appendGsubReverseApplyU16(
            data, static_cast<uint16_t>(substitutes.size()));

        for (uint16_t substitute : substitutes)
            appendGsubReverseApplyU16(data, substitute);


        // Input Coverage.

        const size_t inputCoverageOffset = data.size();

        patchGsubReverseApplyU16(
            data, inputCoveragePatch,
            static_cast<uint16_t>(inputCoverageOffset));

        appendGsubReverseApplyCoverage(data, inputs);


        // Backtrack Coverage tables.

        size_t index = 0;

        for (uint16_t glyphId : backtrack)
        {
            const size_t offset = data.size();

            patchGsubReverseApplyU16(
                data, backtrackPatches[index],
                static_cast<uint16_t>(offset));

            appendGsubReverseApplyCoverage(data, { glyphId });
            ++index;
        }


        // Lookahead Coverage tables.

        index = 0;

        for (uint16_t glyphId : lookahead)
        {
            const size_t offset = data.size();

            patchGsubReverseApplyU16(
                data, lookaheadPatches[index],
                static_cast<uint16_t>(offset));

            appendGsubReverseApplyCoverage(data, { glyphId });
            ++index;
        }

        return data;
    }


    // ====================================================================
    // Type 8 Lookup.
    // ====================================================================

    static void appendGsubReverseApplyType8Lookup(
        std::vector<uint8_t>& data, size_t lookupOffsetPatch,
        const std::vector<uint8_t>& subtable0, uint16_t lookupFlag = 0,
        const std::vector<uint8_t>* subtable1 = nullptr)
    {
        patchGsubReverseApplyU16(
            data, lookupOffsetPatch,
            static_cast<uint16_t>(data.size()));

        const size_t lookupBase = data.size();
        const uint16_t subtableCount = subtable1 ? 2 : 1;

        appendGsubReverseApplyU16(data, 8);
        appendGsubReverseApplyU16(data, lookupFlag);
        appendGsubReverseApplyU16(data, subtableCount);

        const size_t subtable0Patch = data.size();
        appendGsubReverseApplyU16(data, 0);

        size_t subtable1Patch = 0;

        if (subtable1)
        {
            subtable1Patch = data.size();
            appendGsubReverseApplyU16(data, 0);
        }


        const size_t subtable0Offset = data.size() - lookupBase;

        patchGsubReverseApplyU16(
            data, subtable0Patch,
            static_cast<uint16_t>(subtable0Offset));

        data.insert(data.end(), subtable0.begin(), subtable0.end());


        if (subtable1)
        {
            const size_t subtable1Offset = data.size() - lookupBase;

            patchGsubReverseApplyU16(
                data, subtable1Patch,
                static_cast<uint16_t>(subtable1Offset));

            data.insert(data.end(), subtable1->begin(), subtable1->end());
        }
    }


    static std::vector<uint8_t> makeGsubReverseApplyLookupList(
        const std::vector<uint8_t>& subtable0, uint16_t lookupFlag = 0,
        const std::vector<uint8_t>* subtable1 = nullptr)
    {
        std::vector<uint8_t> data;

        appendGsubReverseApplyU16(data, 1);

        const size_t lookupPatch = data.size();
        appendGsubReverseApplyU16(data, 0);

        appendGsubReverseApplyType8Lookup(
            data, lookupPatch, subtable0, lookupFlag, subtable1);

        return data;
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    // ====================================================================

    static std::vector<uint8_t> makeGsubReverseApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGsubReverseApplyU16(data, 1);
        appendGsubReverseApplyU16(data, 0);

        appendGsubReverseApplyU16(data, 12);
        appendGsubReverseApplyU16(data, 0);
        appendGsubReverseApplyU16(data, 0);
        appendGsubReverseApplyU16(data, 0);

        appendGsubReverseApplyU16(data, 2);
        appendGsubReverseApplyU16(data, 1);

        appendGsubReverseApplyU16(data, 100);
        appendGsubReverseApplyU16(data, 102);
        appendGsubReverseApplyU16(data, 3);

        return data;
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================
    static void patchGsubReverseApplyU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    static void appendGsubReverseApplyGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static bool gsubReverseApplyGlyphsEqual(
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

    static void appendGsubReverseApplyU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGsubReverseApplyExtensionSubtable(
        std::vector<uint8_t>& data, const std::vector<uint8_t>& subtable)
    {
        const size_t extensionBase = data.size();

        appendGsubReverseApplyU16(data, 1);
        appendGsubReverseApplyU16(data, 8);

        const size_t extensionOffsetPatch = data.size();
        appendGsubReverseApplyU32(data, 0);

        const size_t type8Offset = data.size();

        patchGsubReverseApplyU32(
            data, extensionOffsetPatch,
            static_cast<uint32_t>(type8Offset - extensionBase));

        data.insert(data.end(), subtable.begin(), subtable.end());
    }

    static std::vector<uint8_t> makeGsubReverseApplyExtensionLookupList(
        const std::vector<uint8_t>& subtable0, uint16_t lookupFlag = 0,
        const std::vector<uint8_t>* subtable1 = nullptr)
    {
        std::vector<uint8_t> data;

        appendGsubReverseApplyU16(data, 1);

        const size_t lookupPatch = data.size();
        appendGsubReverseApplyU16(data, 0);

        patchGsubReverseApplyU16(data, lookupPatch, static_cast<uint16_t>(data.size()));

        const size_t lookupBase = data.size();
        const uint16_t subtableCount = subtable1 ? 2 : 1;

        appendGsubReverseApplyU16(data, 7);
        appendGsubReverseApplyU16(data, lookupFlag);
        appendGsubReverseApplyU16(data, subtableCount);

        const size_t extension0Patch = data.size();
        appendGsubReverseApplyU16(data, 0);

        size_t extension1Patch = 0;

        if (subtable1)
        {
            extension1Patch = data.size();
            appendGsubReverseApplyU16(data, 0);
        }

        patchGsubReverseApplyU16(
            data, extension0Patch,
            static_cast<uint16_t>(data.size() - lookupBase));

        appendGsubReverseApplyExtensionSubtable(data, subtable0);

        if (subtable1)
        {
            patchGsubReverseApplyU16(
                data, extension1Patch,
                static_cast<uint16_t>(data.size() - lookupBase));

            appendGsubReverseApplyExtensionSubtable(data, *subtable1);
        }

        return data;
    }




    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGsubReverseChainSingleApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB ReverseChainSingleSubst apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - Exact-position Type 8.
        //
        //   11 12 | 21 | 30 31
        //
        // Coverage index 1 selects:
        //
        //   21 -> 201
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 20, 21 }, { 200, 201 },
                    { 12, 11 }, { 30, 31 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(subtable);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 1)
                return fail("case 1 LookupList");

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 11, 0);
            appendGsubReverseApplyGlyph(buffer, 12, 1);
            appendGsubReverseApplyGlyph(buffer, 21, 2);
            appendGsubReverseApplyGlyph(buffer, 30, 3);
            appendGsubReverseApplyGlyph(buffer, 31, 4);

            OpenTypeGsubApplyState state;
            OpenTypeGsubEditLog edits;
            const OpenTypeGdefView gdef{};

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubLookupAt(
                    lookups, 0, gdef, buffer, 2, state, edits);

            const uint32_t expected[] =
            {
                11, 12, 201, 30, 31
            };

            if (result != OpenTypeGsubApplyAtResult::Match)
                return fail("case 1 result");

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 1 substitution");

            if (edits.size() != 1 ||
                edits[0].inputPositions.size() != 1 ||
                edits[0].inputPositions[0] != 2 ||
                edits[0].outputCount != 1)
            {
                return fail("case 1 edit");
            }

            if (buffer[2].scalarOffset != 2 ||
                buffer[2].scalarCount != 1)
            {
                return fail("case 1 provenance");
            }

            if (state.nestingDepth != 0)
                return fail("case 1 nesting state");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Whole native Type 8 dispatcher.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 20 }, { 200 },
                    { 12, 11 }, { 30, 31 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(subtable);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 11, 0);
            appendGsubReverseApplyGlyph(buffer, 12, 1);
            appendGsubReverseApplyGlyph(buffer, 20, 2);
            appendGsubReverseApplyGlyph(buffer, 30, 3);
            appendGsubReverseApplyGlyph(buffer, 31, 4);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(
                lookups, 0, gdef, buffer))
            {
                return fail("case 2 dispatcher");
            }

            const uint32_t expected[] =
            {
                11, 12, 200, 30, 31
            };

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 5))
                return fail("case 2 result");

            ++passed;
        }


        // ====================================================================
        // Case 3 - Reverse scan order.
        //
        // Lookup contains two subtables.
        //
        // Subtable 0:
        //
        //   30 -> 300
        //
        // Subtable 1:
        //
        //   20 | lookahead 300 -> 200
        //
        // Start:
        //
        //   20 30
        //
        // Reverse scan:
        //
        //   first 30 -> 300
        //   then 20 sees lookahead 300 -> 200
        //
        // Result:
        //
        //   200 300
        //
        // A forward scan would incorrectly leave:
        //
        //   20 300
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable0 =
                makeGsubReverseApplySubtable(
                    { 30 }, { 300 }, {}, {});

            const std::vector<uint8_t> subtable1 =
                makeGsubReverseApplySubtable(
                    { 20 }, { 200 }, {}, { 300 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(
                    subtable0, 0, &subtable1);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 20, 0);
            appendGsubReverseApplyGlyph(buffer, 30, 1);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(
                lookups, 0, gdef, buffer))
            {
                return fail("case 3 dispatcher");
            }

            const uint32_t expected[] = { 200, 300 };

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 2))
                return fail("case 3 reverse scan");

            ++passed;
        }


        // ====================================================================
        // Case 4 - IgnoreMarks through execution.
        //
        // Physical:
        //
        //   11 M 12 M 20 M 30 M 31
        //
        // Logical context under IgnoreMarks:
        //
        //   11 12 | 20 | 30 31
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 20 }, { 200 },
                    { 12, 11 }, { 30, 31 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(
                    subtable, 0x0008u);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const std::vector<uint8_t> gdefData =
                makeGsubReverseApplyGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            if (!gdef)
                return fail("case 4 GDEF");

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 11, 0);
            appendGsubReverseApplyGlyph(buffer, 100, 1);
            appendGsubReverseApplyGlyph(buffer, 12, 2);
            appendGsubReverseApplyGlyph(buffer, 101, 3);
            appendGsubReverseApplyGlyph(buffer, 20, 4);
            appendGsubReverseApplyGlyph(buffer, 102, 5);
            appendGsubReverseApplyGlyph(buffer, 30, 6);
            appendGsubReverseApplyGlyph(buffer, 100, 7);
            appendGsubReverseApplyGlyph(buffer, 31, 8);

            if (!applyOpenTypeGsubLookup(
                lookups, 0, gdef, buffer))
            {
                return fail("case 4 dispatcher");
            }

            const uint32_t expected[] =
            {
                11, 100,
                12, 101,
                200, 102,
                30, 100,
                31
            };

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 9))
                return fail("case 4 filtered execution");

            if (buffer[4].scalarOffset != 4 ||
                buffer[4].scalarCount != 1)
            {
                return fail("case 4 provenance");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Context gating.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 20 }, { 200 },
                    { 12, 11 }, { 30, 31 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(subtable);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGdefView gdef{};


            // Bad backtrack.

            {
                OpenTypeShapingBuffer buffer;

                appendGsubReverseApplyGlyph(buffer, 11, 0);
                appendGsubReverseApplyGlyph(buffer, 13, 1);
                appendGsubReverseApplyGlyph(buffer, 20, 2);
                appendGsubReverseApplyGlyph(buffer, 30, 3);
                appendGsubReverseApplyGlyph(buffer, 31, 4);

                if (!applyOpenTypeGsubLookup(
                    lookups, 0, gdef, buffer))
                {
                    return fail("case 5 bad backtrack dispatcher");
                }

                const uint32_t expected[] =
                {
                    11, 13, 20, 30, 31
                };

                if (!gsubReverseApplyGlyphsEqual(buffer, expected, 5))
                    return fail("case 5 bad backtrack mutation");
            }


            // Bad lookahead.

            {
                OpenTypeShapingBuffer buffer;

                appendGsubReverseApplyGlyph(buffer, 11, 0);
                appendGsubReverseApplyGlyph(buffer, 12, 1);
                appendGsubReverseApplyGlyph(buffer, 20, 2);
                appendGsubReverseApplyGlyph(buffer, 32, 3);
                appendGsubReverseApplyGlyph(buffer, 31, 4);

                if (!applyOpenTypeGsubLookup(
                    lookups, 0, gdef, buffer))
                {
                    return fail("case 5 bad lookahead dispatcher");
                }

                const uint32_t expected[] =
                {
                    11, 12, 20, 32, 31
                };

                if (!gsubReverseApplyGlyphsEqual(buffer, expected, 5))
                    return fail("case 5 bad lookahead mutation");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Current glyph is not filtered.
        //
        // Glyph 100 is a Mark and the lookup has IgnoreMarks. Since it is the
        // current input glyph it still matches directly.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 100 }, { 300 }, {}, {});

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(
                    subtable, 0x0008u);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const std::vector<uint8_t> gdefData =
                makeGsubReverseApplyGdef();

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubReverseApplyGlyph(buffer, 100, 7);

            if (!applyOpenTypeGsubLookup(
                lookups, 0, gdef, buffer))
            {
                return fail("case 6 dispatcher");
            }

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 300 ||
                buffer[0].scalarOffset != 7 ||
                buffer[0].scalarCount != 1)
            {
                return fail("case 6 current glyph semantics");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - NoMatch is a successful no-op.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 20 }, { 200 }, {}, {});

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(subtable);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubReverseApplyGlyph(buffer, 99, 0);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(
                lookups, 0, gdef, buffer))
            {
                return fail("case 7 dispatcher");
            }

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 99)
            {
                return fail("case 7 mutation");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Transactional rollback.
        //
        // Rightmost glyph first successfully changes:
        //
        //   30 -> 300
        //
        // At the earlier glyph, the first subtable does not match and the
        // second subtable is malformed. The entire lookup must fail and the
        // original buffer must be restored.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> goodSubtable =
                makeGsubReverseApplySubtable(
                    { 30 }, { 300 }, {}, {});

            const std::vector<uint8_t> badSubtable =
            {
                0x00, 0x02
            };

            const std::vector<uint8_t> data =
                makeGsubReverseApplyLookupList(
                    goodSubtable, 0, &badSubtable);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 20, 0);
            appendGsubReverseApplyGlyph(buffer, 30, 1);

            const OpenTypeGdefView gdef{};

            if (applyOpenTypeGsubLookup(
                lookups, 0, gdef, buffer))
            {
                return fail("case 8 malformed lookup accepted");
            }

            const uint32_t expected[] = { 20, 30 };

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 2))
                return fail("case 8 rollback");

            ++passed;
        }

        // ====================================================================
// Case 9 - Type 7 -> Type 8 reverse scan.
//
// Extension subtable 0:
//
//     30 -> 300
//
// Extension subtable 1:
//
//     20 | lookahead 300 -> 200
//
// Reverse execution:
//
//     20 30
//        |
//        v
//     20 300
//     |
//     v
//     200 300
// ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable0 =
                makeGsubReverseApplySubtable({ 30 }, { 300 }, {}, {});

            const std::vector<uint8_t> subtable1 =
                makeGsubReverseApplySubtable({ 20 }, { 200 }, {}, { 300 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyExtensionLookupList(subtable0, 0, &subtable1);

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 1)
                return fail("case 9 LookupList");

            const OpenTypeLayoutLookupView lookup = lookups.lookup(0);

            if (!lookup || lookup.lookupType() != 7)
                return fail("case 9 Type 7 lookup");

            uint16_t effectiveType = 0;

            if (!openTypeGsubEffectiveLookupType(lookup, effectiveType) || effectiveType != 8)
                return fail("case 9 effective type");

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 20, 0);
            appendGsubReverseApplyGlyph(buffer, 30, 1);

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 9 dispatcher");

            const uint32_t expected[] = { 200, 300 };

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 2))
                return fail("case 9 Type 7 -> Type 8");

            ++passed;
        }


        // ====================================================================
// Case 10 - Type 7 outer LookupFlags.
//
// Physical:
//
//   11 M 12 M 20 M 30 M 31
//
// Type 7 has IgnoreMarks. The effective Type 8 must therefore see:
//
//   11 12 | 20 | 30 31
// ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> subtable =
                makeGsubReverseApplySubtable(
                    { 20 }, { 200 },
                    { 12, 11 }, { 30, 31 });

            const std::vector<uint8_t> data =
                makeGsubReverseApplyExtensionLookupList(subtable, 0x0008u);

            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const std::vector<uint8_t> gdefData = makeGsubReverseApplyGdef();
            const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

            if (!gdef)
                return fail("case 10 GDEF");

            OpenTypeShapingBuffer buffer;

            appendGsubReverseApplyGlyph(buffer, 11, 0);
            appendGsubReverseApplyGlyph(buffer, 100, 1);
            appendGsubReverseApplyGlyph(buffer, 12, 2);
            appendGsubReverseApplyGlyph(buffer, 101, 3);
            appendGsubReverseApplyGlyph(buffer, 20, 4);
            appendGsubReverseApplyGlyph(buffer, 102, 5);
            appendGsubReverseApplyGlyph(buffer, 30, 6);
            appendGsubReverseApplyGlyph(buffer, 100, 7);
            appendGsubReverseApplyGlyph(buffer, 31, 8);

            if (!applyOpenTypeGsubLookup(lookups, 0, gdef, buffer))
                return fail("case 10 dispatcher");

            const uint32_t expected[] =
            {
                11, 100,
                12, 101,
                200, 102,
                30, 100,
                31
            };

            if (!gsubReverseApplyGlyphsEqual(buffer, expected, 9))
                return fail("case 10 outer LookupFlags");

            ++passed;
        }



        std::printf(
            "OpenType GSUB ReverseChainSingleSubst apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Exact-position apply:      PASS\n"
            "  Type 8 dispatcher:         PASS\n"
            "  Reverse scan order:        PASS\n"
            "  LookupFlag filtering:      PASS\n"
            "  Context gating:            PASS\n"
            "  Start glyph semantics:     PASS\n"
            "  NoMatch behavior:          PASS\n"
            "  Transactional rollback:    PASS\n"
            "  Type 7 -> Type 8:          PASS\n"
            "  Extension LookupFlags:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs