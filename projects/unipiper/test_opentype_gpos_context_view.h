// test_opentype_gpos_context_view.h
#pragma once

#include "../unitils/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_context_view.h"

namespace waavs
{
    static void appendGposContextViewU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGposContextViewU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static uint16_t readGposContextViewU16(const std::vector<uint8_t>& data, size_t offset)
    {
        return static_cast<uint16_t>((uint16_t(data[offset]) << 8) | uint16_t(data[offset + 1]));
    }


    static void appendGposContextViewCoverage1(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposContextViewU16(data, 1);
        appendGposContextViewU16(data, 1);
        appendGposContextViewU16(data, glyphId);
    }


    static void appendGposContextViewClassRange(
        std::vector<uint8_t>& data, uint16_t first, uint16_t last, uint16_t classValue)
    {
        appendGposContextViewU16(data, first);
        appendGposContextViewU16(data, last);
        appendGposContextViewU16(data, classValue);
    }


    // ====================================================================
    // Format 1
    //
    // Input:
    //
    //   10 20 30
    //
    // Actions:
    //
    //   sequence 0 -> lookup 4
    //   sequence 2 -> lookup 5
    // ====================================================================

    static std::vector<uint8_t> makeGposContextViewFormat1()
    {
        std::vector<uint8_t> data;

        appendGposContextViewU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposContextViewU16(data, 0);

        appendGposContextViewU16(data, 1);

        const size_t ruleSetPatch = data.size();
        appendGposContextViewU16(data, 0);


        // Coverage.

        patchGposContextViewU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextViewCoverage1(data, 10);


        // SequenceRuleSet.

        patchGposContextViewU16(data, ruleSetPatch, static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposContextViewU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposContextViewU16(data, 0);

        patchGposContextViewU16(
            data, rulePatch,
            static_cast<uint16_t>(data.size() - setBegin));


        // SequenceRule.

        appendGposContextViewU16(data, 3);
        appendGposContextViewU16(data, 2);

        appendGposContextViewU16(data, 20);
        appendGposContextViewU16(data, 30);

        appendGposContextViewU16(data, 0);
        appendGposContextViewU16(data, 4);

        appendGposContextViewU16(data, 2);
        appendGposContextViewU16(data, 5);

        return data;
    }


    // ====================================================================
    // Format 2
    //
    // Classes:
    //
    //   glyph 10 -> class 1
    //   glyph 20 -> class 2
    //   glyph 30 -> class 3
    //
    // Context:
    //
    //   class 1, class 2, class 3
    //
    // Action:
    //
    //   sequence 1 -> lookup 7
    // ====================================================================

    static std::vector<uint8_t> makeGposContextViewFormat2()
    {
        std::vector<uint8_t> data;

        appendGposContextViewU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGposContextViewU16(data, 0);

        const size_t classDefPatch = data.size();
        appendGposContextViewU16(data, 0);

        appendGposContextViewU16(data, 2);

        appendGposContextViewU16(data, 0);

        const size_t classSet1Patch = data.size();
        appendGposContextViewU16(data, 0);


        // Coverage.

        patchGposContextViewU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposContextViewCoverage1(data, 10);


        // ClassDef Format 2.

        patchGposContextViewU16(data, classDefPatch, static_cast<uint16_t>(data.size()));

        appendGposContextViewU16(data, 2);
        appendGposContextViewU16(data, 3);

        appendGposContextViewClassRange(data, 10, 10, 1);
        appendGposContextViewClassRange(data, 20, 20, 2);
        appendGposContextViewClassRange(data, 30, 30, 3);


        // ClassSequenceRuleSet for class 1.

        patchGposContextViewU16(data, classSet1Patch, static_cast<uint16_t>(data.size()));

        const size_t setBegin = data.size();

        appendGposContextViewU16(data, 1);

        const size_t rulePatch = data.size();
        appendGposContextViewU16(data, 0);

        patchGposContextViewU16(
            data, rulePatch,
            static_cast<uint16_t>(data.size() - setBegin));


        // ClassSequenceRule.

        appendGposContextViewU16(data, 3);
        appendGposContextViewU16(data, 1);

        appendGposContextViewU16(data, 2);
        appendGposContextViewU16(data, 3);

        appendGposContextViewU16(data, 1);
        appendGposContextViewU16(data, 7);

        return data;
    }


    // ====================================================================
    // Format 3
    //
    // Position 0 -> glyph 10
    // Position 1 -> glyph 20
    // Position 2 -> glyph 30
    //
    // Actions:
    //
    //   sequence 0 -> lookup 4
    //   sequence 2 -> lookup 5
    // ====================================================================

    static std::vector<uint8_t> makeGposContextViewFormat3()
    {
        std::vector<uint8_t> data;

        appendGposContextViewU16(data, 3);
        appendGposContextViewU16(data, 3);
        appendGposContextViewU16(data, 2);

        const size_t coverage0Patch = data.size();
        appendGposContextViewU16(data, 0);

        const size_t coverage1Patch = data.size();
        appendGposContextViewU16(data, 0);

        const size_t coverage2Patch = data.size();
        appendGposContextViewU16(data, 0);


        // SequenceLookupRecords.

        appendGposContextViewU16(data, 0);
        appendGposContextViewU16(data, 4);

        appendGposContextViewU16(data, 2);
        appendGposContextViewU16(data, 5);


        patchGposContextViewU16(data, coverage0Patch, static_cast<uint16_t>(data.size()));
        appendGposContextViewCoverage1(data, 10);

        patchGposContextViewU16(data, coverage1Patch, static_cast<uint16_t>(data.size()));
        appendGposContextViewCoverage1(data, 20);

        patchGposContextViewU16(data, coverage2Patch, static_cast<uint16_t>(data.size()));
        appendGposContextViewCoverage1(data, 30);

        return data;
    }


    static bool testOpenTypeGposContextView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Context view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - Format 1 parent geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposContextViewFormat1();

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            if (!context ||
                context.format() != 1 ||
                context.ruleSetCount() != 1)
            {
                return fail("case 1 Format 1 geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Format 1 rule.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposContextViewFormat1();

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGposContextRuleSetView set =
                context.ruleSet(0);

            if (!set || set.size() != 1)
                return fail("case 2 RuleSet");

            const OpenTypeGposContextRuleView rule =
                set.rule(0);

            if (!rule ||
                rule.glyphCount() != 3 ||
                rule.sequenceLookupCount() != 2)
            {
                return fail("case 2 rule geometry");
            }

            uint16_t glyphId = 0;

            if (!rule.inputGlyphId(0, glyphId) || glyphId != 20)
                return fail("case 2 input glyph 20");

            if (!rule.inputGlyphId(1, glyphId) || glyphId != 30)
                return fail("case 2 input glyph 30");

            OpenTypeSequenceLookup action{};

            if (!rule.sequenceLookup(0, action) ||
                action.sequenceIndex != 0 ||
                action.lookupListIndex != 4)
            {
                return fail("case 2 action 0");
            }

            if (!rule.sequenceLookup(1, action) ||
                action.sequenceIndex != 2 ||
                action.lookupListIndex != 5)
            {
                return fail("case 2 action 1");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Format 1 Coverage.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposContextViewFormat1();

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            const OpenTypeCoverageView coverage =
                context.coverage();

            if (!coverage)
                return fail("case 3 Coverage");

            uint16_t index = 999;

            if (!coverage.find(10, index) || index != 0)
                return fail("case 3 Coverage mapping");

            ++passed;
        }


        // ================================================================
        // Case 4 - Format 2 class geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposContextViewFormat2();

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            if (!context ||
                context.format() != 2 ||
                context.classSetCount() != 2)
            {
                return fail("case 4 Format 2 geometry");
            }

            const OpenTypeClassDefView classDef =
                context.classDef();

            if (!classDef)
                return fail("case 4 ClassDef");

            uint16_t classValue = 0;

            if (!classDef.classValue(10, classValue) || classValue != 1)
                return fail("case 4 class 1");

            if (!classDef.classValue(20, classValue) || classValue != 2)
                return fail("case 4 class 2");

            if (!classDef.classValue(30, classValue) || classValue != 3)
                return fail("case 4 class 3");

            ++passed;
        }


        // ================================================================
        // Case 5 - Format 2 ClassRule.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposContextViewFormat2();

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            uint16_t nullOffset = 999;

            if (!context.classSetOffset(0, nullOffset) ||
                nullOffset != 0)
            {
                return fail("case 5 NULL ClassSet");
            }

            const OpenTypeGposContextClassSetView set =
                context.classSet(1);

            if (!set || set.size() != 1)
                return fail("case 5 ClassSet");

            const OpenTypeGposContextClassRuleView rule =
                set.rule(0);

            if (!rule ||
                rule.glyphCount() != 3 ||
                rule.sequenceLookupCount() != 1)
            {
                return fail("case 5 ClassRule geometry");
            }

            uint16_t classValue = 0;

            if (!rule.inputClass(0, classValue) || classValue != 2)
                return fail("case 5 input class 2");

            if (!rule.inputClass(1, classValue) || classValue != 3)
                return fail("case 5 input class 3");

            OpenTypeSequenceLookup action{};

            if (!rule.sequenceLookup(0, action) ||
                action.sequenceIndex != 1 ||
                action.lookupListIndex != 7)
            {
                return fail("case 5 action");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Format 3.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                makeGposContextViewFormat3();

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            if (!context ||
                context.format() != 3 ||
                context.glyphCount() != 3 ||
                context.sequenceLookupCount() != 2)
            {
                return fail("case 6 Format 3 geometry");
            }

            const uint16_t expectedGlyphs[] = { 10, 20, 30 };

            for (uint16_t i = 0; i < 3; ++i)
            {
                const OpenTypeCoverageView coverage =
                    context.inputCoverage(i);

                if (!coverage)
                    return fail("case 6 input Coverage");

                uint16_t index = 999;

                if (!coverage.find(expectedGlyphs[i], index) ||
                    index != 0)
                {
                    return fail("case 6 Coverage mapping");
                }
            }

            OpenTypeSequenceLookup action{};

            if (!context.sequenceLookup(0, action) ||
                action.sequenceIndex != 0 ||
                action.lookupListIndex != 4)
            {
                return fail("case 6 action 0");
            }

            if (!context.sequenceLookup(1, action) ||
                action.sequenceIndex != 2 ||
                action.lookupListIndex != 5)
            {
                return fail("case 6 action 1");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - SequenceLookup decoding is structural only.
        //
        // GPOS execution will later validate sequenceIndex against the
        // matched physical position array. The binary view preserves the
        // encoded value without assigning execution semantics to it.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                makeGposContextViewFormat3();

            // First SequenceLookupRecord begins after:
            //
            //   format             2
            //   glyphCount         2
            //   seqLookupCount     2
            //   coverageOffsets    6
            //
            // total               12

            patchGposContextViewU16(data, 12, 0xFFFFu);

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            if (!context)
                return fail("case 7 parent rejected sequenceIndex");

            OpenTypeSequenceLookup action{};

            if (!context.sequenceLookup(0, action) ||
                action.sequenceIndex != 0xFFFFu ||
                action.lookupListIndex != 4)
            {
                return fail("case 7 SequenceLookup decoding");
            }

            if (context.sequenceLookup(2, action))
                return fail("case 7 SequenceLookup bounds");

            ++passed;
        }


        // ================================================================
        // Case 8 - Lazy child validation.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                makeGposContextViewFormat1();

            const uint16_t coverageOffset =
                readGposContextViewU16(data, 2);

            if (size_t(coverageOffset) + 2 > data.size())
                return fail("case 8 synthetic Coverage bounds");

            patchGposContextViewU16(
                data, coverageOffset, 9);

            const OpenTypeGposContextPosView context(
                ByteSpan(data.data(), data.size()));

            if (!context)
                return fail("case 8 parent rejected child");

            if (context.coverage())
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

                const OpenTypeGposContextPosView context(
                    ByteSpan(bytes, sizeof(bytes)));

                if (context)
                    return fail("case 9 unsupported format");
            }


            // Truncated Format 1.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x01,
                    0x00, 0x06
                };

                const OpenTypeGposContextPosView context(
                    ByteSpan(bytes, sizeof(bytes)));

                if (context)
                    return fail("case 9 truncated Format 1");
            }


            // Truncated Format 3.

            {
                const uint8_t bytes[] =
                {
                    0x00, 0x03,
                    0x00, 0x03,
                    0x00, 0x01
                };

                const OpenTypeGposContextPosView context(
                    ByteSpan(bytes, sizeof(bytes)));

                if (context)
                    return fail("case 9 truncated Format 3");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Context view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Format 1 geometry:          PASS\n"
            "  Format 1 rules:             PASS\n"
            "  Format 1 Coverage:          PASS\n"
            "  Format 2 classes:           PASS\n"
            "  Format 2 rules:             PASS\n"
            "  Format 3:                   PASS\n"
            "  SequenceLookup decoding:    PASS\n"
            "  Lazy child validation:      PASS\n"
            "  Structural failures:        PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs