// test_script_recognition_interpreter_backtracking.h
#pragma once

#include "test_core.h"

#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"
#include "script_recognition_interpreter.h"

namespace waavs
{
    static inline bool runScriptRecognitionInterpreterBacktracking()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef a = dsl.kind("A");
        const ScriptItemKindRef b = dsl.kind("B");
        const ScriptItemKindRef c = dsl.kind("C");

        if (!a || !b || !c)
            return false;

        const ScriptExprRef A = dsl.match(a);
        const ScriptExprRef B = dsl.match(b);
        const ScriptExprRef C = dsl.match(c);

        // ------------------------------------------------------------
        // Grammar:
        //
        // seq(
        //     choice(
        //         seq(A, B),
        //         A
        //     ),
        //     C
        // )
        //
        // Input:
        //
        //     A C
        //
        // The first alternative consumes A, then fails on B.
        // Recognition must backtrack and try the second alternative A,
        // allowing the following C to complete the unit.
        // ------------------------------------------------------------

        const ScriptExprRef expression =
            dsl.seq({
                dsl.choice({
                    dsl.seq({
                        A,
                        B
                    }),
                    A
                }),
                C
                });

        if (!expression)
            return false;

        if (!dsl.unit("BacktrackingUnit", expression))
            return false;


        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;


        // ------------------------------------------------------------
        // Backtracking-required case.
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> kinds =
        {
            a.id,
            c.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.kinds != kinds)
            return false;

        if (result.units.size() != 1)
            return false;

        if (!result.roles.empty())
            return false;

        if (result.units[0].span.first != 0 ||
            result.units[0].span.count != 2)
        {
            return false;
        }


        // ------------------------------------------------------------
        // First alternative succeeds normally.
        //
        // A B C
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> firstAlternative =
        {
            a.id,
            b.id,
            c.id
        };

        ScriptRecognitionResult firstAlternativeResult;

        if (!recognizeScriptKinds(ir, firstAlternative, firstAlternativeResult))
            return false;

        if (firstAlternativeResult.units.size() != 1)
            return false;

        if (firstAlternativeResult.units[0].span.first != 0 ||
            firstAlternativeResult.units[0].span.count != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Neither alternative can complete the outer sequence.
        //
        // A B
        //
        // seq(A,B) succeeds, but the trailing C is missing.
        // The A alternative also cannot make the trailing B satisfy C.
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> incomplete =
        {
            a.id,
            b.id
        };

        ScriptRecognitionResult incompleteResult;

        if (!recognizeScriptKinds(ir, incomplete, incompleteResult))
            return false;

        if (!incompleteResult.units.empty())
            return false;


        // ------------------------------------------------------------
        // No choice alternative matches.
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> noChoice =
        {
            c.id
        };

        ScriptRecognitionResult noChoiceResult;

        if (!recognizeScriptKinds(ir, noChoice, noChoiceResult))
            return false;

        if (!noChoiceResult.units.empty())
            return false;


        printf(
            "Script recognition interpreter backtracking: PASS\n"
            "  Later-context backtracking: PASS\n"
            "  First alternative: PASS\n"
            "  Incomplete sequence: PASS\n"
            "  No matching alternative: PASS\n");

        return true;
    }


    static inline void testScriptRecognitionInterpreterBacktracking()
    {
        runScriptRecognitionInterpreterBacktracking();
    }

} // namespace waavs