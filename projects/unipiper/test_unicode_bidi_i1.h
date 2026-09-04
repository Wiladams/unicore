// test_unicode_bidi_i1.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"


namespace waavs
{
    // ========================================================================
    // runBidiI1Test
    // ========================================================================

    static bool runBidiI1Test(
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


        return resolveBidiImplicitLevelsI1(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            types.empty() ? nullptr : types.data(),
            levels.empty() ? nullptr : levels.data(),
            static_cast<uint32_t>(types.size()));
    }


    // ========================================================================
    // testUnicodeBidiI1
    // ========================================================================

    static bool testUnicodeBidiI1()
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi I1: FAIL: %s\n",
                    message);

                return false;
            };


        uint32_t cases = 0;
        uint32_t passed = 0;


        // ====================================================================
        // Case 1
        //
        // Even level L remains unchanged.
        //
        //      level 0, L -> 0
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };


            if (!runBidiI1Test(types, levels))
                return fail("even L execution");

            if (levels[0] != 0)
                return fail("even L result");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Even level R increases by one.
        //
        //      level 0, R -> 1
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };


            if (!runBidiI1Test(types, levels))
                return fail("even R execution");

            if (levels[0] != 1)
                return fail("even R result");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Even level EN increases by two.
        //
        //      level 0, EN -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::EuropeanNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };


            if (!runBidiI1Test(types, levels))
                return fail("even EN execution");

            if (levels[0] != 2)
                return fail("even EN result");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Even level AN increases by two.
        //
        //      level 0, AN -> 2
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };


            if (!runBidiI1Test(types, levels))
                return fail("even AN execution");

            if (levels[0] != 2)
                return fail("even AN result");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // I1 must not modify odd-level L.
        //
        // I2 will process it later.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI1Test(types, levels))
                return fail("odd L execution");

            if (levels[0] != 1)
                return fail("odd L changed by I1");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Odd-level R remains unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI1Test(types, levels))
                return fail("odd R execution");

            if (levels[0] != 1)
                return fail("odd R changed by I1");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Odd-level EN remains unchanged by I1.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::EuropeanNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI1Test(types, levels))
                return fail("odd EN execution");

            if (levels[0] != 1)
                return fail("odd EN changed by I1");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Odd-level AN remains unchanged by I1.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                1
            };


            if (!runBidiI1Test(types, levels))
                return fail("odd AN execution");

            if (levels[0] != 1)
                return fail("odd AN changed by I1");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Higher even levels follow the same rules.
        //
        //      L  at 4 -> 4
        //      R  at 4 -> 5
        //      EN at 4 -> 6
        //      AN at 4 -> 6
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
                4,
                4,
                4,
                4
            };


            if (!runBidiI1Test(types, levels))
                return fail("higher even levels execution");


            if (levels[0] != 4 ||
                levels[1] != 5 ||
                levels[2] != 6 ||
                levels[3] != 6)
            {
                return fail("higher even levels result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Mixed even and odd input.
        //
        // Only even-level entries are processed by I1.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber,
                UnicodeBidiClass::LeftToRight,

                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                2,
                2,
                2,
                2,

                3,
                3,
                3,
                3
            };


            if (!runBidiI1Test(types, levels))
                return fail("mixed parity execution");


            if (levels[0] != 3 ||
                levels[1] != 4 ||
                levels[2] != 4 ||
                levels[3] != 2)
            {
                return fail("mixed parity even results");
            }


            if (levels[4] != 3 ||
                levels[5] != 3 ||
                levels[6] != 3 ||
                levels[7] != 3)
            {
                return fail("mixed parity odd levels changed");
            }

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // I1 operates only on the post-X9 index sequence.
        //
        // Scalar 1 represents an X9-removed character. It is deliberately not
        // present in bidiIndices and must not be inspected or modified.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::BoundaryNeutral,
                UnicodeBidiClass::EuropeanNumber
            };

            std::vector<UnicodeBidiLevel> levels{
                0,
                17,
                0
            };

            const std::vector<uint32_t> bidiIndices{
                0,
                2
            };


            if (!resolveBidiImplicitLevelsI1(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                types.data(),
                levels.data(),
                static_cast<uint32_t>(types.size())))
            {
                return fail("post-X9 indexing execution");
            }


            if (levels[0] != 1)
                return fail("post-X9 first scalar result");

            if (levels[1] != 17)
                return fail("X9-removed scalar was modified");

            if (levels[2] != 2)
                return fail("post-X9 final scalar result");

            ++passed;
        }


        // ====================================================================
        // Case 12
        //
        // Maximum legal I1 result.
        //
        // The largest even explicit level is 124:
        //
        //      R      -> 125
        //      EN/AN  -> 126
        //
        // Level 126 is legal as an implicit result.
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
                124,
                124,
                124,
                124
            };


            if (!runBidiI1Test(types, levels))
                return fail("maximum implicit level execution");


            if (levels[0] != 124 ||
                levels[1] != 125 ||
                levels[2] != 126 ||
                levels[3] != 126)
            {
                return fail("maximum implicit level result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 13
        //
        // Level 126 is not a valid input embedding level.
        //
        // It may be produced by I1, but X1-X8 cannot supply it to I1.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                126
            };


            if (runBidiI1Test(types, levels))
                return fail("invalid input level 126 was accepted");

            ++passed;
        }


        // ====================================================================
        // Case 14
        //
        // After W1-W7 and N0-N2, ON must no longer survive into I1.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::OtherNeutral
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };


            if (runBidiI1Test(types, levels))
                return fail("surviving ON was accepted");

            ++passed;
        }


        // ====================================================================
        // Case 15
        //
        // Likewise, AL must already have been converted to R by W3.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::ArabicLetter
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };


            if (runBidiI1Test(types, levels))
                return fail("surviving AL was accepted");

            ++passed;
        }


        // ====================================================================
        // Case 16
        //
        // Invalid post-X9 scalar index must be rejected.
        // ====================================================================

        {
            ++cases;

            const std::vector<UnicodeBidiClass> types{
                UnicodeBidiClass::LeftToRight
            };

            std::vector<UnicodeBidiLevel> levels{
                0
            };

            const std::vector<uint32_t> bidiIndices{
                1
            };


            if (resolveBidiImplicitLevelsI1(
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
        // Case 17
        //
        // Empty post-X9 input is valid.
        // ====================================================================

        {
            ++cases;

            if (!resolveBidiImplicitLevelsI1(
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
            "Unicode bidi I1: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }

} // namespace waavs