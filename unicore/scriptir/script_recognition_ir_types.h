// script_recognition_ir_types.h
#pragma once

#include <cstdint>

#include "script_recognition_types.h"

namespace waavs
{
    using ScriptRecognitionIRInstructionId = uint32_t;

    static constexpr ScriptRecognitionIRInstructionId kScriptRecognitionIRInvalid = 0xFFFFFFFFu;


    enum class ScriptRecognitionIROp : uint8_t
    {
        Invalid = 0,

        MatchKind,

        BeginSequence,
        EndSequence,

        BeginChoice,
        NextChoice,
        EndChoice,

        BeginOptional,
        EndOptional,

        BeginZeroOrMore,
        EndZeroOrMore,

        BeginOneOrMore,
        EndOneOrMore,

        BeginCapture,
        EndCapture,

        AcceptUnit
    };


    struct ScriptRecognitionIRInstruction
    {
        ScriptRecognitionIROp op{ ScriptRecognitionIROp::Invalid };

        ScriptItemKindId kindId{ kScriptItemKindInvalid };
        ScriptRoleId roleId{ kScriptRoleInvalid };
        ScriptUnitTypeId unitTypeId{ kScriptUnitTypeInvalid };

        uint32_t endIndex{ kScriptRecognitionIRInvalid };
    };


    [[nodiscard]]
    static constexpr bool isScriptRecognitionIRMatch(ScriptRecognitionIROp op) noexcept
    {
        return op == ScriptRecognitionIROp::MatchKind;
    }


    [[nodiscard]]
    static constexpr bool isScriptRecognitionIRBegin(ScriptRecognitionIROp op) noexcept
    {
        switch (op)
        {
        case ScriptRecognitionIROp::BeginSequence:
        case ScriptRecognitionIROp::BeginChoice:
        case ScriptRecognitionIROp::BeginOptional:
        case ScriptRecognitionIROp::BeginZeroOrMore:
        case ScriptRecognitionIROp::BeginOneOrMore:
        case ScriptRecognitionIROp::BeginCapture:
            return true;

        default:
            return false;
        }
    }


    [[nodiscard]]
    static constexpr bool isScriptRecognitionIREnd(ScriptRecognitionIROp op) noexcept
    {
        switch (op)
        {
        case ScriptRecognitionIROp::EndSequence:
        case ScriptRecognitionIROp::EndChoice:
        case ScriptRecognitionIROp::EndOptional:
        case ScriptRecognitionIROp::EndZeroOrMore:
        case ScriptRecognitionIROp::EndOneOrMore:
        case ScriptRecognitionIROp::EndCapture:
            return true;

        default:
            return false;
        }
    }


    [[nodiscard]]
    static constexpr bool isScriptRecognitionIRRepetition(ScriptRecognitionIROp op) noexcept
    {
        return op == ScriptRecognitionIROp::BeginZeroOrMore ||
            op == ScriptRecognitionIROp::BeginOneOrMore;
    }

} // namespace waavs