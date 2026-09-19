// script_recognition_ir.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "script_recognition_ir_types.h"

namespace waavs
{
    struct ScriptRecognitionIR
    {
        std::vector<ScriptRecognitionIRInstruction> instructions{};

        void clear()
        {
            instructions.clear();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return instructions.empty();
        }

        [[nodiscard]]
        size_t size() const noexcept
        {
            return instructions.size();
        }

        [[nodiscard]]
        const ScriptRecognitionIRInstruction* instruction(
            ScriptRecognitionIRInstructionId id) const noexcept
        {
            return id < instructions.size()
                ? &instructions[id]
                : nullptr;
        }

        [[nodiscard]]
        ScriptRecognitionIRInstruction* instruction(
            ScriptRecognitionIRInstructionId id) noexcept
        {
            return id < instructions.size()
                ? &instructions[id]
                : nullptr;
        }
    };

} // namespace waavs