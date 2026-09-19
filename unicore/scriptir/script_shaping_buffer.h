// script_shaping_buffer.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "font_run.h"

namespace waavs
{
    // ========================================================================
    // ScriptShapingItem
    //
    // One scalar-domain shaping item.
    //
    // value:
    //   Unicode scalar value presented to script shaping and eventually cmap.
    //
    // scalarOffset/scalarCount:
    //   Logical source extent within the original FontRunView scalar sequence.
    //
    // Initially every source scalar becomes exactly one item:
    //
    //   value        = input.scalars[i].value
    //   scalarOffset = i
    //   scalarCount  = 1
    //
    // Later scalar-domain shaping may expand, contract, insert or reorder
    // items while retaining provenance through this source extent.
    // ========================================================================

    enum ScriptShapingItemFlags : uint32_t
    {
        ScriptShapingItemFlagNone = 0,
        ScriptShapingItemFlagGenerated = 1u << 0
    };

    struct ScriptShapingItem
    {
        uint32_t value{ 0 };
        uint32_t scalarOffset{ 0 };
        uint32_t scalarCount{ 0 };
        uint32_t flags{ ScriptShapingItemFlagNone };
    };


    // ========================================================================
    // ScriptShapingBuffer
    //
    // Mutable scalar-domain shaping sequence between FontRunView and cmap.
    //
    // mInput always refers to the real font-selected run. The mutable item
    // sequence may later differ from that run's original scalar sequence.
    //
    // Do not synthesize a replacement FontRunView when script shaping rewrites
    // the scalar sequence.
    // ========================================================================

    class ScriptShapingBuffer
    {
    public:
        void clear() noexcept
        {
            mItems.clear();
            mInput = nullptr;
        }

        [[nodiscard]]
        bool reset(const FontRunView& input)
        {
            clear();

            if (input.scalarCount != 0 && !input.scalars)
                return false;

            mInput = &input;
            mItems.reserve(input.scalarCount);

            for (uint32_t i = 0; i < input.scalarCount; ++i)
            {
                ScriptShapingItem item{};

                item.value = input.scalars[i].value;
                item.scalarOffset = i;
                item.scalarCount = 1;

                mItems.push_back(item);
            }

            return true;
        }

        [[nodiscard]] const FontRunView* input() const noexcept { return mInput; }

        [[nodiscard]] size_t size() const noexcept { return mItems.size(); }
        [[nodiscard]] bool empty() const noexcept { return mItems.empty(); }

        ScriptShapingItem& operator[](size_t index) noexcept { return mItems[index]; }
        const ScriptShapingItem& operator[](size_t index) const noexcept { return mItems[index]; }

        auto begin() noexcept { return mItems.begin(); }
        auto end() noexcept { return mItems.end(); }

        auto begin() const noexcept { return mItems.begin(); }
        auto end() const noexcept { return mItems.end(); }

        void pushBack(const ScriptShapingItem& item)
        {
            mItems.push_back(item);
        }

        std::vector<ScriptShapingItem>& items() noexcept { return mItems; }
        const std::vector<ScriptShapingItem>& items() const noexcept { return mItems; }

    private:
        const FontRunView* mInput{ nullptr };
        std::vector<ScriptShapingItem> mItems{};
    };

} // namespace waavs