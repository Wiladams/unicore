// test_unicode_bidi_visual_order.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <initializer_list>
#include <vector>

#include "unicode_bidi_visual_order.h"

namespace waavs
{
    static bool bidiVisualOrderMatches(const std::vector<size_t>& actual,
        std::initializer_list<size_t> expected)
    {
        if (actual.size() != expected.size())
            return false;

        size_t i = 0;

        for (size_t value : expected)
        {
            if (actual[i] != value)
                return false;

            ++i;
        }

        return true;
    }


    static bool testUnicodeBidiVisualOrder()
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "Unicode bidi visual run order: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        size_t cases = 0;
        size_t passed = 0;


        // ================================================================
        // Case 1
        //
        // Empty input.
        // ================================================================

        {
            ++cases;

            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(nullptr, 0, order))
                return fail("empty input rejected");

            if (!order.empty())
                return fail("empty input produced visual runs");

            ++passed;
        }


        // ================================================================
        // Case 2
        //
        // Ordinary LTR.
        //
        // Logical:
        //
        //     0 1 2 3
        //
        // Levels:
        //
        //     0 0 0 0
        //
        // Visual:
        //
        //     0 1 2 3
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 0, 0, 0, 0 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 4, order))
                return fail("LTR ordering failed");

            if (!bidiVisualOrderMatches(order, { 0, 1, 2, 3 }))
                return fail("LTR visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Case 3
        //
        // One Hebrew run inside an LTR paragraph.
        //
        // Logical runs:
        //
        //     Latin Hebrew Latin
        //
        // Levels:
        //
        //       0     1     0
        //
        // The run boxes remain in the same order. The Hebrew glyphs will
        // later paint RTL inside run 1.
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 0, 1, 0 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 3, order))
                return fail("single RTL run ordering failed");

            if (!bidiVisualOrderMatches(order, { 0, 1, 2 }))
                return fail("single RTL run visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Case 4
        //
        // Two logical shaping runs belonging to one level-1 sequence.
        //
        // They reverse as run units.
        //
        // Levels:
        //
        //     0 1 1 0
        //
        // Visual:
        //
        //     0 2 1 3
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 0, 1, 1, 0 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 4, order))
                return fail("adjacent RTL run ordering failed");

            if (!bidiVisualOrderMatches(order, { 0, 2, 1, 3 }))
                return fail("adjacent RTL visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Case 5
        //
        // Nested even run inside RTL context.
        //
        // Levels:
        //
        //     0 1 2 1 0
        //
        // L2:
        //
        //     level 2: run 2 alone
        //     level 1: reverse runs 1..3
        //
        // Visual:
        //
        //     0 3 2 1 4
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 0, 1, 2, 1, 0 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 5, order))
                return fail("nested bidi ordering failed");

            if (!bidiVisualOrderMatches(order, { 0, 3, 2, 1, 4 }))
                return fail("nested bidi visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Case 6
        //
        // RTL paragraph containing an embedded LTR run.
        //
        // Logical:
        //
        //     Hebrew LTR Hebrew
        //
        // Levels:
        //
        //       1     2    1
        //
        // Visual:
        //
        //       2     1    0
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 1, 2, 1 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 3, order))
                return fail("RTL paragraph ordering failed");

            if (!bidiVisualOrderMatches(order, { 2, 1, 0 }))
                return fail("RTL paragraph visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Case 7
        //
        // Multiple level-2 runs nested inside level 1.
        //
        // Levels:
        //
        //     1 2 2 1
        //
        // Level 2:
        //
        //     1 2 -> 2 1
        //
        // Level 1:
        //
        //     reverse entire sequence
        //
        // Visual:
        //
        //     3 1 2 0
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 1, 2, 2, 1 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 4, order))
                return fail("nested multi-run ordering failed");

            if (!bidiVisualOrderMatches(order, { 3, 1, 2, 0 }))
                return fail("nested multi-run visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Case 8
        //
        // Even levels only remain LTR.
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] = { 0, 2, 2, 0 };
            std::vector<size_t> order;

            if (!makeBidiVisualRunOrder(levels, 4, order))
                return fail("even-level ordering failed");

            if (!bidiVisualOrderMatches(order, { 0, 1, 2, 3 }))
                return fail("even-level visual order mismatch");

            ++passed;
        }


        // ================================================================
        // Invalid level.
        // ================================================================

        {
            ++cases;

            const UnicodeBidiLevel levels[] =
            {
                0,
                static_cast<UnicodeBidiLevel>(kUnicodeBidiMaxDepth + 1u)
            };

            std::vector<size_t> order;

            if (makeBidiVisualRunOrder(levels, 2, order))
                return fail("invalid bidi level accepted");

            if (!order.empty())
                return fail("failure left partial visual order");

            ++passed;
        }


        std::printf(
            "Unicode bidi visual run order: PASS\n"
            "  Cases:   %zu\n"
            "  Passed:  %zu\n",
            cases,
            passed);

        return passed == cases;
    }

} // namespace waavs