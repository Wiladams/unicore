// test_unicode_bidi_n0_trailing_nsm.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"


namespace waavs
{
    // ========================================================================
    // applyBidiN0TrailingNsmTest
    //
    // Build one simple isolating run sequence over an identity post-X9 index
    // mapping and apply the N0 trailing-NSM rule to one bracket position.
    // ========================================================================

    static bool applyBidiN0TrailingNsmTest(
        const std::vector<UnicodeBidiClass>& originalTypes,
        std::vector<UnicodeBidiClass>& types,
        UnicodeBidiLevel level,
        uint32_t bracketPosition)
    {
        if (originalTypes.size() != types.size())
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


        return applyBidiBracketTrailingNsmN0(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            originalTypes.empty() ? nullptr : originalTypes.data(),
            types.empty() ? nullptr : types.data(),
            static_cast<uint32_t>(types.size()),
            runs,
            sequenceRunIndices,
            sequence,
            bracketPosition);
    }


    // ========================================================================
    // testUnicodeBidiN0TrailingNsm
    // ========================================================================

    static bool testUnicodeBidiN0TrailingNsm()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi N0 trailing NSM: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;


        // ====================================================================
        // Case 1
        //
        // One original NSM after an L-resolved bracket.
        //
        //      bracket NSM
        //      L       ?
        //
        //      -> L L
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("single L trailing NSM failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight)
                return fail("single L trailing NSM result");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // One original NSM after an R-resolved bracket.
        //
        //      -> R R
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                1,
                0))
            {
                return fail("single R trailing NSM failed");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft)
                return fail("single R trailing NSM result");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Multiple consecutive original NSMs.
        //
        //      bracket NSM NSM NSM
        //
        // All inherit the bracket type.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("multiple trailing NSMs failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[2] != UnicodeBidiClass::LeftToRight ||
                types[3] != UnicodeBidiClass::LeftToRight)
            {
                return fail("multiple trailing NSM result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Stop at first character that was not originally NSM.
        //
        //      bracket NSM ON NSM
        //
        //      -> L L ON ...
        //
        // The final NSM must not be changed.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("NSM stopping condition failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight)
                return fail("first NSM was not changed");

            if (types[2] != UnicodeBidiClass::OtherNeutral)
                return fail("non-NSM character was changed");

            if (types[3] != UnicodeBidiClass::OtherNeutral)
                return fail("NSM after stopping point was changed");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Detection must use originalTypes, not the current working type.
        //
        // W1 may already have changed:
        //
        //      original: NSM
        //      working:  R
        //
        // N0 still recognizes it as trailing NSM and replaces it with the
        // bracket's resolved L type.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("original-vs-working NSM detection failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight)
            {
                return fail(
                    "original NSM was not replaced despite working type");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // A character whose working type is NSM but whose original type was
        // not NSM must stop the scan.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::OtherNeutral
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("working-only NSM stopping test failed");
            }


            if (types[1] != UnicodeBidiClass::NonspacingMark)
                return fail("working-only NSM was changed");

            if (types[2] != UnicodeBidiClass::OtherNeutral)
                return fail("scan continued past working-only NSM");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Trailing NSMs may continue across constituent level-run boundaries
        // of the same isolating run sequence.
        //
        // Post-X9 positions:
        //
        //      run 0             run 2
        //      +---------+       +---------+
        //      | bracket |       | NSM NSM |
        //      +---------+       +---------+
        //
        // run 1 lies in the numeric gap but is not part of this IRS.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::NonspacingMark
            };


            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral
            };


            const std::vector<uint32_t> bidiIndices{
                0, 1, 2, 3, 4
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 1, 0 },
                { 1, 3, 1 },
                { 3, 5, 0 }
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


            if (!applyBidiBracketTrailingNsmN0(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                original.data(),
                types.data(),
                static_cast<uint32_t>(types.size()),
                runs,
                sequenceRunIndices,
                sequence,
                0))
            {
                return fail("cross-run trailing NSM propagation failed");
            }


            if (types[3] != UnicodeBidiClass::LeftToRight ||
                types[4] != UnicodeBidiClass::LeftToRight)
            {
                return fail(
                    "trailing NSMs did not cross constituent runs");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[2] != UnicodeBidiClass::RightToLeft)
            {
                return fail(
                    "non-sequence gap was modified");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // The same rule applies after a closing bracket.
        //
        //      ... close NSM NSM
        //
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                1,
                1))
            {
                return fail("closing-bracket trailing NSMs failed");
            }


            if (types[2] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("closing-bracket trailing NSM result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // No following character.
        //
        // Valid operation, nothing to change.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };


            if (!applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("bracket at end of sequence failed");
            }


            if (types[0] != UnicodeBidiClass::LeftToRight)
                return fail("bracket at end was modified");

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // The bracket must already have been resolved to L or R.
        //
        // An unresolved ON bracket is invalid input for this helper.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> original{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::NonspacingMark
            };

            std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral
            };


            if (applyBidiN0TrailingNsmTest(
                original,
                types,
                0,
                0))
            {
                return fail("unresolved bracket was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi N0 trailing NSM: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs