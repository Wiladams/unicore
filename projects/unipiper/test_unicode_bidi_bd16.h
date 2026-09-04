// test_unicode_bidi_bd16.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // bidiBD16PairsEqual
    // ========================================================================

    static bool bidiBD16PairsEqual(
        const std::vector<BidiBracketPair>& actual,
        std::initializer_list<BidiBracketPair> expected) noexcept
    {
        if (actual.size() != expected.size())
            return false;

        size_t index = 0;

        for (const BidiBracketPair& pair : expected)
        {
            if (actual[index].openPosition != pair.openPosition ||
                actual[index].closePosition != pair.closePosition)
            {
                return false;
            }

            ++index;
        }

        return true;
    }


    // ========================================================================
    // buildBidiBD16TestPairs
    //
    // Build the simplest possible isolating run sequence:
    //
    //      one level run
    //      identity post-X9 scalar indices
    //
    // and invoke BD16 directly.
    //
    // forceTypePosition may be used to simulate a bracket whose current
    // post-W7 working type is no longer ON.
    // ========================================================================

    static bool buildBidiBD16TestPairs(
        const UnicodeDatabase& database,
        const std::vector<uint32_t>& values,
        std::vector<BidiBracketPair>& pairs,
        uint32_t forceTypePosition = kBidiIndexInvalid,
        UnicodeBidiClass forceType = UnicodeBidiClass::OtherNeutral)
    {
        std::vector<UnicodeScalar> scalars;
        std::vector<UnicodeBidiClass> types;
        std::vector<uint32_t> bidiIndices;

        scalars.resize(values.size());
        types.resize(values.size());
        bidiIndices.resize(values.size());


        for (uint32_t i = 0;
            i < static_cast<uint32_t>(values.size());
            ++i)
        {
            scalars[i].value = values[i];
            types[i] = database.bidiClass(values[i]);
            bidiIndices[i] = i;
        }


        if (forceTypePosition != kBidiIndexInvalid)
        {
            if (forceTypePosition >= types.size())
                return false;

            types[forceTypePosition] = forceType;
        }


        std::vector<BidiLevelRun> runs;

        runs.push_back(BidiLevelRun{
            0,
            static_cast<uint32_t>(values.size()),
            0
            });


        std::vector<uint32_t> sequenceRunIndices{
            0
        };


        BidiIsolatingRunSequence sequence{};

        sequence.runOffset = 0;
        sequence.runCount = 1;
        sequence.level = 0;
        sequence.sos = UnicodeBidiClass::LeftToRight;
        sequence.eos = UnicodeBidiClass::LeftToRight;


        return buildBidiBracketPairs(
            bidiIndices.empty() ? nullptr : bidiIndices.data(),
            static_cast<uint32_t>(bidiIndices.size()),
            scalars.empty() ? nullptr : scalars.data(),
            types.empty() ? nullptr : types.data(),
            static_cast<uint32_t>(scalars.size()),
            database,
            runs,
            sequenceRunIndices,
            sequence,
            pairs);
    }


    // ========================================================================
    // testUnicodeBidiBD16
    // ========================================================================

    static bool testUnicodeBidiBD16(const ByteSpan& databaseData)
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi BD16: FAIL: %s\n",
                    message);

                return false;
            };


        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("unable to load Unicode database");

        if (!database.hasBidiClass())
            return fail("database has no Bidi_Class");

        if (!database.hasBidiBrackets())
            return fail("database has no BidiBrackets");


        uint32_t cases = 0;
        uint32_t passed = 0;

        std::vector<BidiBracketPair> pairs;


        // ====================================================================
        // Case 1
        //
        // Simple pair.
        //
        //      ( A )
        //
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x0041, 0x0029 },
                pairs))
            {
                return fail("simple pair discovery failed");
            }

            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 2 }
                }))
            {
                return fail("simple pair result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Nested pairs.
        //
        //      ( [ ] )
        //
        // Pairs must be returned in opening-position order, not discovery
        // order.
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x005B, 0x005D, 0x0029 },
                pairs))
            {
                return fail("nested pair discovery failed");
            }

            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 3 },
                    { 1, 2 }
                }))
            {
                return fail("nested pair ordering");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Sequential pairs.
        //
        //      ( ) [ ]
        //
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x0029, 0x005B, 0x005D },
                pairs))
            {
                return fail("sequential pair discovery failed");
            }

            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 1 },
                    { 2, 3 }
                }))
            {
                return fail("sequential pair result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // An unmatched closer does not disturb the opener stack.
        //
        //      ( ] )
        //
        // The ']' is ignored. ')' still matches '('.
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x005D, 0x0029 },
                pairs))
            {
                return fail("unmatched closer discovery failed");
            }

            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 2 }
                }))
            {
                return fail("unmatched closer changed stack");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Matching an opener below the top of the stack discards all openers
        // above the match.
        //
        //      ( [ ) ]
        //
        // ')' matches '(' and discards '['. The final ']' is therefore
        // unmatched.
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x005B, 0x0029, 0x005D },
                pairs))
            {
                return fail("lower-stack opener discovery failed");
            }

            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 2 }
                }))
            {
                return fail("lower-stack opener pop behavior");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // BD16 canonical-equivalence special case.
        //
        //      U+2329 ... U+3009
        //      U+3008 ... U+232A
        //
        // U+232A and U+3009 are treated as equivalent closing brackets.
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x2329, 0x3009, 0x3008, 0x232A },
                pairs))
            {
                return fail("canonical-equivalent pair discovery failed");
            }

            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 1 },
                    { 2, 3 }
                }))
            {
                return fail("canonical-equivalent bracket matching");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // An opening bracket whose current working type is not ON does not
        // participate in BD16.
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x0029 },
                pairs,
                0,
                UnicodeBidiClass::LeftToRight))
            {
                return fail("non-ON opener discovery failed");
            }

            if (!pairs.empty())
                return fail("non-ON opener participated in BD16");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // A closing bracket whose current working type is not ON does not
        // participate in BD16.
        // ====================================================================

        {
            ++cases;

            if (!buildBidiBD16TestPairs(
                database,
                { 0x0028, 0x0029 },
                pairs,
                1,
                UnicodeBidiClass::RightToLeft))
            {
                return fail("non-ON closer discovery failed");
            }

            if (!pairs.empty())
                return fail("non-ON closer participated in BD16");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // One isolating run sequence may contain multiple level runs.
        //
        // Post-X9 positions:
        //
        //      run 0:  ( A
        //      run 1:  X        not part of this isolating run sequence
        //      run 2:  B )
        //
        // The pair spans constituent level runs.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint32_t> values{
                0x0028,
                0x0041,
                0x0058,
                0x0042,
                0x0029
            };


            std::vector<UnicodeScalar> scalars(values.size());
            std::vector<UnicodeBidiClass> types(values.size());
            std::vector<uint32_t> bidiIndices(values.size());


            for (uint32_t i = 0;
                i < static_cast<uint32_t>(values.size());
                ++i)
            {
                scalars[i].value = values[i];
                types[i] = database.bidiClass(values[i]);
                bidiIndices[i] = i;
            }


            const std::vector<BidiLevelRun> runs{
                { 0, 2, 0 },
                { 2, 3, 1 },
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


            if (!buildBidiBracketPairs(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                scalars.data(),
                types.data(),
                static_cast<uint32_t>(scalars.size()),
                database,
                runs,
                sequenceRunIndices,
                sequence,
                pairs))
            {
                return fail("multi-run pair discovery failed");
            }


            if (!bidiBD16PairsEqual(
                pairs,
                {
                    { 0, 4 }
                }))
            {
                return fail("pair did not span constituent level runs");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Exactly 63 nested opening brackets are permitted.
        // ====================================================================

        {
            ++cases;

            std::vector<uint32_t> values;

            values.reserve(126);

            for (uint32_t i = 0; i < 63; ++i)
                values.push_back(0x0028);

            for (uint32_t i = 0; i < 63; ++i)
                values.push_back(0x0029);


            if (!buildBidiBD16TestPairs(
                database,
                values,
                pairs))
            {
                return fail("63-deep pair discovery failed");
            }


            if (pairs.size() != 63)
                return fail("63-deep pair count");


            for (uint32_t i = 0; i < 63; ++i)
            {
                if (pairs[i].openPosition != i ||
                    pairs[i].closePosition != 125u - i)
                {
                    return fail("63-deep pair positions");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // A 64th nested opening bracket overflows the BD16 stack.
        //
        // BD16 requires the resulting pair list for this isolating run
        // sequence to be empty.
        // ====================================================================

        {
            ++cases;

            std::vector<uint32_t> values;

            values.reserve(128);

            for (uint32_t i = 0; i < 64; ++i)
                values.push_back(0x0028);

            for (uint32_t i = 0; i < 64; ++i)
                values.push_back(0x0029);


            if (!buildBidiBD16TestPairs(
                database,
                values,
                pairs))
            {
                return fail("64-deep overflow discovery failed");
            }


            if (!pairs.empty())
                return fail("64-deep overflow did not produce empty pair list");

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi BD16: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static bool testUnicodeBidiBD16(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Unicode bidi BD16: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testUnicodeBidiBD16(databaseData);
    }

} // namespace waavs
