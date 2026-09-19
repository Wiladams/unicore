// script_recognition_interpreter.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "script_recognition_ir.h"
#include "script_recognition_result.h"

namespace waavs
{
    struct ScriptRecognitionMatchState
    {
        uint32_t inputIndex{ 0 };
        std::vector<ScriptRoleBinding> roles{};
    };


    static inline bool matchScriptRecognitionIRRange(
        const ScriptRecognitionIR& ir,
        ScriptRecognitionIRInstructionId first,
        ScriptRecognitionIRInstructionId end,
        const ScriptItemKindId* kinds,
        uint32_t kindCount,
        const ScriptRecognitionMatchState& initial,
        std::vector<ScriptRecognitionMatchState>& results);


    static inline bool scriptRecognitionChoiceRanges(
        const ScriptRecognitionIR& ir,
        ScriptRecognitionIRInstructionId first,
        ScriptRecognitionIRInstructionId end,
        std::vector<ScriptSpan>& ranges)
    {
        ranges.clear();

        ScriptRecognitionIRInstructionId rangeFirst = first;
        ScriptRecognitionIRInstructionId pc = first;

        while (pc < end)
        {
            const ScriptRecognitionIRInstruction* instruction = ir.instruction(pc);

            if (!instruction)
                return false;

            if (instruction->op == ScriptRecognitionIROp::NextChoice)
            {
                ranges.push_back({ rangeFirst, pc - rangeFirst });
                rangeFirst = pc + 1;
                ++pc;
                continue;
            }

            if (isScriptRecognitionIRBegin(instruction->op))
            {
                if (instruction->endIndex == kScriptRecognitionIRInvalid ||
                    instruction->endIndex >= end ||
                    instruction->endIndex < pc)
                {
                    return false;
                }

                pc = instruction->endIndex + 1;
                continue;
            }

            ++pc;
        }

        ranges.push_back({ rangeFirst, end - rangeFirst });
        return true;
    }


    static inline bool matchScriptRecognitionIRRepeat(
        const ScriptRecognitionIR& ir,
        ScriptRecognitionIRInstructionId first,
        ScriptRecognitionIRInstructionId end,
        const ScriptItemKindId* kinds,
        uint32_t kindCount,
        const ScriptRecognitionMatchState& state,
        bool allowZero,
        std::vector<ScriptRecognitionMatchState>& results)
    {
        std::vector<ScriptRecognitionMatchState> next;

        if (!matchScriptRecognitionIRRange(ir, first, end, kinds, kindCount, state, next))
            return false;

        // Greedy ordering: deeper repetitions are emitted before the current state.
        for (const ScriptRecognitionMatchState& matched : next)
        {
            if (matched.inputIndex == state.inputIndex)
                return false;

            if (!matchScriptRecognitionIRRepeat(ir, first, end, kinds, kindCount, matched, true, results))
                return false;
        }

        if (allowZero)
            results.push_back(state);

        return true;
    }


    static inline bool matchScriptRecognitionIRConstruct(
        const ScriptRecognitionIR& ir,
        ScriptRecognitionIRInstructionId pc,
        ScriptRecognitionIRInstructionId rangeEnd,
        const ScriptItemKindId* kinds,
        uint32_t kindCount,
        const ScriptRecognitionMatchState& state,
        ScriptRecognitionIRInstructionId& nextPC,
        std::vector<ScriptRecognitionMatchState>& results)
    {
        results.clear();

        const ScriptRecognitionIRInstruction* instruction = ir.instruction(pc);

        if (!instruction)
            return false;


        // ------------------------------------------------------------
        // MatchKind
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::MatchKind)
        {
            nextPC = pc + 1;

            if (state.inputIndex >= kindCount)
                return true;

            if (kinds[state.inputIndex] != instruction->kindId)
                return true;

            ScriptRecognitionMatchState matched = state;
            ++matched.inputIndex;
            results.push_back(std::move(matched));
            return true;
        }


        // ------------------------------------------------------------
        // Structured constructs.
        // ------------------------------------------------------------

        if (!isScriptRecognitionIRBegin(instruction->op))
            return false;

        if (instruction->endIndex == kScriptRecognitionIRInvalid ||
            instruction->endIndex >= rangeEnd ||
            instruction->endIndex <= pc)
        {
            return false;
        }

        const ScriptRecognitionIRInstructionId childFirst = pc + 1;
        const ScriptRecognitionIRInstructionId childEnd = instruction->endIndex;
        nextPC = instruction->endIndex + 1;


        // ------------------------------------------------------------
        // Sequence
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::BeginSequence)
            return matchScriptRecognitionIRRange(ir, childFirst, childEnd, kinds, kindCount, state, results);


        // ------------------------------------------------------------
        // Choice
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::BeginChoice)
        {
            std::vector<ScriptSpan> ranges;

            if (!scriptRecognitionChoiceRanges(ir, childFirst, childEnd, ranges))
                return false;

            for (const ScriptSpan& range : ranges)
            {
                std::vector<ScriptRecognitionMatchState> branch;

                if (!matchScriptRecognitionIRRange(
                    ir,
                    range.first,
                    range.end(),
                    kinds,
                    kindCount,
                    state,
                    branch))
                {
                    return false;
                }

                results.insert(
                    results.end(),
                    std::make_move_iterator(branch.begin()),
                    std::make_move_iterator(branch.end()));
            }

            return true;
        }


        // ------------------------------------------------------------
        // Optional
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::BeginOptional)
        {
            if (!matchScriptRecognitionIRRange(ir, childFirst, childEnd, kinds, kindCount, state, results))
                return false;

            // Greedy: matched form precedes the empty form.
            results.push_back(state);
            return true;
        }


        // ------------------------------------------------------------
        // ZeroOrMore
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::BeginZeroOrMore)
            return matchScriptRecognitionIRRepeat(ir, childFirst, childEnd, kinds, kindCount, state, true, results);


        // ------------------------------------------------------------
        // OneOrMore
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::BeginOneOrMore)
        {
            std::vector<ScriptRecognitionMatchState> firstMatches;

            if (!matchScriptRecognitionIRRange(ir, childFirst, childEnd, kinds, kindCount, state, firstMatches))
                return false;

            for (const ScriptRecognitionMatchState& matched : firstMatches)
            {
                if (matched.inputIndex == state.inputIndex)
                    return false;

                if (!matchScriptRecognitionIRRepeat(ir, childFirst, childEnd, kinds, kindCount, matched, true, results))
                    return false;
            }

            return true;
        }


        // ------------------------------------------------------------
        // Capture
        // ------------------------------------------------------------

        if (instruction->op == ScriptRecognitionIROp::BeginCapture)
        {
            ScriptRecognitionMatchState captured = state;

            const size_t roleIndex = captured.roles.size();

            ScriptRoleBinding binding{};
            binding.role = instruction->roleId;
            binding.span.first = state.inputIndex;
            captured.roles.push_back(binding);

            std::vector<ScriptRecognitionMatchState> matches;

            if (!matchScriptRecognitionIRRange(ir, childFirst, childEnd, kinds, kindCount, captured, matches))
                return false;

            for (ScriptRecognitionMatchState& matched : matches)
            {
                if (roleIndex >= matched.roles.size())
                    return false;

                matched.roles[roleIndex].span.count = matched.inputIndex - state.inputIndex;
                results.push_back(std::move(matched));
            }

            return true;
        }


        return false;
    }


    static inline bool matchScriptRecognitionIRRange(
        const ScriptRecognitionIR& ir,
        ScriptRecognitionIRInstructionId first,
        ScriptRecognitionIRInstructionId end,
        const ScriptItemKindId* kinds,
        uint32_t kindCount,
        const ScriptRecognitionMatchState& initial,
        std::vector<ScriptRecognitionMatchState>& results)
    {
        results.clear();

        std::vector<ScriptRecognitionMatchState> current;
        current.push_back(initial);

        ScriptRecognitionIRInstructionId pc = first;

        while (pc < end)
        {
            std::vector<ScriptRecognitionMatchState> next;
            ScriptRecognitionIRInstructionId nextPC = pc + 1;

            for (const ScriptRecognitionMatchState& state : current)
            {
                std::vector<ScriptRecognitionMatchState> matched;
                ScriptRecognitionIRInstructionId stateNextPC = pc + 1;

                if (!matchScriptRecognitionIRConstruct(
                    ir,
                    pc,
                    end,
                    kinds,
                    kindCount,
                    state,
                    stateNextPC,
                    matched))
                {
                    return false;
                }

                if (stateNextPC != nextPC && next.empty())
                    nextPC = stateNextPC;
                else if (stateNextPC != nextPC)
                    return false;

                next.insert(
                    next.end(),
                    std::make_move_iterator(matched.begin()),
                    std::make_move_iterator(matched.end()));
            }

            if (next.empty())
            {
                results.clear();
                return true;
            }

            current = std::move(next);
            pc = nextPC;
        }

        results = std::move(current);
        return true;
    }


    static inline bool matchScriptRecognitionIRUnit(
        const ScriptRecognitionIR& ir,
        ScriptRecognitionIRInstructionId first,
        ScriptRecognitionIRInstructionId acceptIndex,
        const ScriptItemKindId* kinds,
        uint32_t kindCount,
        uint32_t inputIndex,
        ScriptRecognitionMatchState& result)
    {
        const ScriptRecognitionIRInstruction* accept = ir.instruction(acceptIndex);

        if (!accept || accept->op != ScriptRecognitionIROp::AcceptUnit)
            return false;

        ScriptRecognitionMatchState initial{};
        initial.inputIndex = inputIndex;

        std::vector<ScriptRecognitionMatchState> matches;

        if (!matchScriptRecognitionIRRange(ir, first, acceptIndex, kinds, kindCount, initial, matches))
            return false;

        if (matches.empty())
            return false;

        // Alternatives are ordered left-to-right and repetition is greedy.
        result = std::move(matches.front());
        return true;
    }


    [[nodiscard]]
    static inline bool recognizeScriptKinds(
        const ScriptRecognitionIR& ir,
        const ScriptItemKindId* kinds,
        uint32_t kindCount,
        ScriptRecognitionResult& result)
    {
        ScriptRecognitionResult working;

        if (kindCount != 0 && !kinds)
            return false;

        working.kinds.assign(kinds, kinds + kindCount);

        std::vector<ScriptRecognitionIRInstructionId> unitStarts;
        std::vector<ScriptRecognitionIRInstructionId> unitAccepts;

        ScriptRecognitionIRInstructionId start = 0;

        for (ScriptRecognitionIRInstructionId i = 0; i < ir.size(); ++i)
        {
            const ScriptRecognitionIRInstruction* instruction = ir.instruction(i);

            if (!instruction)
                return false;

            if (instruction->op != ScriptRecognitionIROp::AcceptUnit)
                continue;

            unitStarts.push_back(start);
            unitAccepts.push_back(i);
            start = i + 1;
        }

        if (unitStarts.empty())
        {
            result = std::move(working);
            return true;
        }


        uint32_t inputIndex = 0;

        while (inputIndex < kindCount)
        {
            bool recognized = false;

            for (size_t unitIndex = 0; unitIndex < unitStarts.size(); ++unitIndex)
            {
                ScriptRecognitionMatchState match;

                if (!matchScriptRecognitionIRUnit(
                    ir,
                    unitStarts[unitIndex],
                    unitAccepts[unitIndex],
                    kinds,
                    kindCount,
                    inputIndex,
                    match))
                {
                    continue;
                }

                if (match.inputIndex <= inputIndex)
                    return false;

                const ScriptRecognitionIRInstruction* accept = ir.instruction(unitAccepts[unitIndex]);

                if (!accept)
                    return false;

                if (working.roles.size() > std::numeric_limits<uint32_t>::max())
                    return false;

                if (match.roles.size() > std::numeric_limits<uint16_t>::max())
                    return false;

                ScriptRecognitionUnit unit{};
                unit.span.first = inputIndex;
                unit.span.count = match.inputIndex - inputIndex;
                unit.roleOffset = static_cast<uint32_t>(working.roles.size());
                unit.type = accept->unitTypeId;
                unit.roleCount = static_cast<uint16_t>(match.roles.size());

                working.roles.insert(
                    working.roles.end(),
                    std::make_move_iterator(match.roles.begin()),
                    std::make_move_iterator(match.roles.end()));

                working.units.push_back(unit);

                inputIndex = match.inputIndex;
                recognized = true;
                break;
            }

            // Recognition is allowed to be partial. An unmatched item simply
            // does not belong to a recognized unit.
            if (!recognized)
                ++inputIndex;
        }


        result = std::move(working);
        return true;
    }


    [[nodiscard]]
    static inline bool recognizeScriptKinds(
        const ScriptRecognitionIR& ir,
        const std::vector<ScriptItemKindId>& kinds,
        ScriptRecognitionResult& result)
    {
        if (kinds.size() > std::numeric_limits<uint32_t>::max())
            return false;

        return recognizeScriptKinds(ir, kinds.data(), static_cast<uint32_t>(kinds.size()), result);
    }

} // namespace waavs