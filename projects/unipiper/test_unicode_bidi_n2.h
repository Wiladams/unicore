// test_unicode_bidi_n2.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"


namespace waavs
{
    // ========================================================================
    // runBidiN2SingleSequence
    //
    // Build one simple isolating run sequence over an identity post-X9 index
    // mapping and apply N2.
    // ========================================================================

    static bool runBidiN2SingleSequence(
        std::vector<UnicodeBidiClass>& types,
        UnicodeBidiLevel level)
    {
        if (types.empty())
            return false;


        std::vector<uint32_t> bidiIndices(
            types.size());

        for (uint32_t i = 0;
            i < static_cast<uint32_t>(bidiIndices.size());
            ++i)
        {
            bidiIndices[i] = i;
        }


        std::vector<BidiLevelRun> runs{
            {
                0,
                static_cast<uint32_t>(types.size()),
                level
            }
        };


        std::vector<uint32_t> sequenceRunIndices{
            0
        };


        BidiIsolatingRunSequence sequence{};

        sequence.runOffset = 0;
        sequence.runCount = 1;
        sequence.level = level;
        sequence.sos = bidiTypeFromLevel(level);
        sequence.eos = bidiTypeFromLevel(level);


        const std::vector<BidiIsolatingRunSequence> sequences{
            sequence
        };


        return resolveBidiNeutralTypesN2(
            bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.data(),
            static_cast<uint32_t>(types.size()),
            runs,
            sequenceRunIndices,
            sequences);
    }


    // ========================================================================
    // testUnicodeBidiN2
    // ========================================================================

    static bool testUnicodeBidiN2()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi N2: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;


        // ====================================================================
        // Case 1
        //
        // Even embedding level:
        //
        //      ON -> L
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral
            };


            if (!runBidiN2SingleSequence(types, 0))
                return fail("even-level ON execution");


            if (types[0] != UnicodeBidiClass::LeftToRight)
                return fail("even-level ON result");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Odd embedding level:
        //
        //      ON -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral
            };


            if (!runBidiN2SingleSequence(types, 1))
                return fail("odd-level ON execution");


            if (types[0] != UnicodeBidiClass::RightToLeft)
                return fail("odd-level ON result");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Every NI type at an even level becomes L.
        //
        // NI:
        //
        //      B, S, WS, ON, FSI, LRI, RLI, PDI
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ParagraphSeparator,
                UnicodeBidiClass::SegmentSeparator,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::PopDirectionalIsolate
            };


            if (!runBidiN2SingleSequence(types, 0))
                return fail("complete even-level NI set execution");


            for (UnicodeBidiClass type : types)
            {
                if (type != UnicodeBidiClass::LeftToRight)
                    return fail("complete even-level NI set result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Every NI type at an odd level becomes R.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ParagraphSeparator,
                UnicodeBidiClass::SegmentSeparator,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::PopDirectionalIsolate
            };


            if (!runBidiN2SingleSequence(types, 1))
                return fail("complete odd-level NI set execution");


            for (UnicodeBidiClass type : types)
            {
                if (type != UnicodeBidiClass::RightToLeft)
                    return fail("complete odd-level NI set result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Already resolved L/R types must not be changed.
        //
        // This covers neutrals previously resolved by N0 or N1.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN2SingleSequence(types, 0))
                return fail("resolved L/R preservation execution");


            if (types[0] != UnicodeBidiClass::LeftToRight ||
                types[1] != UnicodeBidiClass::RightToLeft ||
                types[2] != UnicodeBidiClass::LeftToRight ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("resolved L/R types were modified");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Numbers must not be changed by N2.
        //
        //      EN ON AN
        //
        // At even level:
        //
        //      EN L AN
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::ArabicNumber
            };


            if (!runBidiN2SingleSequence(types, 0))
                return fail("number preservation execution");


            if (types[0] != UnicodeBidiClass::EuropeanNumber ||
                types[1] != UnicodeBidiClass::LeftToRight ||
                types[2] != UnicodeBidiClass::ArabicNumber)
            {
                return fail("number preservation result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Multiple unresolved NI sequences are all assigned the embedding
        // direction independently.
        //
        //      L ON R WS L S R
        //
        // level 0:
        //
        //      ON, WS, S -> L
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::SegmentSeparator,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN2SingleSequence(types, 0))
                return fail("multiple NI sequences execution");


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[3] != UnicodeBidiClass::LeftToRight ||
                types[5] != UnicodeBidiClass::LeftToRight)
            {
                return fail("multiple NI sequences result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Same situation at odd embedding level.
        //
        //      L ON R WS L
        //
        //      ON, WS -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::LeftToRight
            };


            if (!runBidiN2SingleSequence(types, 1))
                return fail("odd multiple NI sequences execution");


            if (types[1] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("odd multiple NI sequences result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Non-contiguous isolating run sequence.
        //
        // Post-X9 positions:
        //
        //      0   1       2   3       4   5
        //
        //      L  ON       WS  ON      WS  R
        //      |----|                 |-----|
        //      run 0                   run 2
        //
        // IRS contains runs 0 and 2 only.
        //
        // level 0:
        //
        //      positions 1 and 4 -> L
        //
        // Positions 2 and 3 belong to a different IRS and must remain
        // untouched.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,

                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::OtherNeutral,

                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::RightToLeft
            };


            const std::vector<uint32_t> bidiIndices{
                0, 1, 2, 3, 4, 5
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 2, 0 },
                { 2, 4, 1 },
                { 4, 6, 0 }
            };


            const std::vector<uint32_t> sequenceRunIndices{
                0,
                2
            };


            BidiIsolatingRunSequence sequence{};

            sequence.runOffset = 0;
            sequence.runCount = 2;
            sequence.level = 0;
            sequence.sos = UnicodeBidiClass::LeftToRight;
            sequence.eos = UnicodeBidiClass::LeftToRight;


            const std::vector<BidiIsolatingRunSequence> sequences{
                sequence
            };


            if (!resolveBidiNeutralTypesN2(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                static_cast<uint32_t>(types.size()),
                runs,
                sequenceRunIndices,
                sequences))
            {
                return fail("non-contiguous IRS execution");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[4] != UnicodeBidiClass::LeftToRight)
            {
                return fail("non-contiguous IRS result");
            }


            if (types[2] != UnicodeBidiClass::WhiteSpace ||
                types[3] != UnicodeBidiClass::OtherNeutral)
            {
                return fail("non-contiguous IRS gap modified");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Multiple isolating run sequences use their own embedding levels.
        //
        // IRS 0, level 0:
        //
        //      ON -> L
        //
        // IRS 1, level 1:
        //
        //      WS -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::WhiteSpace
            };


            const std::vector<uint32_t> bidiIndices{
                0, 1
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 1, 0 },
                { 1, 2, 1 }
            };


            const std::vector<uint32_t> sequenceRunIndices{
                0,
                1
            };


            BidiIsolatingRunSequence sequence0{};

            sequence0.runOffset = 0;
            sequence0.runCount = 1;
            sequence0.level = 0;
            sequence0.sos = UnicodeBidiClass::LeftToRight;
            sequence0.eos = UnicodeBidiClass::LeftToRight;


            BidiIsolatingRunSequence sequence1{};

            sequence1.runOffset = 1;
            sequence1.runCount = 1;
            sequence1.level = 1;
            sequence1.sos = UnicodeBidiClass::RightToLeft;
            sequence1.eos = UnicodeBidiClass::RightToLeft;


            const std::vector<BidiIsolatingRunSequence> sequences{
                sequence0,
                sequence1
            };


            if (!resolveBidiNeutralTypesN2(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                static_cast<uint32_t>(types.size()),
                runs,
                sequenceRunIndices,
                sequences))
            {
                return fail("multiple IRS execution");
            }


            if (types[0] != UnicodeBidiClass::LeftToRight)
                return fail("even IRS result");

            if (types[1] != UnicodeBidiClass::RightToLeft)
                return fail("odd IRS result");

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // Higher embedding levels still use parity only.
        //
        //      level 4 -> L
        //      level 5 -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> evenTypes{
                UnicodeBidiClass::OtherNeutral
            };

            std::vector<UnicodeBidiClass> oddTypes{
                UnicodeBidiClass::OtherNeutral
            };


            if (!runBidiN2SingleSequence(evenTypes, 4))
                return fail("higher even level execution");

            if (!runBidiN2SingleSequence(oddTypes, 5))
                return fail("higher odd level execution");


            if (evenTypes[0] != UnicodeBidiClass::LeftToRight)
                return fail("higher even level result");

            if (oddTypes[0] != UnicodeBidiClass::RightToLeft)
                return fail("higher odd level result");

            ++passed;
        }


        // ====================================================================
        // Case 12
        //
        // Constituent level runs must match the isolating run sequence level.
        //
        // The N2 implementation explicitly checks this X10 invariant.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral
            };


            const std::vector<uint32_t> bidiIndices{
                0
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 1, 1 }
            };


            const std::vector<uint32_t> sequenceRunIndices{
                0
            };


            BidiIsolatingRunSequence sequence{};

            sequence.runOffset = 0;
            sequence.runCount = 1;
            sequence.level = 0;
            sequence.sos = UnicodeBidiClass::LeftToRight;
            sequence.eos = UnicodeBidiClass::LeftToRight;


            const std::vector<BidiIsolatingRunSequence> sequences{
                sequence
            };


            if (resolveBidiNeutralTypesN2(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                static_cast<uint32_t>(types.size()),
                runs,
                sequenceRunIndices,
                sequences))
            {
                return fail("level-run mismatch was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Case 13
        //
        // Empty post-X9 input with no isolating run sequences is valid.
        // ====================================================================

        {
            ++cases;

            const std::vector<BidiLevelRun> runs{};
            const std::vector<uint32_t> sequenceRunIndices{};
            const std::vector<BidiIsolatingRunSequence> sequences{};


            if (!resolveBidiNeutralTypesN2(
                nullptr,
                0,
                nullptr,
                0,
                runs,
                sequenceRunIndices,
                sequences))
            {
                return fail("empty input was rejected");
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi N2: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs