// test_unicode_bidi_n0_resolution.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"


namespace waavs
{
    // ========================================================================
    // resolveBidiN0TestPair
    //
    // Construct one simple isolating run sequence over an identity post-X9
    // index mapping and resolve one bracket pair.
    // ========================================================================

    static bool resolveBidiN0TestPair(
        std::initializer_list<UnicodeBidiClass> inputTypes,
        UnicodeBidiLevel level,
        uint32_t openPosition,
        uint32_t closePosition,
        std::vector<UnicodeBidiClass>& result)
    {
        result.assign(
            inputTypes.begin(),
            inputTypes.end());

        std::vector<uint32_t> bidiIndices(
            result.size());

        for (uint32_t i = 0;
            i < static_cast<uint32_t>(bidiIndices.size());
            ++i)
        {
            bidiIndices[i] = i;
        }


        std::vector<BidiLevelRun> runs{
            {
                0,
                static_cast<uint32_t>(result.size()),
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


        const BidiBracketPair pair{
            openPosition,
            closePosition
        };


        return resolveBidiBracketPairN0(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            result.empty() ? nullptr : result.data(),
            static_cast<uint32_t>(result.size()),
            runs,
            sequenceRunIndices,
            sequence,
            pair);
    }


    // ========================================================================
    // testUnicodeBidiN0Resolution
    // ========================================================================

    static bool testUnicodeBidiN0Resolution()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi N0 resolution: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;

        std::vector<UnicodeBidiClass> types;


        // ====================================================================
        // Case 1
        //
        // LTR embedding, enclosed L.
        //
        //      ( L )
        //
        // Matching embedding direction wins immediately.
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                0,
                2,
                types))
            {
                return fail("LTR enclosed-L resolution failed");
            }


            if (types[0] != UnicodeBidiClass::LeftToRight ||
                types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail("LTR enclosed-L bracket result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // LTR embedding, enclosed R, preceding R.
        //
        //      R ( R )
        //
        // Enclosed direction is opposite embedding direction, and preceding
        // strong direction is also R.
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                1,
                3,
                types))
            {
                return fail("LTR opposite with preceding R failed");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("LTR opposite with preceding R result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // LTR embedding, enclosed R, preceding L.
        //
        //      L ( R )
        //
        // Preceding direction does not match enclosed opposite direction.
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                1,
                3,
                types))
            {
                return fail("LTR opposite with preceding L failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[3] != UnicodeBidiClass::LeftToRight)
            {
                return fail("LTR opposite with preceding L result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // LTR embedding, no strong type inside.
        //
        //      ( ON )
        //
        // N0 does nothing.
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                0,
                2,
                types))
            {
                return fail("LTR neutral-only resolution failed");
            }


            if (types[0] != UnicodeBidiClass::OtherNeutral ||
                types[2] != UnicodeBidiClass::OtherNeutral)
            {
                return fail("neutral-only pair was resolved");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // RTL embedding, enclosed R.
        //
        //      ( R )
        //
        // R matches embedding direction.
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                1,
                0,
                2,
                types))
            {
                return fail("RTL enclosed-R resolution failed");
            }


            if (types[0] != UnicodeBidiClass::RightToLeft ||
                types[2] != UnicodeBidiClass::RightToLeft)
            {
                return fail("RTL enclosed-R bracket result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // RTL embedding, enclosed L, preceding L.
        //
        //      L ( L )
        //
        // Enclosed direction is opposite embedding direction and preceding
        // direction matches it.
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral
                },
                1,
                1,
                3,
                types))
            {
                return fail("RTL opposite with preceding L failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[3] != UnicodeBidiClass::LeftToRight)
            {
                return fail("RTL opposite with preceding L result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // RTL embedding, enclosed L, preceding R.
        //
        //      R ( L )
        //
        // Preceding direction does not match the enclosed opposite direction.
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral
                },
                1,
                1,
                3,
                types))
            {
                return fail("RTL opposite with preceding R failed");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("RTL opposite with preceding R result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // EN and AN count as R in enclosed text.
        //
        // LTR embedding:
        //
        //      R ( EN AN )
        //
        // Only opposite strong direction occurs inside, and preceding strong
        // direction is R.
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::EuropeanNumber,
                    UnicodeBidiClass::ArabicNumber,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                1,
                4,
                types))
            {
                return fail("EN/AN enclosed direction failed");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft ||
                types[4] != UnicodeBidiClass::RightToLeft)
            {
                return fail("EN/AN were not treated as R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // EN also acts as R while finding preceding strong context.
        //
        // LTR embedding:
        //
        //      EN ( R )
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::EuropeanNumber,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                1,
                3,
                types))
            {
                return fail("preceding EN context failed");
            }


            if (types[1] != UnicodeBidiClass::RightToLeft ||
                types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("preceding EN was not treated as R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Matching embedding direction has priority when both directions occur.
        //
        // LTR embedding:
        //
        //      R ( R L )
        //
        // Even though R occurs inside and precedes the pair, the enclosed L
        // matches the embedding direction.
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                1,
                4,
                types))
            {
                return fail("mixed enclosed direction resolution failed");
            }


            if (types[1] != UnicodeBidiClass::LeftToRight ||
                types[4] != UnicodeBidiClass::LeftToRight)
            {
                return fail(
                    "embedding direction did not win mixed enclosed case");
            }

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // sos supplies preceding context when the opening bracket is the first
        // character in the isolating run sequence.
        //
        // LTR embedding:
        //
        //      ( R )
        //
        // sos is L, so:
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                0,
                2,
                types))
            {
                return fail("LTR sos context failed");
            }


            if (types[0] != UnicodeBidiClass::LeftToRight ||
                types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail("LTR sos context result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 12
        //
        // Same sos rule for RTL embedding.
        //
        //      ( L )
        //
        // sos is R, so:
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral
                },
                1,
                0,
                2,
                types))
            {
                return fail("RTL sos context failed");
            }


            if (types[0] != UnicodeBidiClass::RightToLeft ||
                types[2] != UnicodeBidiClass::RightToLeft)
            {
                return fail("RTL sos context result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 13
        //
        // Preceding context must follow the isolating run sequence rather than
        // the numerically contiguous post-X9 range.
        //
        // Post-X9 positions:
        //
        //      0    1    2    3    4    5
        //
        //      R   ON    L    R   ON    R
        //      |------|         |---------|
        //        run 0             run 2
        //
        // run 1 contains L but is not part of this isolating run sequence.
        //
        // The opening bracket is at position 4. Its preceding strong type in
        // the isolating run sequence is therefore R at position 0, not L at
        // position 2.
        //
        // LTR embedding + enclosed R:
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            std::vector<UnicodeBidiClass> localTypes{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral
            };


            const std::vector<uint32_t> bidiIndices{
                0, 1, 2, 3, 4, 5, 6
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 2, 0 },
                { 2, 4, 1 },
                { 4, 7, 0 }
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


            const BidiBracketPair pair{
                4,
                6
            };


            if (!resolveBidiBracketPairN0(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                localTypes.data(),
                static_cast<uint32_t>(localTypes.size()),
                runs,
                sequenceRunIndices,
                sequence,
                pair))
            {
                return fail(
                    "non-contiguous preceding-context resolution failed");
            }


            if (localTypes[4] != UnicodeBidiClass::RightToLeft ||
                localTypes[6] != UnicodeBidiClass::RightToLeft)
            {
                return fail(
                    "non-sequence gap affected preceding strong context");
            }

            ++passed;
        }


        // ====================================================================
        // Case 14
        //
        // Invalid pair geometry must fail.
        // ====================================================================

        {
            ++cases;

            if (resolveBidiN0TestPair(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                1,
                1,
                types))
            {
                return fail("invalid pair geometry was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi N0 resolution: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs