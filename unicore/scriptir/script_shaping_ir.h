// script_shaping_ir.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "script_shaping_ir_types.h"

namespace waavs
{
    // ========================================================================
    // ScriptShapingIR
    //
    // Finalized owning representation of one compiled script-shaping program.
    //
    // The builder may use richer temporary objects and references while
    // constructing the program. finalize() lowers that mutable representation
    // into the compact pools stored here.
    //
    // Once successfully finalized, this object should be treated as immutable
    // by shaping execution.
    //
    // Initial pools:
    //
    //   instructions
    //       Ordered semantic operations.
    //
    //   featureStages
    //       Payloads referenced by GsubFeatureStage and GposFeatureStage
    //       instructions.
    //
    //   featureTags
    //       Contiguous feature-tag storage referenced by feature stages.
    //
    // Additional semantic operations may add their own typed payload pools
    // without changing the instruction representation.
    // ========================================================================

    struct ScriptShapingIR
    {
        std::vector<ScriptShapingIRInstruction> instructions{};
        uint32_t derivedSelectionCount{ 0 };

        // scalarReplacementValues and scalarReplacements are used by the ScalarReplace operation.
        std::vector<uint32_t> scalarReplacementValues{};
        std::vector<ScriptShapingIRScalarReplace> scalarReplacements{};
        std::vector<ScriptShapingIRScalarMoveLeftAcrossRange> scalarMoveLeftAcrossRanges{};
        std::vector<ScriptShapingIRResolveIndicBase> indicBaseResolvers{};
        std::vector<ScriptShapingIRResolveIndicHalfCandidates> indicHalfCandidateResolvers{};
        std::vector<ScriptShapingIRResolveIndicPreBaseInitialAnchor> indicPreBaseInitialAnchorResolvers{};
        std::vector<ScriptShapingIRResolveIndicPreBaseAnchor> indicPreBaseAnchorResolvers{};
        std::vector<ScriptShapingIRResolveIndicRephAnchor> indicRephAnchorResolvers{};
        std::vector<ScriptShapingIRMoveSelection> moveSelections{};

        std::vector<uint32_t> featureTags{};
        std::vector<ScriptShapingIRFeatureStage> featureStages{};
        



        void clear() noexcept
        {
            instructions.clear();
            derivedSelectionCount = 0;

            featureTags.clear();
            featureStages.clear();

            scalarReplacementValues.clear();
            scalarReplacements.clear();
            scalarMoveLeftAcrossRanges.clear();
            indicBaseResolvers.clear();
            indicHalfCandidateResolvers.clear();
            indicPreBaseInitialAnchorResolvers.clear();
            indicPreBaseAnchorResolvers.clear();
            indicRephAnchorResolvers.clear();
            moveSelections.clear();
        }


        [[nodiscard]] bool empty() const noexcept
        {
            return instructions.empty();
        }


        [[nodiscard]] size_t size() const noexcept
        {
            return instructions.size();
        }


        [[nodiscard]] const ScriptShapingIRInstruction* instruction(size_t index) const noexcept
        {
            return index < instructions.size() ? &instructions[index] : nullptr;
        }

        [[nodiscard]]
        bool hasDerivedSelection(ScriptShapingSelectionId id) const noexcept
        {
            return id != kScriptShapingSelectionInvalid &&
                id <= derivedSelectionCount;
        }

        // Accessors
        // Op: ScalarReplace
        [[nodiscard]]
        const ScriptShapingIRScalarReplace* scalarReplace(ScriptShapingIRScalarReplaceId id) const noexcept
        {
            return id < scalarReplacements.size() ? &scalarReplacements[id] : nullptr;
        }


        [[nodiscard]]
        const uint32_t* scalarReplacementData(const ScriptShapingIRScalarReplace& replacement) const noexcept
        {
            if (replacement.replacementOffset > scalarReplacementValues.size())
                return nullptr;

            if (replacement.replacementCount > scalarReplacementValues.size() - replacement.replacementOffset)
                return nullptr;

            return scalarReplacementValues.data() + replacement.replacementOffset;
        }

        [[nodiscard]]
        uint32_t scalarReplacementValue(const ScriptShapingIRScalarReplace& replacement, size_t index) const noexcept
        {
            if (index >= replacement.replacementCount)
                return 0;

            const size_t offset =
                size_t(replacement.replacementOffset) + index;

            return offset < scalarReplacementValues.size()
                ? scalarReplacementValues[offset]
                : 0;
        }

        // Op: ScalarMoveLeftAcrossRange
        [[nodiscard]]
        const ScriptShapingIRScalarMoveLeftAcrossRange* scalarMoveLeftAcrossRange(
            ScriptShapingIRScalarMoveLeftAcrossRangeId id) const noexcept
        {
            return id < scalarMoveLeftAcrossRanges.size()
                ? &scalarMoveLeftAcrossRanges[id]
                : nullptr;
        }

        // Op: ResolveIndicHalfCandidates
        [[nodiscard]]
        const ScriptShapingIRResolveIndicHalfCandidates* indicHalfCandidatesResolver(
            ScriptShapingIRResolveIndicHalfCandidatesId id) const noexcept
        {
            return id < indicHalfCandidateResolvers.size() ? &indicHalfCandidateResolvers[id] : nullptr;
        }

        // Op: ResolveIndicPreBaseInitialAnchor
        [[nodiscard]]
        const ScriptShapingIRResolveIndicPreBaseInitialAnchor* indicPreBaseInitialAnchorResolver(
            ScriptShapingIRResolveIndicPreBaseInitialAnchorId id) const noexcept
        {
            return id < indicPreBaseInitialAnchorResolvers.size() ? &indicPreBaseInitialAnchorResolvers[id] : nullptr;
        }

        // Op: ResolveIndicPreBaseAnchor
        [[nodiscard]]
        const ScriptShapingIRResolveIndicPreBaseAnchor* indicPreBaseAnchorResolver(
            ScriptShapingIRResolveIndicPreBaseAnchorId id) const noexcept
        {
            return id < indicPreBaseAnchorResolvers.size() ? &indicPreBaseAnchorResolvers[id] : nullptr;
        }

        // Op: ResolveIndicRephAnchor
        [[nodiscard]]
        const ScriptShapingIRResolveIndicRephAnchor* indicRephAnchorResolver(
            ScriptShapingIRResolveIndicRephAnchorId id) const noexcept
        {
            return id < indicRephAnchorResolvers.size() ? &indicRephAnchorResolvers[id] : nullptr;
        }

        // Op: MoveSelection
        [[nodiscard]]
        const ScriptShapingIRMoveSelection* moveSelection(uint32_t index) const noexcept
        {
            return index < moveSelections.size()
                ? &moveSelections[index]
                : nullptr;
        }

        // Op: ResolveIndicBase
        [[nodiscard]]
        const ScriptShapingIRResolveIndicBase* indicBaseResolver(
            ScriptShapingIRResolveIndicBaseId id) const noexcept
        {
            return id < indicBaseResolvers.size()
                ? &indicBaseResolvers[id]
                : nullptr;
        }

        // Op: GsubFeatureStage, GposFeatureStage
        [[nodiscard]] bool hasFeatureStages() const noexcept
        {
            return !featureStages.empty();
        }

        [[nodiscard]] const ScriptShapingIRFeatureStage* featureStage(ScriptShapingIRFeatureStageId id) const noexcept
        {
            return id < featureStages.size() ? &featureStages[id] : nullptr;
        }


        [[nodiscard]] const uint32_t* featureTagData(const ScriptShapingIRFeatureStage& stage) const noexcept
        {
            if (stage.featureOffset > featureTags.size())
                return nullptr;

            if (stage.featureCount > featureTags.size() - stage.featureOffset)
                return nullptr;

            return featureTags.data() + stage.featureOffset;
        }


        [[nodiscard]] uint32_t featureTag(const ScriptShapingIRFeatureStage& stage, size_t index) const noexcept
        {
            if (index >= stage.featureCount)
                return 0;

            const size_t offset = size_t(stage.featureOffset) + index;

            return offset < featureTags.size() ? featureTags[offset] : 0;
        }
    };

} // namespace waavs