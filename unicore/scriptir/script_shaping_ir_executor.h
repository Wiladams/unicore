// script_shaping_ir_executor.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_layout_selection.h"
#include "opentype_shaping_buffer.h"
#include "opentype_shaping_ir_plan.h"
#include "script_shaping_buffer.h"
#include "script_shaping_ir.h"
#include "shaped_glyph_buffer.h"

namespace waavs
{
    // ========================================================================
    // selectScriptShapingIRLayoutPlan
    //
    // Resolve one finalized feature-stage instruction against one GSUB or
    // GPOS table.
    //
    // selected:
    //
    //   true  -> Script/LangSys resolved and one or more lookups selected
    //   false -> legal no-op because no applicable layout plan was produced
    //
    // Malformed layout data remains a hard failure.
    // ========================================================================

    static inline bool selectScriptShapingIRLayoutPlan(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const ScriptShapingIR& ir, const ScriptShapingIRFeatureStage& stage,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        OpenTypeLayoutLookupPlan& plan, bool& selected)
    {
        selected = false;
        plan.clear();

        if (!tableData || scriptTag == 0)
            return false;

        const uint32_t* featureTags = ir.featureTagData(stage);

        if (stage.featureCount != 0 && !featureTags)
            return false;

        OpenTypeLayoutFeatureRequest request;

        request.scriptTag = scriptTag;
        request.languageTag = languageTag;
        request.featureTags = featureTags;
        request.featureTagCount = stage.featureCount;
        request.includeRequiredFeature = stage.includeRequiredFeature != 0;
        request.fallbackToDefaultScript = fallbackToDefaultScript;
        request.fallbackToDefaultLanguage = fallbackToDefaultLanguage;

        const OpenTypeLayoutSelectionResult result =
            selectOpenTypeLayoutLookups(tableData, request, plan);

        switch (result)
        {
        case OpenTypeLayoutSelectionResult::Success:
            selected = !plan.empty();
            return true;

        case OpenTypeLayoutSelectionResult::NoScript:
        case OpenTypeLayoutSelectionResult::NoLanguageSystem:
            plan.clear();
            return true;

        default:
            plan.clear();
            return false;
        }
    }


    // ========================================================================
    // applyScriptShapingIRScalarReplace
    //
    // Execute one ScalarReplace instruction.
    //
    // The instruction performs exactly one pass over the input sequence.
    //
    // Each matching item:
    //
    //     inputValue
    //
    // becomes:
    //
    //     replacement[0]
    //     replacement[1]
    //     ...
    //
    // Every generated item inherits scalarOffset/scalarCount from the item it
    // replaces.
    //
    // Generated items are not recursively processed by this same instruction.
    // A later Script IR instruction may process them.
    // ========================================================================

    [[nodiscard]]
    static inline bool applyScriptShapingIRScalarReplace(
        const ScriptShapingIR& ir,
        const ScriptShapingIRScalarReplace& replacement,
        ScriptShapingBuffer& buffer)
    {
        if (replacement.replacementCount == 0)
            return false;

        const uint32_t* values =
            ir.scalarReplacementData(replacement);

        if (!values)
            return false;

        size_t matchCount = 0;

        for (const ScriptShapingItem& item : buffer)
        {
            if (item.value == replacement.inputValue)
                ++matchCount;
        }

        if (matchCount == 0)
            return true;


        // ------------------------------------------------------------
        // Calculate output size safely.
        // ------------------------------------------------------------

        const size_t extraPerMatch =
            size_t(replacement.replacementCount) - 1;

        if (extraPerMatch != 0 &&
            matchCount > (size_t(-1) - buffer.size()) / extraPerMatch)
        {
            return false;
        }

        const size_t outputSize =
            buffer.size() + matchCount * extraPerMatch;


        // ------------------------------------------------------------
        // Build replacement sequence transactionally.
        // ------------------------------------------------------------

        std::vector<ScriptShapingItem> working;
        working.reserve(outputSize);

        for (const ScriptShapingItem& item : buffer)
        {
            if (item.value != replacement.inputValue)
            {
                working.push_back(item);
                continue;
            }

            for (uint32_t i = 0; i < replacement.replacementCount; ++i)
            {
                ScriptShapingItem output = item;
                output.value = values[i];
                output.flags |= ScriptShapingItemFlagGenerated;

                working.push_back(output);
            }
        }

        buffer.items().swap(working);
        return true;
    }


    // ========================================================================
// applyScriptShapingIRScalarMoveLeftAcrossRange
//
// Move matching scalar-domain items left across the immediately preceding
// contiguous run of values in the inclusive acrossFirst..acrossLast range.
//
// A target matches when:
//
//     item.value == targetValue
//
// and every required transient flag is present:
//
//     (item.flags & requiredFlags) == requiredFlags
//
// Provenance and transient flags move with the item unchanged.
// ========================================================================

    [[nodiscard]]
    static inline bool applyScriptShapingIRScalarMoveLeftAcrossRange(
        const ScriptShapingIRScalarMoveLeftAcrossRange& move,
        ScriptShapingBuffer& buffer)
    {
        if (move.acrossFirst > move.acrossLast)
            return false;

        std::vector<ScriptShapingItem>& items = buffer.items();

        for (size_t i = 0; i < items.size(); ++i)
        {
            const ScriptShapingItem& candidate = items[i];

            if (candidate.value != move.targetValue)
                continue;

            if ((candidate.flags & move.requiredFlags) != move.requiredFlags)
                continue;

            size_t destination = i;

            while (destination > 0)
            {
                const uint32_t previousValue = items[destination - 1].value;

                if (previousValue < move.acrossFirst || previousValue > move.acrossLast)
                    break;

                --destination;
            }

            if (destination == i)
                continue;

            const ScriptShapingItem moving = items[i];

            for (size_t j = i; j > destination; --j)
                items[j] = items[j - 1];

            items[destination] = moving;
        }

        return true;
    }


    // ========================================================================
    // applyScriptShapingIRScalars
    //
    // Execute all scalar-domain instructions in Script IR program order.
    //
    // Feature-stage instructions are ignored during this phase.
    //
    // Current scalar operations:
    //
    //     ScalarReplace
    //
    // This phase executes before cmap.
    // ========================================================================

    [[nodiscard]]
    static inline bool applyScriptShapingIRScalars(
        const ScriptShapingIR& ir,
        ScriptShapingBuffer& buffer)
    {
        if (!buffer.input())
            return false;

        for (const ScriptShapingIRInstruction& instruction : ir.instructions)
        {
            switch (instruction.op)
            {
            case ScriptShapingIROp::ScalarReplace:
            {
                const ScriptShapingIRScalarReplace* replacement =
                    ir.scalarReplace(instruction.payloadIndex);

                if (!replacement)
                    return false;

                if (!applyScriptShapingIRScalarReplace(
                    ir, *replacement, buffer))
                {
                    return false;
                }

                break;
            }

            case ScriptShapingIROp::ScalarMoveLeftAcrossRange:
            {
                const ScriptShapingIRScalarMoveLeftAcrossRange* move =
                    ir.scalarMoveLeftAcrossRange(instruction.payloadIndex);

                if (!move)
                    return false;

                if (!applyScriptShapingIRScalarMoveLeftAcrossRange(*move, buffer))
                    return false;

                break;
            }

            case ScriptShapingIROp::GsubFeatureStage:
            case ScriptShapingIROp::GposFeatureStage:
                break;

            default:
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // applyScriptShapingIRGsub
    //
    // Execute GSUB feature-stage instructions in Script IR program order.
    //
    // Scalar and GPOS instructions are ignored during this phase.
    //
    // Each GSUB feature stage:
    //
    //     ScriptShapingIRFeatureStage
    //         ->
    //     OpenTypeLayoutLookupPlan
    //         ->
    //     OpenTypeShapingIRPlan
    //         ->
    //     semantic GSUB execution
    //
    // cmap must already have populated OpenTypeShapingBuffer.
    // ========================================================================

    [[nodiscard]]
    static inline bool applyScriptShapingIRGsub(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const ScriptShapingIR& ir,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer)
    {
        if (!tableData || scriptTag == 0)
            return false;

        for (const ScriptShapingIRInstruction& instruction : ir.instructions)
        {
            switch (instruction.op)
            {
            case ScriptShapingIROp::GsubFeatureStage:
            {
                const ScriptShapingIRFeatureStage* stage =
                    ir.featureStage(instruction.payloadIndex);

                if (!stage)
                    return false;

                OpenTypeLayoutLookupPlan plan;
                bool selected = false;

                if (!selectScriptShapingIRLayoutPlan(
                    tableData, scriptTag, languageTag,
                    ir, *stage,
                    fallbackToDefaultScript,
                    fallbackToDefaultLanguage,
                    plan, selected))
                {
                    return false;
                }

                if (selected &&
                    !compileAndApplyOpenTypeGsubIRPlan(
                        plan, gdef, buffer))
                {
                    return false;
                }

                break;
            }

            case ScriptShapingIROp::ScalarReplace:
            case ScriptShapingIROp::ScalarMoveLeftAcrossRange:
            case ScriptShapingIROp::GposFeatureStage:
                break;

            default:
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // applyScriptShapingIRGpos
    //
    // Execute the GPOS portion of Script IR.
    //
    // Scalar and GSUB instructions are ignored during this phase.
    //
    // Current GPOS execution requires all selected attachment-producing
    // lookups to share one attachment graph and one final resolve. Therefore
    // the initial Script IR executor permits at most one GPOS feature stage.
    //
    // Nominal metrics must already have populated ShapedGlyphBuffer.
    // ========================================================================

    [[nodiscard]]
    static inline bool applyScriptShapingIRGpos(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const ScriptShapingIR& ir,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        if (!tableData || scriptTag == 0)
            return false;

        const ScriptShapingIRFeatureStage* selectedStage = nullptr;

        for (const ScriptShapingIRInstruction& instruction : ir.instructions)
        {
            switch (instruction.op)
            {
            case ScriptShapingIROp::GposFeatureStage:
                if (selectedStage)
                    return false;

                selectedStage =
                    ir.featureStage(instruction.payloadIndex);

                if (!selectedStage)
                    return false;

                break;

            case ScriptShapingIROp::ScalarReplace:
            case ScriptShapingIROp::ScalarMoveLeftAcrossRange:
            case ScriptShapingIROp::GsubFeatureStage:
                break;

            default:
                return false;
            }
        }

        if (!selectedStage)
            return true;

        OpenTypeLayoutLookupPlan plan;
        bool selected = false;

        if (!selectScriptShapingIRLayoutPlan(
            tableData, scriptTag, languageTag,
            ir, *selectedStage,
            fallbackToDefaultScript,
            fallbackToDefaultLanguage,
            plan, selected))
        {
            return false;
        }

        if (selected &&
            !compileAndApplyOpenTypeGposIRPlan(
                plan, gdef, buffer, runRightToLeft))
        {
            return false;
        }

        return true;
    }

} // namespace waavs