// test_opentype_gpos_context_apply.h
#pragma once

#include "../unitils/test_core.h"

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

    static void appendGposContextApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposContextApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposContextApplyU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposContextApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposContextApplyCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposContextApplyU16(data, 1);
        appendGposContextApplyU16(data, 1);
        appendGposContextApplyU16(data, glyphId);
    }

    static void appendGposContextApplyAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposContextApplyU16(data, 1);
        appendGposContextApplyS16(data, x);
        appendGposContextApplyS16(data, y);
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyph 100 is a Mark.
    // ====================================================================

    static std::vector<uint8_t> makeGposContextApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, 1);
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 12);
        appendGposContextApplyU16(data, 0);
        appendGposContextApplyU16(data, 0);
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 2);
        appendGposContextApplyU16(data, 1);

        appendGposContextApplyU16(data, 100);
        appendGposContextApplyU16(data, 100);
        appendGposContextApplyU16(data, 3);

        return data;
    }


    // ====================================================================
    // Type 7 Format 3 ContextPos.
    // ====================================================================

    static std::vector<uint8_t> makeGposContextApplyContext3(
        const uint16_t* glyphs, uint16_t glyphCount,
        const OpenTypeSequenceLookup* actions, uint16_t actionCount)
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, 3);
        appendGposContextApplyU16(data, glyphCount);
        appendGposContextApplyU16(data, actionCount);

        std::vector<size_t> coveragePatches;
        coveragePatches.reserve(glyphCount);

        for (uint16_t i = 0; i < glyphCount; ++i)
        {
            coveragePatches.push_back(data.size());
            appendGposContextApplyU16(data, 0);
        }

        for (uint16_t i = 0; i < actionCount; ++i)
        {
            appendGposContextApplyU16(data, actions[i].sequenceIndex);
            appendGposContextApplyU16(data, actions[i].lookupListIndex);
        }

        for (uint16_t i = 0; i < glyphCount; ++i)
        {
            patchGposContextApplyU16(data, coveragePatches[i], static_cast<uint16_t>(data.size()));
            appendGposContextApplyCoverage(data, glyphs[i]);
        }

        return data;
    }


    // ====================================================================
    // Type 1 SinglePos Format 1.
    //
    // Apply xPlacement.
    // ====================================================================

    static std::vector<uint8_t> makeGposContextApplySingle(uint16_t glyphId, int16_t xPlacement)
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 0x0001u);
        appendGposContextApplyS16(data, xPlacement);

        patchGposContextApplyU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextApplyCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Type 2 PairPos Format 1.
    //
    // Apply xAdvance to first glyph when followed by secondGlyph.
    // ====================================================================

    static std::vector<uint8_t> makeGposContextApplyPair(
        uint16_t firstGlyph, uint16_t secondGlyph, int16_t xAdvance)
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 0x0004u);
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 1);

        const size_t pairSetPatch = data.size();
        appendGposContextApplyU16(data, 0);


        patchGposContextApplyU16(data, pairSetPatch, static_cast<uint16_t>(data.size()));

        appendGposContextApplyU16(data, 1);
        appendGposContextApplyU16(data, secondGlyph);
        appendGposContextApplyS16(data, xAdvance);


        patchGposContextApplyU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextApplyCoverage(data, firstGlyph);

        return data;
    }


    // ====================================================================
    // Type 4 Mark-to-Base.
    //
    //   mark 100 anchor = (10,20)
    //   base 10 anchor  = (300,400)
    // ====================================================================

    static std::vector<uint8_t> makeGposContextApplyMarkBase()
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposContextApplyU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 1);

        const size_t markArrayPatch = data.size();
        appendGposContextApplyU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposContextApplyU16(data, 0);


        // Mark Coverage.

        patchGposContextApplyU16(data, markCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextApplyCoverage(data, 100);


        // Base Coverage.

        patchGposContextApplyU16(data, baseCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextApplyCoverage(data, 10);


        // MarkArray.

        patchGposContextApplyU16(data, markArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposContextApplyU16(data, 1);
            appendGposContextApplyU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposContextApplyU16(data, 0);

            patchGposContextApplyU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposContextApplyAnchor(data, 10, 20);
        }


        // BaseArray.

        patchGposContextApplyU16(data, baseArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposContextApplyU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposContextApplyU16(data, 0);

            patchGposContextApplyU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposContextApplyAnchor(data, 300, 400);
        }

        return data;
    }


    // ====================================================================
    // Type 6 Mark-to-Mark.
    //
    //   Mark1 101 anchor = (30,40)
    //   Mark2 100 anchor = (350,450)
    // ====================================================================

    static std::vector<uint8_t> makeGposContextApplyMarkMark()
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, 1);

        const size_t mark1CoveragePatch = data.size();
        appendGposContextApplyU16(data, 0);

        const size_t mark2CoveragePatch = data.size();
        appendGposContextApplyU16(data, 0);

        appendGposContextApplyU16(data, 1);

        const size_t mark1ArrayPatch = data.size();
        appendGposContextApplyU16(data, 0);

        const size_t mark2ArrayPatch = data.size();
        appendGposContextApplyU16(data, 0);


        // Mark1 Coverage.

        patchGposContextApplyU16(data, mark1CoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextApplyCoverage(data, 101);


        // Mark2 Coverage.

        patchGposContextApplyU16(data, mark2CoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextApplyCoverage(data, 100);


        // Mark1Array.

        patchGposContextApplyU16(data, mark1ArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposContextApplyU16(data, 1);
            appendGposContextApplyU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposContextApplyU16(data, 0);

            patchGposContextApplyU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposContextApplyAnchor(data, 30, 40);
        }


        // Mark2Array.

        patchGposContextApplyU16(data, mark2ArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposContextApplyU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposContextApplyU16(data, 0);

            patchGposContextApplyU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposContextApplyAnchor(data, 350, 450);
        }

        return data;
    }


    // ====================================================================
    // LookupList builder.
    // ====================================================================

    struct GposContextApplyLookupSpec
    {
        uint16_t type{ 0 };
        uint16_t flag{ 0 };
        std::vector<uint8_t> subtable{};
    };

    static std::vector<uint8_t> makeGposContextApplyLookupList(
        const std::vector<GposContextApplyLookupSpec>& specs)
    {
        std::vector<uint8_t> data;

        appendGposContextApplyU16(data, static_cast<uint16_t>(specs.size()));

        std::vector<size_t> lookupPatches;
        lookupPatches.reserve(specs.size());

        for (size_t i = 0; i < specs.size(); ++i)
        {
            lookupPatches.push_back(data.size());
            appendGposContextApplyU16(data, 0);
        }

        for (size_t i = 0; i < specs.size(); ++i)
        {
            patchGposContextApplyU16(data, lookupPatches[i], static_cast<uint16_t>(data.size()));

            appendGposContextApplyU16(data, specs[i].type);
            appendGposContextApplyU16(data, specs[i].flag);
            appendGposContextApplyU16(data, 1);
            appendGposContextApplyU16(data, 8);

            data.insert(data.end(), specs[i].subtable.begin(), specs[i].subtable.end());
        }

        return data;
    }


    // ====================================================================
    // Glyph helpers.
    // ====================================================================

    static ShapedGlyph makeGposContextApplyGlyph(uint32_t glyphId, int32_t advanceX = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;

        return glyph;
    }

    static ShapedGlyphBuffer makeGposContextApplyBuffer(
        const uint32_t* glyphIds, const int32_t* advances, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
            buffer.pushBack(makeGposContextApplyGlyph(glyphIds[i], advances ? advances[i] : 0));

        return buffer;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposContextApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Context apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        const std::vector<uint8_t> gdefData = makeGposContextApplyGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));
        const OpenTypeGdefView emptyGdef{};

        if (!gdef)
            return fail("synthetic GDEF invalid");


        // ================================================================
        // Case 1 - Nested Type 1 + Type 2.
        //
        // Context:
        //
        //   10 20 30
        //
        // Actions:
        //
        //   sequence 0 -> SinglePos 10 xPlacement +11
        //   sequence 1 -> PairPos 20,30 xAdvance +25
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20, 30 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 1, 2 }
            };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 3, actions, 2) },
                { 1, 0, makeGposContextApplySingle(10, 11) },
                { 2, 0, makeGposContextApplyPair(20, 30, 25) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 20, 30 };
            const int32_t advances[] = { 500, 400, 300 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, advances, 3);

            if (!applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 1 application");

            if (buffer[0].placement.offsetX != 11)
                return fail("case 1 nested SinglePos");

            if (buffer[1].placement.advanceX != 425)
                return fail("case 1 nested PairPos");

            if (buffer[2].placement.advanceX != 300 ||
                buffer[2].placement.offsetX != 0)
            {
                return fail("case 1 unrelated glyph changed");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Filtered physical positions.
        //
        // Physical:
        //
        //   10 M 20 M 30
        //    0 1  2 3  4
        //
        // Outer IgnoreMarks gives:
        //
        //   positions = {0,2,4}
        //
        // sequence 1 must therefore target physical glyph 2.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20, 30 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0x0008u, makeGposContextApplyContext3(glyphs, 3, actions, 1) },
                { 1, 0, makeGposContextApplySingle(20, 17) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 100, 20, 100, 30 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 5);

            if (!applyOpenTypeGposContextLookup(lookups, 0, gdef, buffer, false))
                return fail("case 2 application");

            if (buffer[2].placement.offsetX != 17)
                return fail("case 2 physical target");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0 ||
                buffer[3].placement.offsetX != 0 ||
                buffer[4].placement.offsetX != 0)
            {
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
        // Outer Type 7 ignores M, so context 10 20 matches.
        //
        // Nested PairPos without IgnoreMarks sees M and must not match.
        // Nested PairPos with IgnoreMarks skips M and must match.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const uint32_t bufferGlyphs[] = { 10, 100, 20 };
            const int32_t advances[] = { 500, 0, 400 };


            // Nested lookup does not IgnoreMarks.

            {
                const std::vector<GposContextApplyLookupSpec> specs =
                {
                    { 7, 0x0008u, makeGposContextApplyContext3(glyphs, 2, actions, 1) },
                    { 2, 0, makeGposContextApplyPair(10, 20, 25) }
                };

                const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
                const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

                ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, advances, 3);

                if (!applyOpenTypeGposContextLookup(lookups, 0, gdef, buffer, false))
                    return fail("case 3 unfiltered nested lookup");

                if (buffer[0].placement.advanceX != 500)
                    return fail("case 3 outer flags leaked into nested lookup");
            }


            // Nested lookup independently ignores Marks.

            {
                const std::vector<GposContextApplyLookupSpec> specs =
                {
                    { 7, 0x0008u, makeGposContextApplyContext3(glyphs, 2, actions, 1) },
                    { 2, 0x0008u, makeGposContextApplyPair(10, 20, 25) }
                };

                const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
                const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

                ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, advances, 3);

                if (!applyOpenTypeGposContextLookup(lookups, 0, gdef, buffer, false))
                    return fail("case 3 filtered nested lookup");

                if (buffer[0].placement.advanceX != 525)
                    return fail("case 3 nested IgnoreMarks");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Context range boundary.
        //
        // Physical:
        //
        //   10 20 | 30
        //   context  outside
        //
        // Nested PairPos is invoked at sequence 1 / glyph 20 and wants 30.
        // It must NOT escape the matched context range.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 2, actions, 1) },
                { 2, 0, makeGposContextApplyPair(20, 30, 50) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 20, 30 };
            const int32_t advances[] = { 500, 400, 300 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, advances, 3);

            if (!applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 4 application");

            if (buffer[1].placement.advanceX != 400)
                return fail("case 4 nested lookup escaped context");

            ++passed;
        }


        // ================================================================
        // Case 5 - Nested NoMatch is legal.
        //
        // Context matches, but nested SinglePos covers glyph 999.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 2, actions, 1) },
                { 1, 0, makeGposContextApplySingle(999, 44) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 2);

            if (!applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 5 outer context rejected nested NoMatch");

            if (buffer[1].placement.offsetX != 0)
                return fail("case 5 nested NoMatch mutated placement");

            ++passed;
        }


        // ================================================================
        // Case 6 - Oversized sequenceIndex is ignored.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 99, 1 } };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 2, actions, 1) },
                { 1, 0, makeGposContextApplySingle(10, 77) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 2);

            if (!applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 6 oversized sequenceIndex");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 6 ignored action executed");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Invalid LookupList index is transactional.
        //
        // First nested action succeeds.
        // Second references missing Lookup 99.
        //
        // Whole Type 7 application must fail without retaining action 1.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 1, 99 }
            };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 2, actions, 2) },
                { 1, 0, makeGposContextApplySingle(10, 33) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 2);

            if (applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 7 invalid lookup accepted");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 7 transactional rollback");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Shared attachment graph.
        //
        // Context:
        //
        //   base10 mark100 mark101
        //
        // Actions:
        //
        //   Type 4: mark100 -> base10
        //   Type 6: mark101 -> mark100
        //
        // Before finalization:
        //
        //   10 <- 100 <- 101
        //
        // Base advance = 500.
        //
        // mark100:
        //
        //   local = (300,400) - (10,20)
        //         = (290,380)
        //
        //   final = (290 - 500, 380)
        //         = (-210,380)
        //
        // mark101:
        //
        //   local = (350,450) - (30,40)
        //         = (320,410)
        //
        //   final = local + resolved mark100
        //         = (110,790)
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 100, 101 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 1, 1 },
                { 2, 2 }
            };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 3, actions, 2) },
                { 4, 0, makeGposContextApplyMarkBase() },
                { 6, 0, makeGposContextApplyMarkMark() }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 100, 101 };
            const int32_t advances[] = { 500, 0, 0 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, advances, 3);

            if (!applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 8 application");

            if (buffer[1].placement.offsetX != -210 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 8 first attachment");
            }

            if (buffer[2].placement.offsetX != 110 ||
                buffer[2].placement.offsetY != 790)
            {
                return fail("case 8 chained attachment");
            }

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[1].shaping.glyphId != 100 ||
                buffer[2].shaping.glyphId != 101)
            {
                return fail("case 8 shaping identity");
            }

            ++passed;
        }


        // ================================================================
        // Case 9 - Recursive Type 7 guard.
        //
        // Lookup 0:
        //
        //   Context glyph 10
        //   sequence 0 -> Lookup 0
        //
        // Deliberate self recursion must terminate through max depth.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 0 } };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 1, actions, 1) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;
            state.maxNestingDepth = 4;

            if (applyOpenTypeGposContextLookup(
                lookups, 0, emptyGdef, buffer,
                attachments, state, false))
            {
                return fail("case 9 recursive lookup accepted");
            }

            if (state.nestingDepth != 0)
                return fail("case 9 recursion depth leaked");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[0].placement.offsetY != 0)
            {
                return fail("case 9 recursive mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 10 - Malformed nested lookup is transactional.
        //
        // First action applies +55 xPlacement.
        // Second nested Type 1 has a truncated subtable.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 1, 2 }
            };

            const std::vector<uint8_t> malformed =
            {
                0x00, 0x01
            };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 2, actions, 2) },
                { 1, 0, makeGposContextApplySingle(10, 55) },
                { 1, 0, malformed }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 2);

            if (applyOpenTypeGposContextLookup(lookups, 0, emptyGdef, buffer, false))
                return fail("case 10 malformed nested lookup accepted");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 10 transactional rollback");
            }

            ++passed;
        }


        // ================================================================
        // Case 11 - Operation budget.
        //
        // One operation is enough to enter the outer Type 7, but not enough
        // to execute its nested SinglePos.
        // ================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const std::vector<GposContextApplyLookupSpec> specs =
            {
                { 7, 0, makeGposContextApplyContext3(glyphs, 1, actions, 1) },
                { 1, 0, makeGposContextApplySingle(10, 99) }
            };

            const std::vector<uint8_t> data = makeGposContextApplyLookupList(specs);
            const OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const uint32_t bufferGlyphs[] = { 10 };

            ShapedGlyphBuffer buffer = makeGposContextApplyBuffer(bufferGlyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;
            state.operationBudget = 1;

            if (applyOpenTypeGposContextLookup(
                lookups, 0, emptyGdef, buffer,
                attachments, state, false))
            {
                return fail("case 11 operation budget ignored");
            }

            if (state.operationBudget != 0)
                return fail("case 11 operation budget accounting");

            if (state.nestingDepth != 0)
                return fail("case 11 nesting depth leaked");

            if (buffer[0].placement.offsetX != 0)
                return fail("case 11 budget failure mutated buffer");

            ++passed;
        }


        std::printf(
            "OpenType GPOS Context apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Nested Type 1/2:            PASS\n"
            "  Filtered physical positions:PASS\n"
            "  Nested LookupFlags:         PASS\n"
            "  Context range boundary:     PASS\n"
            "  Nested NoMatch:             PASS\n"
            "  Oversized sequenceIndex:    PASS\n"
            "  Invalid lookup index:       PASS\n"
            "  Shared attachment graph:    PASS\n"
            "  Recursive Type 7 guard:     PASS\n"
            "  Transactional malformed:    PASS\n"
            "  Operation budget:           PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs