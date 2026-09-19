// test_script_recognition_result.h
#pragma once

#include "test_core.h"

#include "script_recognition_result.h"

namespace waavs
{
    static inline bool runScriptRecognitionResult()
    {
        ScriptRecognitionResult result;

        if (!result.empty())
            return false;

        if (result.kindCount() != 0)
            return false;

        if (result.unitCount() != 0)
            return false;

        if (result.roleCount() != 0)
            return false;


        // ------------------------------------------------------------
        // Classes.
        // ------------------------------------------------------------

        result.kinds =
        {
            1,
            2,
            1,
            3
        };

        if (result.kindCount() != 4)
            return false;

        if (result.kindAt(0) != 1)
            return false;

        if (result.kindAt(3) != 3)
            return false;

        if (result.kindAt(4) != kScriptItemKindInvalid)
            return false;


        // ------------------------------------------------------------
        // Roles.
        // ------------------------------------------------------------

        ScriptRoleBinding base{};
        base.role = 1;
        base.span = { 2, 1 };

        ScriptRoleBinding preBase{};
        preBase.role = 2;
        preBase.span = { 3, 1 };

        result.roles.push_back(base);
        result.roles.push_back(preBase);


        // ------------------------------------------------------------
        // Unit.
        // ------------------------------------------------------------

        ScriptRecognitionUnit unit{};

        unit.span = { 0, 4 };
        unit.type = 1;
        unit.flags = 0;
        unit.roleOffset = 0;
        unit.roleCount = 2;

        result.units.push_back(unit);

        if (result.empty())
            return false;

        if (result.unitCount() != 1)
            return false;

        if (result.roleCount() != 2)
            return false;


        // ------------------------------------------------------------
        // Accessors.
        // ------------------------------------------------------------

        const ScriptRecognitionUnit* storedUnit =
            result.unit(0);

        if (!storedUnit)
            return false;

        if (storedUnit->span.first != 0 || storedUnit->span.count != 4)
            return false;

        if (storedUnit->type != 1)
            return false;

        if (result.unit(1) != nullptr)
            return false;


        const ScriptRoleBinding* roleData =
            result.roleData(*storedUnit);

        if (!roleData)
            return false;

        if (roleData[0].role != 1)
            return false;

        if (roleData[0].span.first != 2 || roleData[0].span.count != 1)
            return false;

        if (roleData[1].role != 2)
            return false;

        if (roleData[1].span.first != 3 || roleData[1].span.count != 1)
            return false;


        const ScriptRoleBinding* foundBase =
            result.roleFor(*storedUnit, 1);

        if (!foundBase)
            return false;

        if (foundBase->span.first != 2 || foundBase->span.count != 1)
            return false;

        const ScriptRoleBinding* foundPreBase =
            result.roleFor(*storedUnit, 2);

        if (!foundPreBase)
            return false;

        if (foundPreBase->span.first != 3 || foundPreBase->span.count != 1)
            return false;

        if (result.roleFor(*storedUnit, 3) != nullptr)
            return false;


        // ------------------------------------------------------------
        // Clear.
        // ------------------------------------------------------------

        result.clear();

        if (!result.empty())
            return false;

        if (result.kindCount() != 0)
            return false;

        if (result.unitCount() != 0)
            return false;

        if (result.roleCount() != 0)
            return false;

        return true;
    }


    static inline void testScriptRecognitionResult()
    {
        const bool passed = runScriptRecognitionResult();

        printf(
            "Script recognition result: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs