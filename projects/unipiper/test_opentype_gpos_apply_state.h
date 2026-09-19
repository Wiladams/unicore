// test_opentype_gpos_apply_state.h
#pragma once

#include "../unitils/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "opentype_gpos_apply_state.h"

namespace waavs
{
    static bool testOpenTypeGposApplyState()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS apply state: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - Range geometry.
        // ================================================================

        {
            ++cases;

            const OpenTypeGposApplyRange range{ 2, 6 };

            if (!range.isValid() ||
                !range.validFor(10) ||
                range.empty() ||
                range.size() != 4)
            {
                return fail("case 1 range geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Half-open containment.
        // ================================================================

        {
            ++cases;

            const OpenTypeGposApplyRange range{ 2, 6 };

            if (!range.contains(2) ||
                !range.contains(5) ||
                range.contains(1) ||
                range.contains(6))
            {
                return fail("case 2 containment");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Range bounds.
        // ================================================================

        {
            ++cases;

            const OpenTypeGposApplyRange reversed{ 6, 2 };
            const OpenTypeGposApplyRange tooLarge{ 2, 11 };
            const OpenTypeGposApplyRange empty{ 4, 4 };

            if (reversed.isValid())
                return fail("case 3 reversed range");

            if (tooLarge.validFor(10))
                return fail("case 3 oversized range");

            if (!empty.isValid() ||
                !empty.validFor(10) ||
                !empty.empty())
            {
                return fail("case 3 empty range");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Whole-buffer range.
        // ================================================================

        {
            ++cases;

            const OpenTypeGposApplyRange range =
                OpenTypeGposApplyRange::whole(7);

            if (!range.validFor(7) ||
                range.begin != 0 ||
                range.end != 7 ||
                range.size() != 7 ||
                !range.contains(0) ||
                !range.contains(6) ||
                range.contains(7))
            {
                return fail("case 4 whole range");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Recursion state.
        // ================================================================

        {
            ++cases;

            OpenTypeGposApplyState state;

            if (state.nestingDepth != 0)
                return fail("case 5 initial depth");

            if (!state.enter() ||
                state.nestingDepth != 1)
            {
                return fail("case 5 first enter");
            }

            if (!state.enter() ||
                state.nestingDepth != 2)
            {
                return fail("case 5 second enter");
            }

            state.leave();

            if (state.nestingDepth != 1)
                return fail("case 5 first leave");

            state.leave();

            if (state.nestingDepth != 0)
                return fail("case 5 second leave");

            ++passed;
        }


        // ================================================================
        // Case 6 - Nesting limit.
        // ================================================================

        {
            ++cases;

            OpenTypeGposApplyState state;
            state.maxNestingDepth = 2;

            if (!state.enter())
                return fail("case 6 depth 1");

            if (!state.enter())
                return fail("case 6 depth 2");

            if (state.enter())
                return fail("case 6 depth limit");

            if (state.nestingDepth != 2)
                return fail("case 6 failed enter changed depth");

            state.leave();
            state.leave();

            ++passed;
        }


        // ================================================================
        // Case 7 - RAII scope.
        // ================================================================

        {
            ++cases;

            OpenTypeGposApplyState state;

            {
                OpenTypeGposApplyScope outer(state);

                if (!outer ||
                    state.nestingDepth != 1)
                {
                    return fail("case 7 outer scope");
                }

                {
                    OpenTypeGposApplyScope inner(state);

                    if (!inner ||
                        state.nestingDepth != 2)
                    {
                        return fail("case 7 inner scope");
                    }
                }

                if (state.nestingDepth != 1)
                    return fail("case 7 inner unwind");
            }

            if (state.nestingDepth != 0)
                return fail("case 7 outer unwind");

            ++passed;
        }


        // ================================================================
        // Case 8 - Operation budget.
        // ================================================================

        {
            ++cases;

            OpenTypeGposApplyState state;
            state.operationBudget = 3;

            if (!state.consumeOperation())
                return fail("case 8 operation 1");

            if (!state.consumeOperation(2))
                return fail("case 8 operations 2-3");

            if (state.operationBudget != 0)
                return fail("case 8 remaining budget");

            if (state.consumeOperation())
                return fail("case 8 exhausted budget");

            if (!state.consumeOperation(0))
                return fail("case 8 zero operation");

            ++passed;
        }


        std::printf(
            "OpenType GPOS apply state: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Range geometry:             PASS\n"
            "  Half-open containment:      PASS\n"
            "  Range bounds:               PASS\n"
            "  Whole-buffer range:         PASS\n"
            "  Recursion state:            PASS\n"
            "  Nesting limit:              PASS\n"
            "  RAII scope:                 PASS\n"
            "  Operation budget:           PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs