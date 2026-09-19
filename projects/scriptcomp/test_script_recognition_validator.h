// test_script_recognition_validator.h
#pragma once

#include "test_core.h"

#include "script_recognition_dsl.h"
#include "script_recognition_validator.h"

namespace waavs
{
    static inline bool runScriptRecognitionValidator()
    {
        // ------------------------------------------------------------
        // Valid description.
        // ------------------------------------------------------------

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

        const ScriptExprRef syllable =
            dsl.seq({
                C,
                H,
                dsl.capture(base, C),
                dsl.opt(M)
                });

        if (!syllable)
            return false;

        if (!dsl.unit("ConsonantSyllable", syllable))
            return false;


        ScriptRecognitionValidationResult validation =
            validateScriptRecognitionDescription(
                dsl.description());

        if (!validation)
            return false;


        // ------------------------------------------------------------
        // Invalid Kind reference.
        // ------------------------------------------------------------

        {
            ScriptRecognitionDescription broken =
                dsl.description();

            ScriptRecognitionExpr* expr = nullptr;

            for (ScriptRecognitionExpr& candidate : broken.expressions)
            {
                if (candidate.kind == ScriptRecognitionExprKind::Kind)
                {
                    expr = &candidate;
                    break;
                }
            }

            if (!expr)
                return false;

            expr->kindId =
                static_cast<ScriptItemKindId>(
                    broken.kinds.size() + 1);

            validation =
                validateScriptRecognitionDescription(broken);

            if (validation.error !=
                ScriptRecognitionValidationError::InvalidKindReference)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid Role reference.
        // ------------------------------------------------------------

        {
            ScriptRecognitionDescription broken =
                dsl.description();

            ScriptRecognitionExpr* expr = nullptr;

            for (ScriptRecognitionExpr& candidate : broken.expressions)
            {
                if (candidate.kind == ScriptRecognitionExprKind::Capture)
                {
                    expr = &candidate;
                    break;
                }
            }

            if (!expr)
                return false;

            expr->roleId =
                static_cast<ScriptRoleId>(
                    broken.roles.size() + 1);

            validation =
                validateScriptRecognitionDescription(broken);

            if (validation.error !=
                ScriptRecognitionValidationError::InvalidRoleReference)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid expression arity.
        // ------------------------------------------------------------

        {
            ScriptRecognitionDescription broken =
                dsl.description();

            ScriptRecognitionExpr* expr = nullptr;

            for (ScriptRecognitionExpr& candidate : broken.expressions)
            {
                if (candidate.kind == ScriptRecognitionExprKind::Optional)
                {
                    expr = &candidate;
                    break;
                }
            }

            if (!expr)
                return false;

            expr->childCount = 0;

            validation =
                validateScriptRecognitionDescription(broken);

            if (validation.error !=
                ScriptRecognitionValidationError::InvalidExpressionArity)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Nullable repetition.
        //
        //     (C?)*
        // ------------------------------------------------------------

        {
            ScriptRecognitionDSL invalid;

            const ScriptItemKindRef kind =
                invalid.kind("C");

            const ScriptExprRef C =
                invalid.match(kind);

            const ScriptExprRef expression =
                invalid.zeroOrMore(
                    invalid.opt(C));

            if (!expression)
                return false;

            if (!invalid.unit(
                "BadUnit",
                invalid.seq({
                    C,
                    expression
                    })))
            {
                return false;
            }

            validation =
                validateScriptRecognitionDescription(
                    invalid.description());

            if (validation.error !=
                ScriptRecognitionValidationError::NullableRepetition)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Nullable capture.
        //
        //     capture(Role, C?)
        // ------------------------------------------------------------

        {
            ScriptRecognitionDSL invalid;

            const ScriptItemKindRef kind =
                invalid.kind("C");

            const ScriptRoleRef role =
                invalid.role("Role");

            const ScriptExprRef C =
                invalid.match(kind);

            const ScriptExprRef expression =
                invalid.capture(
                    role,
                    invalid.opt(C));

            if (!expression)
                return false;

            if (!invalid.unit(
                "BadUnit",
                invalid.seq({
                    C,
                    expression
                    })))
            {
                return false;
            }

            validation =
                validateScriptRecognitionDescription(
                    invalid.description());

            if (validation.error !=
                ScriptRecognitionValidationError::NullableCapture)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Nullable Unit.
        //
        //     C?
        // ------------------------------------------------------------

        {
            ScriptRecognitionDSL invalid;

            const ScriptItemKindRef kind =
                invalid.kind("C");

            const ScriptExprRef expression =
                invalid.opt(
                    invalid.match(kind));

            if (!expression)
                return false;

            if (!invalid.unit("BadUnit", expression))
                return false;

            validation =
                validateScriptRecognitionDescription(
                    invalid.description());

            if (validation.error !=
                ScriptRecognitionValidationError::NullableUnit)
            {
                return false;
            }
        }


        printf(
            "Script recognition validator: PASS\n"
            "  Valid description\n"
            "  Invalid kind reference\n"
            "  Invalid role reference\n"
            "  Invalid expression arity\n"
            "  Nullable repetition\n"
            "  Nullable capture\n"
            "  Nullable unit\n");

        return true;
    }


    static inline void testScriptRecognitionValidator()
    {
        runScriptRecognitionValidator();
    }

} // namespace waavs