// test_script_recognition_compiler_control.h
#pragma once

#include "test_core.h"

#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"

namespace waavs
{
    static inline bool runScriptRecognitionCompilerControl()
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
        // Expected:
        //
        //  0 BeginSequence          end=15
        //
        //  1   BeginChoice          end=7
        //  2     MatchKind A
        //  3   NextChoice
        //  4     MatchKind B
        //  5   NextChoice
        //  6     MatchKind C
        //  7   EndChoice
        //
        //  8   BeginZeroOrMore      end=10
        //  9     MatchKind D
        // 10   EndZeroOrMore
        //
        // 11   BeginOneOrMore       end=13
        // 12     MatchKind E
        // 13   EndOneOrMore
        //
        // 14? No. EndSequence is 14.
        // 15? AcceptUnit is 15.
        //
        // Exact stream:
        //
        //  0 BeginSequence
        //  1 BeginChoice
        //  2 MatchKind A
        //  3 NextChoice
        //  4 MatchKind B
        //  5 NextChoice
        //  6 MatchKind C
        //  7 EndChoice
        //  8 BeginZeroOrMore
        //  9 MatchKind D
        // 10 EndZeroOrMore
        // 11 BeginOneOrMore
        // 12 MatchKind E
        // 13 EndOneOrMore
        // 14 EndSequence
        // 15 AcceptUnit
        // ------------------------------------------------------------

        if (ir.size() != 16)
            return false;


        const ScriptRecognitionIRInstruction* i0 = ir.instruction(0);
        const ScriptRecognitionIRInstruction* i1 = ir.instruction(1);
        const ScriptRecognitionIRInstruction* i2 = ir.instruction(2);
        const ScriptRecognitionIRInstruction* i3 = ir.instruction(3);
        const ScriptRecognitionIRInstruction* i4 = ir.instruction(4);
        const ScriptRecognitionIRInstruction* i5 = ir.instruction(5);
        const ScriptRecognitionIRInstruction* i6 = ir.instruction(6);
        const ScriptRecognitionIRInstruction* i7 = ir.instruction(7);
        const ScriptRecognitionIRInstruction* i8 = ir.instruction(8);
        const ScriptRecognitionIRInstruction* i9 = ir.instruction(9);
        const ScriptRecognitionIRInstruction* i10 = ir.instruction(10);
        const ScriptRecognitionIRInstruction* i11 = ir.instruction(11);
        const ScriptRecognitionIRInstruction* i12 = ir.instruction(12);
        const ScriptRecognitionIRInstruction* i13 = ir.instruction(13);
        const ScriptRecognitionIRInstruction* i14 = ir.instruction(14);
        const ScriptRecognitionIRInstruction* i15 = ir.instruction(15);

        if (!i0 || !i1 || !i2 || !i3 || !i4 || !i5 || !i6 || !i7 ||
            !i8 || !i9 || !i10 || !i11 || !i12 || !i13 || !i14 || !i15)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Sequence.
        // ------------------------------------------------------------

        if (i0->op != ScriptRecognitionIROp::BeginSequence)
            return false;

        if (i0->endIndex != 14)
            return false;


        // ------------------------------------------------------------
        // Choice.
        // ------------------------------------------------------------

        if (i1->op != ScriptRecognitionIROp::BeginChoice)
            return false;

        if (i1->endIndex != 7)
            return false;

        if (i2->op != ScriptRecognitionIROp::MatchKind ||
            i2->kindId != a.id)
        {
            return false;
        }

        if (i3->op != ScriptRecognitionIROp::NextChoice)
            return false;

        if (i4->op != ScriptRecognitionIROp::MatchKind ||
            i4->kindId != b.id)
        {
            return false;
        }

        if (i5->op != ScriptRecognitionIROp::NextChoice)
            return false;

        if (i6->op != ScriptRecognitionIROp::MatchKind ||
            i6->kindId != c.id)
        {
            return false;
        }

        if (i7->op != ScriptRecognitionIROp::EndChoice)
            return false;


        // ------------------------------------------------------------
        // ZeroOrMore.
        // ------------------------------------------------------------

        if (i8->op != ScriptRecognitionIROp::BeginZeroOrMore)
            return false;

        if (i8->endIndex != 10)
            return false;

        if (i9->op != ScriptRecognitionIROp::MatchKind ||
            i9->kindId != d.id)
        {
            return false;
        }

        if (i10->op != ScriptRecognitionIROp::EndZeroOrMore)
            return false;


        // ------------------------------------------------------------
        // OneOrMore.
        // ------------------------------------------------------------

        if (i11->op != ScriptRecognitionIROp::BeginOneOrMore)
            return false;

        if (i11->endIndex != 13)
            return false;

        if (i12->op != ScriptRecognitionIROp::MatchKind ||
            i12->kindId != e.id)
        {
            return false;
        }

        if (i13->op != ScriptRecognitionIROp::EndOneOrMore)
            return false;


        // ------------------------------------------------------------
        // Unit termination.
        // ------------------------------------------------------------

        if (i14->op != ScriptRecognitionIROp::EndSequence)
            return false;

        if (i15->op != ScriptRecognitionIROp::AcceptUnit)
            return false;

        if (i15->unitTypeId == kScriptUnitTypeInvalid)
            return false;


        printf(
            "Script recognition compiler control: PASS\n"
            "  Instructions: %zu\n"
            "  Choice alternatives: 3\n"
            "  ZeroOrMore: 1\n"
            "  OneOrMore: 1\n",
            ir.size());

        return true;
    }


    static inline void testScriptRecognitionCompilerControl()
    {
        runScriptRecognitionCompilerControl();
    }

} // namespace waavs