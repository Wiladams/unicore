// test_script_recognition_ir_types.h
#pragma once

#include "test_core.h"

#include "script_recognition_ir_types.h"

namespace waavs
{
    static inline bool runScriptRecognitionIRTypes()
    {
        ScriptRecognitionIRInstruction instruction{};

        if (instruction.op != ScriptRecognitionIROp::Invalid)
            return false;

        if (instruction.kindId != kScriptItemKindInvalid)
            return false;

        if (instruction.roleId != kScriptRoleInvalid)
            return false;

        if (instruction.unitTypeId != kScriptUnitTypeInvalid)
            return false;

        if (instruction.endIndex != kScriptRecognitionIRInvalid)
            return false;


        if (!isScriptRecognitionIRMatch(ScriptRecognitionIROp::MatchKind))
            return false;

        if (isScriptRecognitionIRMatch(ScriptRecognitionIROp::BeginSequence))
            return false;


        if (!isScriptRecognitionIRBegin(ScriptRecognitionIROp::BeginSequence))
            return false;

        if (!isScriptRecognitionIRBegin(ScriptRecognitionIROp::BeginChoice))
            return false;

        if (!isScriptRecognitionIRBegin(ScriptRecognitionIROp::BeginOptional))
            return false;

        if (!isScriptRecognitionIRBegin(ScriptRecognitionIROp::BeginZeroOrMore))
            return false;

        if (!isScriptRecognitionIRBegin(ScriptRecognitionIROp::BeginOneOrMore))
            return false;

        if (!isScriptRecognitionIRBegin(ScriptRecognitionIROp::BeginCapture))
            return false;

        if (isScriptRecognitionIRBegin(ScriptRecognitionIROp::MatchKind))
            return false;


        if (!isScriptRecognitionIREnd(ScriptRecognitionIROp::EndSequence))
            return false;

        if (!isScriptRecognitionIREnd(ScriptRecognitionIROp::EndChoice))
            return false;

        if (!isScriptRecognitionIREnd(ScriptRecognitionIROp::EndOptional))
            return false;

        if (!isScriptRecognitionIREnd(ScriptRecognitionIROp::EndZeroOrMore))
            return false;

        if (!isScriptRecognitionIREnd(ScriptRecognitionIROp::EndOneOrMore))
            return false;

        if (!isScriptRecognitionIREnd(ScriptRecognitionIROp::EndCapture))
            return false;

        if (isScriptRecognitionIREnd(ScriptRecognitionIROp::AcceptUnit))
            return false;


        if (!isScriptRecognitionIRRepetition(ScriptRecognitionIROp::BeginZeroOrMore))
            return false;

        if (!isScriptRecognitionIRRepetition(ScriptRecognitionIROp::BeginOneOrMore))
            return false;

        if (isScriptRecognitionIRRepetition(ScriptRecognitionIROp::BeginOptional))
            return false;


        ScriptRecognitionIRInstruction match{};
        match.op = ScriptRecognitionIROp::MatchKind;
        match.kindId = 7;

        if (match.kindId != 7)
            return false;


        ScriptRecognitionIRInstruction capture{};
        capture.op = ScriptRecognitionIROp::BeginCapture;
        capture.roleId = 3;
        capture.endIndex = 9;

        if (capture.roleId != 3)
            return false;

        if (capture.endIndex != 9)
            return false;


        ScriptRecognitionIRInstruction accept{};
        accept.op = ScriptRecognitionIROp::AcceptUnit;
        accept.unitTypeId = 5;

        if (accept.unitTypeId != 5)
            return false;

        return true;
    }


    static inline void testScriptRecognitionIRTypes()
    {
        const bool passed = runScriptRecognitionIRTypes();

        printf(
            "Script recognition IR types: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs