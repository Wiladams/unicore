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
        // GSUB feature stages
        // ====================================================================

        [[nodiscard]]
        bool addGsubFeatureStage(const uint32_t* tags, size_t count, bool includeRequiredFeature = false)
        {
            return addFeatureStage(
                ScriptShapingIROp::GsubFeatureStage,
                tags,
                count,
                includeRequiredFeature);
        }

        template<size_t N>
        [[nodiscard]]
        bool addGsubFeatureStage(const uint32_t(&tags)[N], bool includeRequiredFeature = false)
        {
            return addGsubFeatureStage(tags, N, includeRequiredFeature);
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
                includeRequiredFeature);
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
            bool includeRequiredFeature)
        {
            if (!isScriptShapingIRFeatureStage(op))
                return false;

            if (count != 0 && !tags)
                return false;

            if (count > std::numeric_limits<uint32_t>::max())
                return false;

            ScriptShapingIRBuilderInstruction instruction;

            instruction.op = op;
            instruction.includeRequiredFeature = includeRequiredFeature;

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

                    break;
                }

                default:
                    return false;
                }
            }

            return true;
        }


        std::vector<ScriptShapingIRBuilderInstruction> mInstructions{};
    };

} // namespace waavs