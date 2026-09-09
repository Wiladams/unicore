// test_opentype_gsub_sequence_state.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "opentype_gsub_apply_state.h"
#include "opentype_gsub_edit.h"
#include "opentype_gsub_sequence_state.h"


namespace waavs
{
    static bool gsubSequencePositionsEqual(
        const OpenTypeGsubSequenceState& sequence,
        const size_t* expected,
        size_t count) noexcept
    {
        if (sequence.size() != count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            size_t position = 0;

            if (!sequence.position(i, position) || position != expected[i])
                return false;
        }

        return true;
    }


    static bool testOpenTypeGsubSequenceState()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB sequence state: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1 - Initial sequence mapping.
        //
        // Physical:
        //
        //   A mark B mark C
        //   0  1   2  3   4
        //
        // Context sequence:
        //
        //   { 0, 2, 4 }
        // ====================================================================

        {
            ++cases;

            const size_t positions[] = { 0, 2, 4 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(positions, 3))
                return fail("case 1 reset");

            if (sequence.empty() || sequence.size() != 3)
                return fail("case 1 size");

            if (!gsubSequencePositionsEqual(sequence, positions, 3))
                return fail("case 1 positions");

            size_t position = 99;

            if (!sequence.position(2, position) || position != 4)
                return fail("case 1 position lookup");

            if (sequence.position(3, position))
                return fail("case 1 out-of-range position");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Sequence member expansion.
        //
        // Before:
        //
        //   A mark B mark C
        //   0  1   2  3   4
        //
        // sequence = { 0, 2, 4 }
        //
        // B -> X Y Z
        //
        // edit:
        //
        //   inputPositions = { 2 }
        //   outputCount = 3
        //
        // After:
        //
        //   A mark X Y Z mark C
        //   0  1   2 3 4  5   6
        //
        // sequence = { 0, 2, 3, 4, 6 }
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 0, 2, 4 };
            const size_t expected[] = { 0, 2, 3, 4, 6 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 3))
                return fail("case 2 reset");

            OpenTypeGsubEdit edit;
            edit.inputPositions = { 2 };
            edit.outputCount = 3;

            if (!sequence.applyEdit(edit))
                return fail("case 2 expansion");

            if (!gsubSequencePositionsEqual(sequence, expected, 5))
                return fail("case 2 expanded positions");

            ++passed;
        }


        // ====================================================================
        // Case 3 - Filtered non-contiguous ligature contraction.
        //
        // Physical sequence members:
        //
        //   A mark B mark C mark D
        //   0  1   2  3   4  5   6
        //
        // sequence = { 0, 2, 4, 6 }
        //
        // B + C -> X
        //
        // The mark at physical position 3 is ignored and survives.
        //
        // edit:
        //
        //   inputPositions = { 2, 4 }
        //   outputCount = 1
        //
        // After:
        //
        //   A mark X mark mark D
        //   0  1   2  3   4   5
        //
        // sequence = { 0, 2, 5 }
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 0, 2, 4, 6 };
            const size_t expected[] = { 0, 2, 5 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 4))
                return fail("case 3 reset");

            OpenTypeGsubEdit edit;
            edit.inputPositions = { 2, 4 };
            edit.outputCount = 1;

            if (!sequence.applyEdit(edit))
                return fail("case 3 ligature contraction");

            if (!gsubSequencePositionsEqual(sequence, expected, 3))
                return fail("case 3 contracted positions");

            ++passed;
        }


        // ====================================================================
        // Case 4 - Non-member expansion.
        //
        // Outer contextual sequence:
        //
        //   A mark B
        //   0  1   2
        //
        // sequence = { 0, 2 }
        //
        // A nested lookup modifies the mark:
        //
        //   mark -> M1 M2
        //
        // The new glyphs are NOT members of the outer sequence.
        //
        // After:
        //
        //   A M1 M2 B
        //   0  1  2  3
        //
        // sequence = { 0, 3 }
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 0, 2 };
            const size_t expected[] = { 0, 3 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 2))
                return fail("case 4 reset");

            OpenTypeGsubEdit edit;
            edit.inputPositions = { 1 };
            edit.outputCount = 2;

            if (!sequence.applyEdit(edit))
                return fail("case 4 non-member expansion");

            if (!gsubSequencePositionsEqual(sequence, expected, 2))
                return fail("case 4 positions");

            ++passed;
        }


        // ====================================================================
        // Case 5 - Non-member anchor consumes a sequence member.
        //
        // Before:
        //
        //   A X B C
        //   0 1 2 3
        //
        // outer sequence = { 0, 2, 3 }
        //
        // Nested substitution:
        //
        //   X + B -> Y
        //
        // edit:
        //
        //   inputPositions = { 1, 2 }
        //   outputCount = 1
        //
        // Anchor X was not an outer sequence member, so Y does not become
        // one. B was an outer member and disappears.
        //
        // After:
        //
        //   A Y C
        //   0 1 2
        //
        // outer sequence = { 0, 2 }
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 0, 2, 3 };
            const size_t expected[] = { 0, 2 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 3))
                return fail("case 5 reset");

            OpenTypeGsubEdit edit;
            edit.inputPositions = { 1, 2 };
            edit.outputCount = 1;

            if (!sequence.applyEdit(edit))
                return fail("case 5 non-member anchor");

            if (!gsubSequencePositionsEqual(sequence, expected, 2))
                return fail("case 5 positions");

            ++passed;
        }


        // ====================================================================
        // Case 6 - Successive edits.
        //
        // Initial sequence:
        //
        //   { 0, 2, 4 }
        //
        // First:
        //
        //   position 2 -> three outputs
        //
        //   { 0, 2, 3, 4, 6 }
        //
        // Then:
        //
        //   physical positions 3 and 4 -> one ligature
        //
        //   { 0, 2, 3, 5 }
        //
        // This proves later state is interpreted in the coordinate system
        // produced by the preceding edit.
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 0, 2, 4 };
            const size_t afterExpansion[] = { 0, 2, 3, 4, 6 };
            const size_t finalPositions[] = { 0, 2, 3, 5 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 3))
                return fail("case 6 reset");

            OpenTypeGsubEdit first;
            first.inputPositions = { 2 };
            first.outputCount = 3;

            if (!sequence.applyEdit(first))
                return fail("case 6 first edit");

            if (!gsubSequencePositionsEqual(sequence, afterExpansion, 5))
                return fail("case 6 expansion state");

            OpenTypeGsubEdit second;
            second.inputPositions = { 3, 4 };
            second.outputCount = 1;

            if (!sequence.applyEdit(second))
                return fail("case 6 second edit");

            if (!gsubSequencePositionsEqual(sequence, finalPositions, 4))
                return fail("case 6 final state");

            ++passed;
        }


        // ====================================================================
        // Case 7 - Sequence may become empty.
        //
        // Outer sequence contains only B at physical position 2.
        //
        // A nested lookup anchored on non-member X at position 1 consumes B.
        //
        //   inputPositions = { 1, 2 }
        //
        // Since the anchor was not a sequence member, the replacement is not
        // added to the outer sequence.
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 2 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 1))
                return fail("case 7 reset");

            OpenTypeGsubEdit edit;
            edit.inputPositions = { 1, 2 };
            edit.outputCount = 1;

            if (!sequence.applyEdit(edit))
                return fail("case 7 edit");

            if (!sequence.empty() || sequence.size() != 0)
                return fail("case 7 sequence not empty");

            size_t position = 0;

            if (sequence.position(0, position))
                return fail("case 7 empty sequence position");

            ++passed;
        }


        // ====================================================================
        // Case 8 - Validation and transactionality.
        // ====================================================================

        {
            ++cases;

            const size_t initial[] = { 0, 2, 4 };

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(initial, 3))
                return fail("case 8 reset");


            // Zero outputs are not a valid GSUB edit.

            OpenTypeGsubEdit zeroOutput;
            zeroOutput.inputPositions = { 2 };
            zeroOutput.outputCount = 0;

            if (sequence.applyEdit(zeroOutput))
                return fail("case 8 zero-output edit accepted");

            if (!gsubSequencePositionsEqual(sequence, initial, 3))
                return fail("case 8 zero-output changed state");


            // Physical input positions must be strictly increasing.

            OpenTypeGsubEdit reversed;
            reversed.inputPositions = { 4, 2 };
            reversed.outputCount = 1;

            if (sequence.applyEdit(reversed))
                return fail("case 8 reversed edit accepted");

            if (!gsubSequencePositionsEqual(sequence, initial, 3))
                return fail("case 8 reversed edit changed state");


            // Duplicate input positions are invalid.

            OpenTypeGsubEdit duplicate;
            duplicate.inputPositions = { 2, 2 };
            duplicate.outputCount = 1;

            if (sequence.applyEdit(duplicate))
                return fail("case 8 duplicate edit accepted");

            if (!gsubSequencePositionsEqual(sequence, initial, 3))
                return fail("case 8 duplicate edit changed state");


            // Invalid initial mappings are rejected.

            const size_t unordered[] = { 0, 4, 2 };

            OpenTypeGsubSequenceState invalidSequence;

            if (invalidSequence.reset(unordered, 3))
                return fail("case 8 unordered reset accepted");

            if (invalidSequence.reset(nullptr, 0))
                return fail("case 8 empty reset accepted");

            ++passed;
        }


        // ====================================================================
        // Case 9 - Recursive apply state and operation budget.
        // ====================================================================

        {
            ++cases;

            OpenTypeGsubApplyState state;
            state.maxNestingDepth = 2;
            state.operationBudget = 3;


            // ------------------------------------------------------------
            // Explicit nesting.
            // ------------------------------------------------------------

            if (!state.enter() || state.nestingDepth != 1)
                return fail("case 9 first enter");

            if (!state.enter() || state.nestingDepth != 2)
                return fail("case 9 second enter");

            if (state.enter())
                return fail("case 9 nesting limit");

            if (state.nestingDepth != 2)
                return fail("case 9 failed enter changed depth");

            state.leave();

            if (state.nestingDepth != 1)
                return fail("case 9 first leave");

            state.leave();

            if (state.nestingDepth != 0)
                return fail("case 9 second leave");


            // ------------------------------------------------------------
            // Operation budget.
            // ------------------------------------------------------------

            if (!state.consumeOperation())
                return fail("case 9 operation 1");

            if (!state.consumeOperation(2))
                return fail("case 9 operation 2");

            if (state.operationBudget != 0)
                return fail("case 9 budget value");

            if (state.consumeOperation())
                return fail("case 9 exhausted budget accepted");


            // ------------------------------------------------------------
            // RAII nesting guard.
            // ------------------------------------------------------------

            {
                OpenTypeGsubApplyScope scope(state);

                if (!scope || state.nestingDepth != 1)
                    return fail("case 9 scope enter");
            }

            if (state.nestingDepth != 0)
                return fail("case 9 scope leave");

            ++passed;
        }


        std::printf(
            "OpenType GSUB sequence state: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Initial mapping:          PASS\n"
            "  Member expansion:         PASS\n"
            "  Ligature contraction:     PASS\n"
            "  Non-member expansion:     PASS\n"
            "  Non-member consumption:   PASS\n"
            "  Successive edits:         PASS\n"
            "  Empty sequence:           PASS\n"
            "  Validation:               PASS\n"
            "  Apply state:              PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs