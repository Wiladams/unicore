// test_script_shaping_provenance.h
#pragma once

#include "test_core.h"

#include "script_shaping_provenance.h"

#include <cstdio>
#include <vector>

namespace waavs
{
    static bool testScriptShapingProvenance()
    {
        ScriptShapingBuffer buffer;


        // ------------------------------------------------------------
        // Simulate:
        //
        // source:
        //
        //     0 1 2 3
        //
        // shaping sequence:
        //
        //     source 0
        //     source 2
        //     source 1
        //     source 2 generated
        //     source 3
        //
        // Source scalar 2 has expanded into two items and those items
        // are no longer necessarily adjacent to their original neighbors.
        // ------------------------------------------------------------

        ScriptShapingItem item{};

        item.value = 0x0041;
        item.scalarOffset = 0;
        item.scalarCount = 1;
        buffer.pushBack(item);

        item = {};
        item.value = 0x0043;
        item.scalarOffset = 2;
        item.scalarCount = 1;
        buffer.pushBack(item);

        item = {};
        item.value = 0x0042;
        item.scalarOffset = 1;
        item.scalarCount = 1;
        buffer.pushBack(item);

        item = {};
        item.value = 0x0301;
        item.scalarOffset = 2;
        item.scalarCount = 1;
        item.flags = ScriptShapingItemFlagGenerated;
        buffer.pushBack(item);

        item = {};
        item.value = 0x0044;
        item.scalarOffset = 3;
        item.scalarCount = 1;
        buffer.pushBack(item);


        // Recognition says that semantic role R came from source scalar 2.

        const ScriptSpan roleSpan =
        {
            2,
            1
        };


        std::vector<uint32_t> indices;

        if (!collectScriptShapingItemIndicesForSourceSpan(
            buffer,
            roleSpan,
            indices))
        {
            return false;
        }


        if (indices.size() != 2 ||
            indices[0] != 1 ||
            indices[1] != 3)
        {
            std::printf(
                "Script shaping provenance: FAIL: expanded/reordered selection\n");

            return false;
        }


        if (!scriptShapingItemWithinSourceSpan(
            buffer[1],
            roleSpan))
        {
            return false;
        }


        if (!scriptShapingItemWithinSourceSpan(
            buffer[3],
            roleSpan))
        {
            return false;
        }


        if (scriptShapingItemIntersectsSourceSpan(
            buffer[2],
            roleSpan))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Simulate a contraction representing source scalars [1,4).
        // ------------------------------------------------------------

        ScriptShapingItem contracted{};

        contracted.value = 123;
        contracted.scalarOffset = 1;
        contracted.scalarCount = 3;

        if (!scriptShapingItemContainsSourceSpan(
            contracted,
            roleSpan))
        {
            return false;
        }


        std::printf(
            "Script shaping provenance: PASS\n"
            "  Expanded source item:       PASS\n"
            "  Reordered source item:      PASS\n"
            "  Generated provenance:       PASS\n"
            "  Contracted provenance:      PASS\n");

        return true;
    }

} // namespace waavs