// script_shaping_ir_builder.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "script_shaping_ir.h"
#include "script_shaping_buffer.h"

namespace waavs
{
    // ========================================================================
    // ScriptShapingIRBuilderInstruction
    //
    // Loose construction representation for one Script IR instruction.
    //
    // values:
    //
    //   ScalarReplace:
    //       replacement scalar values
    //
    //   GsubFeatureStage / GposFeatureStage:
    //       OpenType feature tags
    //
    // inputValue:
    //
    //   ScalarReplace:
    //       scalar value to replace
    //
    // includeRequiredFeature:
    //
    //   GsubFeatureStage / GposFeatureStage:
    //       include the LangSys required feature when selecting lookups
    //
    // Construction-only state is normalized and packed by finalize().
    // ========================================================================

    struct ScriptShapingIRBuilderInstruction
    {
        ScriptShapingIROp op{ ScriptShapingIROp::Invalid };

        std::vector<uint32_t> values{};

        uint32_t inputValue{ 0 };
        uint32_t requiredFlags{ 0 };
        uint32_t acrossFirst{ 0 };
        uint32_t acrossLast{ 0 };

        ScriptShapingSelectionRef inputSelection{};
        ScriptShapingSelectionId outputChangedSelection{ kScriptShapingSelectionInvalid };

        ScriptShapingSelectionRef moveSelection{};
        ScriptShapingSelectionRef anchorSelection{};
        ScriptShapingIRMovePlacement movePlacement{ ScriptShapingIRMovePlacement::Invalid };

        ScriptShapingSelectionRef indicConsonantSequence{};
        ScriptShapingSelectionRef indicBaseCandidate{};
        ScriptShapingSelectionRef indicReph{};
        ScriptShapingSelectionRef indicBase{};
        ScriptShapingSelectionRef indicPostBaseForms{};

        ScriptShapingSelectionId indicOutputSelection{ kScriptShapingSelectionInvalid };

        ScriptItemKindId indicRaKind{ kScriptItemKindInvalid };
        ScriptItemKindId indicConsonantKind{ kScriptItemKindInvalid };
        ScriptItemKindId indicNuktaKind{ kScriptItemKindInvalid };
        ScriptItemKindId indicHalantKind{ kScriptItemKindInvalid };
        ScriptItemKindId indicZwjKind{ kScriptItemKindInvalid };
        ScriptItemKindId indicZwnjKind{ kScriptItemKindInvalid };
        ScriptShapingIndicModel indicModel{ ScriptShapingIndicModel::Auto };

        bool includeRequiredFeature{ false };

    };


    // ========================================================================
    // ScriptShapingIRBuilder
    //
    // Mutable construction representation for ScriptShapingIR.
    //
    // The builder deliberately favors convenient authoring over compact
    // runtime storage. finalize() translates this representation into the
    // index/offset-based finalized IR.
    //
    // Finalization is non-consuming and transactional. On failure, result is
    // left unchanged.
    // ========================================================================

    class ScriptShapingIRBuilder
    {
    public:
        void clear() noexcept
        {
            mInstructions.clear();
            mDerivedSelectionCount = 0;
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return mInstructions.empty();
        }

        [[nodiscard]]
        size_t size() const noexcept
        {
            return mInstructions.size();
        }

        // ===============================
        // Derived selection management
        // ===============================
        [[nodiscard]]
        ScriptShapingSelectionId addDerivedSelection() noexcept
        {
            if (mDerivedSelectionCount >=
                std::numeric_limits<ScriptShapingSelectionId>::max())
            {
                return kScriptShapingSelectionInvalid;
            }

            ++mDerivedSelectionCount;

            return static_cast<ScriptShapingSelectionId>(
                mDerivedSelectionCount);
        }


        // ====================================================================
        // Scalar replacement
        //
        // Replace each matching scalar-domain item with an ordered sequence of
        // replacement values.
        //
        // Runtime provenance semantics are defined by the executor: every
        // generated item inherits the scalarOffset/scalarCount of the item it
        // replaces.
        // ====================================================================

        [[nodiscard]]
        bool addScalarReplace(uint32_t inputValue, const uint32_t* replacements, size_t count)
        {
            if (count == 0 || !replacements)
                return false;

            if (count > std::numeric_limits<uint32_t>::max())
                return false;

            ScriptShapingIRBuilderInstruction instruction;

            instruction.op = ScriptShapingIROp::ScalarReplace;
            instruction.inputValue = inputValue;
            instruction.values.assign(replacements, replacements + count);

            mInstructions.push_back(std::move(instruction));
            return true;
        }

        template<size_t N>
        [[nodiscard]]
        bool addScalarReplace(uint32_t inputValue, const uint32_t(&replacements)[N])
        {
            return addScalarReplace(inputValue, replacements, N);
        }

        // Op: ScalarMoveLeftAcrossRange
        [[nodiscard]]
        bool addScalarMoveLeftAcrossRange(
            uint32_t targetValue,
            uint32_t requiredFlags,
            uint32_t acrossFirst,
            uint32_t acrossLast)
        {
            if (acrossFirst > acrossLast)
                return false;

            ScriptShapingIRBuilderInstruction instruction;

            instruction.op = ScriptShapingIROp::ScalarMoveLeftAcrossRange;
            instruction.inputValue = targetValue;
            instruction.requiredFlags = requiredFlags;
            instruction.acrossFirst = acrossFirst;
            instruction.acrossLast = acrossLast;

            mInstructions.push_back(std::move(instruction));
            return true;
        }

        // ====================================================================
        // ResolveIndicBase
        // ====================================================================

        [[nodiscard]]
        bool addResolveIndicBase(
            const ScriptShapingSelectionRef& consonantSequence,
            const ScriptShapingSelectionRef& baseCandidate,
            const ScriptShapingSelectionRef& reph,
            ScriptShapingSelectionId outputBaseSelection,
            ScriptItemKindId raKind,
            ScriptItemKindId consonantKind,
            ScriptItemKindId nuktaKind,
            ScriptItemKindId halantKind,
            ScriptItemKindId zwjKind,
            ScriptItemKindId zwnjKind,
            ScriptShapingIndicModel model = ScriptShapingIndicModel::Auto)
        {
            if (!consonantSequence.valid() ||
                !baseCandidate.valid() ||
                outputBaseSelection == kScriptShapingSelectionInvalid)
            {
                return false;
            }

            if (raKind == kScriptItemKindInvalid ||
                consonantKind == kScriptItemKindInvalid ||
                nuktaKind == kScriptItemKindInvalid ||
                halantKind == kScriptItemKindInvalid ||
                zwjKind == kScriptItemKindInvalid ||
                zwnjKind == kScriptItemKindInvalid)
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;

            instruction.op = ScriptShapingIROp::ResolveIndicBase;

            instruction.indicConsonantSequence = consonantSequence;
            instruction.indicBaseCandidate = baseCandidate;
            instruction.indicReph = reph;
            instruction.indicOutputSelection = outputBaseSelection;

            instruction.indicRaKind = raKind;
            instruction.indicConsonantKind = consonantKind;
            instruction.indicNuktaKind = nuktaKind;
            instruction.indicHalantKind = halantKind;
            instruction.indicZwjKind = zwjKind;
            instruction.indicZwnjKind = zwnjKind;
            instruction.indicModel = model;

            mInstructions.push_back(std::move(instruction));
            return true;
        }

        // ====================================================================
        // Indic semantic resolvers
        // ====================================================================

        [[nodiscard]]
        bool addResolveIndicHalfCandidates(
            const ScriptShapingSelectionRef& consonantSequence,
            ScriptShapingSelectionId outputSelection,
            ScriptItemKindId raKind, ScriptItemKindId consonantKind, ScriptItemKindId nuktaKind,
            ScriptItemKindId halantKind, ScriptItemKindId zwjKind, ScriptItemKindId zwnjKind)
        {
            if (!consonantSequence.valid() || outputSelection == kScriptShapingSelectionInvalid)
                return false;

            if (raKind == kScriptItemKindInvalid || consonantKind == kScriptItemKindInvalid ||
                nuktaKind == kScriptItemKindInvalid || halantKind == kScriptItemKindInvalid ||
                zwjKind == kScriptItemKindInvalid || zwnjKind == kScriptItemKindInvalid)
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;
            instruction.op = ScriptShapingIROp::ResolveIndicHalfCandidates;
            instruction.indicConsonantSequence = consonantSequence;
            instruction.indicOutputSelection = outputSelection;
            instruction.indicRaKind = raKind;
            instruction.indicConsonantKind = consonantKind;
            instruction.indicNuktaKind = nuktaKind;
            instruction.indicHalantKind = halantKind;
            instruction.indicZwjKind = zwjKind;
            instruction.indicZwnjKind = zwnjKind;
            mInstructions.push_back(std::move(instruction));
            return true;
        }

        [[nodiscard]]
        bool addResolveIndicPreBaseInitialAnchor(
            const ScriptShapingSelectionRef& consonantSequence,
            const ScriptShapingSelectionRef& baseCandidate,
            ScriptShapingSelectionId outputAnchorSelection)
        {
            if (!consonantSequence.valid() || !baseCandidate.valid() ||
                outputAnchorSelection == kScriptShapingSelectionInvalid)
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;
            instruction.op = ScriptShapingIROp::ResolveIndicPreBaseInitialAnchor;
            instruction.indicConsonantSequence = consonantSequence;
            instruction.indicBaseCandidate = baseCandidate;
            instruction.indicOutputSelection = outputAnchorSelection;
            mInstructions.push_back(std::move(instruction));
            return true;
        }

        [[nodiscard]]
        bool addResolveIndicPreBaseAnchor(
            const ScriptShapingSelectionRef& base,
            ScriptShapingSelectionId outputAnchorSelection,
            ScriptItemKindId halantKind, ScriptItemKindId zwjKind, ScriptItemKindId zwnjKind)
        {
            if (!base.valid() || outputAnchorSelection == kScriptShapingSelectionInvalid ||
                halantKind == kScriptItemKindInvalid || zwjKind == kScriptItemKindInvalid ||
                zwnjKind == kScriptItemKindInvalid)
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;
            instruction.op = ScriptShapingIROp::ResolveIndicPreBaseAnchor;
            instruction.indicBase = base;
            instruction.indicOutputSelection = outputAnchorSelection;
            instruction.indicHalantKind = halantKind;
            instruction.indicZwjKind = zwjKind;
            instruction.indicZwnjKind = zwnjKind;
            mInstructions.push_back(std::move(instruction));
            return true;
        }

        [[nodiscard]]
        bool addResolveIndicRephAnchor(
            const ScriptShapingSelectionRef& reph,
            const ScriptShapingSelectionRef& base,
            const ScriptShapingSelectionRef& postBaseForms,
            ScriptShapingSelectionId outputAnchorSelection,
            ScriptItemKindId raKind, ScriptItemKindId consonantKind, ScriptItemKindId nuktaKind,
            ScriptItemKindId halantKind, ScriptItemKindId zwjKind, ScriptItemKindId zwnjKind)
        {
            if (!reph.valid() || !base.valid() || !postBaseForms.valid() ||
                outputAnchorSelection == kScriptShapingSelectionInvalid ||
                raKind == kScriptItemKindInvalid || consonantKind == kScriptItemKindInvalid ||
                nuktaKind == kScriptItemKindInvalid || halantKind == kScriptItemKindInvalid ||
                zwjKind == kScriptItemKindInvalid || zwnjKind == kScriptItemKindInvalid)
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;
            instruction.op = ScriptShapingIROp::ResolveIndicRephAnchor;
            instruction.indicReph = reph;
            instruction.indicBase = base;
            instruction.indicPostBaseForms = postBaseForms;
            instruction.indicOutputSelection = outputAnchorSelection;
            instruction.indicRaKind = raKind;
            instruction.indicConsonantKind = consonantKind;
            instruction.indicNuktaKind = nuktaKind;
            instruction.indicHalantKind = halantKind;
            instruction.indicZwjKind = zwjKind;
            instruction.indicZwnjKind = zwnjKind;
            mInstructions.push_back(std::move(instruction));
            return true;
        }


        // ====================================================================
        // MoveSelection
        //
        // Move a semantic glyph-domain selection immediately before or after
        // another semantic selection. Both selections are resolved through
        // provenance at execution time.
        // ====================================================================

        [[nodiscard]]
        bool addMoveSelection(
            const ScriptShapingSelectionRef& move,
            const ScriptShapingSelectionRef& anchor,
            ScriptShapingIRMovePlacement placement)
        {
            if (!move.valid() ||
                !anchor.valid() ||
                placement == ScriptShapingIRMovePlacement::Invalid)
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;

            instruction.op = ScriptShapingIROp::MoveSelection;
            instruction.moveSelection = move;
            instruction.anchorSelection = anchor;
            instruction.movePlacement = placement;

            mInstructions.push_back(std::move(instruction));
            return true;
        }


        // ====================================================================
        // GSUB feature stages
        // ====================================================================

        [[nodiscard]]
        bool addGsubFeatureStage(const uint32_t* tags, size_t count, bool includeRequiredFeature = false)
        {
            return addFeatureStage(
                ScriptShapingIROp::GsubFeatureStage,
                tags,
                count,
                includeRequiredFeature,
                {},
                kScriptShapingSelectionInvalid);
        }

        template<size_t N>
        [[nodiscard]]
        bool addGsubFeatureStage(const uint32_t(&tags)[N], bool includeRequiredFeature = false)
        {
            return addGsubFeatureStage(tags, N, includeRequiredFeature);
        }

        [[nodiscard]]
        bool addGsubFeatureStage(
            const uint32_t* tags,
            size_t count,
            const ScriptShapingSelectionRef& inputSelection,
            ScriptShapingSelectionId outputChangedSelection = kScriptShapingSelectionInvalid,
            bool includeRequiredFeature = false)
        {
            if (!inputSelection.valid())
                return false;

            return addFeatureStage(
                ScriptShapingIROp::GsubFeatureStage,
                tags,
                count,
                includeRequiredFeature,
                inputSelection,
                outputChangedSelection);
        }

        template<size_t N>
        [[nodiscard]]
        bool addGsubFeatureStage(
            const uint32_t(&tags)[N],
            const ScriptShapingSelectionRef& inputSelection,
            ScriptShapingSelectionId outputChangedSelection = kScriptShapingSelectionInvalid,
            bool includeRequiredFeature = false)
        {
            return addGsubFeatureStage(
                tags,
                N,
                inputSelection,
                outputChangedSelection,
                includeRequiredFeature);
        }


        // ====================================================================
        // GPOS feature stages
        // ====================================================================

        [[nodiscard]]
        bool addGposFeatureStage(const uint32_t* tags, size_t count, bool includeRequiredFeature = false)
        {
            return addFeatureStage(
                ScriptShapingIROp::GposFeatureStage,
                tags,
                count,
                includeRequiredFeature,
                {},
                kScriptShapingSelectionInvalid);
        }

        template<size_t N>
        [[nodiscard]]
        bool addGposFeatureStage(const uint32_t(&tags)[N], bool includeRequiredFeature = false)
        {
            return addGposFeatureStage(tags, N, includeRequiredFeature);
        }


        // ====================================================================
        // finalize
        //
        // Pack construction records into finalized pools.
        //
        // ScalarReplace:
        //
        //   builder.values
        //       -> scalarReplacementValues[]
        //
        //   builder instruction
        //       -> scalarReplacements[]
        //       -> instruction.payloadIndex
        //
        // Feature stage:
        //
        //   builder.values
        //       -> featureTags[]
        //
        //   builder instruction
        //       -> featureStages[]
        //       -> instruction.payloadIndex
        //
        // On success, result is replaced transactionally.
        // ====================================================================

        [[nodiscard]]
        bool finalize(ScriptShapingIR& result) const
        {
            ScriptShapingIR working;
            working.derivedSelectionCount = mDerivedSelectionCount;

            if (mInstructions.size() > std::numeric_limits<uint32_t>::max())
                return false;


            // ------------------------------------------------------------
            // Preflight pool sizes.
            // ------------------------------------------------------------

            size_t totalScalarReplacementValues = 0;
            size_t totalFeatureTags = 0;
            size_t scalarReplacementCount = 0;
            size_t scalarMoveLeftAcrossRangeCount = 0;
            size_t featureStageCount = 0;
            size_t indicBaseResolverCount = 0;
            size_t indicHalfCandidateResolverCount = 0;
            size_t indicPreBaseInitialAnchorResolverCount = 0;
            size_t indicPreBaseAnchorResolverCount = 0;
            size_t indicRephAnchorResolverCount = 0;
            size_t moveSelectionCount = 0;

            for (const ScriptShapingIRBuilderInstruction& source : mInstructions)
            {
                switch (source.op)
                {
                case ScriptShapingIROp::ScalarReplace:
                    if (source.values.empty())
                        return false;

                    if (source.values.size() > std::numeric_limits<uint32_t>::max())
                        return false;

                    if (source.values.size() >
                        std::numeric_limits<uint32_t>::max() - totalScalarReplacementValues)
                    {
                        return false;
                    }

                    totalScalarReplacementValues += source.values.size();

                    if (scalarReplacementCount == std::numeric_limits<uint32_t>::max())
                        return false;

                    ++scalarReplacementCount;
                    break;

                case ScriptShapingIROp::ScalarMoveLeftAcrossRange:
                    if (source.acrossFirst > source.acrossLast)
                        return false;

                    if (scalarMoveLeftAcrossRangeCount == std::numeric_limits<uint32_t>::max())
                        return false;

                    ++scalarMoveLeftAcrossRangeCount;
                    break;

                case ScriptShapingIROp::ResolveIndicBase:
                    if (indicBaseResolverCount == std::numeric_limits<uint32_t>::max())
                        return false;

                    ++indicBaseResolverCount;
                    break;

                case ScriptShapingIROp::ResolveIndicHalfCandidates:
                    if (indicHalfCandidateResolverCount == std::numeric_limits<uint32_t>::max()) return false;
                    ++indicHalfCandidateResolverCount;
                    break;

                case ScriptShapingIROp::ResolveIndicPreBaseInitialAnchor:
                    if (indicPreBaseInitialAnchorResolverCount == std::numeric_limits<uint32_t>::max()) return false;
                    ++indicPreBaseInitialAnchorResolverCount;
                    break;

                case ScriptShapingIROp::ResolveIndicPreBaseAnchor:
                    if (indicPreBaseAnchorResolverCount == std::numeric_limits<uint32_t>::max()) return false;
                    ++indicPreBaseAnchorResolverCount;
                    break;

                case ScriptShapingIROp::ResolveIndicRephAnchor:
                    if (indicRephAnchorResolverCount == std::numeric_limits<uint32_t>::max()) return false;
                    ++indicRephAnchorResolverCount;
                    break;

                case ScriptShapingIROp::MoveSelection:
                    if (!source.moveSelection.valid() ||
                        !source.anchorSelection.valid() ||
                        source.movePlacement == ScriptShapingIRMovePlacement::Invalid)
                    {
                        return false;
                    }

                    if (moveSelectionCount == std::numeric_limits<uint32_t>::max())
                        return false;

                    ++moveSelectionCount;
                    break;

                case ScriptShapingIROp::GsubFeatureStage:
                case ScriptShapingIROp::GposFeatureStage:
                    if (source.values.size() > std::numeric_limits<uint32_t>::max())
                        return false;

                    if (source.values.size() >
                        std::numeric_limits<uint32_t>::max() - totalFeatureTags)
                    {
                        return false;
                    }

                    totalFeatureTags += source.values.size();

                    if (featureStageCount == std::numeric_limits<uint32_t>::max())
                        return false;

                    ++featureStageCount;
                    break;

                default:
                    return false;
                }
            }


            // ------------------------------------------------------------
            // Reserve finalized storage.
            // ------------------------------------------------------------

            working.instructions.reserve(mInstructions.size());

            working.scalarReplacementValues.reserve(totalScalarReplacementValues);
            working.scalarReplacements.reserve(scalarReplacementCount);
            working.scalarMoveLeftAcrossRanges.reserve(scalarMoveLeftAcrossRangeCount);
            working.indicBaseResolvers.reserve(indicBaseResolverCount);
            working.indicHalfCandidateResolvers.reserve(indicHalfCandidateResolverCount);
            working.indicPreBaseInitialAnchorResolvers.reserve(indicPreBaseInitialAnchorResolverCount);
            working.indicPreBaseAnchorResolvers.reserve(indicPreBaseAnchorResolverCount);
            working.indicRephAnchorResolvers.reserve(indicRephAnchorResolverCount);
            working.moveSelections.reserve(moveSelectionCount);

            working.featureTags.reserve(totalFeatureTags);
            working.featureStages.reserve(featureStageCount);


            // ------------------------------------------------------------
            // Pack instructions.
            // ------------------------------------------------------------

            for (const ScriptShapingIRBuilderInstruction& source : mInstructions)
            {
                ScriptShapingIRInstruction instruction{};
                instruction.op = source.op;

                switch (source.op)
                {
                case ScriptShapingIROp::ScalarReplace:
                {
                    if (working.scalarReplacements.size() >= std::numeric_limits<uint32_t>::max())
                        return false;

                    if (working.scalarReplacementValues.size() > std::numeric_limits<uint32_t>::max())
                        return false;

                    if (source.values.size() >
                        std::numeric_limits<uint32_t>::max() - working.scalarReplacementValues.size())
                    {
                        return false;
                    }

                    const ScriptShapingIRScalarReplaceId replacementId =
                        static_cast<ScriptShapingIRScalarReplaceId>(
                            working.scalarReplacements.size());

                    ScriptShapingIRScalarReplace replacement{};

                    replacement.inputValue = source.inputValue;
                    replacement.replacementOffset =
                        static_cast<uint32_t>(working.scalarReplacementValues.size());
                    replacement.replacementCount =
                        static_cast<uint32_t>(source.values.size());

                    working.scalarReplacementValues.insert(
                        working.scalarReplacementValues.end(),
                        source.values.begin(),
                        source.values.end());

                    working.scalarReplacements.push_back(replacement);

                    instruction.payloadIndex = replacementId;
                    break;
                }

                case ScriptShapingIROp::ScalarMoveLeftAcrossRange:
                {
                    if (working.scalarMoveLeftAcrossRanges.size() >= std::numeric_limits<uint32_t>::max())
                        return false;

                    const ScriptShapingIRScalarMoveLeftAcrossRangeId moveId =
                        static_cast<ScriptShapingIRScalarMoveLeftAcrossRangeId>(
                            working.scalarMoveLeftAcrossRanges.size());

                    ScriptShapingIRScalarMoveLeftAcrossRange move{};

                    move.targetValue = source.inputValue;
                    move.requiredFlags = source.requiredFlags;
                    move.acrossFirst = source.acrossFirst;
                    move.acrossLast = source.acrossLast;

                    working.scalarMoveLeftAcrossRanges.push_back(move);

                    instruction.payloadIndex = moveId;
                    break;
                }

                case ScriptShapingIROp::ResolveIndicBase:
                {
                    if (working.indicBaseResolvers.size() >= std::numeric_limits<uint32_t>::max())
                        return false;

                    const ScriptShapingIRResolveIndicBaseId resolverId =
                        static_cast<ScriptShapingIRResolveIndicBaseId>(
                            working.indicBaseResolvers.size());

                    ScriptShapingIRResolveIndicBase resolver{};

                    resolver.consonantSequence = source.indicConsonantSequence;
                    resolver.baseCandidate = source.indicBaseCandidate;
                    resolver.reph = source.indicReph;
                    resolver.outputBaseSelection = source.indicOutputSelection;

                    resolver.raKind = source.indicRaKind;
                    resolver.consonantKind = source.indicConsonantKind;
                    resolver.nuktaKind = source.indicNuktaKind;
                    resolver.halantKind = source.indicHalantKind;
                    resolver.zwjKind = source.indicZwjKind;
                    resolver.zwnjKind = source.indicZwnjKind;
                    resolver.model = source.indicModel;

                    working.indicBaseResolvers.push_back(resolver);

                    instruction.payloadIndex = resolverId;
                    break;
                }

                case ScriptShapingIROp::ResolveIndicHalfCandidates:
                {
                    ScriptShapingIRResolveIndicHalfCandidates resolver{};
                    resolver.consonantSequence = source.indicConsonantSequence;
                    resolver.outputSelection = source.indicOutputSelection;
                    resolver.raKind = source.indicRaKind;
                    resolver.consonantKind = source.indicConsonantKind;
                    resolver.nuktaKind = source.indicNuktaKind;
                    resolver.halantKind = source.indicHalantKind;
                    resolver.zwjKind = source.indicZwjKind;
                    resolver.zwnjKind = source.indicZwnjKind;
                    instruction.payloadIndex = static_cast<uint32_t>(working.indicHalfCandidateResolvers.size());
                    working.indicHalfCandidateResolvers.push_back(resolver);
                    break;
                }

                case ScriptShapingIROp::ResolveIndicPreBaseInitialAnchor:
                {
                    ScriptShapingIRResolveIndicPreBaseInitialAnchor resolver{};
                    resolver.consonantSequence = source.indicConsonantSequence;
                    resolver.baseCandidate = source.indicBaseCandidate;
                    resolver.outputAnchorSelection = source.indicOutputSelection;
                    instruction.payloadIndex = static_cast<uint32_t>(working.indicPreBaseInitialAnchorResolvers.size());
                    working.indicPreBaseInitialAnchorResolvers.push_back(resolver);
                    break;
                }

                case ScriptShapingIROp::ResolveIndicPreBaseAnchor:
                {
                    ScriptShapingIRResolveIndicPreBaseAnchor resolver{};
                    resolver.base = source.indicBase;
                    resolver.outputAnchorSelection = source.indicOutputSelection;
                    resolver.halantKind = source.indicHalantKind;
                    resolver.zwjKind = source.indicZwjKind;
                    resolver.zwnjKind = source.indicZwnjKind;
                    instruction.payloadIndex = static_cast<uint32_t>(working.indicPreBaseAnchorResolvers.size());
                    working.indicPreBaseAnchorResolvers.push_back(resolver);
                    break;
                }

                case ScriptShapingIROp::ResolveIndicRephAnchor:
                {
                    ScriptShapingIRResolveIndicRephAnchor resolver{};
                    resolver.reph = source.indicReph;
                    resolver.base = source.indicBase;
                    resolver.postBaseForms = source.indicPostBaseForms;
                    resolver.outputAnchorSelection = source.indicOutputSelection;
                    resolver.raKind = source.indicRaKind;
                    resolver.consonantKind = source.indicConsonantKind;
                    resolver.nuktaKind = source.indicNuktaKind;
                    resolver.halantKind = source.indicHalantKind;
                    resolver.zwjKind = source.indicZwjKind;
                    resolver.zwnjKind = source.indicZwnjKind;
                    instruction.payloadIndex = static_cast<uint32_t>(working.indicRephAnchorResolvers.size());
                    working.indicRephAnchorResolvers.push_back(resolver);
                    break;
                }

                case ScriptShapingIROp::MoveSelection:
                {
                    if (working.moveSelections.size() >= std::numeric_limits<uint32_t>::max())
                        return false;

                    const ScriptShapingIRMoveSelectionId moveId =
                        static_cast<ScriptShapingIRMoveSelectionId>(
                            working.moveSelections.size());

                    ScriptShapingIRMoveSelection move{};

                    move.move = source.moveSelection;
                    move.anchor = source.anchorSelection;
                    move.placement = source.movePlacement;

                    working.moveSelections.push_back(move);

                    instruction.payloadIndex = moveId;
                    break;
                }

                case ScriptShapingIROp::GsubFeatureStage:
                case ScriptShapingIROp::GposFeatureStage:
                {
                    if (working.featureStages.size() >= std::numeric_limits<uint32_t>::max())
                        return false;

                    if (working.featureTags.size() > std::numeric_limits<uint32_t>::max())
                        return false;

                    if (source.values.size() >
                        std::numeric_limits<uint32_t>::max() - working.featureTags.size())
                    {
                        return false;
                    }

                    const ScriptShapingIRFeatureStageId stageId =
                        static_cast<ScriptShapingIRFeatureStageId>(
                            working.featureStages.size());

                    ScriptShapingIRFeatureStage stage{};

                    stage.featureOffset =
                        static_cast<uint32_t>(working.featureTags.size());

                    stage.featureCount =
                        static_cast<uint32_t>(source.values.size());

                    stage.inputSelection = source.inputSelection;
                    stage.outputChangedSelection = source.outputChangedSelection;

                    stage.includeRequiredFeature =
                        source.includeRequiredFeature ? 1u : 0u;

                    working.featureTags.insert(
                        working.featureTags.end(),
                        source.values.begin(),
                        source.values.end());

                    working.featureStages.push_back(stage);

                    instruction.payloadIndex = stageId;
                    break;
                }

                default:
                    return false;
                }

                working.instructions.push_back(instruction);
            }


            // ------------------------------------------------------------
            // Validate finalized representation before publication.
            // ------------------------------------------------------------

            if (!validateFinalizedIR(working))
                return false;

            result = std::move(working);
            return true;
        }


    private:
        // ====================================================================
        // addFeatureStage
        // ====================================================================

        [[nodiscard]]
        bool addFeatureStage(
            ScriptShapingIROp op,
            const uint32_t* tags,
            size_t count,
            bool includeRequiredFeature,
            const ScriptShapingSelectionRef& inputSelection,
            ScriptShapingSelectionId outputChangedSelection)
        {
            if (!isScriptShapingIRFeatureStage(op))
                return false;

            if (count != 0 && !tags)
                return false;

            if (count > std::numeric_limits<uint32_t>::max())
                return false;

            // Selection-aware feature stages are currently GSUB-only.
            if (op == ScriptShapingIROp::GposFeatureStage &&
                (inputSelection.valid() ||
                    outputChangedSelection != kScriptShapingSelectionInvalid))
            {
                return false;
            }

            ScriptShapingIRBuilderInstruction instruction;

            instruction.op = op;
            instruction.includeRequiredFeature = includeRequiredFeature;
            instruction.inputSelection = inputSelection;
            instruction.outputChangedSelection = outputChangedSelection;

            if (count != 0)
                instruction.values.assign(tags, tags + count);

            mInstructions.push_back(std::move(instruction));
            return true;
        }


        // ====================================================================
        // validateFinalizedIR
        //
        // Initial builder-local validation for finalized Script IR.
        //
        // This can later move into a shared validateScriptShapingIR() routine
        // as the semantic instruction set grows.
        // ====================================================================

        [[nodiscard]]
        static bool validateFinalizedIR(const ScriptShapingIR& ir) noexcept
        {
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

                    if (replacement->replacementCount == 0)
                        return false;

                    if (replacement->replacementOffset >
                        ir.scalarReplacementValues.size())
                    {
                        return false;
                    }

                    if (replacement->replacementCount >
                        ir.scalarReplacementValues.size() -
                        replacement->replacementOffset)
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

                    if (move->acrossFirst > move->acrossLast)
                        return false;

                    break;
                }

                case ScriptShapingIROp::ResolveIndicBase:
                {
                    const ScriptShapingIRResolveIndicBase* resolver =
                        ir.indicBaseResolver(instruction.payloadIndex);

                    if (!resolver)
                        return false;

                    if (!resolver->consonantSequence.valid() ||
                        !resolver->baseCandidate.valid())
                    {
                        return false;
                    }

                    if (resolver->outputBaseSelection == kScriptShapingSelectionInvalid ||
                        !ir.hasDerivedSelection(resolver->outputBaseSelection))
                    {
                        return false;
                    }

                    if (resolver->reph.valid() &&
                        resolver->reph.kind == ScriptShapingSelectionKind::Derived &&
                        !ir.hasDerivedSelection(resolver->reph.id))
                    {
                        return false;
                    }

                    if (resolver->raKind == kScriptItemKindInvalid ||
                        resolver->consonantKind == kScriptItemKindInvalid ||
                        resolver->nuktaKind == kScriptItemKindInvalid ||
                        resolver->halantKind == kScriptItemKindInvalid ||
                        resolver->zwjKind == kScriptItemKindInvalid ||
                        resolver->zwnjKind == kScriptItemKindInvalid)
                    {
                        return false;
                    }

                    break;
                }

                case ScriptShapingIROp::ResolveIndicHalfCandidates:
                {
                    const auto* resolver = ir.indicHalfCandidatesResolver(instruction.payloadIndex);
                    if (!resolver || !resolver->consonantSequence.valid() ||
                        resolver->outputSelection == kScriptShapingSelectionInvalid ||
                        !ir.hasDerivedSelection(resolver->outputSelection) ||
                        resolver->raKind == kScriptItemKindInvalid ||
                        resolver->consonantKind == kScriptItemKindInvalid ||
                        resolver->nuktaKind == kScriptItemKindInvalid ||
                        resolver->halantKind == kScriptItemKindInvalid ||
                        resolver->zwjKind == kScriptItemKindInvalid ||
                        resolver->zwnjKind == kScriptItemKindInvalid) return false;
                    break;
                }

                case ScriptShapingIROp::ResolveIndicPreBaseInitialAnchor:
                {
                    const auto* resolver = ir.indicPreBaseInitialAnchorResolver(instruction.payloadIndex);
                    if (!resolver || !resolver->consonantSequence.valid() || !resolver->baseCandidate.valid() ||
                        resolver->outputAnchorSelection == kScriptShapingSelectionInvalid ||
                        !ir.hasDerivedSelection(resolver->outputAnchorSelection)) return false;
                    break;
                }

                case ScriptShapingIROp::ResolveIndicPreBaseAnchor:
                {
                    const auto* resolver = ir.indicPreBaseAnchorResolver(instruction.payloadIndex);
                    if (!resolver || !resolver->base.valid() ||
                        (resolver->base.kind == ScriptShapingSelectionKind::Derived && !ir.hasDerivedSelection(resolver->base.id)) ||
                        resolver->outputAnchorSelection == kScriptShapingSelectionInvalid ||
                        !ir.hasDerivedSelection(resolver->outputAnchorSelection) ||
                        resolver->halantKind == kScriptItemKindInvalid ||
                        resolver->zwjKind == kScriptItemKindInvalid ||
                        resolver->zwnjKind == kScriptItemKindInvalid) return false;
                    break;
                }

                case ScriptShapingIROp::ResolveIndicRephAnchor:
                {
                    const auto* resolver = ir.indicRephAnchorResolver(instruction.payloadIndex);
                    if (!resolver || !resolver->reph.valid() || !resolver->base.valid() ||
                        !resolver->postBaseForms.valid() ||
                        (resolver->reph.kind == ScriptShapingSelectionKind::Derived && !ir.hasDerivedSelection(resolver->reph.id)) ||
                        (resolver->base.kind == ScriptShapingSelectionKind::Derived && !ir.hasDerivedSelection(resolver->base.id)) ||
                        (resolver->postBaseForms.kind == ScriptShapingSelectionKind::Derived && !ir.hasDerivedSelection(resolver->postBaseForms.id)) ||
                        resolver->outputAnchorSelection == kScriptShapingSelectionInvalid ||
                        !ir.hasDerivedSelection(resolver->outputAnchorSelection) ||
                        resolver->raKind == kScriptItemKindInvalid ||
                        resolver->consonantKind == kScriptItemKindInvalid ||
                        resolver->nuktaKind == kScriptItemKindInvalid ||
                        resolver->halantKind == kScriptItemKindInvalid ||
                        resolver->zwjKind == kScriptItemKindInvalid ||
                        resolver->zwnjKind == kScriptItemKindInvalid) return false;
                    break;
                }

                case ScriptShapingIROp::MoveSelection:
                {
                    const ScriptShapingIRMoveSelection* move =
                        ir.moveSelection(instruction.payloadIndex);

                    if (!move ||
                        !move->move.valid() ||
                        !move->anchor.valid() ||
                        move->placement == ScriptShapingIRMovePlacement::Invalid)
                    {
                        return false;
                    }

                    if (move->move.kind == ScriptShapingSelectionKind::Derived &&
                        !ir.hasDerivedSelection(move->move.id))
                    {
                        return false;
                    }

                    if (move->anchor.kind == ScriptShapingSelectionKind::Derived &&
                        !ir.hasDerivedSelection(move->anchor.id))
                    {
                        return false;
                    }

                    break;
                }

                case ScriptShapingIROp::GsubFeatureStage:
                case ScriptShapingIROp::GposFeatureStage:
                {
                    const ScriptShapingIRFeatureStage* stage =
                        ir.featureStage(instruction.payloadIndex);

                    if (!stage)
                        return false;

                    if (stage->featureOffset > ir.featureTags.size())
                        return false;

                    if (stage->featureCount >
                        ir.featureTags.size() - stage->featureOffset)
                    {
                        return false;
                    }

                    if (stage->includeRequiredFeature > 1)
                        return false;

                    if (instruction.op == ScriptShapingIROp::GposFeatureStage)
                    {
                        if (stage->inputSelection.valid())
                            return false;

                        if (stage->outputChangedSelection != kScriptShapingSelectionInvalid)
                            return false;
                    }

                    if (stage->inputSelection.kind ==
                        ScriptShapingSelectionKind::Derived)
                    {
                        if (!ir.hasDerivedSelection(
                            static_cast<ScriptShapingSelectionId>(
                                stage->inputSelection.id)))
                        {
                            return false;
                        }
                    }

                    if (stage->outputChangedSelection !=
                        kScriptShapingSelectionInvalid)
                    {
                        if (!ir.hasDerivedSelection(
                            stage->outputChangedSelection))
                        {
                            return false;
                        }
                    }

                    break;
                }

                default:
                    return false;
                }
            }

            return true;
        }


        std::vector<ScriptShapingIRBuilderInstruction> mInstructions{};
        uint32_t mDerivedSelectionCount{ 0 };
    };

} // namespace waavs