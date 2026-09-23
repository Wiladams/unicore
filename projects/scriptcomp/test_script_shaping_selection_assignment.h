// test_script_shaping_selection_assignment.h
#pragma once

#include "test_core.h"

#include "script_shaping_selection.h"

#include <cstdio>

namespace waavs
{
    static bool testScriptShapingSelectionAssignment()
    {
        ScriptShapingBuffer buffer;

        ScriptShapingItem item{};

        // source 0
        item.value = 10;
        item.scalarOffset = 0;
        item.scalarCount = 1;
        buffer.pushBack(item);

        // source [2,3)
        item = {};
        item.value = 20;
        item.scalarOffset = 2;
        item.scalarCount = 1;
        buffer.pushBack(item);

        // generated descendant of source [2,3)
        item = {};
        item.value = 21;
        item.scalarOffset = 2;
        item.scalarCount = 1;
        item.flags = ScriptShapingItemFlagGenerated;
        buffer.pushBack(item);

        // source [3,4), adjacent to source 2
        item = {};
        item.value = 30;
        item.scalarOffset = 3;
        item.scalarCount = 1;
        buffer.pushBack(item);

        // source [7,9), simulating contraction
        item = {};
        item.value = 40;
        item.scalarOffset = 7;
        item.scalarCount = 2;
        buffer.pushBack(item);


        ScriptShapingResolvedSelection resolved;

        resolved.itemIndices =
        {
            1,
            2,
            3,
            4
        };


        ScriptShapingSelectionState state;

        if (!state.reset(1))
            return false;


        if (!assignScriptShapingDerivedSelection(
            state,
            ScriptShapingSelectionId(1),
            buffer,
            resolved))
        {
            return false;
        }


        const ScriptShapingDerivedSelection* selection =
            state.selection(
                ScriptShapingSelectionId(1));

        if (!selection)
            return false;


        // source 2 + duplicate source 2 + adjacent source 3
        // collapse to [2,4).
        //
        // contracted source [7,9) remains separate.

        if (selection->sourceSpans.size() != 2)
            return false;

        if (selection->sourceSpans[0].first != 2 ||
            selection->sourceSpans[0].count != 2)
        {
            return false;
        }

        if (selection->sourceSpans[1].first != 7 ||
            selection->sourceSpans[1].count != 2)
        {
            return false;
        }


        // Verify the derived selection can resolve back against the
        // current buffer.

        ScriptRecognitionResult recognition;
        ScriptRecognitionUnit unit{};

        ScriptShapingResolvedSelection roundTrip;

        const ScriptShapingSelectionRef ref =
            scriptShapingDerivedSelection(
                ScriptShapingSelectionId(1));

        if (!resolveScriptShapingSelection(
            ref,
            recognition,
            unit,
            state,
            buffer,
            roundTrip))
        {
            return false;
        }


        if (roundTrip.itemIndices.size() != 4 ||
            roundTrip.itemIndices[0] != 1 ||
            roundTrip.itemIndices[1] != 2 ||
            roundTrip.itemIndices[2] != 3 ||
            roundTrip.itemIndices[3] != 4)
        {
            return false;
        }


        std::printf(
            "Script shaping selection assignment: PASS\n"
            "  Duplicate provenance:    PASS\n"
            "  Adjacent span merge:     PASS\n"
            "  Contracted provenance:   PASS\n"
            "  Round-trip resolution:   PASS\n");

        return true;
    }

} // namespace waavs