// test_script_recognition_interpreter_control.h
#pragma once

#include "test_core.h"

#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"
#include "script_recognition_interpreter.h"

namespace waavs
{
    static inline bool runScriptRecognitionInterpreterControl()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef a = dsl.kind("A");
        const ScriptItemKindRef b = dsl.kind("B");
        const ScriptItemKindRef c = dsl.kind("C");
        const ScriptItemKindRef d = dsl.kind("D");
        const ScriptItemKindRef e = dsl.kind("E");

        if (!a || !b || !c || !d || !e)
            return false;

        const ScriptExprRef A = dsl.match(a);
        const ScriptExprRef B = dsl.match(b);
        const ScriptExprRef C = dsl.match(c);
        const ScriptExprRef D = dsl.match(d);
        const ScriptExprRef E = dsl.match(e);

        const ScriptExprRef expression =
            dsl.seq({
                dsl.choice({
                    A,
                    B,
                    C
                }),
                dsl.zeroOrMore(D),
                dsl.oneOrMore(E)
                });

        if (!expression)
            return false;

        if (!dsl.unit("TestUnit", expression))
            return false;


        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;


        // ------------------------------------------------------------
        // B D D D E E
        //
        // Expected:
        //   choice -> B
        //   D*     -> D D D
        //   E+     -> E E
        //
        // Whole input should become one recognized unit.
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> kinds =
        {
            b.id,
            d.id,
            d.id,
            d.id,
            e.id,
            e.id
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

        const ScriptRecognitionUnit& unit = result.units[0];

        if (unit.span.first != 0 || unit.span.count != 6)
            return false;

        if (unit.roleCount != 0)
            return false;


        // ------------------------------------------------------------
        // ZeroOrMore must accept zero repetitions.
        //
        // C E
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> zeroRepeat =
        {
            c.id,
            e.id
        };

        ScriptRecognitionResult zeroRepeatResult;

        if (!recognizeScriptKinds(ir, zeroRepeat, zeroRepeatResult))
            return false;

        if (zeroRepeatResult.units.size() != 1)
            return false;

        if (zeroRepeatResult.units[0].span.first != 0 ||
            zeroRepeatResult.units[0].span.count != 2)
        {
            return false;
        }


        // ------------------------------------------------------------
        // OneOrMore must require at least one E.
        //
        // A D D
        //
        // No complete unit should be recognized.
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> missingRequired =
        {
            a.id,
            d.id,
            d.id
        };

        ScriptRecognitionResult missingRequiredResult;

        if (!recognizeScriptKinds(ir, missingRequired, missingRequiredResult))
            return false;

        if (!missingRequiredResult.units.empty())
            return false;


        // ------------------------------------------------------------
        // Choice alternatives B and C must both work.
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> choiceC =
        {
            c.id,
            d.id,
            e.id
        };

        ScriptRecognitionResult choiceCResult;

        if (!recognizeScriptKinds(ir, choiceC, choiceCResult))
            return false;

        if (choiceCResult.units.size() != 1)
            return false;

        if (choiceCResult.units[0].span.first != 0 ||
            choiceCResult.units[0].span.count != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Multiple units should be recognized sequentially.
        //
        // B E | A D E E
        // ------------------------------------------------------------

        const std::vector<ScriptItemKindId> multiple =
        {
            b.id,
            e.id,

            a.id,
            d.id,
            e.id,
            e.id
        };

        ScriptRecognitionResult multipleResult;

        if (!recognizeScriptKinds(ir, multiple, multipleResult))
            return false;

        if (multipleResult.units.size() != 2)
            return false;

        if (multipleResult.units[0].span.first != 0 ||
            multipleResult.units[0].span.count != 2)
        {
            return false;
        }

        if (multipleResult.units[1].span.first != 2 ||
            multipleResult.units[1].span.count != 4)
        {
            return false;
        }


        printf(
            "Script recognition interpreter control: PASS\n"
            "  Choice: PASS\n"
            "  ZeroOrMore: PASS\n"
            "  OneOrMore: PASS\n"
            "  Multiple units: PASS\n");

        return true;
    }


    static inline void testScriptRecognitionInterpreterControl()
    {
        runScriptRecognitionInterpreterControl();
    }

} // namespace waavs