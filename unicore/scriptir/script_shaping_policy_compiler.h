// script_shaping_policy_compiler.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_shaping_policy.h"
#include "script_shaping_ir_builder.h"

namespace waavs
{
    // ========================================================================
    // appendOpenTypeShapingPolicy
    //
    // Append the current declarative OpenType shaping policy to an existing
    // ScriptShapingIRBuilder.
    //
    // This deliberately does not finalize the builder. Script-specific
    // semantic operations may be added before or after these feature stages.
    // ========================================================================

    [[nodiscard]]
    static inline bool appendOpenTypeShapingPolicy(
        const OpenTypeShapingPolicy& policy,
        ScriptShapingIRBuilder& builder)
    {
        if ((policy.gsubStageCount != 0 && !policy.gsubStages) ||
            (policy.gposStageCount != 0 && !policy.gposStages))
        {
            return false;
        }

        for (size_t i = 0; i < policy.gsubStageCount; ++i)
        {
            const OpenTypeShapingFeatureStage& stage = policy.gsubStages[i];

            if (stage.featureTagCount != 0 && !stage.featureTags)
                return false;

            if (!builder.addGsubFeatureStage(
                stage.featureTags,
                stage.featureTagCount,
                stage.includeRequiredFeature))
            {
                return false;
            }
        }

        for (size_t i = 0; i < policy.gposStageCount; ++i)
        {
            const OpenTypeShapingFeatureStage& stage = policy.gposStages[i];

            if (stage.featureTagCount != 0 && !stage.featureTags)
                return false;

            if (!builder.addGposFeatureStage(
                stage.featureTags,
                stage.featureTagCount,
                stage.includeRequiredFeature))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // compileScriptShapingIR
    //
    // Compile a plain OpenTypeShapingPolicy into finalized ScriptShapingIR.
    //
    // This remains useful for generic policy-only compilation and preserves
    // the previous behavior exactly.
    // ========================================================================

    [[nodiscard]]
    static inline bool compileScriptShapingIR(
        const OpenTypeShapingPolicy& policy,
        ScriptShapingIR& result)
    {
        ScriptShapingIRBuilder builder;

        if (!appendOpenTypeShapingPolicy(policy, builder))
            return false;

        return builder.finalize(result);
    }


    // ========================================================================
    // compileScriptShapingIRForScript
    //
    // Compile the complete semantic shaping program for one script.
    //
    // Script-specific scalar-domain semantics are emitted first, followed by
    // the script's normal ordered OpenType feature stages.
    //
    // Current script-specific semantics:
    //
    //   Thai:
    //
    //       U+0E33 SARA AM
    //           ->
    //       U+0E4D NIKHAHIT
    //       U+0E32 SARA AA
    //
    // Both replacement items inherit the source provenance of the original
    // scalar at execution time.
    // ========================================================================

    [[nodiscard]]
    static inline bool compileScriptShapingIRForScript(
        uint32_t scriptTag,
        ScriptShapingIR& result)
    {
        if (scriptTag == 0)
            return false;

        ScriptShapingIRBuilder builder;


        // ------------------------------------------------------------
        // Script-specific scalar-domain semantics.
        // ------------------------------------------------------------

        if (scriptTag == OTAG("thai"))
        {
            static constexpr uint32_t saraAmReplacement[] =
            {
                0x0E4D,
                0x0E32
            };

            if (!builder.addScalarReplace(
                0x0E33,
                saraAmReplacement))
            {
                return false;
            }

            if (!builder.addScalarMoveLeftAcrossRange(
                0x0E4D,
                ScriptShapingItemFlagGenerated,
                0x0E48,
                0x0E4B))
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Ordered OpenType feature stages.
        // ------------------------------------------------------------

        const OpenTypeShapingPolicy& policy =
            openTypeShapingPolicyForScript(scriptTag);

        if (!appendOpenTypeShapingPolicy(policy, builder))
            return false;

        return builder.finalize(result);
    }

} // namespace waavs