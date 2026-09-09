// test_opentype_gpos_chain_context_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGposChainApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposChainApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposChainApplyU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposChainApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposChainApplyCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposChainApplyU16(data, 1);
        appendGposChainApplyU16(data, 1);
        appendGposChainApplyU16(data, glyphId);
    }

    static void appendGposChainApplyAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposChainApplyU16(data, 1);
        appendGposChainApplyS16(data, x);
        appendGposChainApplyS16(data, y);
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are Marks.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 1);
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 12);
        appendGposChainApplyU16(data, 0);
        appendGposChainApplyU16(data, 0);
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 2);
        appendGposChainApplyU16(data, 1);

        appendGposChainApplyU16(data, 100);
        appendGposChainApplyU16(data, 102);
        appendGposChainApplyU16(data, 3);

        return data;
    }


    // ====================================================================
    // Type 8 Format 3.
    //
    // Generic chained-context builder.
    //
    // backtrack[] is stored nearest-first.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplyChain3(
        const uint16_t* backtrack, uint16_t backtrackCount,
        const uint16_t* input, uint16_t inputCount,
        const uint16_t* lookahead, uint16_t lookaheadCount,
        const OpenTypeSequenceLookup* actions, uint16_t actionCount)
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 3);

        appendGposChainApplyU16(data, backtrackCount);

        std::vector<size_t> backtrackPatches;
        backtrackPatches.reserve(backtrackCount);

        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            backtrackPatches.push_back(data.size());
            appendGposChainApplyU16(data, 0);
        }

        appendGposChainApplyU16(data, inputCount);

        std::vector<size_t> inputPatches;
        inputPatches.reserve(inputCount);

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            inputPatches.push_back(data.size());
            appendGposChainApplyU16(data, 0);
        }

        appendGposChainApplyU16(data, lookaheadCount);

        std::vector<size_t> lookaheadPatches;
        lookaheadPatches.reserve(lookaheadCount);

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            lookaheadPatches.push_back(data.size());
            appendGposChainApplyU16(data, 0);
        }

        appendGposChainApplyU16(data, actionCount);

        for (uint16_t i = 0; i < actionCount; ++i)
        {
            appendGposChainApplyU16(data, actions[i].sequenceIndex);
            appendGposChainApplyU16(data, actions[i].lookupListIndex);
        }

        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            patchGposChainApplyU16(data, backtrackPatches[i], static_cast<uint16_t>(data.size()));
            appendGposChainApplyCoverage(data, backtrack[i]);
        }

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            patchGposChainApplyU16(data, inputPatches[i], static_cast<uint16_t>(data.size()));
            appendGposChainApplyCoverage(data, input[i]);
        }

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            patchGposChainApplyU16(data, lookaheadPatches[i], static_cast<uint16_t>(data.size()));
            appendGposChainApplyCoverage(data, lookahead[i]);
        }

        return data;
    }


    // ====================================================================
    // Type 7 Format 3.
    //
    // Used for nested Type 7 and Type 7 <-> Type 8 recursion tests.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplyContext3(
        const uint16_t* glyphs, uint16_t glyphCount,
        const OpenTypeSequenceLookup* actions, uint16_t actionCount)
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 3);
        appendGposChainApplyU16(data, glyphCount);
        appendGposChainApplyU16(data, actionCount);

        std::vector<size_t> coveragePatches;
        coveragePatches.reserve(glyphCount);

        for (uint16_t i = 0; i < glyphCount; ++i)
        {
            coveragePatches.push_back(data.size());
            appendGposChainApplyU16(data, 0);
        }

        for (uint16_t i = 0; i < actionCount; ++i)
        {
            appendGposChainApplyU16(data, actions[i].sequenceIndex);
            appendGposChainApplyU16(data, actions[i].lookupListIndex);
        }

        for (uint16_t i = 0; i < glyphCount; ++i)
        {
            patchGposChainApplyU16(data, coveragePatches[i], static_cast<uint16_t>(data.size()));
            appendGposChainApplyCoverage(data, glyphs[i]);
        }

        return data;
    }


    // ====================================================================
    // Type 1 SinglePos Format 1.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplySingle(uint16_t glyphId, int16_t xPlacement)
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 0x0001u);
        appendGposChainApplyS16(data, xPlacement);

        patchGposChainApplyU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainApplyCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Type 2 PairPos Format 1.
    //
    // Adjust first glyph xAdvance.
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplyPair(
        uint16_t firstGlyph, uint16_t secondGlyph, int16_t xAdvance)
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 0x0004u);
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 1);

        const size_t pairSetPatch = data.size();
        appendGposChainApplyU16(data, 0);

        patchGposChainApplyU16(data, pairSetPatch, static_cast<uint16_t>(data.size()));

        appendGposChainApplyU16(data, 1);
        appendGposChainApplyU16(data, secondGlyph);
        appendGposChainApplyS16(data, xAdvance);

        patchGposChainApplyU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainApplyCoverage(data, firstGlyph);

        return data;
    }


    // ====================================================================
    // Type 4 Mark-to-Base.
    //
    // mark 100 anchor = (10,20)
    // base 10 anchor  = (300,400)
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplyMarkBase()
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposChainApplyU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 1);

        const size_t markArrayPatch = data.size();
        appendGposChainApplyU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposChainApplyU16(data, 0);


        patchGposChainApplyU16(data, markCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainApplyCoverage(data, 100);

        patchGposChainApplyU16(data, baseCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainApplyCoverage(data, 10);


        patchGposChainApplyU16(data, markArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposChainApplyU16(data, 1);
            appendGposChainApplyU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposChainApplyU16(data, 0);

            patchGposChainApplyU16(data, anchorPatch, static_cast<uint16_t>(data.size() - arrayBegin));
            appendGposChainApplyAnchor(data, 10, 20);
        }


        patchGposChainApplyU16(data, baseArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposChainApplyU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposChainApplyU16(data, 0);

            patchGposChainApplyU16(data, anchorPatch, static_cast<uint16_t>(data.size() - arrayBegin));
            appendGposChainApplyAnchor(data, 300, 400);
        }

        return data;
    }


    // ====================================================================
    // Type 6 Mark-to-Mark.
    //
    // Mark1 101 anchor = (30,40)
    // Mark2 100 anchor = (350,450)
    // ====================================================================

    static std::vector<uint8_t> makeGposChainApplyMarkMark()
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, 1);

        const size_t mark1CoveragePatch = data.size();
        appendGposChainApplyU16(data, 0);

        const size_t mark2CoveragePatch = data.size();
        appendGposChainApplyU16(data, 0);

        appendGposChainApplyU16(data, 1);

        const size_t mark1ArrayPatch = data.size();
        appendGposChainApplyU16(data, 0);

        const size_t mark2ArrayPatch = data.size();
        appendGposChainApplyU16(data, 0);


        patchGposChainApplyU16(data, mark1CoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainApplyCoverage(data, 101);

        patchGposChainApplyU16(data, mark2CoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainApplyCoverage(data, 100);


        patchGposChainApplyU16(data, mark1ArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposChainApplyU16(data, 1);
            appendGposChainApplyU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposChainApplyU16(data, 0);

            patchGposChainApplyU16(data, anchorPatch, static_cast<uint16_t>(data.size() - arrayBegin));
            appendGposChainApplyAnchor(data, 30, 40);
        }


        patchGposChainApplyU16(data, mark2ArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposChainApplyU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposChainApplyU16(data, 0);

            patchGposChainApplyU16(data, anchorPatch, static_cast<uint16_t>(data.size() - arrayBegin));
            appendGposChainApplyAnchor(data, 350, 450);
        }

        return data;
    }


    // ====================================================================
    // LookupList builder.
    // ====================================================================

    struct GposChainApplyLookupSpec
    {
        uint16_t type{ 0 };
        uint16_t flag{ 0 };
        std::vector<uint8_t> subtable{};
    };


    static std::vector<uint8_t> makeGposChainApplyLookupList(
        const std::vector<GposChainApplyLookupSpec>& specs)
    {
        std::vector<uint8_t> data;

        appendGposChainApplyU16(data, static_cast<uint16_t>(specs.size()));

        std::vector<size_t> lookupPatches;
        lookupPatches.reserve(specs.size());

        for (size_t i = 0; i < specs.size(); ++i)
        {
            lookupPatches.push_back(data.size());
            appendGposChainApplyU16(data, 0);
        }

        for (size_t i = 0; i < specs.size(); ++i)
        {
            patchGposChainApplyU16(data, lookupPatches[i], static_cast<uint16_t>(data.size()));

            appendGposChainApplyU16(data, specs[i].type);
            appendGposChainApplyU16(data, specs[i].flag);
            appendGposChainApplyU16(data, 1);
            appendGposChainApplyU16(data, 8);

            data.insert(data.end(), specs[i].subtable.begin(), specs[i].subtable.end());
        }

        return data;
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static ShapedGlyph makeGposChainApplyGlyph(uint32_t glyphId, int32_t advanceX = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;

        return glyph;
    }


    static ShapedGlyphBuffer makeGposChainApplyBuffer(
        const uint32_t* glyphIds, const int32_t* advances, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
            buffer.pushBack(makeGposChainApplyGlyph(glyphIds[i], advances ? advances[i] : 0));

        return buffer;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposChainContextApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS ChainContext apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> gdefData = makeGposChainApplyGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));
        const OpenTypeGdefView emptyGdef{};

        if (!gdef)
            return fail("synthetic GDEF invalid");


        // ================================================================
        // Case 1 - Exact-position Type 8 dispatch with nested Type 1/2.
        //
        //   9 | 10 20 30 | 40
        //
        // sequence 0 -> SinglePos 10 xPlacement +11
        // sequence 1 -> PairPos 20,30 xAdvance +25
        //
        // This exercises Type 8 through applyOpenTypeGposLookupAt().
        // ================================================================

        {
            ++cases;

            const uint16_t backtrack[] = { 9 };
            const uint16_t input[] = { 10, 20, 30 };
            const uint16_t lookahead[] = { 40 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 1, 2 }
            };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        backtrack, 1, input, 3, lookahead, 1, actions, 2)
                },
                { 1, 0, makeGposChainApplySingle(10, 11) },
                { 2, 0, makeGposChainApplyPair(20, 30, 25) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 9, 10, 20, 30, 40 };
            const int32_t advances[] = { 300, 500, 400, 300, 200 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, advances, 5);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyOpenTypeGposLookupAt(
                    lookups, 0, emptyGdef, buffer, 1,
                    attachments, state, false,
                    OpenTypeGposApplyRange::whole(buffer.size()));

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 1 exact-position result");

            if (buffer[1].placement.offsetX != 11)
                return fail("case 1 nested SinglePos");

            if (buffer[2].placement.advanceX != 425)
                return fail("case 1 nested PairPos");

            if (buffer[0].placement.advanceX != 300 ||
                buffer[3].placement.advanceX != 300 ||
                buffer[4].placement.advanceX != 200)
            {
                return fail("case 1 unrelated placement");
            }

            if (state.nestingDepth != 0)
                return fail("case 1 nesting state");

            ++passed;
        }


        // ================================================================
        // Case 2 - Filtered backtrack/input/lookahead execution.
        //
        // Physical:
        //
        //   9 M 10 M 20 M 30 M 40
        //
        // Outer IgnoreMarks gives input positions:
        //
        //   {2,4,6}
        //
        // sequence 1 therefore targets physical glyph 4.
        // ================================================================

        {
            ++cases;

            const uint16_t backtrack[] = { 9 };
            const uint16_t input[] = { 10, 20, 30 };
            const uint16_t lookahead[] = { 40 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 1, 1 }
            };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0x0008u,
                    makeGposChainApplyChain3(
                        backtrack, 1, input, 3, lookahead, 1, actions, 1)
                },
                { 1, 0, makeGposChainApplySingle(20, 17) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] =
            {
                9, 100,
                10, 101,
                20, 102,
                30, 100,
                40
            };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 9);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, gdef, buffer, false))
                return fail("case 2 application");

            if (buffer[4].placement.offsetX != 17)
                return fail("case 2 physical input target");

            for (size_t i = 0; i < buffer.size(); ++i)
            {
                if (i != 4 && buffer[i].placement.offsetX != 0)
                    return fail("case 2 unrelated placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Nested lookup uses its own LookupFlag.
        //
        // Physical:
        //
        //   10 M 20
        //
        // Outer Type 8 ignores M and matches input {10,20}.
        //
        // Nested PairPos without IgnoreMarks sees M and must fail.
        // Nested PairPos with IgnoreMarks skips M and must succeed.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const uint32_t glyphs[] = { 10, 100, 20 };
            const int32_t advances[] = { 500, 0, 400 };


            // Nested lookup does not IgnoreMarks.

            {
                const std::vector<GposChainApplyLookupSpec> specs =
                {
                    {
                        8, 0x0008u,
                        makeGposChainApplyChain3(
                            nullptr, 0, input, 2, nullptr, 0, actions, 1)
                    },
                    { 2, 0, makeGposChainApplyPair(10, 20, 25) }
                };

                const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
                const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

                ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, advances, 3);

                if (!applyOpenTypeGposChainContextLookup(lookups, 0, gdef, buffer, false))
                    return fail("case 3 nested unfiltered application");

                if (buffer[0].placement.advanceX != 500)
                    return fail("case 3 outer LookupFlag leaked");
            }


            // Nested lookup independently ignores Marks.

            {
                const std::vector<GposChainApplyLookupSpec> specs =
                {
                    {
                        8, 0x0008u,
                        makeGposChainApplyChain3(
                            nullptr, 0, input, 2, nullptr, 0, actions, 1)
                    },
                    { 2, 0x0008u, makeGposChainApplyPair(10, 20, 25) }
                };

                const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
                const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

                ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, advances, 3);

                if (!applyOpenTypeGposChainContextLookup(lookups, 0, gdef, buffer, false))
                    return fail("case 3 nested filtered application");

                if (buffer[0].placement.advanceX != 525)
                    return fail("case 3 nested IgnoreMarks");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Backtrack and lookahead gate execution.
        //
        // Neither context-only region is positioned. Either one failing must
        // prevent the input action from running.
        // ================================================================

        {
            ++cases;

            const uint16_t backtrack[] = { 9 };
            const uint16_t input[] = { 10, 20, 30 };
            const uint16_t lookahead[] = { 40 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        backtrack, 1, input, 3, lookahead, 1, actions, 1)
                },
                { 1, 0, makeGposChainApplySingle(20, 44) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));
            const OpenTypeLayoutLookupView lookup = lookups.lookup(0);


            // Bad backtrack.

            {
                const uint32_t glyphs[] = { 8, 10, 20, 30, 40 };

                ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 5);

                OpenTypeGposAttachmentState attachments;
                attachments.reset(buffer.size());

                OpenTypeGposApplyState state;

                const OpenTypeGposApplyAtResult result =
                    applyOpenTypeGposChainContextAt(
                        lookups, lookup, emptyGdef, buffer, 1,
                        attachments, state, false);

                if (result != OpenTypeGposApplyAtResult::NoMatch)
                    return fail("case 4 bad backtrack result");

                if (buffer[2].placement.offsetX != 0)
                    return fail("case 4 bad backtrack mutation");
            }


            // Bad lookahead.

            {
                const uint32_t glyphs[] = { 9, 10, 20, 30, 41 };

                ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 5);

                OpenTypeGposAttachmentState attachments;
                attachments.reset(buffer.size());

                OpenTypeGposApplyState state;

                const OpenTypeGposApplyAtResult result =
                    applyOpenTypeGposChainContextAt(
                        lookups, lookup, emptyGdef, buffer, 1,
                        attachments, state, false);

                if (result != OpenTypeGposApplyAtResult::NoMatch)
                    return fail("case 4 bad lookahead result");

                if (buffer[2].placement.offsetX != 0)
                    return fail("case 4 bad lookahead mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Lookahead boundary isolation.
        //
        //   input:     10 20
        //   lookahead: 30
        //
        // Nested PairPos at sequence 1 wants:
        //
        //   20 + 30
        //
        // but glyph 30 exists only in lookahead and is outside the Type 8
        // input execution range. PairPos must therefore return NoMatch.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };
            const uint16_t lookahead[] = { 30 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 2, lookahead, 1, actions, 1)
                },
                { 2, 0, makeGposChainApplyPair(20, 30, 50) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 20, 30 };
            const int32_t advances[] = { 500, 400, 300 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, advances, 3);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 5 application");

            if (buffer[1].placement.advanceX != 400)
                return fail("case 5 PairPos escaped into lookahead");

            ++passed;
        }


        // ================================================================
        // Case 6 - Backtrack boundary isolation.
        //
        //   backtrack: base 10
        //   input:     mark 100
        //
        // Nested Mark-to-Base at mark 100 must NOT see base 10 because that
        // base belongs only to backtrack and lies outside the input range.
        // ================================================================

        {
            ++cases;

            const uint16_t backtrack[] = { 10 };
            const uint16_t input[] = { 100 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        backtrack, 1, input, 1, nullptr, 0, actions, 1)
                },
                { 4, 0, makeGposChainApplyMarkBase() }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 100 };
            const int32_t advances[] = { 500, 0 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, advances, 2);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, gdef, buffer, false))
                return fail("case 6 application");

            if (buffer[1].placement.offsetX != 0 ||
                buffer[1].placement.offsetY != 0)
            {
                return fail("case 6 Mark-to-Base escaped into backtrack");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Nested NoMatch is legal.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 2, nullptr, 0, actions, 1)
                },
                { 1, 0, makeGposChainApplySingle(999, 44) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 2);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 7 nested NoMatch rejected");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 7 nested NoMatch mutated placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Oversized sequenceIndex is ignored.
        //
        // lookupListIndex is deliberately invalid as well. sequenceIndex must
        // be rejected first, making the entire action irrelevant.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 99, 99 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 2, nullptr, 0, actions, 1)
                }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 2);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 8 oversized sequenceIndex");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 8 ignored action mutated placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 9 - Invalid LookupList index is transactional.
        //
        // First action succeeds.
        // Second action references missing Lookup 99.
        //
        // The successful first placement must be rolled back.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 1, 99 }
            };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 2, nullptr, 0, actions, 2)
                },
                { 1, 0, makeGposChainApplySingle(10, 33) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 2);

            if (applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 9 invalid lookup accepted");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 9 rollback");
            }

            ++passed;
        }


        // ================================================================
        // Case 10 - Shared attachment graph.
        //
        // Input:
        //
        //   base10 mark100 mark101
        //
        // Actions:
        //
        //   sequence 1 -> Type 4: mark100 -> base10
        //   sequence 2 -> Type 6: mark101 -> mark100
        //
        // base10 advance = 500
        //
        // mark100 local:
        //
        //   (300,400) - (10,20) = (290,380)
        //
        // final:
        //
        //   (-210,380)
        //
        // mark101 local:
        //
        //   (350,450) - (30,40) = (320,410)
        //
        // final through mark100:
        //
        //   (110,790)
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 100, 101 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 1, 1 },
                { 2, 2 }
            };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 3, nullptr, 0, actions, 2)
                },
                { 4, 0, makeGposChainApplyMarkBase() },
                { 6, 0, makeGposChainApplyMarkMark() }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 100, 101 };
            const int32_t advances[] = { 500, 0, 0 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, advances, 3);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, gdef, buffer, false))
                return fail("case 10 application");

            if (buffer[1].placement.offsetX != -210 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 10 first attachment");
            }

            if (buffer[2].placement.offsetX != 110 ||
                buffer[2].placement.offsetY != 790)
            {
                return fail("case 10 chained attachment");
            }

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[1].shaping.glyphId != 100 ||
                buffer[2].shaping.glyphId != 101)
            {
                return fail("case 10 shaping identity");
            }

            ++passed;
        }


        // ================================================================
        // Case 11 - Type 8 -> Type 7 -> Type 1.
        //
        // Outer Type 8:
        //
        //   input 10 -> Lookup 1
        //
        // Nested Type 7:
        //
        //   input 10 -> Lookup 2
        //
        // Lookup 2:
        //
        //   SinglePos +31
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs10[] = { 10 };

            const OpenTypeSequenceLookup outerActions[] = { { 0, 1 } };
            const OpenTypeSequenceLookup innerActions[] = { { 0, 2 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, glyphs10, 1, nullptr, 0, outerActions, 1)
                },
                {
                    7, 0,
                    makeGposChainApplyContext3(
                        glyphs10, 1, innerActions, 1)
                },
                { 1, 0, makeGposChainApplySingle(10, 31) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 1);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 11 nested Type 7");

            if (buffer[0].placement.offsetX != 31)
                return fail("case 11 nested Type 7 result");

            ++passed;
        }


        // ================================================================
        // Case 12 - Type 8 -> Type 8 -> Type 1.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };

            const OpenTypeSequenceLookup outerActions[] = { { 0, 1 } };
            const OpenTypeSequenceLookup innerActions[] = { { 0, 2 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 1, nullptr, 0, outerActions, 1)
                },
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 1, nullptr, 0, innerActions, 1)
                },
                { 1, 0, makeGposChainApplySingle(10, 37) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 1);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 12 nested Type 8");

            if (buffer[0].placement.offsetX != 37)
                return fail("case 12 nested Type 8 result");

            ++passed;
        }


        // ================================================================
        // Case 13 - Recursive Type 8 guard.
        //
        // Lookup 0 invokes itself at sequence 0.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 0 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 1, nullptr, 0, actions, 1)
                }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;
            state.maxNestingDepth = 4;

            if (applyOpenTypeGposChainContextLookup(
                lookups, 0, emptyGdef, buffer,
                attachments, state, false))
            {
                return fail("case 13 recursive Type 8 accepted");
            }

            if (state.nestingDepth != 0)
                return fail("case 13 nesting depth leaked");

            if (buffer[0].placement.offsetX != 0)
                return fail("case 13 recursive mutation");

            ++passed;
        }


        // ================================================================
        // Case 14 - Type 8 <-> Type 7 mutual recursion guard.
        //
        // Lookup 0 Type 8 -> Lookup 1
        // Lookup 1 Type 7 -> Lookup 0
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };

            const OpenTypeSequenceLookup type8Actions[] = { { 0, 1 } };
            const OpenTypeSequenceLookup type7Actions[] = { { 0, 0 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 1, nullptr, 0, type8Actions, 1)
                },
                {
                    7, 0,
                    makeGposChainApplyContext3(
                        input, 1, type7Actions, 1)
                }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;
            state.maxNestingDepth = 5;

            if (applyOpenTypeGposChainContextLookup(
                lookups, 0, emptyGdef, buffer,
                attachments, state, false))
            {
                return fail("case 14 mutual recursion accepted");
            }

            if (state.nestingDepth != 0)
                return fail("case 14 nesting depth leaked");

            if (buffer[0].placement.offsetX != 0)
                return fail("case 14 mutual recursion mutation");

            ++passed;
        }


        // ================================================================
        // Case 15 - Operation budget.
        //
        // One operation is consumed by the outer whole-lookup scan.
        // No budget remains for the nested SinglePos dispatcher.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 1, nullptr, 0, actions, 1)
                },
                { 1, 0, makeGposChainApplySingle(10, 99) }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;
            state.operationBudget = 1;

            if (applyOpenTypeGposChainContextLookup(
                lookups, 0, emptyGdef, buffer,
                attachments, state, false))
            {
                return fail("case 15 operation budget ignored");
            }

            if (state.operationBudget != 0)
                return fail("case 15 operation budget accounting");

            if (state.nestingDepth != 0)
                return fail("case 15 nesting depth leaked");

            if (buffer[0].placement.offsetX != 0)
                return fail("case 15 budget failure mutated buffer");

            ++passed;
        }


        // ================================================================
        // Case 16 - Malformed nested lookup is transactional.
        //
        // First action succeeds.
        // Second nested Type 1 has a truncated subtable.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 1, 2 }
            };

            const std::vector<uint8_t> malformed =
            {
                0x00, 0x01
            };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 2, nullptr, 0, actions, 2)
                },
                { 1, 0, makeGposChainApplySingle(10, 55) },
                { 1, 0, malformed }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 2);

            if (applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 16 malformed nested lookup accepted");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 16 transactional rollback");
            }

            ++passed;
        }


        // ================================================================
        // Case 17 - Zero-action Match.
        //
        // A valid Type 8 chain with no SequenceLookup records is still a
        // successful match and must leave placement untouched.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };

            const std::vector<GposChainApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    makeGposChainApplyChain3(
                        nullptr, 0, input, 1, nullptr, 0, nullptr, 0)
                }
            };

            const std::vector<uint8_t> data = makeGposChainApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposChainApplyBuffer(glyphs, nullptr, 1);

            if (!applyOpenTypeGposChainContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 17 zero-action match rejected");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[0].placement.offsetY != 0 ||
                buffer[0].shaping.glyphId != 10)
            {
                return fail("case 17 zero-action mutation");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS ChainContext apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Nested Type 1/2:            PASS\n"
            "  Filtered execution:         PASS\n"
            "  Nested LookupFlags:         PASS\n"
            "  Context gating:             PASS\n"
            "  Lookahead isolation:        PASS\n"
            "  Backtrack isolation:        PASS\n"
            "  Nested NoMatch:             PASS\n"
            "  Oversized sequenceIndex:    PASS\n"
            "  Invalid lookup rollback:    PASS\n"
            "  Shared attachment graph:    PASS\n"
            "  Nested Type 7:              PASS\n"
            "  Nested Type 8:              PASS\n"
            "  Recursive Type 8 guard:     PASS\n"
            "  Type 7/8 recursion guard:   PASS\n"
            "  Operation budget:           PASS\n"
            "  Transactional malformed:    PASS\n"
            "  Zero-action match:          PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs