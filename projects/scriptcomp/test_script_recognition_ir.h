// test_script_recognition_ir.h
#pragma once

#include "test_core.h"

#include "script_recognition_ir.h"

namespace waavs
{
    static inline bool runScriptRecognitionIR()
    {
        ScriptRecognitionIR ir;

        if (!ir.empty())
            return false;

        if (ir.size() != 0)
            return false;

        if (ir.instruction(0) != nullptr)
            return false;


        ScriptRecognitionIRInstruction match{};
        match.op = ScriptRecognitionIROp::MatchKind;
        match.kindId = 3;

        ScriptRecognitionIRInstruction capture{};
        capture.op = ScriptRecognitionIROp::BeginCapture;
        capture.roleId = 2;
        capture.endIndex = 2;

        ScriptRecognitionIRInstruction endCapture{};
        endCapture.op = ScriptRecognitionIROp::EndCapture;

        ScriptRecognitionIRInstruction accept{};
        accept.op = ScriptRecognitionIROp::AcceptUnit;
        accept.unitTypeId = 4;

        ir.instructions.push_back(match);
        ir.instructions.push_back(capture);
        ir.instructions.push_back(endCapture);
        ir.instructions.push_back(accept);


        if (ir.empty())
            return false;

        if (ir.size() != 4)
            return false;


        const ScriptRecognitionIRInstruction* instruction0 =
            ir.instruction(0);

        if (!instruction0)
            return false;

        if (instruction0->op != ScriptRecognitionIROp::MatchKind)
            return false;

        if (instruction0->kindId != 3)
            return false;


        const ScriptRecognitionIRInstruction* instruction1 =
            ir.instruction(1);

        if (!instruction1)
            return false;

        if (instruction1->op != ScriptRecognitionIROp::BeginCapture)
            return false;

        if (instruction1->roleId != 2)
            return false;

        if (instruction1->endIndex != 2)
            return false;


        const ScriptRecognitionIRInstruction* instruction3 =
            ir.instruction(3);

        if (!instruction3)
            return false;

        if (instruction3->op != ScriptRecognitionIROp::AcceptUnit)
            return false;

        if (instruction3->unitTypeId != 4)
            return false;


        if (ir.instruction(4) != nullptr)
            return false;


        ScriptRecognitionIRInstruction* mutableInstruction =
            ir.instruction(0);

        if (!mutableInstruction)
            return false;

        mutableInstruction->kindId = 7;

        if (ir.instruction(0)->kindId != 7)
            return false;


        ir.clear();

        if (!ir.empty())
            return false;

        if (ir.size() != 0)
            return false;

        return true;
    }


    static inline void testScriptRecognitionIR()
    {
        const bool passed = runScriptRecognitionIR();

        printf(
            "Script recognition IR: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs