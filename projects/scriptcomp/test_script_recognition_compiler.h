// test_script_recognition_compiler.h
#pragma once

#include "test_core.h"

#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"

namespace waavs
{
    static inline bool runScriptRecognitionCompiler()
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

        if (!expression)
            return false;

        if (!dsl.unit("ConsonantSyllable", expression))
            return false;


        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;


        // ------------------------------------------------------------
        // Expected:
        //
        //  0 BeginSequence        end=10
        //  1   MatchKind C
        //  2   MatchKind H
        //  3   BeginCapture Base  end=5
        //  4     MatchKind C
        //  5   EndCapture
        //  6   BeginOptional      end=8
        //  7     MatchKind M
        //  8   EndOptional
        //  9?  <- verify carefully
        //
        // Let's use actual count derived below:
        //
        //  0 BeginSequence
        //  1 MatchKind C
        //  2 MatchKind H
        //  3 BeginCapture
        //  4 MatchKind C
        //  5 EndCapture
        //  6 BeginOptional
        //  7 MatchKind M
        //  8 EndOptional
        //  9 EndSequence
        // 10 AcceptUnit
        // ------------------------------------------------------------

        if (ir.size() != 11)
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

        if (!i0 || !i1 || !i2 || !i3 || !i4 || !i5 ||
            !i6 || !i7 || !i8 || !i9 || !i10)
        {
            return false;
        }


        if (i0->op != ScriptRecognitionIROp::BeginSequence)
            return false;

        if (i0->endIndex != 9)
            return false;


        if (i1->op != ScriptRecognitionIROp::MatchKind ||
            i1->kindId != consonant.id)
        {
            return false;
        }


        if (i2->op != ScriptRecognitionIROp::MatchKind ||
            i2->kindId != halant.id)
        {
            return false;
        }


        if (i3->op != ScriptRecognitionIROp::BeginCapture ||
            i3->roleId != base.id ||
            i3->endIndex != 5)
        {
            return false;
        }


        if (i4->op != ScriptRecognitionIROp::MatchKind ||
            i4->kindId != consonant.id)
        {
            return false;
        }


        if (i5->op != ScriptRecognitionIROp::EndCapture)
            return false;


        if (i6->op != ScriptRecognitionIROp::BeginOptional ||
            i6->endIndex != 8)
        {
            return false;
        }


        if (i7->op != ScriptRecognitionIROp::MatchKind ||
            i7->kindId != matraPre.id)
        {
            return false;
        }


        if (i8->op != ScriptRecognitionIROp::EndOptional)
            return false;


        if (i9->op != ScriptRecognitionIROp::EndSequence)
            return false;


        if (i10->op != ScriptRecognitionIROp::AcceptUnit)
            return false;

        if (i10->unitTypeId == kScriptUnitTypeInvalid)
            return false;


        // ------------------------------------------------------------
        // Compilation must be transactional.
        // ------------------------------------------------------------

        ScriptRecognitionDescription broken =
            dsl.description();

        broken.units[0].expression =
            kScriptRecognitionExprInvalid;

        const size_t previousSize = ir.size();

        if (compileScriptRecognitionIR(broken, ir))
            return false;

        if (ir.size() != previousSize)
            return false;


        printf(
            "Script recognition compiler: PASS\n"
            "  Instructions: %zu\n",
            ir.size());

        return true;
    }


    static inline void testScriptRecognitionCompiler()
    {
        runScriptRecognitionCompiler();
    }

} // namespace waavs