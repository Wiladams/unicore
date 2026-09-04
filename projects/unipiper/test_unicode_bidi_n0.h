// test_unicode_bidi_n0.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // BidiN0TestState
    //
    // Synthetic post-W7 paragraph state used to exercise the complete N0
    // driver without involving P/X/W processing.
    // ========================================================================

    struct BidiN0TestState
    {
        std::vector<UnicodeScalar> scalars{};
        std::vector<UnicodeBidiClass> originalTypes{};
        std::vector<UnicodeBidiClass> types{};

        std::vector<uint32_t> bidiIndices{};

        std::vector<BidiLevelRun> runs{};
        std::vector<uint32_t> sequenceRunIndices{};
        std::vector<BidiIsolatingRunSequence> sequences{};
    };


    // ========================================================================
    // initializeBidiN0Scalars
    // ========================================================================

    static void initializeBidiN0Scalars(
        const UnicodeDatabase& database,
        const std::vector<uint32_t>& values,
        BidiN0TestState& state)
    {
        state.scalars.clear();
        state.originalTypes.clear();
        state.types.clear();
        state.bidiIndices.clear();
        state.runs.clear();
        state.sequenceRunIndices.clear();
        state.sequences.clear();


        state.scalars.resize(values.size());
        state.originalTypes.resize(values.size());
        state.types.resize(values.size());
        state.bidiIndices.resize(values.size());


        for (uint32_t i = 0;
            i < static_cast<uint32_t>(values.size());
            ++i)
        {
            state.scalars[i].value = values[i];

            state.originalTypes[i] =
                database.bidiClass(values[i]);

            state.types[i] =
                state.originalTypes[i];

            state.bidiIndices[i] = i;
        }
    }


    // ========================================================================
    // configureBidiN0SingleSequence
    // ========================================================================

    static bool configureBidiN0SingleSequence(
        BidiN0TestState& state,
        UnicodeBidiLevel level)
    {
        if (state.scalars.empty())
            return false;


        state.runs.clear();
        state.sequenceRunIndices.clear();
        state.sequences.clear();


        state.runs.push_back(BidiLevelRun{
            0,
            static_cast<uint32_t>(state.scalars.size()),
            level
            });


        state.sequenceRunIndices.push_back(0);


        BidiIsolatingRunSequence sequence{};

        sequence.runOffset = 0;
        sequence.runCount = 1;
        sequence.level = level;
        sequence.sos = bidiTypeFromLevel(level);
        sequence.eos = bidiTypeFromLevel(level);

        state.sequences.push_back(sequence);


        return true;
    }


    // ========================================================================
    // runBidiN0TestState
    // ========================================================================

    static bool runBidiN0TestState(
        const UnicodeDatabase& database,
        BidiN0TestState& state)
    {
        return resolveBidiNeutralTypesN0(
            state.bidiIndices.empty()
            ? nullptr
            : state.bidiIndices.data(),
            static_cast<uint32_t>(state.bidiIndices.size()),
            state.scalars.empty()
            ? nullptr
            : state.scalars.data(),
            state.originalTypes.empty()
            ? nullptr
            : state.originalTypes.data(),
            state.types.empty()
            ? nullptr
            : state.types.data(),
            static_cast<uint32_t>(state.scalars.size()),
            database,
            state.runs,
            state.sequenceRunIndices,
            state.sequences);
    }


    // ========================================================================
    // testUnicodeBidiN0
    // ========================================================================

    static bool testUnicodeBidiN0(const ByteSpan& databaseData)
    {
        auto fail =
            [](const char* message) noexcept -> bool
            {
                std::printf(
                    "Unicode bidi N0: FAIL: %s\n",
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

        BidiN0TestState state;


        // ====================================================================
        // Case 1
        //
        // LTR embedding, enclosed L.
        //
        //      ( A )
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0041, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 1 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 1 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight ||
                state.types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail("LTR enclosed L");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // RTL embedding, enclosed R.
        //
        //      ( HEBREW )
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x05D0, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 1))
                return fail("case 2 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 2 execution");


            if (state.types[0] != UnicodeBidiClass::RightToLeft ||
                state.types[2] != UnicodeBidiClass::RightToLeft)
            {
                return fail("RTL enclosed R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // LTR embedding, enclosed R, established preceding R context.
        //
        //      R ( R )
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x05D0, 0x0028, 0x05D1, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 3 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 3 execution");


            if (state.types[1] != UnicodeBidiClass::RightToLeft ||
                state.types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("LTR opposite with preceding R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // LTR embedding, enclosed R, no preceding strong character.
        //
        // sos is L.
        //
        //      ( R )
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x05D0, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 4 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 4 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight ||
                state.types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail("LTR opposite with sos L");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // RTL embedding, enclosed L, established preceding L context.
        //
        //      L ( L )
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0041, 0x0028, 0x0042, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 1))
                return fail("case 5 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 5 execution");


            if (state.types[1] != UnicodeBidiClass::LeftToRight ||
                state.types[3] != UnicodeBidiClass::LeftToRight)
            {
                return fail("RTL opposite with preceding L");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // RTL embedding, enclosed L, sos is R.
        //
        //      ( L )
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0041, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 1))
                return fail("case 6 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 6 execution");


            if (state.types[0] != UnicodeBidiClass::RightToLeft ||
                state.types[2] != UnicodeBidiClass::RightToLeft)
            {
                return fail("RTL opposite with sos R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Mixed enclosed directions.
        //
        // RTL embedding:
        //
        //      ( L R )
        //
        // Matching embedding direction has priority.
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0041, 0x05D0, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 1))
                return fail("case 7 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 7 execution");


            if (state.types[0] != UnicodeBidiClass::RightToLeft ||
                state.types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("mixed enclosed directions");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // No enclosed strong type.
        //
        //      ( & )
        //
        // N0 leaves the pair ON for later N1/N2 processing.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0026, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 8 setup");


            // Ensure this case is specifically neutral regardless of any
            // accidental test-data assumption.

            state.types[1] =
                UnicodeBidiClass::OtherNeutral;


            if (!runBidiN0TestState(database, state))
                return fail("case 8 execution");


            if (state.types[0] != UnicodeBidiClass::OtherNeutral ||
                state.types[2] != UnicodeBidiClass::OtherNeutral)
            {
                return fail("neutral-only pair changed under N0");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // EN and AN act as strong R within N0.
        //
        // RTL embedding:
        //
        //      ( EN AN )
        //
        //      brackets -> R
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0031, 0x0661, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 1))
                return fail("case 9 setup");


            if (state.types[1] != UnicodeBidiClass::EuropeanNumber)
                return fail("expected EN test character");

            if (state.types[2] != UnicodeBidiClass::ArabicNumber)
                return fail("expected AN test character");


            if (!runBidiN0TestState(database, state))
                return fail("case 9 execution");


            if (state.types[0] != UnicodeBidiClass::RightToLeft ||
                state.types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("EN/AN did not act as R");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10
        //
        // Multiple independent bracket pairs in one sequence.
        //
        //      R ( R ) L [ L ]
        //
        //      first pair  -> R
        //      second pair -> L
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                {
                    0x05D0,
                    0x0028,
                    0x05D1,
                    0x0029,
                    0x0041,
                    0x005B,
                    0x0042,
                    0x005D
                },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 10 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 10 execution");


            if (state.types[1] != UnicodeBidiClass::RightToLeft ||
                state.types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("first independent pair");
            }

            if (state.types[5] != UnicodeBidiClass::LeftToRight ||
                state.types[7] != UnicodeBidiClass::LeftToRight)
            {
                return fail("second independent pair");
            }

            ++passed;
        }


        // ====================================================================
        // Case 11
        //
        // Unicode N0 sequential nested-pair pattern.
        //
        // RTL embedding:
        //
        //      R ( R [ ON L ] ON ) L
        //
        // Outer pair contains embedding-direction R, so it resolves R.
        //
        // Inner pair contains only opposite L, but has established preceding
        // R context, so it also resolves R.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                {
                    0x05D0,
                    0x0028,
                    0x05D1,
                    0x005B,
                    0x0026,
                    0x0041,
                    0x005D,
                    0x0021,
                    0x0029,
                    0x0042
                },
                state);

            if (!configureBidiN0SingleSequence(state, 1))
                return fail("case 11 setup");

            state.types[4] = UnicodeBidiClass::OtherNeutral;
            state.types[7] = UnicodeBidiClass::OtherNeutral;


            if (!runBidiN0TestState(database, state))
                return fail("case 11 execution");


            if (state.types[1] != UnicodeBidiClass::RightToLeft ||
                state.types[3] != UnicodeBidiClass::RightToLeft ||
                state.types[6] != UnicodeBidiClass::RightToLeft ||
                state.types[8] != UnicodeBidiClass::RightToLeft)
            {
                return fail("Unicode sequential nested-pair pattern");
            }

            ++passed;
        }


        // ====================================================================
        // Case 12
        //
        // Earlier pair resolution must affect later pair context.
        //
        // LTR embedding:
        //
        //      R ( [ R ] L )
        //
        // Pair order by opening position:
        //
        //      outer pair first
        //      inner pair second
        //
        // Outer contains L, so outer -> L.
        //
        // When the inner pair is subsequently processed, the newly resolved
        // outer opener is now its nearest preceding strong type. Therefore:
        //
        //      inner -> L
        //
        // If pairs were resolved independently or in closing order, the inner
        // pair would incorrectly see the initial R as its preceding context.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                {
                    0x05D0,
                    0x0028,
                    0x005B,
                    0x05D1,
                    0x005D,
                    0x0041,
                    0x0029
                },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 12 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 12 execution");


            if (state.types[1] != UnicodeBidiClass::LeftToRight ||
                state.types[2] != UnicodeBidiClass::LeftToRight ||
                state.types[4] != UnicodeBidiClass::LeftToRight ||
                state.types[6] != UnicodeBidiClass::LeftToRight)
            {
                return fail(
                    "earlier pair did not influence later pair context");
            }

            ++passed;
        }


        // ====================================================================
        // Case 13
        //
        // Original NSM immediately following an opening bracket.
        //
        // Simulate W1 having already changed the NSM working type to R.
        //
        //      ( NSM L )
        //
        // Pair resolves L, and the original NSM must then become L.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0300, 0x0041, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 13 setup");

            if (state.originalTypes[1] !=
                UnicodeBidiClass::NonspacingMark)
            {
                return fail("expected original NSM");
            }


            state.types[1] =
                UnicodeBidiClass::RightToLeft;


            if (!runBidiN0TestState(database, state))
                return fail("case 13 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight ||
                state.types[1] != UnicodeBidiClass::LeftToRight ||
                state.types[3] != UnicodeBidiClass::LeftToRight)
            {
                return fail("NSM after opening bracket");
            }

            ++passed;
        }


        // ====================================================================
        // Case 14
        //
        // Multiple original NSMs following a closing bracket.
        //
        //      ( L ) NSM NSM ON NSM
        //
        // Only the consecutive original NSMs immediately after the closing
        // bracket change.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                {
                    0x0028,
                    0x0041,
                    0x0029,
                    0x0300,
                    0x0300,
                    0x0026,
                    0x0300
                },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 14 setup");


            if (state.originalTypes[3] != UnicodeBidiClass::NonspacingMark ||
                state.originalTypes[4] != UnicodeBidiClass::NonspacingMark ||
                state.originalTypes[6] != UnicodeBidiClass::NonspacingMark)
            {
                return fail("expected trailing NSMs");
            }


            state.types[3] = UnicodeBidiClass::RightToLeft;
            state.types[4] = UnicodeBidiClass::RightToLeft;
            state.types[5] = UnicodeBidiClass::OtherNeutral;
            state.types[6] = UnicodeBidiClass::RightToLeft;


            if (!runBidiN0TestState(database, state))
                return fail("case 14 execution");


            if (state.types[3] != UnicodeBidiClass::LeftToRight ||
                state.types[4] != UnicodeBidiClass::LeftToRight)
            {
                return fail("consecutive trailing NSMs");
            }

            if (state.types[5] != UnicodeBidiClass::OtherNeutral)
                return fail("NSM scan changed stopping character");

            if (state.types[6] != UnicodeBidiClass::RightToLeft)
                return fail("NSM scan continued past stopping character");

            ++passed;
        }


        // ====================================================================
        // Case 15
        //
        // NSM propagation happens only when the bracket pair actually changed
        // under N0.
        //
        //      ( ON ) NSM
        //
        // No strong type is enclosed, so the pair remains ON and the trailing
        // NSM must retain its existing working type.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0026, 0x0029, 0x0300 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 15 setup");

            state.types[1] = UnicodeBidiClass::OtherNeutral;
            state.types[3] = UnicodeBidiClass::RightToLeft;


            if (!runBidiN0TestState(database, state))
                return fail("case 15 execution");


            if (state.types[0] != UnicodeBidiClass::OtherNeutral ||
                state.types[2] != UnicodeBidiClass::OtherNeutral)
            {
                return fail("neutral pair unexpectedly resolved");
            }

            if (state.types[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail(
                    "NSM changed after unresolved bracket pair");
            }

            ++passed;
        }


        // ====================================================================
        // Case 16
        //
        // Non-contiguous isolating run sequence.
        //
        // Post-X9 positions:
        //
        //      0    1       2    3       4    5
        //
        //      (   ON       R   ON       L    )
        //      |------|                 |------|
        //        run 0                    run 2
        //
        // run 1 is not part of this isolating run sequence.
        //
        // The R in the gap must be ignored. The enclosed IRS content contains
        // L, therefore:
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                {
                    0x0028,
                    0x0026,
                    0x05D0,
                    0x0026,
                    0x0041,
                    0x0029
                },
                state);


            state.types[1] = UnicodeBidiClass::OtherNeutral;
            state.types[3] = UnicodeBidiClass::OtherNeutral;


            state.runs = {
                { 0, 2, 0 },
                { 2, 4, 1 },
                { 4, 6, 0 }
            };

            state.sequenceRunIndices = {
                0,
                2
            };


            BidiIsolatingRunSequence sequence{};

            sequence.runOffset = 0;
            sequence.runCount = 2;
            sequence.level = 0;
            sequence.sos = UnicodeBidiClass::LeftToRight;
            sequence.eos = UnicodeBidiClass::LeftToRight;

            state.sequences = {
                sequence
            };


            if (!runBidiN0TestState(database, state))
                return fail("case 16 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight ||
                state.types[5] != UnicodeBidiClass::LeftToRight)
            {
                return fail(
                    "non-contiguous IRS included gap characters");
            }

            ++passed;
        }


        // ====================================================================
        // Case 17
        //
        // Two independent isolating run sequences.
        //
        //      sequence 0, level 0: ( L )
        //      sequence 1, level 1: ( R )
        //
        // Each sequence must use its own embedding direction and context.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                {
                    0x0028,
                    0x0041,
                    0x0029,

                    0x0028,
                    0x05D0,
                    0x0029
                },
                state);


            state.runs = {
                { 0, 3, 0 },
                { 3, 6, 1 }
            };

            state.sequenceRunIndices = {
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


            state.sequences = {
                sequence0,
                sequence1
            };


            if (!runBidiN0TestState(database, state))
                return fail("case 17 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight ||
                state.types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail("first isolating run sequence");
            }

            if (state.types[3] != UnicodeBidiClass::RightToLeft ||
                state.types[5] != UnicodeBidiClass::RightToLeft)
            {
                return fail("second isolating run sequence");
            }

            ++passed;
        }


        // ====================================================================
        // Case 18
        //
        // BD16 canonical-equivalence bracket matching integrated with N0.
        //
        //      U+2329 L U+3009
        //
        // U+3009 is treated as equivalent to U+232A while matching.
        //
        //      brackets -> L
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x2329, 0x0041, 0x3009 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 18 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 18 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight ||
                state.types[2] != UnicodeBidiClass::LeftToRight)
            {
                return fail(
                    "canonical-equivalent pair did not resolve");
            }

            ++passed;
        }


        // ====================================================================
        // Case 19
        //
        // BD14/BD15 use current working type.
        //
        // Force the opening parenthesis to L before N0:
        //
        //      L-bracket L )
        //
        // It is no longer an opening paired bracket for BD16. No pair should
        // be discovered, and the closing parenthesis remains ON.
        // ====================================================================

        {
            ++cases;

            initializeBidiN0Scalars(
                database,
                { 0x0028, 0x0041, 0x0029 },
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 19 setup");


            state.types[0] =
                UnicodeBidiClass::LeftToRight;


            if (!runBidiN0TestState(database, state))
                return fail("case 19 execution");


            if (state.types[0] != UnicodeBidiClass::LeftToRight)
                return fail("non-ON opener was modified");

            if (state.types[2] != UnicodeBidiClass::OtherNeutral)
                return fail("unmatched closer was modified");

            ++passed;
        }


        // ====================================================================
        // Case 20
        //
        // Exactly 63 nested openers are permitted by BD16.
        //
        // Place one L character in the center:
        //
        //      ((( ... 63 ... A ... 63 ... )))
        //
        // Every bracket pair encloses L and therefore resolves L.
        // ====================================================================

        {
            ++cases;

            std::vector<uint32_t> values;

            values.reserve(127);

            for (uint32_t i = 0; i < 63; ++i)
                values.push_back(0x0028);

            values.push_back(0x0041);

            for (uint32_t i = 0; i < 63; ++i)
                values.push_back(0x0029);


            initializeBidiN0Scalars(
                database,
                values,
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 20 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 20 execution");


            for (uint32_t i = 0; i < 63; ++i)
            {
                if (state.types[i] !=
                    UnicodeBidiClass::LeftToRight)
                {
                    return fail("63-deep opening bracket");
                }
            }


            for (uint32_t i = 64; i < 127; ++i)
            {
                if (state.types[i] !=
                    UnicodeBidiClass::LeftToRight)
                {
                    return fail("63-deep closing bracket");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 21
        //
        // A 64th nested opener overflows the fixed BD16 stack.
        //
        // BD16 must return an empty pair list for the entire IRS. Therefore,
        // even though L occurs inside all the apparent parentheses, none of
        // the brackets may be resolved by N0.
        // ====================================================================

        {
            ++cases;

            std::vector<uint32_t> values;

            values.reserve(129);

            for (uint32_t i = 0; i < 64; ++i)
                values.push_back(0x0028);

            values.push_back(0x0041);

            for (uint32_t i = 0; i < 64; ++i)
                values.push_back(0x0029);


            initializeBidiN0Scalars(
                database,
                values,
                state);

            if (!configureBidiN0SingleSequence(state, 0))
                return fail("case 21 setup");

            if (!runBidiN0TestState(database, state))
                return fail("case 21 execution");


            for (uint32_t i = 0; i < 64; ++i)
            {
                if (state.types[i] !=
                    UnicodeBidiClass::OtherNeutral)
                {
                    return fail(
                        "BD16 overflow resolved an opening bracket");
                }
            }


            for (uint32_t i = 65; i < 129; ++i)
            {
                if (state.types[i] !=
                    UnicodeBidiClass::OtherNeutral)
                {
                    return fail(
                        "BD16 overflow resolved a closing bracket");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode bidi N0: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            cases,
            passed);


        return passed == cases;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static bool testUnicodeBidiN0(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Unicode bidi N0: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testUnicodeBidiN0(databaseData);
    }

} // namespace waavs