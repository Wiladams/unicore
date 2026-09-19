// opentype_gsub_orchestrator.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "opentype_layout_selection.h"
#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    // ====================================================================
    // validateOpenTypeGsubLookupPlan
    //
    // OpenTypeLayoutLookupPlan normally comes from
    // selectOpenTypeLayoutLookups(), which guarantees that lookup indices
    // are unique and emitted in LookupList order.
    //
    // Validate that invariant again at the execution boundary.
    // ====================================================================

    static inline bool validateOpenTypeGsubLookupPlan(
        const OpenTypeLayoutLookupPlan& plan,
        const OpenTypeLayoutLookupListView& lookups) noexcept
    {
        if (!plan.lookupListData)
            return false;

        bool havePrevious = false;
        uint16_t previous = 0;

        for (uint16_t lookupIndex : plan.lookupIndices)
        {
            if (lookupIndex >= lookups.size())
                return false;

            if (havePrevious && lookupIndex <= previous)
                return false;

            previous = lookupIndex;
            havePrevious = true;
        }

        return true;
    }


    // ====================================================================
    // applyOpenTypeGsubLookupPlan
    //
    // Apply all selected GSUB LookupList entries in plan order.
    //
    // The complete plan is transactional:
    //
    //   source buffer
    //       |
    //       v
    //   working copy
    //       |
    //       +-- lookup 0
    //       +-- lookup 1
    //       +-- ...
    //       |
    //       v
    //   commit only after complete success
    //
    // Individual GSUB lookup executors already provide their own internal
    // transactional behavior. This outer working copy extends that guarantee
    // across the entire selected lookup sequence.
    // ====================================================================

    static inline bool applyOpenTypeGsubLookupPlan(
        const OpenTypeLayoutLookupPlan& plan,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        if (!plan.lookupListData)
            return false;

        const OpenTypeLayoutLookupListView lookups(plan.lookupListData);

        if (!lookups)
            return false;

        if (!validateOpenTypeGsubLookupPlan(plan, lookups))
            return false;


        // No selected lookups is a successful no-op.

        if (plan.lookupIndices.empty())
            return true;


        OpenTypeShapingBuffer working = buffer;

        for (uint16_t lookupIndex : plan.lookupIndices)
        {
            if (!applyOpenTypeGsubLookup(
                lookups, lookupIndex, gdef, working))
            {
                return false;
            }
        }

        buffer = std::move(working);
        return true;
    }


    // ====================================================================
    // Convenience overload without GDEF.
    //
    // Useful for fonts/lookups that do not require glyph-class filtering.
    // ====================================================================

    static inline bool applyOpenTypeGsubLookupPlan(
        const OpenTypeLayoutLookupPlan& plan,
        OpenTypeShapingBuffer& buffer)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGsubLookupPlan(plan, gdef, buffer);
    }

} // namespace waavs