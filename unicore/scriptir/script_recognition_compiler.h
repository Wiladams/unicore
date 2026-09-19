// script_recognition_compiler.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "script_recognition_description.h"
#include "script_recognition_ir.h"
#include "script_recognition_validator.h"

namespace waavs
{
    static inline bool appendScriptRecognitionIRInstruction(
        ScriptRecognitionIR& ir,
        const ScriptRecognitionIRInstruction& instruction,
        ScriptRecognitionIRInstructionId& id)
    {
        if (ir.instructions.size() >= std::numeric_limits<uint32_t>::max())
            return false;

        id = static_cast<ScriptRecognitionIRInstructionId>(ir.instructions.size());
        ir.instructions.push_back(instruction);
        return true;
    }


    static inline bool compileScriptRecognitionExpression(
        const ScriptRecognitionDescription& description,
        ScriptRecognitionExprId exprId,
        ScriptRecognitionIR& ir)
    {
        const ScriptRecognitionExpr* expr = description.expression(exprId);

        if (!expr)
            return false;

        const ScriptRecognitionExprId* children = description.children(*expr);

        if (expr->childCount != 0 && !children)
            return false;


        // ------------------------------------------------------------
        // Kind
        // ------------------------------------------------------------

        if (expr->kind == ScriptRecognitionExprKind::Kind)
        {
            ScriptRecognitionIRInstruction instruction{};
            instruction.op = ScriptRecognitionIROp::MatchKind;
            instruction.kindId = expr->kindId;

            ScriptRecognitionIRInstructionId id;
            return appendScriptRecognitionIRInstruction(ir, instruction, id);
        }


        // ------------------------------------------------------------
        // Capture
        // ------------------------------------------------------------

        if (expr->kind == ScriptRecognitionExprKind::Capture)
        {
            if (expr->childCount != 1)
                return false;

            ScriptRecognitionIRInstruction begin{};
            begin.op = ScriptRecognitionIROp::BeginCapture;
            begin.roleId = expr->roleId;

            ScriptRecognitionIRInstructionId beginId;

            if (!appendScriptRecognitionIRInstruction(ir, begin, beginId))
                return false;

            if (!compileScriptRecognitionExpression(description, children[0], ir))
                return false;

            ScriptRecognitionIRInstruction end{};
            end.op = ScriptRecognitionIROp::EndCapture;

            ScriptRecognitionIRInstructionId endId;

            if (!appendScriptRecognitionIRInstruction(ir, end, endId))
                return false;

            ir.instructions[beginId].endIndex = endId;
            return true;
        }


        // ------------------------------------------------------------
        // Sequence
        // ------------------------------------------------------------

        if (expr->kind == ScriptRecognitionExprKind::Sequence)
        {
            if (expr->childCount == 0)
                return false;

            ScriptRecognitionIRInstruction begin{};
            begin.op = ScriptRecognitionIROp::BeginSequence;

            ScriptRecognitionIRInstructionId beginId;

            if (!appendScriptRecognitionIRInstruction(ir, begin, beginId))
                return false;

            for (uint32_t i = 0; i < expr->childCount; ++i)
            {
                if (!compileScriptRecognitionExpression(description, children[i], ir))
                    return false;
            }

            ScriptRecognitionIRInstruction end{};
            end.op = ScriptRecognitionIROp::EndSequence;

            ScriptRecognitionIRInstructionId endId;

            if (!appendScriptRecognitionIRInstruction(ir, end, endId))
                return false;

            ir.instructions[beginId].endIndex = endId;
            return true;
        }


        // ------------------------------------------------------------
        // Choice
        // ------------------------------------------------------------

        if (expr->kind == ScriptRecognitionExprKind::Choice)
        {
            if (expr->childCount == 0)
                return false;

            ScriptRecognitionIRInstruction begin{};
            begin.op = ScriptRecognitionIROp::BeginChoice;

            ScriptRecognitionIRInstructionId beginId;

            if (!appendScriptRecognitionIRInstruction(ir, begin, beginId))
                return false;

            for (uint32_t i = 0; i < expr->childCount; ++i)
            {
                if (i != 0)
                {
                    ScriptRecognitionIRInstruction next{};
                    next.op = ScriptRecognitionIROp::NextChoice;

                    ScriptRecognitionIRInstructionId nextId;

                    if (!appendScriptRecognitionIRInstruction(ir, next, nextId))
                        return false;
                }

                if (!compileScriptRecognitionExpression(description, children[i], ir))
                    return false;
            }

            ScriptRecognitionIRInstruction end{};
            end.op = ScriptRecognitionIROp::EndChoice;

            ScriptRecognitionIRInstructionId endId;

            if (!appendScriptRecognitionIRInstruction(ir, end, endId))
                return false;

            ir.instructions[beginId].endIndex = endId;
            return true;
        }


        // ------------------------------------------------------------
        // Optional / repetition.
        // ------------------------------------------------------------

        ScriptRecognitionIROp beginOp = ScriptRecognitionIROp::Invalid;
        ScriptRecognitionIROp endOp = ScriptRecognitionIROp::Invalid;

        switch (expr->kind)
        {
        case ScriptRecognitionExprKind::Optional:
            beginOp = ScriptRecognitionIROp::BeginOptional;
            endOp = ScriptRecognitionIROp::EndOptional;
            break;

        case ScriptRecognitionExprKind::ZeroOrMore:
            beginOp = ScriptRecognitionIROp::BeginZeroOrMore;
            endOp = ScriptRecognitionIROp::EndZeroOrMore;
            break;

        case ScriptRecognitionExprKind::OneOrMore:
            beginOp = ScriptRecognitionIROp::BeginOneOrMore;
            endOp = ScriptRecognitionIROp::EndOneOrMore;
            break;

        default:
            return false;
        }

        if (expr->childCount != 1)
            return false;

        ScriptRecognitionIRInstruction begin{};
        begin.op = beginOp;

        ScriptRecognitionIRInstructionId beginId;

        if (!appendScriptRecognitionIRInstruction(ir, begin, beginId))
            return false;

        if (!compileScriptRecognitionExpression(description, children[0], ir))
            return false;

        ScriptRecognitionIRInstruction end{};
        end.op = endOp;

        ScriptRecognitionIRInstructionId endId;

        if (!appendScriptRecognitionIRInstruction(ir, end, endId))
            return false;

        ir.instructions[beginId].endIndex = endId;
        return true;
    }


    [[nodiscard]]
    static inline bool compileScriptRecognitionIR(
        const ScriptRecognitionDescription& description,
        ScriptRecognitionIR& result)
    {
        const ScriptRecognitionValidationResult validation =
            validateScriptRecognitionDescription(description);

        if (!validation)
            return false;


        // ------------------------------------------------------------
        // Build transactionally so failure never partially modifies result.
        // ------------------------------------------------------------

        ScriptRecognitionIR working;


        for (const ScriptRecognitionUnitDescription& unit : description.units)
        {
            if (!compileScriptRecognitionExpression(
                description,
                unit.expression,
                working))
            {
                return false;
            }

            ScriptRecognitionIRInstruction accept{};
            accept.op = ScriptRecognitionIROp::AcceptUnit;
            accept.unitTypeId = unit.type;

            ScriptRecognitionIRInstructionId acceptId;

            if (!appendScriptRecognitionIRInstruction(
                working,
                accept,
                acceptId))
            {
                return false;
            }
        }


        result = std::move(working);
        return true;
    }

} // namespace waavs