// test_script_recognition_interpreter.h
#pragma once

#include "test_core.h"

#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"
#include "script_recognition_interpreter.h"

namespace waavs
{
    static inline bool runScriptRecognitionInterpreter()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef halant = dsl.kind("Halant");
        const ScriptItemKindRef matraPre = dsl.kind("MatraPre");
        const ScriptRoleRef base = dsl.role("Base");

        if (!consonant || !halant || !matraPre || !base)
            return false;

        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef H = dsl.match(halant);
        const ScriptExprRef M = dsl.match(matraPre);

        const ScriptExprRef expression =
            dsl.seq({
                C,
                H,
                dsl.capture(base, C),
                dsl.opt(M)
                });

        if (!expression || !dsl.unit("ConsonantSyllable", expression))
            return false;


        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;


        const std::vector<ScriptItemKindId> kinds =
        {
            consonant.id,
            halant.id,
            consonant.id,
            matraPre.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.kinds != kinds)
            return false;

        if (result.units.size() != 1)
            return false;

        if (result.roles.size() != 1)
            return false;


        const ScriptRecognitionUnit& unit = result.units[0];

        if (unit.span.first != 0 || unit.span.count != 4)
            return false;

        if (unit.roleOffset != 0 || unit.roleCount != 1)
            return false;


        const ScriptRoleBinding& role = result.roles[0];

        if (role.role != base.id)
            return false;

        if (role.span.first != 2 || role.span.count != 1)
            return false;


        printf(
            "Script recognition interpreter: PASS\n"
            "  Input kinds: %zu\n"
            "  Units: %zu\n"
            "  Roles: %zu\n"
            "  Unit span: [%u,%u)\n"
            "  Base span: [%u,%u)\n",
            kinds.size(),
            result.units.size(),
            result.roles.size(),
            unit.span.first,
            unit.span.end(),
            role.span.first,
            role.span.end());

        return true;
    }


    static inline void testScriptRecognitionInterpreter()
    {
        runScriptRecognitionInterpreter();
    }

} // namespace waavs