// test_unicode_bidi_i2.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"


namespace waavs
{
    // ========================================================================
    // runBidiI2Test
    // ========================================================================

    static bool runBidiI2Test(
        const std::vector<UnicodeBidiClass>& types,
        std::vector<UnicodeBidiLevel>& levels)
    {
        if (types.size() != levels.size())
            return false;


        std::vector<uint32_t> bidiIndices(
            types.size());

        for (uint32_t i = 0;
            i < static_cast<uint32_t>(bidiIndices.size());
            ++i)
        {
            bidiIndices[i] = i;
        }


        return resolveBidiImplicitLevelsI2(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.empty() ? nullptr : types.data(),
            levels.empty() ? nullptr : levels.data(),
            static_cast<uint32_t>(types.size()));
    }


    // ========================================================================
    // runBidiI1I2Test
    //
    // Apply the complete implicit-resolution pair in specification order.
    // ========================================================================

    static bool runBidiI1I2Test(
        const std::vector<UnicodeBidiClass>& types,
        std::vector<UnicodeBidiLevel>& levels)
    {
        if (types.size() != levels.size())
            return false;


        std::vector<uint32_t> bidiIndices(
            types.size());

        for (uint32_t i = 0;
            i < static_cast<uint32_t>(bidiIndices.size());
            ++i)
        {
            bidiIndices[i] = i;
        }


        if (!resolveBidiImplicitLevelsI1(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.empty() ? nullptr : types.data(),
            levels.empty() ? nullptr : levels.data(),
            static_cast<uint32_t>(types.size())))
        {
            return false;
        }


        return resolveBidiImplicitLevelsI2(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.empty() ? nullptr : types.data(),
            levels.empty() ? nullptr : levels.data(),
            static_cast<uint32_t>(types.size()));
    }


    // ========================================================================
    // testUnicodeBidiI2
    // ========================================================================

    static bool testUnicodeBidiI2()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi I2: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;


        // ====================================================================
        // Case 1
        //
        // Odd level L increases by one.
        //
        //      level 1, L -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI2Test(types, levels))
                return fail("odd L execution");

            if (levels[0] != 2)
                return fail("odd L result");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Odd level R remains unchanged.
        //
        //      level 1, R -> 1
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI2Test(types, levels))
                return fail("odd R execution");

            if (levels[0] != 1)
                return fail("odd R result");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Odd level EN increases by one.
        //
        //      level 1, EN -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::EuropeanNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI2Test(types, levels))
                return fail("odd EN execution");

            if (levels[0] != 2)
                return fail("odd EN result");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Odd level AN increases by one.
        //
        //      level 1, AN -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI2Test(types, levels))
                return fail("odd AN execution");

            if (levels[0] != 2)
                return fail("odd AN result");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Even levels are not modified by I2.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                2,
                2,
                2,
                2
            };


            if (!runBidiI2Test(types, levels))
                return fail("even-level preservation execution");


            if (levels[0] != 2 ||
                levels[1] != 2 ||
                levels[2] != 2 ||
                levels[3] != 2)
            {
                return fail("even levels changed by I2");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Higher odd levels follow the same rule.
        //
        //      L  at 5 -> 6
        //      R  at 5 -> 5
        //      EN at 5 -> 6
        //      AN at 5 -> 6
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                5,
                5,
                5,
                5
            };


            if (!runBidiI2Test(types, levels))
                return fail("higher odd levels execution");


            if (levels[0] != 6 ||
                levels[1] != 5 ||
                levels[2] != 6 ||
                levels[3] != 6)
            {
                return fail("higher odd levels result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Maximum explicit odd level.
        //
        //      level 125:
        //
        //      L      -> 126
        //      R      -> 125
        //      EN/AN  -> 126
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                125,
                125,
                125,
                125
            };


            if (!runBidiI2Test(types, levels))
                return fail("maximum odd level execution");


            if (levels[0] != 126 ||
                levels[1] != 125 ||
                levels[2] != 126 ||
                levels[3] != 126)
            {
                return fail("maximum odd level result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // I2 must accept level 126 produced by I1.
        //
        // These levels represent:
        //
        //      R  at 124 -> 125
        //      EN at 124 -> 126
        //      AN at 124 -> 126
        //      L  at 124 -> 124
        //
        // I2 must not apply a second adjustment to any of them.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber,
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                125,
                126,
                126,
                124
            };


            if (!runBidiI2Test(types, levels))
                return fail("I1-produced levels execution");


            if (levels[0] != 125 ||
                levels[1] != 126 ||
                levels[2] != 126 ||
                levels[3] != 124)
            {
                return fail("I1-produced levels changed by I2");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Complete I1/I2 table at level 0.
        //
        //      L      0 -> 0
        //      R      0 -> 1
        //      EN     0 -> 2
        //      AN     0 -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                0,
                0,
                0,
                0
            };


            if (!runBidiI1I2Test(types, levels))
                return fail("combined even-level execution");


            if (levels[0] != 0 ||
                levels[1] != 1 ||
                levels[2] != 2 ||
                levels[3] != 2)
            {
                return fail("combined even-level result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Complete I1/I2 table at level 1.
        //
        //      L      1 -> 2
        //      R      1 -> 1
        //      EN     1 -> 2
        //      AN     1 -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                1,
                1,
                1,
                1
            };


            if (!runBidiI1I2Test(types, levels))
                return fail("combined odd-level execution");


            if (levels[0] != 2 ||
                levels[1] != 1 ||
                levels[2] != 2 ||
                levels[3] != 2)
            {
                return fail("combined odd-level result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // Mixed explicit levels processed by complete I1/I2.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber,

                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                4,
                4,
                4,
                4,

                5,
                5,
                5,
                5
            };


            if (!runBidiI1I2Test(types, levels))
                return fail("combined mixed parity execution");


            if (levels[0] != 4 ||
                levels[1] != 5 ||
                levels[2] != 6 ||
                levels[3] != 6)
            {
                return fail("combined even half result");
            }


            if (levels[4] != 6 ||
                levels[5] != 5 ||
                levels[6] != 6 ||
                levels[7] != 6)
            {
                return fail("combined odd half result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 12
        //
        // Maximum results through the complete I1/I2 sequence.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber,

                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                124,
                124,
                124,

                125,
                125,
                125,
                125
            };


            if (!runBidiI1I2Test(types, levels))
                return fail("combined maximum-level execution");


            if (levels[0] != 125 ||
                levels[1] != 126 ||
                levels[2] != 126)
            {
                return fail("combined maximum even results");
            }


            if (levels[3] != 126 ||
                levels[4] != 125 ||
                levels[5] != 126 ||
                levels[6] != 126)
            {
                return fail("combined maximum odd results");
            }

            ++passed;
        }


        // ====================================================================
        // Case 13
        //
        // Verify the useful final parity invariant:
        //
        //      L       -> even level
        //      R       -> odd level
        //      EN/AN   -> even level
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber,

                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                2, 2, 2, 2,
                3, 3, 3, 3
            };


            if (!runBidiI1I2Test(types, levels))
                return fail("final parity invariant execution");


            for (uint32_t i = 0;
                i < static_cast<uint32_t>(types.size());
                ++i)
            {
                const bool odd =
                    (levels[i] & 1u) != 0;


                if (types[i] == UnicodeBidiClass::RightToLeft)
                {
                    if (!odd)
                        return fail("R ended on even level");
                }
                else
                {
                    if (odd)
                        return fail("L/EN/AN ended on odd level");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 14
        //
        // I2 operates only on the post-X9 index sequence.
        //
        // Scalar 1 represents an X9-removed character.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::BoundaryNeutral,
                UnicodeBidiClass::EuropeanNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                1,
                17,
                1
            };

            const std::vector<uint32_t> bidiIndices{
                0,
                2
            };


            if (!resolveBidiImplicitLevelsI2(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                levels.data(),
                static_cast<uint32_t>(types.size())))
            {
                return fail("post-X9 indexing execution");
            }


            if (levels[0] != 2)
                return fail("post-X9 first scalar result");

            if (levels[1] != 17)
                return fail("X9-removed scalar was modified");

            if (levels[2] != 2)
                return fail("post-X9 final scalar result");

            ++passed;
        }


        // ====================================================================
        // Case 15
        //
        // Level 126 is a valid I2 input because I1 may have produced it.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::EuropeanNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                126
            };


            if (!runBidiI2Test(types, levels))
                return fail("level 126 was rejected");

            if (levels[0] != 126)
                return fail("level 126 was modified");

            ++passed;
        }


        // ====================================================================
        // Case 16
        //
        // Level 127 is outside the valid implicit-level range.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                127
            };


            if (runBidiI2Test(types, levels))
                return fail("invalid level 127 was accepted");

            ++passed;
        }


        // ====================================================================
        // Case 17
        //
        // ON must not survive N0-N2 into I2.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (runBidiI2Test(types, levels))
                return fail("surviving ON was accepted");

            ++passed;
        }


        // ====================================================================
        // Case 18
        //
        // AL must already have become R under W3.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicLetter
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (runBidiI2Test(types, levels))
                return fail("surviving AL was accepted");

            ++passed;
        }


        // ====================================================================
        // Case 19
        //
        // Invalid scalar index is rejected.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };

            const std::vector<uint32_t> bidiIndices{
                1
            };


            if (resolveBidiImplicitLevelsI2(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                levels.data(),
                static_cast<uint32_t>(types.size())))
            {
                return fail("out-of-range scalar index was accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Case 20
        //
        // Empty post-X9 input is valid.
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiImplicitLevelsI2(
                nullptr,
                0,
                nullptr,
                nullptr,
                0))
            {
                return fail("empty input was rejected");
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi I2: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs