// unicode_bidi_visual_order.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "unicode_bidi_analysis.h"

namespace waavs
{
    // ====================================================================
    // makeBidiVisualRunOrder
    //
    // Apply UAX #9 L2 to a sequence of already-itemized runs.
    //
    // levels contains one resolved bidi level per logical run.
    //
    // visualOrder receives logical run indices in visual left-to-right
    // order.
    //
    // IMPORTANT:
    //
    // This reorders RUNS only.
    //
    // It does not reorder scalars and it does not reverse shaped glyph
    // buffers. RTL glyph buffers remain in logical order and will later be
    // positioned by an RTL compositor.
    //
    // Example:
    //
    // Logical runs:
    //
    //     A       Hebrew       B
    //     level 0 level 1      level 0
    //
    // Visual run order remains:
    //
    //     0 1 2
    //
    // The Hebrew run itself will later paint right-to-left inside its
    // assigned horizontal extent.
    //
    // More interesting:
    //
    //     levels: 0 1 2 1 0
    //
    //     visual: 0 3 2 1 4
    // ====================================================================

    [[nodiscard]]
    static bool makeBidiVisualRunOrder(const UnicodeBidiLevel* levels,
        size_t count, std::vector<size_t>& visualOrder)
    {
        visualOrder.clear();

        if (count == 0)
            return true;

        if (!levels)
            return false;

        visualOrder.resize(count);

        UnicodeBidiLevel highestLevel = 0;
        UnicodeBidiLevel lowestOddLevel = kUnicodeBidiLevelInvalid;

        for (size_t i = 0; i < count; ++i)
        {
            const UnicodeBidiLevel level = levels[i];

            if (level > kUnicodeBidiMaxDepth)
            {
                visualOrder.clear();
                return false;
            }

            visualOrder[i] = i;

            if (level > highestLevel)
                highestLevel = level;

            if ((level & 1u) != 0 &&
                (lowestOddLevel == kUnicodeBidiLevelInvalid ||
                    level < lowestOddLevel))
            {
                lowestOddLevel = level;
            }
        }


        // No odd level means the complete run sequence remains LTR.

        if (lowestOddLevel == kUnicodeBidiLevelInvalid)
            return true;


        // ---------------------------------------------------------------
        // UAX #9 L2
        //
        // From the highest resolved level down through the lowest odd
        // level, reverse each contiguous sequence whose level is greater
        // than or equal to the current level.
        //
        // visualOrder changes as each level is processed, so level tests
        // always refer back through the logical run index.
        // ---------------------------------------------------------------

        for (int currentLevel = static_cast<int>(highestLevel);
            currentLevel >= static_cast<int>(lowestOddLevel);
            --currentLevel)
        {
            size_t begin = 0;

            while (begin < count)
            {
                while (begin < count &&
                    levels[visualOrder[begin]] < currentLevel)
                {
                    ++begin;
                }

                if (begin == count)
                    break;

                size_t end = begin + 1;

                while (end < count &&
                    levels[visualOrder[end]] >= currentLevel)
                {
                    ++end;
                }

                std::reverse(
                    visualOrder.begin() + static_cast<std::ptrdiff_t>(begin),
                    visualOrder.begin() + static_cast<std::ptrdiff_t>(end));

                begin = end;
            }
        }

        return true;
    }


    // ====================================================================
    // Convenience overload
    // ====================================================================

    [[nodiscard]]
    static bool makeBidiVisualRunOrder(const std::vector<UnicodeBidiLevel>& levels,
        std::vector<size_t>& visualOrder)
    {
        return makeBidiVisualRunOrder(
            levels.empty() ? nullptr : levels.data(),
            levels.size(),
            visualOrder);
    }

} // namespace waavs