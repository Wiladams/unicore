// test_script_recognition_types.h
#pragma once

#include "test_core.h"

#include "script_recognition_types.h"

namespace waavs
{
    static inline bool runScriptRecognitionTypes()
    {
        ScriptSpan span{ 4, 3 };

        if (span.first != 4)
            return false;

        if (span.count != 3)
            return false;

        if (span.end() != 7)
            return false;

        if (span.empty())
            return false;

        if (!span.contains(4))
            return false;

        if (!span.contains(6))
            return false;

        if (span.contains(3))
            return false;

        if (span.contains(7))
            return false;


        ScriptRoleBinding role{};

        role.role = 7;
        role.span = { 2, 3 };

        if (role.role != 7)
            return false;

        if (role.span.first != 2 || role.span.count != 3)
            return false;


        ScriptRecognitionUnit unit{};

        unit.span = { 0, 6 };
        unit.type = 3;
        unit.flags = 0x10;
        unit.roleOffset = 4;
        unit.roleCount = 2;

        if (unit.span.first != 0 || unit.span.count != 6)
            return false;

        if (unit.type != 3)
            return false;

        if (unit.flags != 0x10)
            return false;

        if (unit.roleOffset != 4 || unit.roleCount != 2)
            return false;

        return true;
    }


    static inline void testScriptRecognitionTypes()
    {
        const bool passed = runScriptRecognitionTypes();

        printf(
            "Script recognition types: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs