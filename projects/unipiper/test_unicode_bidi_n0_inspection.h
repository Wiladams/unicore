// test_unicode_bidi_n0_inspection.h

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
    // inspectBidiN0TestTypes
    //
    // Construct one simple isolating run sequence over an identity post-X9
    // index mapping and inspect one bracket pair.
    // ========================================================================

    static bool inspectBidiN0TestTypes(
        std::initializer_list<UnicodeBidiClass> inputTypes,
        uint32_t openPosition, uint32_t closePosition,
        BidiBracketStrongTypes& strongTypes)
    {
        std::vector<UnicodeBidiClass> types(
            inputTypes.begin(),
            inputTypes.end());

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
                0
            }
        };

        std::vector<uint32_t> sequenceRunIndices{
            0
        };


        BidiIsolatingRunSequence sequence{};

        sequence.runOffset = 0;
        sequence.runCount = 1;
        sequence.level = 0;
        sequence.sos = UnicodeBidiClass::LeftToRight;
        sequence.eos = UnicodeBidiClass::LeftToRight;


        const BidiBracketPair pair{
            openPosition,
            closePosition
        };


        return inspectBidiBracketPairContents(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.empty() ? nullptr : types.data(),
            static_cast<uint32_t>(types.size()),
            runs,
            sequenceRunIndices,
            sequence,
            pair,
            strongTypes);
    }


    // ========================================================================
    // testUnicodeBidiN0Inspection
    // ========================================================================

    static bool testUnicodeBidiN0Inspection()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi N0 inspection: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;

        BidiBracketStrongTypes strong{};


        // ====================================================================
        // Case 1
        //
        // Neutral contents only.
        //
        //      ( ON )
        //
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                2,
                strong))
            {
                return fail("neutral-only inspection failed");
            }

            if (!strong.empty())
                return fail("neutral-only contents produced strong direction");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // L inside pair.
        //
        //      ( L )
        //
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                2,
                strong))
            {
                return fail("L inspection failed");
            }

            if (!strong.leftToRight ||
                strong.rightToLeft)
            {
                return fail("L inspection result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // R inside pair.
        //
        //      ( R )
        //
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                2,
                strong))
            {
                return fail("R inspection failed");
            }

            if (strong.leftToRight ||
                !strong.rightToLeft)
            {
                return fail("R inspection result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // EN contributes as R for N0.
        //
        //      ( EN )
        //
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::EuropeanNumber,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                2,
                strong))
            {
                return fail("EN inspection failed");
            }

            if (strong.leftToRight ||
                !strong.rightToLeft)
            {
                return fail("EN was not treated as R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // AN contributes as R for N0.
        //
        //      ( AN )
        //
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::ArabicNumber,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                2,
                strong))
            {
                return fail("AN inspection failed");
            }

            if (strong.leftToRight ||
                !strong.rightToLeft)
            {
                return fail("AN was not treated as R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Mixed strong contents.
        //
        //      ( L R )
        //
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                0,
                3,
                strong))
            {
                return fail("mixed inspection failed");
            }

            if (!strong.mixed())
                return fail("mixed strong directions not detected");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Only characters strictly inside the pair participate.
        //
        //      L ( ON ) R
        //
        // The L and R outside the pair must not contribute.
        // ====================================================================

        {
            ++cases;

            if (!inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::RightToLeft
                },
                1,
                3,
                strong))
            {
                return fail("pair-boundary inspection failed");
            }

            if (!strong.empty())
                return fail("strong type outside pair was included");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // An isolating run sequence may contain non-contiguous level runs.
        //
        // Post-X9 positions:
        //
        //      0   1   2   3   4   5
        //
        //      (  ON   R  ON   L   )
        //      |-----|        |-----|
        //       run 0          run 2
        //
        // run 1, containing R, is not part of this isolating run sequence.
        //
        // The result must therefore contain L only.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral
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


            const BidiBracketPair pair{
                0,
                5
            };


            if (!inspectBidiBracketPairContents(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                static_cast<uint32_t>(types.size()),
                runs,
                sequenceRunIndices,
                sequence,
                pair,
                strong))
            {
                return fail("non-contiguous sequence inspection failed");
            }


            if (!strong.leftToRight ||
                strong.rightToLeft)
            {
                return fail(
                    "strong type from non-sequence gap was included");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Invalid pair geometry.
        //
        //      close <= open
        //
        // ====================================================================

        {
            ++cases;

            if (inspectBidiN0TestTypes(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::OtherNeutral
                },
                1,
                1,
                strong))
            {
                return fail("invalid pair geometry was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // The close position must actually belong to the isolating run
        // sequence.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::OtherNeutral
            };


            const std::vector<uint32_t> bidiIndices{
                0, 1, 2, 3
            };


            const std::vector<BidiLevelRun> runs{
                { 0, 2, 0 },
                { 2, 4, 1 }
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


            const BidiBracketPair pair{
                0,
                3
            };


            if (inspectBidiBracketPairContents(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                static_cast<uint32_t>(types.size()),
                runs,
                sequenceRunIndices,
                sequence,
                pair,
                strong))
            {
                return fail(
                    "close outside isolating run sequence was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi N0 inspection: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs