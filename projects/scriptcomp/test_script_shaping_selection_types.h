// test_script_shaping_selection_types.h
#pragma once

#include "test_core.h"

#include "script_shaping_selection_types.h"

#include <cstdio>

namespace waavs
{
    static bool testScriptShapingSelectionTypes()
    {
        const ScriptShapingSelectionRef invalid{};

        if (invalid.valid())
            return false;


        const ScriptShapingSelectionRef role =
            scriptShapingRoleSelection(
                ScriptRoleId(7));

        if (!role.valid() ||
            role.kind != ScriptShapingSelectionKind::Role ||
            role.id != 7)
        {
            return false;
        }


        const ScriptShapingSelectionRef derived =
            scriptShapingDerivedSelection(
                ScriptShapingSelectionId(3));

        if (!derived.valid() ||
            derived.kind != ScriptShapingSelectionKind::Derived ||
            derived.id != 3)
        {
            return false;
        }


        if (scriptShapingRoleSelection(
            kScriptRoleInvalid).valid())
        {
            return false;
        }


        if (scriptShapingDerivedSelection(
            kScriptShapingSelectionInvalid).valid())
        {
            return false;
        }


        std::printf(
            "Script shaping selection types: PASS\n"
            "  Invalid selection:     PASS\n"
            "  Role selection:        PASS\n"
            "  Derived selection:     PASS\n");

        return true;
    }

} // namespace waavs