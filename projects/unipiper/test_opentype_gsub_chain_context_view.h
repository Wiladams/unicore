// test_opentype_gsub_chain_context_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_chain_context_view.h"

namespace waavs
{
    // ====================================================================
    // Binary construction helpers.
    // ====================================================================

    static void appendGsubChainContextU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubChainContextU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static void appendGsubChainContextCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, glyphId);
    }


    static void appendGsubChainContextClassDef2(std::vector<uint8_t>& data, const uint16_t* starts,
        const uint16_t* ends, const uint16_t* classes, uint16_t count)
    {
        appendGsubChainContextU16(data, 2);
        appendGsubChainContextU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
        {
            appendGsubChainContextU16(data, starts[i]);
            appendGsubChainContextU16(data, ends[i]);
            appendGsubChainContextU16(data, classes[i]);
        }
    }


    // ====================================================================
    // Format 1.
    //
    // Coverage:
    //
    //   glyph 10 -> RuleSet 0
    //
    // RuleSet 0:
    //
    //   backtrack stored nearest-first:
    //
    //       12, 11
    //
    //   input:
    //
    //       10, 20, 30
    //
    //   lookahead:
    //
    //       40, 50
    //
    //   SequenceLookup:
    //
    //       { 1,      7 }
    //       { 0xFFFF, 8 }
    //
    // RuleSet 1 is NULL.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainContextFormat1()
    {
        std::vector<uint8_t> data;

        appendGsubChainContextU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 2);

        const size_t ruleSet0Patch = data.size();
        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 0);


        // RuleSet 0.
        //
        // RuleSet offsets in the parent are relative to the Type 6 subtable.

        const size_t ruleSet0Base = data.size();
        patchGsubChainContextU16(data, ruleSet0Patch, static_cast<uint16_t>(ruleSet0Base));

        appendGsubChainContextU16(data, 1);

        const size_t rule0Patch = data.size();
        appendGsubChainContextU16(data, 0);


        // Rule 0.
        //
        // Rule offsets are relative to the RuleSet.

        const size_t rule0Offset = data.size() - ruleSet0Base;
        patchGsubChainContextU16(data, rule0Patch, static_cast<uint16_t>(rule0Offset));


        // Backtrack: nearest-first 12, 11.

        appendGsubChainContextU16(data, 2);
        appendGsubChainContextU16(data, 12);
        appendGsubChainContextU16(data, 11);


        // Input sequence: 10, 20, 30.
        //
        // The first glyph, 10, is supplied by Coverage.

        appendGsubChainContextU16(data, 3);
        appendGsubChainContextU16(data, 20);
        appendGsubChainContextU16(data, 30);


        // Lookahead.

        appendGsubChainContextU16(data, 2);
        appendGsubChainContextU16(data, 40);
        appendGsubChainContextU16(data, 50);


        // SequenceLookups.

        appendGsubChainContextU16(data, 2);

        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 7);

        appendGsubChainContextU16(data, 0xFFFFu);
        appendGsubChainContextU16(data, 8);


        // Coverage.
        //
        // Coverage offset is relative to the Type 6 subtable.

        const size_t coverageOffset = data.size();
        patchGsubChainContextU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainContextCoverage1(data, 10);

        return data;
    }


    // ====================================================================
    // Format 2.
    //
    // Current input glyph:
    //
    //   10 -> input class 2 -> ClassSet 2
    //
    // Backtrack:
    //
    //   12 -> class 1
    //   11 -> class 2
    //
    // Input:
    //
    //   10 -> class 2
    //   20 -> class 3
    //   30 -> class 4
    //
    // Lookahead:
    //
    //   40 -> class 5
    //   50 -> class 6
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainContextFormat2()
    {
        std::vector<uint8_t> data;

        appendGsubChainContextU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t backtrackClassDefPatch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t inputClassDefPatch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t lookaheadClassDefPatch = data.size();
        appendGsubChainContextU16(data, 0);


        // Four ClassSet entries. Only class 2 has a rule set.

        appendGsubChainContextU16(data, 4);

        appendGsubChainContextU16(data, 0);
        appendGsubChainContextU16(data, 0);

        const size_t classSet2Patch = data.size();
        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 0);


        // ClassSet 2.

        const size_t classSet2Base = data.size();
        patchGsubChainContextU16(data, classSet2Patch, static_cast<uint16_t>(classSet2Base));

        appendGsubChainContextU16(data, 1);

        const size_t classRulePatch = data.size();
        appendGsubChainContextU16(data, 0);


        // ClassRule offset is relative to ClassSet 2.

        const size_t classRuleOffset = data.size() - classSet2Base;
        patchGsubChainContextU16(data, classRulePatch, static_cast<uint16_t>(classRuleOffset));


        // Backtrack classes.

        appendGsubChainContextU16(data, 2);
        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 2);


        // Input classes.
        //
        // Position 0 class 2 is selected by the parent input ClassDef.

        appendGsubChainContextU16(data, 3);
        appendGsubChainContextU16(data, 3);
        appendGsubChainContextU16(data, 4);


        // Lookahead classes.

        appendGsubChainContextU16(data, 2);
        appendGsubChainContextU16(data, 5);
        appendGsubChainContextU16(data, 6);


        // SequenceLookups.

        appendGsubChainContextU16(data, 2);

        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 7);

        appendGsubChainContextU16(data, 2);
        appendGsubChainContextU16(data, 8);


        // Coverage.

        const size_t coverageOffset = data.size();
        patchGsubChainContextU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainContextCoverage1(data, 10);


        // Backtrack ClassDef.

        const size_t backtrackClassDefOffset = data.size();
        patchGsubChainContextU16(data, backtrackClassDefPatch, static_cast<uint16_t>(backtrackClassDefOffset));

        {
            const uint16_t starts[] = { 11, 12 };
            const uint16_t ends[] = { 11, 12 };
            const uint16_t classes[] = { 2, 1 };

            appendGsubChainContextClassDef2(data, starts, ends, classes, 2);
        }


        // Input ClassDef.

        const size_t inputClassDefOffset = data.size();
        patchGsubChainContextU16(data, inputClassDefPatch, static_cast<uint16_t>(inputClassDefOffset));

        {
            const uint16_t starts[] = { 10, 20, 30 };
            const uint16_t ends[] = { 10, 20, 30 };
            const uint16_t classes[] = { 2, 3, 4 };

            appendGsubChainContextClassDef2(data, starts, ends, classes, 3);
        }


        // Lookahead ClassDef.

        const size_t lookaheadClassDefOffset = data.size();
        patchGsubChainContextU16(data, lookaheadClassDefPatch, static_cast<uint16_t>(lookaheadClassDefOffset));

        {
            const uint16_t starts[] = { 40, 50 };
            const uint16_t ends[] = { 40, 50 };
            const uint16_t classes[] = { 5, 6 };

            appendGsubChainContextClassDef2(data, starts, ends, classes, 2);
        }

        return data;
    }


    // ====================================================================
    // Format 3.
    //
    // Backtrack Coverages:
    //
    //   0 -> 12
    //   1 -> 11
    //
    // Input Coverages:
    //
    //   0 -> 10
    //   1 -> 20
    //   2 -> 30
    //
    // Lookahead Coverages:
    //
    //   0 -> 40
    //   1 -> 50
    //
    // SequenceLookup:
    //
    //   { 1,      7 }
    //   { 0xFFFF, 8 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainContextFormat3()
    {
        std::vector<uint8_t> data;

        appendGsubChainContextU16(data, 3);


        // Backtrack.

        appendGsubChainContextU16(data, 2);

        const size_t backtrack0Patch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t backtrack1Patch = data.size();
        appendGsubChainContextU16(data, 0);


        // Input.

        appendGsubChainContextU16(data, 3);

        const size_t input0Patch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t input1Patch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t input2Patch = data.size();
        appendGsubChainContextU16(data, 0);


        // Lookahead.

        appendGsubChainContextU16(data, 2);

        const size_t lookahead0Patch = data.size();
        appendGsubChainContextU16(data, 0);

        const size_t lookahead1Patch = data.size();
        appendGsubChainContextU16(data, 0);


        // SequenceLookups.

        appendGsubChainContextU16(data, 2);

        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 7);

        appendGsubChainContextU16(data, 0xFFFFu);
        appendGsubChainContextU16(data, 8);


        auto appendCoverage = [&](size_t patch, uint16_t glyphId)
            {
                const size_t offset = data.size();

                patchGsubChainContextU16(data, patch, static_cast<uint16_t>(offset));
                appendGsubChainContextCoverage1(data, glyphId);
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


    // ====================================================================
    // Format 3 with no backtrack and no lookahead.
    //
    // This is legal and represents an input-only chained context.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainContextFormat3InputOnly()
    {
        std::vector<uint8_t> data;

        appendGsubChainContextU16(data, 3);

        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 1);

        const size_t inputPatch = data.size();
        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 0);


        const size_t coverageOffset = data.size();
        patchGsubChainContextU16(data, inputPatch, static_cast<uint16_t>(coverageOffset));

        appendGsubChainContextCoverage1(data, 10);

        return data;
    }


    // ====================================================================
    // Valid Format 1 parent with malformed Coverage child.
    //
    // Parent:
    //
    //   format = 1
    //   coverageOffset = 8
    //   ruleSetCount = 1
    //   ruleSetOffset[0] = NULL
    //
    // Child Coverage begins with unsupported format 9.
    // ====================================================================

    static std::vector<uint8_t> makeGsubChainContextLazyInvalidCoverage()
    {
        std::vector<uint8_t> data;

        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 8);
        appendGsubChainContextU16(data, 1);
        appendGsubChainContextU16(data, 0);

        appendGsubChainContextU16(data, 9);

        return data;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGsubChainContextView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB ChainContextSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1 - Format 1 parent, Coverage and offset bases.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat1();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst || subst.format() != 1 || subst.ruleSetCount() != 2)
                return fail("case 1 Format 1 header");

            const OpenTypeCoverageView coverage = subst.coverage();

            if (!coverage)
                return fail("case 1 Coverage");

            uint16_t coverageIndex = 0;

            if (!coverage.find(10, coverageIndex) || coverageIndex != 0)
                return fail("case 1 Coverage glyph");

            if (coverage.find(11, coverageIndex))
                return fail("case 1 Coverage non-member");

            uint16_t ruleSetOffset = 0;

            if (!subst.ruleSetOffset(0, ruleSetOffset) || ruleSetOffset != 10)
                return fail("case 1 RuleSet offset base");

            if (subst.ruleSet(1))
                return fail("case 1 NULL RuleSet");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 1 rule geometry and reverse backtrack storage.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat1();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));
            const OpenTypeGsubChainContextRuleSetView set = subst.ruleSet(0);

            if (!set || set.size() != 1)
                return fail("case 2 RuleSet");

            uint16_t ruleOffset = 0;

            if (!set.ruleOffset(0, ruleOffset) || ruleOffset != 4)
                return fail("case 2 rule offset base");

            const OpenTypeGsubChainContextRuleView rule = set.rule(0);

            if (!rule)
                return fail("case 2 rule");

            if (rule.backtrackGlyphCount() != 2 ||
                rule.inputGlyphCount() != 3 ||
                rule.lookaheadGlyphCount() != 2 ||
                rule.sequenceLookupCount() != 2)
            {
                return fail("case 2 rule counts");
            }

            uint16_t value = 0;

            if (!rule.backtrackGlyphId(0, value) || value != 12)
                return fail("case 2 nearest backtrack");

            if (!rule.backtrackGlyphId(1, value) || value != 11)
                return fail("case 2 far backtrack");

            if (!rule.inputGlyphId(0, value) || value != 20)
                return fail("case 2 input 1");

            if (!rule.inputGlyphId(1, value) || value != 30)
                return fail("case 2 input 2");

            if (!rule.lookaheadGlyphId(0, value) || value != 40)
                return fail("case 2 lookahead 0");

            if (!rule.lookaheadGlyphId(1, value) || value != 50)
                return fail("case 2 lookahead 1");

            if (rule.backtrackGlyphId(2, value) ||
                rule.inputGlyphId(2, value) ||
                rule.lookaheadGlyphId(2, value))
            {
                return fail("case 2 sequence bounds");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Format 1 SequenceLookup decoding.
        //
        // 0xFFFF must survive view decoding. It is not constrained against
        // the original inputGlyphCount.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat1();
            const OpenTypeGsubChainContextRuleView rule =
                OpenTypeGsubChainContextSubstView(ByteSpan(data.data(), data.size())).ruleSet(0).rule(0);

            OpenTypeSequenceLookup lookup{};

            if (!rule.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 1 ||
                lookup.lookupListIndex != 7)
            {
                return fail("case 3 SequenceLookup 0");
            }

            if (!rule.sequenceLookup(1, lookup) ||
                lookup.sequenceIndex != 0xFFFFu ||
                lookup.lookupListIndex != 8)
            {
                return fail("case 3 dynamic sequenceIndex");
            }

            if (rule.sequenceLookup(2, lookup))
                return fail("case 3 SequenceLookup bounds");

            ++passed;
        }


        // ====================================================================
        // Case 4 - Format 2 parent and three independent ClassDefs.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat2();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst || subst.format() != 2 || subst.classSetCount() != 4)
                return fail("case 4 Format 2 header");

            const OpenTypeCoverageView coverage = subst.coverage();
            const OpenTypeClassDefView backtrack = subst.backtrackClassDef();
            const OpenTypeClassDefView input = subst.inputClassDef();
            const OpenTypeClassDefView lookahead = subst.lookaheadClassDef();

            if (!coverage || !backtrack || !input || !lookahead)
                return fail("case 4 Format 2 children");

            uint16_t coverageIndex = 0;

            if (!coverage.find(10, coverageIndex) || coverageIndex != 0)
                return fail("case 4 Coverage");

            uint16_t classValue = 0;

            if (!backtrack.classValue(12, classValue) || classValue != 1)
                return fail("case 4 backtrack class 12");

            if (!backtrack.classValue(11, classValue) || classValue != 2)
                return fail("case 4 backtrack class 11");

            if (!input.classValue(10, classValue) || classValue != 2)
                return fail("case 4 input class 10");

            if (!input.classValue(20, classValue) || classValue != 3)
                return fail("case 4 input class 20");

            if (!input.classValue(30, classValue) || classValue != 4)
                return fail("case 4 input class 30");

            if (!lookahead.classValue(40, classValue) || classValue != 5)
                return fail("case 4 lookahead class 40");

            if (!lookahead.classValue(50, classValue) || classValue != 6)
                return fail("case 4 lookahead class 50");

            if (!input.classValue(99, classValue) || classValue != 0)
                return fail("case 4 implicit class 0");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Format 2 ClassSet and ClassRule.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat2();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            if (subst.classSet(0) || subst.classSet(1) || subst.classSet(3))
                return fail("case 5 NULL ClassSet");

            uint16_t classSetOffset = 0;

            if (!subst.classSetOffset(2, classSetOffset) || classSetOffset != 20)
                return fail("case 5 ClassSet offset base");

            const OpenTypeGsubChainContextClassSetView set = subst.classSet(2);

            if (!set || set.size() != 1)
                return fail("case 5 ClassSet 2");

            uint16_t ruleOffset = 0;

            if (!set.ruleOffset(0, ruleOffset) || ruleOffset != 4)
                return fail("case 5 ClassRule offset base");

            const OpenTypeGsubChainContextClassRuleView rule = set.rule(0);

            if (!rule ||
                rule.backtrackGlyphCount() != 2 ||
                rule.inputGlyphCount() != 3 ||
                rule.lookaheadGlyphCount() != 2 ||
                rule.sequenceLookupCount() != 2)
            {
                return fail("case 5 ClassRule header");
            }

            uint16_t value = 0;

            if (!rule.backtrackClass(0, value) || value != 1)
                return fail("case 5 backtrack class 0");

            if (!rule.backtrackClass(1, value) || value != 2)
                return fail("case 5 backtrack class 1");

            if (!rule.inputClass(0, value) || value != 3)
                return fail("case 5 input class 1");

            if (!rule.inputClass(1, value) || value != 4)
                return fail("case 5 input class 2");

            if (!rule.lookaheadClass(0, value) || value != 5)
                return fail("case 5 lookahead class 0");

            if (!rule.lookaheadClass(1, value) || value != 6)
                return fail("case 5 lookahead class 1");

            OpenTypeSequenceLookup lookup{};

            if (!rule.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 1 ||
                lookup.lookupListIndex != 7)
            {
                return fail("case 5 SequenceLookup 0");
            }

            if (!rule.sequenceLookup(1, lookup) ||
                lookup.sequenceIndex != 2 ||
                lookup.lookupListIndex != 8)
            {
                return fail("case 5 SequenceLookup 1");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Format 3 Coverage arrays.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat3();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst ||
                subst.format() != 3 ||
                subst.backtrackGlyphCount() != 2 ||
                subst.inputGlyphCount() != 3 ||
                subst.lookaheadGlyphCount() != 2 ||
                subst.sequenceLookupCount() != 2)
            {
                return fail("case 6 Format 3 header");
            }

            const uint16_t backtrackGlyphs[] = { 12, 11 };
            const uint16_t inputGlyphs[] = { 10, 20, 30 };
            const uint16_t lookaheadGlyphs[] = { 40, 50 };

            uint16_t coverageIndex = 0;

            for (uint16_t i = 0; i < 2; ++i)
            {
                const OpenTypeCoverageView coverage = subst.backtrackCoverage(i);

                if (!coverage || !coverage.find(backtrackGlyphs[i], coverageIndex))
                    return fail("case 6 backtrack Coverage");
            }

            for (uint16_t i = 0; i < 3; ++i)
            {
                const OpenTypeCoverageView coverage = subst.inputCoverage(i);

                if (!coverage || !coverage.find(inputGlyphs[i], coverageIndex))
                    return fail("case 6 input Coverage");
            }

            for (uint16_t i = 0; i < 2; ++i)
            {
                const OpenTypeCoverageView coverage = subst.lookaheadCoverage(i);

                if (!coverage || !coverage.find(lookaheadGlyphs[i], coverageIndex))
                    return fail("case 6 lookahead Coverage");
            }

            if (subst.backtrackCoverage(2) ||
                subst.inputCoverage(3) ||
                subst.lookaheadCoverage(2))
            {
                return fail("case 6 Coverage bounds");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Format 3 dynamic SequenceLookup.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat3();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            OpenTypeSequenceLookup lookup{};

            if (!subst.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 1 ||
                lookup.lookupListIndex != 7)
            {
                return fail("case 7 SequenceLookup 0");
            }

            if (!subst.sequenceLookup(1, lookup) ||
                lookup.sequenceIndex != 0xFFFFu ||
                lookup.lookupListIndex != 8)
            {
                return fail("case 7 dynamic sequenceIndex");
            }

            if (subst.sequenceLookup(2, lookup))
                return fail("case 7 SequenceLookup bounds");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Zero backtrack and lookahead are legal.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextFormat3InputOnly();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst ||
                subst.format() != 3 ||
                subst.backtrackGlyphCount() != 0 ||
                subst.inputGlyphCount() != 1 ||
                subst.lookaheadGlyphCount() != 0 ||
                subst.sequenceLookupCount() != 0)
            {
                return fail("case 8 input-only header");
            }

            const OpenTypeCoverageView coverage = subst.inputCoverage(0);

            uint16_t coverageIndex = 0;

            if (!coverage || !coverage.find(10, coverageIndex))
                return fail("case 8 input Coverage");

            if (subst.backtrackCoverage(0) || subst.lookaheadCoverage(0))
                return fail("case 8 empty context Coverage");

            ++passed;
        }


        // ====================================================================
        // Case 9 - Lazy child validation.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubChainContextLazyInvalidCoverage();
            const OpenTypeGsubChainContextSubstView subst(ByteSpan(data.data(), data.size()));

            if (!subst)
                return fail("case 9 valid parent rejected");

            if (subst.coverage())
                return fail("case 9 malformed Coverage accepted");

            uint16_t offset = 0;

            if (!subst.ruleSetOffset(0, offset) || offset != 0)
                return fail("case 9 NULL RuleSet offset");

            if (subst.ruleSet(0))
                return fail("case 9 NULL RuleSet view");

            ++passed;
        }


        // ====================================================================
        // Case 10 - Failure paths.
        // ====================================================================

        {
            ++cases;


            // Unsupported format.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x04
                };

                const OpenTypeGsubChainContextSubstView subst(ByteSpan(bytes, sizeof(bytes)));

                if (subst)
                    return fail("case 10 unsupported format");
            }


            // Truncated Format 1.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x06
                };

                const OpenTypeGsubChainContextSubstView subst(ByteSpan(bytes, sizeof(bytes)));

                if (subst)
                    return fail("case 10 truncated Format 1");
            }


            // Format 3 with inputGlyphCount == 0.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x03,     // format
                    0x00, 0x00,     // backtrackGlyphCount
                    0x00, 0x00      // inputGlyphCount
                };

                const OpenTypeGsubChainContextSubstView subst(ByteSpan(bytes, sizeof(bytes)));

                if (subst)
                    return fail("case 10 zero inputGlyphCount");
            }


            // ChainedSequenceRule with inputGlyphCount == 0.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x00,     // backtrackGlyphCount
                    0x00, 0x00      // inputGlyphCount
                };

                const OpenTypeGsubChainContextRuleView rule(ByteSpan(bytes, sizeof(bytes)));

                if (rule)
                    return fail("case 10 zero rule inputGlyphCount");
            }


            // Truncated ChainedClassSequenceRule.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,     // backtrackGlyphCount
                    0x00, 0x02,     // backtrackSequence[0]
                    0x00            // truncated inputGlyphCount
                };

                const OpenTypeGsubChainContextClassRuleView rule(ByteSpan(bytes, sizeof(bytes)));

                if (rule)
                    return fail("case 10 truncated ClassRule");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ChainContextSubst view: PASS\n"
            "  Cases:                         %u\n"
            "  Passed:                        %u\n"
            "  Format 1 parent/offsets:       PASS\n"
            "  Format 1 rule geometry:        PASS\n"
            "  Dynamic SequenceLookup:        PASS\n"
            "  Format 2 ClassDefs:            PASS\n"
            "  Format 2 class rules:          PASS\n"
            "  Format 3 Coverages:            PASS\n"
            "  Format 3 SequenceLookup:       PASS\n"
            "  Empty backtrack/lookahead:     PASS\n"
            "  Lazy child validation:         PASS\n"
            "  Failure paths:                 PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs