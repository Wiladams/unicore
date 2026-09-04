// test_opentype_gsub_context_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_sequence_lookup.h"
#include "opentype_gsub_context_view.h"

namespace waavs
{
    static void appendGsubContextU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubContextU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Format 1
    //
    // Coverage:
    //   index 0 -> glyph 10 -> RuleSet 0
    //   index 1 -> glyph 20 -> NULL RuleSet
    //
    // RuleSet 0:
    //
    //   Rule 0:
    //     input sequence: 10, 11, 12
    //     SequenceLookup { 1, 7 }
    //     SequenceLookup { 2, 8 }
    //
    //   Rule 1:
    //     input sequence: 10, 13
    //     SequenceLookup { 0, 9 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextFormat1()
    {
        std::vector<uint8_t> data;

        appendGsubContextU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubContextU16(data, 0);

        appendGsubContextU16(data, 2);

        const size_t set0Patch = data.size();
        appendGsubContextU16(data, 0);

        appendGsubContextU16(data, 0);


        // Coverage Format 1.

        const size_t coverageOffset = data.size();
        patchGsubContextU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 10);
        appendGsubContextU16(data, 20);


        // RuleSet 0.

        const size_t set0Base = data.size();
        patchGsubContextU16(data, set0Patch, static_cast<uint16_t>(set0Base));

        appendGsubContextU16(data, 2);

        const size_t rule0Patch = data.size();
        appendGsubContextU16(data, 0);

        const size_t rule1Patch = data.size();
        appendGsubContextU16(data, 0);


        // Rule 0.

        const size_t rule0Offset = data.size() - set0Base;
        patchGsubContextU16(data, rule0Patch, static_cast<uint16_t>(rule0Offset));

        appendGsubContextU16(data, 3);
        appendGsubContextU16(data, 2);

        appendGsubContextU16(data, 11);
        appendGsubContextU16(data, 12);

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 7);

        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 8);


        // Rule 1.

        const size_t rule1Offset = data.size() - set0Base;
        patchGsubContextU16(data, rule1Patch, static_cast<uint16_t>(rule1Offset));

        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 1);

        appendGsubContextU16(data, 13);

        appendGsubContextU16(data, 0);
        appendGsubContextU16(data, 9);

        return data;
    }


    // ====================================================================
    // Format 2
    //
    // Coverage:
    //   glyphs 30, 31
    //
    // ClassDef:
    //   glyph 30 -> class 1
    //   glyph 31 -> class 2
    //
    // ClassSet 0 -> NULL
    // ClassSet 1 -> one rule
    // ClassSet 2 -> NULL
    //
    // Rule:
    //   classes: 1, 4, 5
    //   SequenceLookup { 1, 6 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextFormat2()
    {
        std::vector<uint8_t> data;

        appendGsubContextU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubContextU16(data, 0);

        const size_t classDefPatch = data.size();
        appendGsubContextU16(data, 0);

        appendGsubContextU16(data, 3);

        appendGsubContextU16(data, 0);

        const size_t classSet1Patch = data.size();
        appendGsubContextU16(data, 0);

        appendGsubContextU16(data, 0);


        // Coverage Format 1.

        const size_t coverageOffset = data.size();
        patchGsubContextU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 30);
        appendGsubContextU16(data, 31);


        // ClassDef Format 2.

        const size_t classDefOffset = data.size();
        patchGsubContextU16(data, classDefPatch, static_cast<uint16_t>(classDefOffset));

        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 2);

        appendGsubContextU16(data, 30);
        appendGsubContextU16(data, 30);
        appendGsubContextU16(data, 1);

        appendGsubContextU16(data, 31);
        appendGsubContextU16(data, 31);
        appendGsubContextU16(data, 2);


        // ClassSet 1.

        const size_t classSet1Base = data.size();
        patchGsubContextU16(data, classSet1Patch, static_cast<uint16_t>(classSet1Base));

        appendGsubContextU16(data, 1);

        const size_t classRulePatch = data.size();
        appendGsubContextU16(data, 0);


        // ClassRule.

        const size_t classRuleOffset = data.size() - classSet1Base;
        patchGsubContextU16(data, classRulePatch, static_cast<uint16_t>(classRuleOffset));

        appendGsubContextU16(data, 3);
        appendGsubContextU16(data, 1);

        appendGsubContextU16(data, 4);
        appendGsubContextU16(data, 5);

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 6);

        return data;
    }


    // ====================================================================
    // Format 3
    //
    // Input position 0:
    //   Coverage Format 1 -> glyph 10
    //
    // Input position 1:
    //   Coverage Format 2 -> glyphs 20..21
    //
    // Input position 2:
    //   Coverage Format 1 -> glyph 30
    //
    // SequenceLookups:
    //   { 0, 4 }
    //   { 2, 5 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextFormat3()
    {
        std::vector<uint8_t> data;

        appendGsubContextU16(data, 3);
        appendGsubContextU16(data, 3);
        appendGsubContextU16(data, 2);

        const size_t coverage0Patch = data.size();
        appendGsubContextU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGsubContextU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGsubContextU16(data, 0);


        // SequenceLookup 0.

        appendGsubContextU16(data, 0);
        appendGsubContextU16(data, 4);


        // SequenceLookup 1.

        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 5);


        // Coverage 0 - Format 1.

        const size_t coverage0Offset = data.size();
        patchGsubContextU16(data, coverage0Patch, static_cast<uint16_t>(coverage0Offset));

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 10);


        // Coverage 1 - Format 2.

        const size_t coverage1Offset = data.size();
        patchGsubContextU16(data, coverage1Patch, static_cast<uint16_t>(coverage1Offset));

        appendGsubContextU16(data, 2);
        appendGsubContextU16(data, 1);

        appendGsubContextU16(data, 20);
        appendGsubContextU16(data, 21);
        appendGsubContextU16(data, 0);


        // Coverage 2 - Format 1.

        const size_t coverage2Offset = data.size();
        patchGsubContextU16(data, coverage2Patch, static_cast<uint16_t>(coverage2Offset));

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 30);

        return data;
    }


    // ====================================================================
    // Valid parent, malformed Coverage child.
    // ====================================================================

    static std::vector<uint8_t> makeGsubContextLazyInvalidCoverage()
    {
        std::vector<uint8_t> data;

        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 8);
        appendGsubContextU16(data, 1);
        appendGsubContextU16(data, 0);

        appendGsubContextU16(data, 9);

        return data;
    }


    static bool testOpenTypeGsubContextView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB ContextSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1 - Format 1 parent and Coverage.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextFormat1();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            if (!context)
                return fail("case 1 Format 1 invalid");

            if (context.format() != 1 || context.ruleSetCount() != 2)
                return fail("case 1 Format 1 header");

            const OpenTypeCoverageView coverage = context.coverage();

            if (!coverage)
                return fail("case 1 Coverage invalid");

            uint16_t coverageIndex = 0;

            if (!coverage.find(10, coverageIndex) || coverageIndex != 0)
                return fail("case 1 Coverage glyph 10");

            if (!coverage.find(20, coverageIndex) || coverageIndex != 1)
                return fail("case 1 Coverage glyph 20");

            if (coverage.find(30, coverageIndex))
                return fail("case 1 Coverage non-member");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Format 1 rules and stored rule order.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextFormat1();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));
            const OpenTypeGsubContextRuleSetView set = context.ruleSet(0);

            if (!set || set.size() != 2)
                return fail("case 2 RuleSet");

            const OpenTypeGsubContextRuleView rule0 = set.rule(0);
            const OpenTypeGsubContextRuleView rule1 = set.rule(1);

            if (!rule0 || !rule1)
                return fail("case 2 rule access");

            if (rule0.glyphCount() != 3 || rule0.sequenceLookupCount() != 2)
                return fail("case 2 rule 0 header");

            uint16_t glyph = 0;

            if (!rule0.inputGlyphId(0, glyph) || glyph != 11)
                return fail("case 2 rule 0 input 0");

            if (!rule0.inputGlyphId(1, glyph) || glyph != 12)
                return fail("case 2 rule 0 input 1");

            OpenTypeSequenceLookup lookup{};

            if (!rule0.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 1 ||
                lookup.lookupListIndex != 7)
            {
                return fail("case 2 rule 0 lookup 0");
            }

            if (!rule0.sequenceLookup(1, lookup) ||
                lookup.sequenceIndex != 2 ||
                lookup.lookupListIndex != 8)
            {
                return fail("case 2 rule 0 lookup 1");
            }

            if (rule1.glyphCount() != 2 || rule1.sequenceLookupCount() != 1)
                return fail("case 2 rule 1 header");

            if (!rule1.inputGlyphId(0, glyph) || glyph != 13)
                return fail("case 2 rule order");

            if (!rule1.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 0 ||
                lookup.lookupListIndex != 9)
            {
                return fail("case 2 rule 1 lookup");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Nullable Format 1 RuleSet.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextFormat1();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            uint16_t offset = 0;

            if (!context.ruleSetOffset(1, offset) || offset != 0)
                return fail("case 3 NULL RuleSet offset");

            if (context.ruleSet(1))
                return fail("case 3 NULL RuleSet view");

            if (context.ruleSet(2))
                return fail("case 3 out-of-range RuleSet");

            ++passed;
        }


        // ====================================================================
        // Case 4 - Format 2 Coverage and ClassDef.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextFormat2();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            if (!context || context.format() != 2 || context.classSetCount() != 3)
                return fail("case 4 Format 2 header");

            const OpenTypeCoverageView coverage = context.coverage();
            const OpenTypeClassDefView classDef = context.classDef();

            if (!coverage || !classDef)
                return fail("case 4 Format 2 children");

            uint16_t coverageIndex = 0;

            if (!coverage.find(30, coverageIndex) || coverageIndex != 0)
                return fail("case 4 Coverage glyph 30");

            if (!coverage.find(31, coverageIndex) || coverageIndex != 1)
                return fail("case 4 Coverage glyph 31");

            uint16_t classValue = 0;

            if (!classDef.classValue(30, classValue) || classValue != 1)
                return fail("case 4 class 1");

            if (!classDef.classValue(31, classValue) || classValue != 2)
                return fail("case 4 class 2");

            if (!classDef.classValue(99, classValue) || classValue != 0)
                return fail("case 4 implicit class 0");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Format 2 ClassRule and nullable ClassSet.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextFormat2();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            if (context.classSet(0))
                return fail("case 5 NULL ClassSet 0");

            const OpenTypeGsubContextClassSetView set = context.classSet(1);

            if (!set || set.size() != 1)
                return fail("case 5 ClassSet 1");

            if (context.classSet(2))
                return fail("case 5 NULL ClassSet 2");

            const OpenTypeGsubContextClassRuleView rule = set.rule(0);

            if (!rule || rule.glyphCount() != 3 || rule.sequenceLookupCount() != 1)
                return fail("case 5 ClassRule header");

            uint16_t classValue = 0;

            if (!rule.inputClass(0, classValue) || classValue != 4)
                return fail("case 5 input class 0");

            if (!rule.inputClass(1, classValue) || classValue != 5)
                return fail("case 5 input class 1");

            OpenTypeSequenceLookup lookup{};

            if (!rule.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 1 ||
                lookup.lookupListIndex != 6)
            {
                return fail("case 5 SequenceLookup");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - Format 3 Coverage array and SequenceLookups.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextFormat3();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            if (!context ||
                context.format() != 3 ||
                context.glyphCount() != 3 ||
                context.sequenceLookupCount() != 2)
            {
                return fail("case 6 Format 3 header");
            }

            const OpenTypeCoverageView coverage0 = context.inputCoverage(0);
            const OpenTypeCoverageView coverage1 = context.inputCoverage(1);
            const OpenTypeCoverageView coverage2 = context.inputCoverage(2);

            if (!coverage0 || !coverage1 || !coverage2)
                return fail("case 6 Coverage access");

            uint16_t coverageIndex = 0;

            if (!coverage0.find(10, coverageIndex) || coverageIndex != 0)
                return fail("case 6 Coverage 0");

            if (!coverage1.find(20, coverageIndex) || coverageIndex != 0)
                return fail("case 6 Coverage 1 start");

            if (!coverage1.find(21, coverageIndex) || coverageIndex != 1)
                return fail("case 6 Coverage 1 index");

            if (!coverage2.find(30, coverageIndex) || coverageIndex != 0)
                return fail("case 6 Coverage 2");

            OpenTypeSequenceLookup lookup{};

            if (!context.sequenceLookup(0, lookup) ||
                lookup.sequenceIndex != 0 ||
                lookup.lookupListIndex != 4)
            {
                return fail("case 6 SequenceLookup 0");
            }

            if (!context.sequenceLookup(1, lookup) ||
                lookup.sequenceIndex != 2 ||
                lookup.lookupListIndex != 5)
            {
                return fail("case 6 SequenceLookup 1");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - SequenceLookup validation and bounds.
        //
        // Parent geometry remains valid even if a SequenceLookup contains an
        // invalid sequenceIndex. The record itself fails when accessed.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGsubContextFormat3();

            // Format 3:
            //
            // 0  format
            // 2  glyphCount
            // 4  seqLookupCount
            // 6  Coverage offsets[3]
            // 12 first SequenceLookup.sequenceIndex

            patchGsubContextU16(data, 12, 3);

            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            if (!context)
                return fail("case 7 parent rejected lazy record error");

            OpenTypeSequenceLookup lookup{};

            if (context.sequenceLookup(0, lookup))
                return fail("case 7 invalid sequenceIndex accepted");

            if (!context.sequenceLookup(1, lookup) ||
                lookup.sequenceIndex != 2 ||
                lookup.lookupListIndex != 5)
            {
                return fail("case 7 later valid SequenceLookup");
            }

            if (context.sequenceLookup(2, lookup))
                return fail("case 7 out-of-range SequenceLookup");

            if (context.inputCoverage(3))
                return fail("case 7 out-of-range Coverage");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Lazy child validation.
        //
        // The ContextSubst parent geometry is valid. The Coverage child is
        // deliberately malformed and fails only when inspected.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGsubContextLazyInvalidCoverage();
            const OpenTypeGsubContextSubstView context(ByteSpan(data.data(), data.size()));

            if (!context)
                return fail("case 8 valid parent rejected");

            const OpenTypeCoverageView coverage = context.coverage();

            if (coverage)
                return fail("case 8 malformed Coverage accepted");

            uint16_t offset = 0;

            if (!context.ruleSetOffset(0, offset) || offset != 0)
                return fail("case 8 NULL RuleSet");

            ++passed;
        }


        // ====================================================================
        // Case 9 - Failure paths.
        // ====================================================================

        {
            ++cases;

            // Unsupported format.

            const uint8_t unsupportedBytes[] = { 0x00, 0x04 };
            const OpenTypeGsubContextSubstView unsupported(
                ByteSpan(unsupportedBytes, sizeof(unsupportedBytes)));

            if (unsupported)
                return fail("case 9 unsupported format");


            // Truncated Format 1.

            const uint8_t truncatedBytes[] =
            {
                0x00, 0x01,
                0x00, 0x06
            };

            const OpenTypeGsubContextSubstView truncated(
                ByteSpan(truncatedBytes, sizeof(truncatedBytes)));

            if (truncated)
                return fail("case 9 truncated Format 1");


            // Format 3 with glyphCount == 0.

            const uint8_t emptyFormat3Bytes[] =
            {
                0x00, 0x03,
                0x00, 0x00,
                0x00, 0x00
            };

            const OpenTypeGsubContextSubstView emptyFormat3(
                ByteSpan(emptyFormat3Bytes, sizeof(emptyFormat3Bytes)));

            if (emptyFormat3)
                return fail("case 9 zero glyphCount Format 3");


            // Invalid SequenceRule: glyphCount == 0.

            const uint8_t invalidRuleBytes[] =
            {
                0x00, 0x00,
                0x00, 0x00
            };

            const OpenTypeGsubContextRuleView invalidRule(
                ByteSpan(invalidRuleBytes, sizeof(invalidRuleBytes)));

            if (invalidRule)
                return fail("case 9 zero glyphCount rule");


            // Invalid ClassSequenceRule: truncated.

            const uint8_t invalidClassRuleBytes[] =
            {
                0x00, 0x03,
                0x00, 0x01,
                0x00, 0x04
            };

            const OpenTypeGsubContextClassRuleView invalidClassRule(
                ByteSpan(invalidClassRuleBytes, sizeof(invalidClassRuleBytes)));

            if (invalidClassRule)
                return fail("case 9 truncated class rule");

            ++passed;
        }


        std::printf(
            "OpenType GSUB ContextSubst view: PASS\n"
            "  Cases:                      %u\n"
            "  Passed:                     %u\n"
            "  Format 1:                   PASS\n"
            "  Glyph rules:                PASS\n"
            "  Nullable RuleSet:           PASS\n"
            "  Format 2:                   PASS\n"
            "  Class rules:                PASS\n"
            "  Format 3:                   PASS\n"
            "  SequenceLookup validation:  PASS\n"
            "  Lazy child validation:      PASS\n"
            "  Failure paths:              PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs