// test_unicode_bidi_n1.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"


namespace waavs
{
    // ========================================================================
    // runBidiN1SingleSequence
    //
    // Build one simple isolating run sequence over an identity post-X9 index
    // mapping and apply N1.
    // ========================================================================

    static bool runBidiN1SingleSequence(
        std::vector<UnicodeBidiClass>& types,
        UnicodeBidiClass sos,
        UnicodeBidiClass eos)
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
                sos == UnicodeBidiClass::RightToLeft ? 1u : 0u
            }
        };


        std::vector<uint32_t> sequenceRunIndices{
            0
        };


        BidiIsolatingRunSequence sequence{};

        sequence.runOffset = 0;
        sequence.runCount = 1;
        sequence.level =
            sos == UnicodeBidiClass::RightToLeft ? 1u : 0u;
        sequence.sos = sos;
        sequence.eos = eos;


        const std::vector<BidiIsolatingRunSequence> sequences{
            sequence
        };


        return resolveBidiNeutralTypesN1(
            bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.data(),
            static_cast<uint32_t>(types.size()),
            runs,
            sequenceRunIndices,
            sequences);
    }


    // ========================================================================
    // testUnicodeBidiN1
    // ========================================================================

    static bool testUnicodeBidiN1()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi N1: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;


        // ====================================================================
        // Case 1
        //
        // L ON L -> L L L
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRight))
            {
                return fail("L ON L execution");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight)
                return fail("L ON L result");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // R ON R -> R R R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("R ON R execution");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft)
                return fail("R ON R result");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Conflicting surrounding directions.
        //
        // L ON R
        //
        // N1 must leave ON unchanged for N2.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("L ON R execution");
            }


            if (types[1] != UnicodeBidiClass::OtherNeutral)
                return fail("L ON R was resolved by N1");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Opposite conflict.
        //
        // R WS L
        //
        // WS remains unresolved for N2.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::LeftToRight
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::LeftToRight))
            {
                return fail("R WS L execution");
            }


            if (types[1] != UnicodeBidiClass::WhiteSpace)
                return fail("R WS L was resolved by N1");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // sos supplies the left-hand direction.
        //
        // sos=L ON L
        //
        //      ON -> L
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRight))
            {
                return fail("sos L execution");
            }


            if (types[0] != UnicodeBidiClass::LeftToRight)
                return fail("sos L result");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // sos=R ON R
        //
        //      ON -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("sos R execution");
            }


            if (types[0] != UnicodeBidiClass::RightToLeft)
                return fail("sos R result");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // eos supplies the right-hand direction.
        //
        // L ON eos=L
        //
        //      ON -> L
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRight))
            {
                return fail("eos L execution");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight)
                return fail("eos L result");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // R ON eos=R
        //
        //      ON -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("eos R execution");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft)
                return fail("eos R result");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Conflicting sos.
        //
        // sos=L ON R
        //
        // ON remains unresolved.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("conflicting sos execution");
            }


            if (types[0] != UnicodeBidiClass::OtherNeutral)
                return fail("conflicting sos resolved neutral");

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Conflicting eos.
        //
        // L ON eos=R
        //
        // ON remains unresolved.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("conflicting eos execution");
            }


            if (types[1] != UnicodeBidiClass::OtherNeutral)
                return fail("conflicting eos resolved neutral");

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // EN and AN act as R for N1 directional influence.
        //
        // EN ON WS AN
        //
        //      ON WS -> R R
        //
        // EN and AN themselves remain unchanged.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::ArabicNumber
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("EN/AN influence execution");
            }


            if (types[0] != UnicodeBidiClass::EuropeanNumber ||
                types[1] != UnicodeBidiClass::RightToLeft ||
                types[2] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::ArabicNumber)
            {
                return fail("EN/AN influence result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 12
        //
        // Exercise every NI type defined by UAX #9:
        //
        //      B, S, WS, ON, FSI, LRI, RLI, PDI
        //
        // All occur between L and L, so all become L.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,

                UnicodeBidiClass::ParagraphSeparator,
                UnicodeBidiClass::SegmentSeparator,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::PopDirectionalIsolate,

                UnicodeBidiClass::LeftToRight
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRight))
            {
                return fail("complete NI set execution");
            }


            for (uint32_t i = 1; i <= 8; ++i)
            {
                if (types[i] != UnicodeBidiClass::LeftToRight)
                    return fail("complete NI set result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 13
        //
        // Multiple independent NI sequences in one IRS.
        //
        //      L ON WS L R S ON R
        //
        // First sequence -> L.
        // Second sequence -> R.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::LeftToRight,

                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::SegmentSeparator,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("multiple NI sequences execution");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail("first NI sequence result");
            }

            if (types[5] != UnicodeBidiClass::RightToLeft ||
                types[6] != UnicodeBidiClass::RightToLeft)
            {
                return fail("second NI sequence result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 14
        //
        // A neutral sequence may cross constituent level-run boundaries.
        //
        // Post-X9 positions:
        //
        //      0   1       2   3       4   5
        //
        //      L  ON       R   R      WS   L
        //      |----|                 |-----|
        //      run 0                   run 2
        //
        // run 1 is not part of this isolating run sequence.
        //
        // Within the IRS the logical sequence is:
        //
        //      L ON WS L
        //
        // therefore ON and WS both resolve L.
        //
        // Positions 2 and 3 must not participate or be modified.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,

                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft,

                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::LeftToRight
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


            if (!resolveBidiNeutralTypesN1(
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
                return fail("non-contiguous IRS neutral result");
            }


            if (types[2] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("non-contiguous IRS gap modified");
            }

            ++passed;
        }


        // ====================================================================
        // Case 15
        //
        // Multiple isolating run sequences are processed independently.
        //
        // Sequence 0:
        //
        //      L ON L -> L L L
        //
        // Sequence 1:
        //
        //      R WS R -> R R R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,

                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::RightToLeft
            };


            const std::vector<uint32_t> bidiIndices{
                0, 1, 2, 3, 4, 5
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 3, 0 },
                { 3, 6, 1 }
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


            if (!resolveBidiNeutralTypesN1(
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


            if (types[1] != UnicodeBidiClass::LeftToRight)
                return fail("first IRS result");

            if (types[4] != UnicodeBidiClass::RightToLeft)
                return fail("second IRS result");

            ++passed;
        }


        // ====================================================================
        // Case 16
        //
        // A mixed sequence can contain one N1-resolved run and one unresolved
        // run for N2.
        //
        //      L ON L WS R
        //
        // ON lies between L/L and resolves L.
        //
        // WS lies between L/R and remains WS for N2.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::RightToLeft
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("resolved/unresolved split execution");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight)
                return fail("resolved NI sequence result");

            if (types[3] != UnicodeBidiClass::WhiteSpace)
                return fail("unresolved NI sequence was changed");

            ++passed;
        }


        // ====================================================================
        // Case 17
        //
        // Numbers on both sides need not have the same numeric bidi class.
        //
        //      AN ON EN
        //
        // Both act as R for N1:
        //
        //      ON -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicNumber,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::EuropeanNumber
            };


            if (!runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("AN ON EN execution");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft)
                return fail("AN ON EN result");

            if (types[0] != UnicodeBidiClass::ArabicNumber ||
                types[2] != UnicodeBidiClass::EuropeanNumber)
            {
                return fail("AN/EN themselves were modified");
            }

            ++passed;
        }


        // ====================================================================
        // Case 18
        //
        // Post-W7/post-N0 invariant check.
        //
        // AL should no longer reach N1 because W3 changes AL -> R.
        //
        // The N1 implementation deliberately rejects unexpected surviving
        // weak/strong types rather than silently processing invalid state.
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicLetter
            };


            if (runBidiN1SingleSequence(
                types,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("surviving AL was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi N1: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs