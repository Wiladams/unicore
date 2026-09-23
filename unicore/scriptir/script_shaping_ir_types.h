// script_shaping_ir_types.h
#pragma once

#include <cstdint>
#include <limits>

#include "script_recognition_types.h"
#include "script_shaping_selection_types.h"

namespace waavs
{
    // ========================================================================
    // Stable Script Shaping IR identifiers
    //
    // These are indices into pools owned by the finalized ScriptShapingIR.
    // They are not builder handles and are not OpenType table indices.
    // ========================================================================

    static constexpr uint32_t kScriptShapingIRInvalid = std::numeric_limits<uint32_t>::max();

    using ScriptShapingIRFeatureStageId = uint32_t;
    using ScriptShapingIRScalarReplaceId = uint32_t;
    using ScriptShapingIRScalarMoveLeftAcrossRangeId = uint32_t;
    using ScriptShapingIRResolveIndicBaseId = uint32_t;
    using ScriptShapingIRResolveIndicHalfCandidatesId = uint32_t;
    using ScriptShapingIRResolveIndicPreBaseInitialAnchorId = uint32_t;
    using ScriptShapingIRResolveIndicPreBaseAnchorId = uint32_t;
    using ScriptShapingIRResolveIndicRephAnchorId = uint32_t;


    // ========================================================================
    // ScriptShapingIROp
    //
    // Semantic operations executed by the Script Shaping IR runtime.
    //
    // Keep this deliberately small. New operations should be added only when
    // required by an implemented shaping model.
    //
    // The initial IR supports only ordered OpenType feature stages. These are
    // sufficient to reproduce the current generic and Latin shaping policies.
    //
    // Future operation families are expected to include:
    //
    //   classification
    //   segmentation
    //   contextual state
    //   temporary flags
    //   feature masks
    //   scalar rewriting
    //   synthetic insertion/removal
    //   selection
    //   reordering
    //   neighbor queries
    //   post-substitution observation
    //
    // Those semantics should be added incrementally as real scripts require
    // them rather than reserved here speculatively.
    // ========================================================================

    enum class ScriptShapingIROp : uint8_t
    {
        Invalid = 0,

        GsubFeatureStage,
        GposFeatureStage,

        ScalarReplace,
        ScalarMoveLeftAcrossRange,

        ResolveIndicBase,
        ResolveIndicHalfCandidates,
        ResolveIndicPreBaseInitialAnchor,
        ResolveIndicPreBaseAnchor,
        ResolveIndicRephAnchor,
        MoveSelection,
    };


    // ========================================================================
    // ScriptShapingIRInstruction
    //
    // One semantic operation in execution order.
    //
    // payloadIndex addresses the pool selected by op.
    //
    // For the initial operations:
    //
    //   GsubFeatureStage
    //   GposFeatureStage
    //
    //       payloadIndex -> ScriptShapingIRFeatureStage
    //
    // Keeping the instruction separate from its payload allows future
    // operations to use their own typed pools without changing the core
    // instruction representation.
    // ========================================================================

    struct ScriptShapingIRInstruction
    {
        ScriptShapingIROp op{ ScriptShapingIROp::Invalid };

        uint8_t reserved0{ 0 };
        uint16_t reserved1{ 0 };

        uint32_t payloadIndex{ kScriptShapingIRInvalid };
    };


    // ========================================================================
    // ScriptShapingIRFeatureStage
    //
    // One ordered OpenType feature stage.
    //
    // featureOffset/featureCount address a contiguous slice of the finalized
    // ScriptShapingIR feature-tag pool.
    //
    // All features in the stage are selected together. OpenType layout
    // selection remains responsible for resolving those features through the
    // selected Script/LangSys and for executing their referenced lookups in
    // LookupList order.
    //
    // includeRequiredFeature controls whether the resolved LangSys required
    // feature participates in this stage. Normally only the first stage for a
    // GSUB or GPOS pipeline enables it.
    // ========================================================================

    struct ScriptShapingIRFeatureStage
    {
        uint32_t featureOffset{ 0 };
        uint32_t featureCount{ 0 };

        ScriptShapingSelectionRef inputSelection{};
        ScriptShapingSelectionId outputChangedSelection{ kScriptShapingSelectionInvalid };

        uint8_t includeRequiredFeature{ 0 };
        uint8_t reserved0{ 0 };

        [[nodiscard]] bool empty() const noexcept
        {
            return featureCount == 0;
        }

        [[nodiscard]] bool hasInputSelection() const noexcept
        {
            return inputSelection.valid();
        }

        [[nodiscard]] bool hasOutputChangedSelection() const noexcept
        {
            return outputChangedSelection != kScriptShapingSelectionInvalid;
        }
    };




    struct ScriptShapingIRScalarReplace
    {
        uint32_t inputValue{ 0 };

        uint32_t replacementOffset{ 0 };
        uint32_t replacementCount{ 0 };
    };


    struct ScriptShapingIRScalarMoveLeftAcrossRange
    {
        uint32_t targetValue{ 0 };

        uint32_t requiredFlags{ 0 };

        uint32_t acrossFirst{ 0 };
        uint32_t acrossLast{ 0 };
    };

    // ========================================================================
    // ScriptShapingIRResolveIndicBase
    //
    // Resolve the main consonant of an Indic consonant syllable.
    //
    // consonantSequence:
    //     Structural consonants preceding the final recognized candidate.
    //
    // baseCandidate:
//     Final structural consonant candidate.
//
// reph:
//     Optional derived selection identifying a successfully formed Reph.
//     When present, its source consonant is excluded from base candidates.
//
// outputBaseSelection:
//     Derived selection receiving the resolved base source span.
//
// Item-kind ids provide the script vocabulary needed to identify candidate
// consonants and their structural attachments within recognition spans.
    // ========================================================================

    enum class ScriptShapingIndicModel : uint8_t
    {
        Auto = 0,
        Old,
        New
    };


    struct ScriptShapingIRResolveIndicBase
    {
        ScriptShapingSelectionRef consonantSequence{};
        ScriptShapingSelectionRef baseCandidate{};
        ScriptShapingSelectionRef reph{};

        ScriptShapingSelectionId outputBaseSelection{ kScriptShapingSelectionInvalid };

        ScriptItemKindId raKind{ kScriptItemKindInvalid };
        ScriptItemKindId consonantKind{ kScriptItemKindInvalid };
        ScriptItemKindId nuktaKind{ kScriptItemKindInvalid };
        ScriptItemKindId halantKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwjKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwnjKind{ kScriptItemKindInvalid };

        ScriptShapingIndicModel model{ ScriptShapingIndicModel::Auto };
    };


    struct ScriptShapingIRResolveIndicHalfCandidates
    {
        ScriptShapingSelectionRef consonantSequence{};
        ScriptShapingSelectionId outputSelection{ kScriptShapingSelectionInvalid };

        ScriptItemKindId raKind{ kScriptItemKindInvalid };
        ScriptItemKindId consonantKind{ kScriptItemKindInvalid };
        ScriptItemKindId nuktaKind{ kScriptItemKindInvalid };
        ScriptItemKindId halantKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwjKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwnjKind{ kScriptItemKindInvalid };
    };


    struct ScriptShapingIRResolveIndicPreBaseInitialAnchor
    {
        ScriptShapingSelectionRef consonantSequence{};
        ScriptShapingSelectionRef baseCandidate{};
        ScriptShapingSelectionId outputAnchorSelection{ kScriptShapingSelectionInvalid };
    };


    struct ScriptShapingIRResolveIndicPreBaseAnchor
    {
        ScriptShapingSelectionRef base{};
        ScriptShapingSelectionId outputAnchorSelection{ kScriptShapingSelectionInvalid };

        ScriptItemKindId halantKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwjKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwnjKind{ kScriptItemKindInvalid };
    };


    struct ScriptShapingIRResolveIndicRephAnchor
    {
        ScriptShapingSelectionRef reph{};
        ScriptShapingSelectionRef base{};
        ScriptShapingSelectionRef postBaseForms{};
        ScriptShapingSelectionId outputAnchorSelection{ kScriptShapingSelectionInvalid };

        ScriptItemKindId raKind{ kScriptItemKindInvalid };
        ScriptItemKindId consonantKind{ kScriptItemKindInvalid };
        ScriptItemKindId nuktaKind{ kScriptItemKindInvalid };
        ScriptItemKindId halantKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwjKind{ kScriptItemKindInvalid };
        ScriptItemKindId zwnjKind{ kScriptItemKindInvalid };
    };

    // Op: MoveSelection
    enum class ScriptShapingIRMovePlacement : uint8_t
    {
        Invalid = 0,
        Before,
        After
    };


    using ScriptShapingIRMoveSelectionId = uint32_t;


    struct ScriptShapingIRMoveSelection
    {
        ScriptShapingSelectionRef move{};
        ScriptShapingSelectionRef anchor{};
        ScriptShapingIRMovePlacement placement{ ScriptShapingIRMovePlacement::Invalid };
        uint8_t reserved[3]{};
    };


    // ========================================================================
    // Operation classification helpers
    // ========================================================================

    [[nodiscard]]
    static constexpr bool isScriptShapingIRGsub(ScriptShapingIROp op) noexcept
    {
        return op == ScriptShapingIROp::GsubFeatureStage;
    }


    [[nodiscard]]
    static constexpr bool isScriptShapingIRGpos(ScriptShapingIROp op) noexcept
    {
        return op == ScriptShapingIROp::GposFeatureStage;
    }


    [[nodiscard]]
    static constexpr bool isScriptShapingIRFeatureStage(ScriptShapingIROp op) noexcept
    {
        return isScriptShapingIRGsub(op) || isScriptShapingIRGpos(op);
    }

    [[nodiscard]]
    static constexpr bool isScriptShapingIRScalarOp(ScriptShapingIROp op) noexcept
    {
        return
            op == ScriptShapingIROp::ScalarReplace ||
            op == ScriptShapingIROp::ScalarMoveLeftAcrossRange;
    }

} // namespace waavs
