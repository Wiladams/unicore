// test_script_recognition_dsl.h
#pragma once

#include "test_core.h"

#include "script_recognition_dsl.h"

namespace waavs
{
    static inline bool runScriptRecognitionDSL()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef consonant =
            dsl.kind("Consonant");

        const ScriptItemKindRef halant =
            dsl.kind("Halant");

        const ScriptItemKindRef matraPre =
            dsl.kind("MatraPre");

        const ScriptRoleRef base =
            dsl.role("Base");

        if (!consonant || !halant || !matraPre || !base)
            return false;


        const ScriptExprRef C =
            dsl.match(consonant);

        const ScriptExprRef H =
            dsl.match(halant);

        const ScriptExprRef M =
            dsl.match(matraPre);

        if (!C || !H || !M)
            return false;


        const ScriptExprRef expression =
            dsl.seq({
                C,
                H,
                dsl.capture(base, C),
                dsl.opt(M)
                });

        if (!expression)
            return false;


        if (!dsl.unit("ConsonantSyllable", expression))
            return false;


        const ScriptRecognitionDescription& description =
            dsl.description();

        if (description.kinds.size() != 3)
            return false;

        if (description.roles.size() != 1)
            return false;

        if (description.unitTypes.size() != 1)
            return false;

        if (description.units.size() != 1)
            return false;


        if (description.kinds[0].name != "Consonant")
            return false;

        if (description.kinds[1].name != "Halant")
            return false;

        if (description.kinds[2].name != "MatraPre")
            return false;

        if (description.roles[0].name != "Base")
            return false;

        if (description.unitTypes[0].name != "ConsonantSyllable")
            return false;


        const ScriptRecognitionUnitDescription& unit =
            description.units[0];

        if (unit.expression != expression.id)
            return false;


        const ScriptRecognitionExpr* root =
            description.expression(expression.id);

        if (!root)
            return false;

        if (root->kind != ScriptRecognitionExprKind::Sequence)
            return false;

        if (root->childCount != 4)
            return false;


        const ScriptRecognitionExprId* children =
            description.children(*root);

        if (!children)
            return false;


        const ScriptRecognitionExpr* capture =
            description.expression(children[2]);

        if (!capture)
            return false;

        if (capture->kind != ScriptRecognitionExprKind::Capture)
            return false;

        if (capture->roleId != base.id)
            return false;


        const ScriptRecognitionExpr* optional =
            description.expression(children[3]);

        if (!optional)
            return false;

        if (optional->kind != ScriptRecognitionExprKind::Optional)
            return false;

        return true;
    }


    static inline void testScriptRecognitionDSL()
    {
        const bool passed = runScriptRecognitionDSL();

        printf(
            "Script recognition DSL: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs