// test_opentype_gpos_chain_context_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_chain_context_view.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGposChainViewU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGposChainViewU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static uint16_t readGposChainViewU16(const std::vector<uint8_t>& data, size_t offset)
    {
        return static_cast<uint16_t>((uint16_t(data[offset]) << 8) | uint16_t(data[offset + 1]));
    }


    static void appendGposChainViewCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposChainViewU16(data, 1);
        appendGposChainViewU16(data, 1);
        appendGposChainViewU16(data, glyphId);
    }


    static void appendGposChainViewClassRange(
        std::vector<uint8_t>& data, uint16_t first, uint16_t last, uint16_t classValue)
    {
        appendGposChainViewU16(data, first);
        appendGposChainViewU16(data, last);
        appendGposChainViewU16(data, classValue);
    }


    // ====================================================================
    // Format 1.
    //
    // Logical chain:
    //
    //   11 12 | 10 20 30 | 40 50
    //
    // Backtrack is stored nearest-first:
    //
    //   12 11
    //
    // Actions:
    //
    //   sequence 1 -> lookup 7
    //   sequence 2 -> lookup 8
    // ====================================================================

    static std::vector<uint8_t> makeGposChainViewFormat1()
    {
        std::vector<uint8_t> data;

        appendGposChainViewU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposChainViewU16(data, 0);

        appendGposChainViewU16(data, 1);

        const size_t setPatch = data.size();
        appendGposChainViewU16(data, 0);


        // Coverage.

        patchGposChainViewU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainViewCoverage(data, 10);


        // ChainedSequenceRuleSet.

        patchGposChainViewU16(data, setPatch, static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposChainViewU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposChainViewU16(data, 0);

        patchGposChainViewU16(data, rulePatch, static_cast<uint16_t>(data.size() - setBegin));


        // ChainedSequenceRule.

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 12);
        appendGposChainViewU16(data, 11);

        appendGposChainViewU16(data, 3);

        appendGposChainViewU16(data, 20);
        appendGposChainViewU16(data, 30);

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 40);
        appendGposChainViewU16(data, 50);

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 1);
        appendGposChainViewU16(data, 7);

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 8);

        return data;
    }


    // ====================================================================
    // Format 2.
    //
    // Backtrack classes:
    //
    //   12 -> 2
    //   11 -> 3
    //
    // Input:
    //
    //   10 -> 1
    //   20 -> 2
    //   30 -> 3
    //
    // Lookahead:
    //
    //   40 -> 4
    //   50 -> 5
    // ====================================================================

    static std::vector<uint8_t> makeGposChainViewFormat2()
    {
        std::vector<uint8_t> data;

        appendGposChainViewU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t backtrackClassDefPatch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t inputClassDefPatch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t lookaheadClassDefPatch = data.size();
        appendGposChainViewU16(data, 0);

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 0);

        const size_t classSetPatch = data.size();
        appendGposChainViewU16(data, 0);


        // Coverage.

        patchGposChainViewU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposChainViewCoverage(data, 10);


        // Backtrack ClassDef Format 2.

        patchGposChainViewU16(data, backtrackClassDefPatch, static_cast<uint16_t>(data.size()));

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 2);

        appendGposChainViewClassRange(data, 11, 11, 3);
        appendGposChainViewClassRange(data, 12, 12, 2);


        // Input ClassDef Format 2.

        patchGposChainViewU16(data, inputClassDefPatch, static_cast<uint16_t>(data.size()));

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 3);

        appendGposChainViewClassRange(data, 10, 10, 1);
        appendGposChainViewClassRange(data, 20, 20, 2);
        appendGposChainViewClassRange(data, 30, 30, 3);


        // Lookahead ClassDef Format 2.

        patchGposChainViewU16(data, lookaheadClassDefPatch, static_cast<uint16_t>(data.size()));

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 2);

        appendGposChainViewClassRange(data, 40, 40, 4);
        appendGposChainViewClassRange(data, 50, 50, 5);


        // ClassSet for input class 1.

        patchGposChainViewU16(data, classSetPatch, static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposChainViewU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposChainViewU16(data, 0);

        patchGposChainViewU16(data, rulePatch, static_cast<uint16_t>(data.size() - setBegin));


        // ChainedClassSequenceRule.

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 3);

        appendGposChainViewU16(data, 3);

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 3);

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 4);
        appendGposChainViewU16(data, 5);

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 1);
        appendGposChainViewU16(data, 7);

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 8);

        return data;
    }


    // ====================================================================
    // Format 3.
    //
    //   backtrack: 12 11
    //   input:     10 20 30
    //   lookahead: 40 50
    // ====================================================================

    static std::vector<uint8_t> makeGposChainViewFormat3()
    {
        std::vector<uint8_t> data;

        appendGposChainViewU16(data, 3);


        // Backtrack.

        appendGposChainViewU16(data, 2);

        const size_t backtrack0Patch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t backtrack1Patch = data.size();
        appendGposChainViewU16(data, 0);


        // Input.

        appendGposChainViewU16(data, 3);

        const size_t input0Patch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t input1Patch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t input2Patch = data.size();
        appendGposChainViewU16(data, 0);


        // Lookahead.

        appendGposChainViewU16(data, 2);

        const size_t lookahead0Patch = data.size();
        appendGposChainViewU16(data, 0);

        const size_t lookahead1Patch = data.size();
        appendGposChainViewU16(data, 0);


        // SequenceLookup records.

        appendGposChainViewU16(data, 2);

        appendGposChainViewU16(data, 1);
        appendGposChainViewU16(data, 7);

        appendGposChainViewU16(data, 2);
        appendGposChainViewU16(data, 8);


        auto appendCoverage = [&](size_t patch, uint16_t glyphId)
            {
                patchGposChainViewU16(data, patch, static_cast<uint16_t>(data.size()));
                appendGposChainViewCoverage(data, glyphId);
            };


        appendCoverage(backtrack0Patch, 12);
        appendCoverage(backtrack1Patch, 11);

        appendCoverage(input0Patch, 10);
        appendCoverage(input1Patch, 20);
        appendCoverage(input2Patch, 30);

        appendCoverage(lookahead0Patch, 40);
        appendCoverage(lookahead1Patch, 50);

        return data;
    }


    static std::vector<uint8_t> makeGposChainViewFormat3InputOnly()
    {
        std::vector<uint8_t> data;

        appendGposChainViewU16(data, 3);

        appendGposChainViewU16(data, 0);

        appendGposChainViewU16(data, 1);

        const size_t inputPatch = data.size();
        appendGposChainViewU16(data, 0);

        appendGposChainViewU16(data, 0);
        appendGposChainViewU16(data, 0);

        patchGposChainViewU16(data, inputPatch, static_cast<uint16_t>(data.size()));
        appendGposChainViewCoverage(data, 10);

        return data;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposChainContextView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS ChainContext view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - Format 1 parent geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat1();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            if (!pos || pos.format() != 1 || pos.ruleSetCount() != 1)
                return fail("case 1 Format 1 geometry");

            const OpenTypeCoverageView coverage = pos.coverage();

            uint16_t coverageIndex = 999;

            if (!coverage || !coverage.find(10, coverageIndex) || coverageIndex != 0)
                return fail("case 1 Coverage");

            ++passed;
        }


        // ================================================================
        // Case 2 - Format 1 chained rule.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat1();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            const OpenTypeGposChainContextRuleSetView set = pos.ruleSet(0);

            if (!set || set.size() != 1)
                return fail("case 2 RuleSet");

            const OpenTypeGposChainContextRuleView rule = set.rule(0);

            if (!rule ||
                rule.backtrackGlyphCount() != 2 ||
                rule.inputGlyphCount() != 3 ||
                rule.lookaheadGlyphCount() != 2 ||
                rule.sequenceLookupCount() != 2)
            {
                return fail("case 2 rule geometry");
            }

            uint16_t value = 0;

            if (!rule.backtrackGlyphId(0, value) || value != 12 ||
                !rule.backtrackGlyphId(1, value) || value != 11)
            {
                return fail("case 2 backtrack");
            }

            if (!rule.inputGlyphId(0, value) || value != 20 ||
                !rule.inputGlyphId(1, value) || value != 30)
            {
                return fail("case 2 input");
            }

            if (!rule.lookaheadGlyphId(0, value) || value != 40 ||
                !rule.lookaheadGlyphId(1, value) || value != 50)
            {
                return fail("case 2 lookahead");
            }

            OpenTypeSequenceLookup action{};

            if (!rule.sequenceLookup(0, action) ||
                action.sequenceIndex != 1 ||
                action.lookupListIndex != 7)
            {
                return fail("case 2 action 0");
            }

            if (!rule.sequenceLookup(1, action) ||
                action.sequenceIndex != 2 ||
                action.lookupListIndex != 8)
            {
                return fail("case 2 action 1");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Format 2 geometry and ClassDefs.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat2();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            if (!pos || pos.format() != 2 || pos.classSetCount() != 2)
                return fail("case 3 Format 2 geometry");

            const OpenTypeClassDefView backtrack = pos.backtrackClassDef();
            const OpenTypeClassDefView input = pos.inputClassDef();
            const OpenTypeClassDefView lookahead = pos.lookaheadClassDef();

            if (!backtrack || !input || !lookahead)
                return fail("case 3 ClassDefs");

            uint16_t classValue = 0;

            if (!backtrack.classValue(12, classValue) || classValue != 2 ||
                !backtrack.classValue(11, classValue) || classValue != 3)
            {
                return fail("case 3 backtrack classes");
            }

            if (!input.classValue(10, classValue) || classValue != 1 ||
                !input.classValue(20, classValue) || classValue != 2 ||
                !input.classValue(30, classValue) || classValue != 3)
            {
                return fail("case 3 input classes");
            }

            if (!lookahead.classValue(40, classValue) || classValue != 4 ||
                !lookahead.classValue(50, classValue) || classValue != 5)
            {
                return fail("case 3 lookahead classes");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Format 2 ClassRule.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat2();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            uint16_t nullOffset = 999;

            if (!pos.classSetOffset(0, nullOffset) || nullOffset != 0)
                return fail("case 4 NULL ClassSet");

            const OpenTypeGposChainContextClassSetView set = pos.classSet(1);

            if (!set || set.size() != 1)
                return fail("case 4 ClassSet");

            const OpenTypeGposChainContextClassRuleView rule = set.rule(0);

            if (!rule ||
                rule.backtrackGlyphCount() != 2 ||
                rule.inputGlyphCount() != 3 ||
                rule.lookaheadGlyphCount() != 2 ||
                rule.sequenceLookupCount() != 2)
            {
                return fail("case 4 ClassRule geometry");
            }

            uint16_t value = 0;

            if (!rule.backtrackClass(0, value) || value != 2 ||
                !rule.backtrackClass(1, value) || value != 3)
            {
                return fail("case 4 backtrack classes");
            }

            if (!rule.inputClass(0, value) || value != 2 ||
                !rule.inputClass(1, value) || value != 3)
            {
                return fail("case 4 input classes");
            }

            if (!rule.lookaheadClass(0, value) || value != 4 ||
                !rule.lookaheadClass(1, value) || value != 5)
            {
                return fail("case 4 lookahead classes");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Format 3 geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat3();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            if (!pos ||
                pos.format() != 3 ||
                pos.backtrackGlyphCount() != 2 ||
                pos.inputGlyphCount() != 3 ||
                pos.lookaheadGlyphCount() != 2 ||
                pos.sequenceLookupCount() != 2)
            {
                return fail("case 5 Format 3 geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Format 3 Coverages and actions.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat3();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            const uint16_t backtrackGlyphs[] = { 12, 11 };
            const uint16_t inputGlyphs[] = { 10, 20, 30 };
            const uint16_t lookaheadGlyphs[] = { 40, 50 };

            uint16_t index = 999;

            for (uint16_t i = 0; i < 2; ++i)
            {
                const OpenTypeCoverageView coverage = pos.backtrackCoverage(i);

                if (!coverage ||
                    !coverage.find(backtrackGlyphs[i], index) ||
                    index != 0)
                {
                    return fail("case 6 backtrack Coverage");
                }
            }

            for (uint16_t i = 0; i < 3; ++i)
            {
                const OpenTypeCoverageView coverage = pos.inputCoverage(i);

                if (!coverage ||
                    !coverage.find(inputGlyphs[i], index) ||
                    index != 0)
                {
                    return fail("case 6 input Coverage");
                }
            }

            for (uint16_t i = 0; i < 2; ++i)
            {
                const OpenTypeCoverageView coverage = pos.lookaheadCoverage(i);

                if (!coverage ||
                    !coverage.find(lookaheadGlyphs[i], index) ||
                    index != 0)
                {
                    return fail("case 6 lookahead Coverage");
                }
            }

            OpenTypeSequenceLookup action{};

            if (!pos.sequenceLookup(0, action) ||
                action.sequenceIndex != 1 ||
                action.lookupListIndex != 7)
            {
                return fail("case 6 action 0");
            }

            if (!pos.sequenceLookup(1, action) ||
                action.sequenceIndex != 2 ||
                action.lookupListIndex != 8)
            {
                return fail("case 6 action 1");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Empty backtrack and lookahead are legal.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGposChainViewFormat3InputOnly();
            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            if (!pos ||
                pos.format() != 3 ||
                pos.backtrackGlyphCount() != 0 ||
                pos.inputGlyphCount() != 1 ||
                pos.lookaheadGlyphCount() != 0 ||
                pos.sequenceLookupCount() != 0)
            {
                return fail("case 7 input-only geometry");
            }

            const OpenTypeCoverageView coverage = pos.inputCoverage(0);
            uint16_t index = 999;

            if (!coverage || !coverage.find(10, index) || index != 0)
                return fail("case 7 input Coverage");

            ++passed;
        }


        // ================================================================
        // Case 8 - Lazy child validation.
        //
        // Corrupt one Format 3 input Coverage. Parent variable geometry
        // remains valid, but the child Coverage must fail when requested.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGposChainViewFormat3();

            // Format 3:
            //
            //   format              0
            //   backtrackCount      2
            //   backtrackOffsets    4,6
            //   inputCount          8
            //   inputOffsets        10,12,14
            //
            // First input Coverage offset is therefore stored at byte 10.

            const uint16_t coverageOffset = readGposChainViewU16(data, 10);

            if (size_t(coverageOffset) + 2 > data.size())
                return fail("case 8 synthetic Coverage bounds");

            patchGposChainViewU16(data, coverageOffset, 9);

            const OpenTypeGposChainContextPosView pos(ByteSpan(data.data(), data.size()));

            if (!pos)
                return fail("case 8 parent rejected child");

            if (pos.inputCoverage(0))
                return fail("case 8 malformed Coverage accepted");

            ++passed;
        }


        // ================================================================
        // Case 9 - Structural failures.
        // ================================================================

        {
            ++cases;


            // Unsupported format.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x04
                };

                const OpenTypeGposChainContextPosView pos(ByteSpan(bytes, sizeof(bytes)));

                if (pos)
                    return fail("case 9 unsupported format");
            }


            // Truncated Format 1.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x06
                };

                const OpenTypeGposChainContextPosView pos(ByteSpan(bytes, sizeof(bytes)));

                if (pos)
                    return fail("case 9 truncated Format 1");
            }


            // Truncated Format 2.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x02,
                    0x00, 0x0C,
                    0x00, 0x00
                };

                const OpenTypeGposChainContextPosView pos(ByteSpan(bytes, sizeof(bytes)));

                if (pos)
                    return fail("case 9 truncated Format 2");
            }


            // Format 3 with zero input count is structurally invalid.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x03,
                    0x00, 0x00,
                    0x00, 0x00
                };

                const OpenTypeGposChainContextPosView pos(ByteSpan(bytes, sizeof(bytes)));

                if (pos)
                    return fail("case 9 zero input count");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS ChainContext view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 geometry:          PASS\n"
            "  Format 1 chained rule:      PASS\n"
            "  Format 2 ClassDefs:         PASS\n"
            "  Format 2 ClassRule:         PASS\n"
            "  Format 3 geometry:          PASS\n"
            "  Format 3 Coverages:         PASS\n"
            "  Empty context sides:        PASS\n"
            "  Lazy child validation:      PASS\n"
            "  Structural failures:        PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs