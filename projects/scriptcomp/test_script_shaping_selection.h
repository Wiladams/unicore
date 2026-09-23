// test_script_shaping_selection.h
#pragma once

#include "test_core.h"

#include "script_shaping_selection.h"

#include <cstdio>

namespace waavs
{
    static bool testScriptShapingSelection()
    {
        ScriptRecognitionResult recognition;

        recognition.roles.push_back({
            ScriptRoleId(1),
            0,
            { 1, 2 }
            });

        ScriptRecognitionUnit unit{};

        unit.span = { 0, 4 };
        unit.roleOffset = 0;
        unit.roleCount = 1;


        ScriptShapingBuffer buffer;

        ScriptShapingItem item{};

        item.value = 10;
        item.scalarOffset = 0;
        item.scalarCount = 1;
        buffer.pushBack(item);

        item = {};
        item.value = 11;
        item.scalarOffset = 1;
        item.scalarCount = 1;
        buffer.pushBack(item);

        item = {};
        item.value = 12;
        item.scalarOffset = 3;
        item.scalarCount = 1;
        buffer.pushBack(item);

        item = {};
        item.value = 13;
        item.scalarOffset = 2;
        item.scalarCount = 1;
        buffer.pushBack(item);


        ScriptShapingSelectionState state;

        if (!state.reset(2))
            return false;


        // ------------------------------------------------------------
        // Resolve current unit [0,4).
        // ------------------------------------------------------------

        ScriptShapingResolvedSelection resolved;

        const ScriptShapingSelectionRef unitRef = scriptShapingUnitSelection();

        if (!resolveScriptShapingSelection(
            unitRef, recognition, unit, state, buffer, resolved))
        {
            return false;
        }

        if (resolved.size() != 4 ||
            resolved.itemIndices[0] != 0 ||
            resolved.itemIndices[1] != 1 ||
            resolved.itemIndices[2] != 2 ||
            resolved.itemIndices[3] != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Resolve recognition role [1,3).
        //
        // Current shaping-buffer positions are 1 and 3.
        // ------------------------------------------------------------

        const ScriptShapingSelectionRef role =
            scriptShapingRoleSelection(
                ScriptRoleId(1));

        if (!resolveScriptShapingSelection(
            role,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (resolved.size() != 2 ||
            resolved.itemIndices[0] != 1 ||
            resolved.itemIndices[1] != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Derived selection containing source scalars 0 and 3.
        //
        // Current shaping-buffer positions are 0 and 2.
        // ------------------------------------------------------------

        ScriptShapingDerivedSelection* derived =
            state.selection(
                ScriptShapingSelectionId(1));

        if (!derived)
            return false;

        derived->sourceSpans.push_back({ 0, 1 });
        derived->sourceSpans.push_back({ 3, 1 });


        const ScriptShapingSelectionRef derivedRef =
            scriptShapingDerivedSelection(
                ScriptShapingSelectionId(1));

        if (!resolveScriptShapingSelection(
            derivedRef,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (resolved.size() != 2 ||
            resolved.itemIndices[0] != 0 ||
            resolved.itemIndices[1] != 2)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Empty derived selection is valid.
        // ------------------------------------------------------------

        const ScriptShapingSelectionRef emptyRef =
            scriptShapingDerivedSelection(
                ScriptShapingSelectionId(2));

        if (!resolveScriptShapingSelection(
            emptyRef,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (!resolved.empty())
            return false;


        // ------------------------------------------------------------
        // Missing optional role is valid and empty.
        // ------------------------------------------------------------

        const ScriptShapingSelectionRef missingRole =
            scriptShapingRoleSelection(
                ScriptRoleId(2));

        if (!resolveScriptShapingSelection(
            missingRole,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (!resolved.empty())
            return false;


        std::printf(
            "Script shaping selection: PASS\n"
            "  Unit resolution:       PASS\n"
            "  Role resolution:       PASS\n"
            "  Derived resolution:    PASS\n"
            "  Empty derived:         PASS\n"
            "  Missing role:          PASS\n");

        return true;
    }

} // namespace waavs