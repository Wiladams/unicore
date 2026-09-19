// test_script_shaping_policy_compiler.h
#pragma once

#include "test_core.h"

#include "script_shaping_policy_compiler.h"
#include "opentype_types.h"

namespace waavs
{
    static inline bool runScriptShapingPolicyCompilerGeneric()
    {
        ScriptShapingIR ir;

        if (!compileScriptShapingIR(kOpenTypeGenericShapingPolicy, ir))
            return false;

        if (ir.instructions.size() != 3)
            return false;

        if (ir.scalarReplacements.size() != 0)
            return false;

        if (ir.scalarReplacementValues.size() != 0)
            return false;

        if (ir.featureStages.size() != 3)
            return false;

        if (ir.featureTags.size() != 6)
            return false;


        // ------------------------------------------------------------
        // Instruction ordering.
        // ------------------------------------------------------------

        if (ir.instructions[0].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (ir.instructions[1].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (ir.instructions[2].op != ScriptShapingIROp::GposFeatureStage)
            return false;


        // ------------------------------------------------------------
        // Stage payloads.
        // ------------------------------------------------------------

        const ScriptShapingIRFeatureStage* stage0 =
            ir.featureStage(ir.instructions[0].payloadIndex);

        const ScriptShapingIRFeatureStage* stage1 =
            ir.featureStage(ir.instructions[1].payloadIndex);

        const ScriptShapingIRFeatureStage* stage2 =
            ir.featureStage(ir.instructions[2].payloadIndex);

        if (!stage0 || !stage1 || !stage2)
            return false;

        if (stage0->featureOffset != 0 ||
            stage0->featureCount != 2 ||
            stage0->includeRequiredFeature != 1)
        {
            return false;
        }

        if (stage1->featureOffset != 2 ||
            stage1->featureCount != 1 ||
            stage1->includeRequiredFeature != 0)
        {
            return false;
        }

        if (stage2->featureOffset != 3 ||
            stage2->featureCount != 3 ||
            stage2->includeRequiredFeature != 1)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Feature contents.
        // ------------------------------------------------------------

        if (ir.featureTag(*stage0, 0) != OTAG("ccmp"))
            return false;

        if (ir.featureTag(*stage0, 1) != OTAG("locl"))
            return false;

        if (ir.featureTag(*stage1, 0) != OTAG("rlig"))
            return false;

        if (ir.featureTag(*stage2, 0) != OTAG("kern"))
            return false;

        if (ir.featureTag(*stage2, 1) != OTAG("mark"))
            return false;

        if (ir.featureTag(*stage2, 2) != OTAG("mkmk"))
            return false;

        return true;
    }


    static inline bool runScriptShapingPolicyCompilerLatin()
    {
        ScriptShapingIR ir;

        if (!compileScriptShapingIR(kOpenTypeLatinShapingPolicy, ir))
            return false;

        if (ir.instructions.size() != 4)
            return false;

        if (ir.scalarReplacements.size() != 0)
            return false;

        if (ir.scalarReplacementValues.size() != 0)
            return false;

        if (ir.featureStages.size() != 4)
            return false;

        if (ir.featureTags.size() != 9)
            return false;


        // ------------------------------------------------------------
        // Instruction ordering.
        // ------------------------------------------------------------

        if (ir.instructions[0].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (ir.instructions[1].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (ir.instructions[2].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (ir.instructions[3].op != ScriptShapingIROp::GposFeatureStage)
            return false;


        // ------------------------------------------------------------
        // Stage payloads.
        // ------------------------------------------------------------

        const ScriptShapingIRFeatureStage* stage0 =
            ir.featureStage(ir.instructions[0].payloadIndex);

        const ScriptShapingIRFeatureStage* stage1 =
            ir.featureStage(ir.instructions[1].payloadIndex);

        const ScriptShapingIRFeatureStage* stage2 =
            ir.featureStage(ir.instructions[2].payloadIndex);

        const ScriptShapingIRFeatureStage* stage3 =
            ir.featureStage(ir.instructions[3].payloadIndex);

        if (!stage0 || !stage1 || !stage2 || !stage3)
            return false;

        if (stage0->featureOffset != 0 ||
            stage0->featureCount != 2 ||
            stage0->includeRequiredFeature != 1)
        {
            return false;
        }

        if (stage1->featureOffset != 2 ||
            stage1->featureCount != 1 ||
            stage1->includeRequiredFeature != 0)
        {
            return false;
        }

        if (stage2->featureOffset != 3 ||
            stage2->featureCount != 3 ||
            stage2->includeRequiredFeature != 0)
        {
            return false;
        }

        if (stage3->featureOffset != 6 ||
            stage3->featureCount != 3 ||
            stage3->includeRequiredFeature != 1)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Feature contents.
        // ------------------------------------------------------------

        if (ir.featureTag(*stage0, 0) != OTAG("ccmp"))
            return false;

        if (ir.featureTag(*stage0, 1) != OTAG("locl"))
            return false;

        if (ir.featureTag(*stage1, 0) != OTAG("rlig"))
            return false;

        if (ir.featureTag(*stage2, 0) != OTAG("liga"))
            return false;

        if (ir.featureTag(*stage2, 1) != OTAG("clig"))
            return false;

        if (ir.featureTag(*stage2, 2) != OTAG("calt"))
            return false;

        if (ir.featureTag(*stage3, 0) != OTAG("kern"))
            return false;

        if (ir.featureTag(*stage3, 1) != OTAG("mark"))
            return false;

        if (ir.featureTag(*stage3, 2) != OTAG("mkmk"))
            return false;

        return true;
    }


    static inline bool runScriptShapingPolicyCompilerForScript()
    {
        ScriptShapingIR latin;
        ScriptShapingIR thai;

        if (!compileScriptShapingIRForScript(OTAG("latn"), latin))
            return false;

        if (!compileScriptShapingIRForScript(OTAG("thai"), thai))
            return false;


        // ------------------------------------------------------------
        // Latin remains the normal four-stage policy.
        // ------------------------------------------------------------

        if (latin.instructions.size() != 4)
            return false;

        if (latin.scalarReplacements.size() != 0)
            return false;

        if (latin.scalarReplacementValues.size() != 0)
            return false;


        // ------------------------------------------------------------
        // Thai now starts with the scalar-domain SARA AM rewrite.
        // ------------------------------------------------------------

        if (thai.instructions.size() != 5)
            return false;

        if (thai.scalarReplacements.size() != 1)
            return false;

        if (thai.scalarReplacementValues.size() != 2)
            return false;

        if (thai.featureStages.size() != 3)
            return false;

        if (thai.featureTags.size() != 6)
            return false;


        // ------------------------------------------------------------
        // Thai instruction ordering.
        // ------------------------------------------------------------

        if (thai.instructions[0].op != ScriptShapingIROp::ScalarReplace)
            return false;

        if (thai.instructions[1].op != ScriptShapingIROp::ScalarMoveLeftAcrossRange)
            return false;

        if (thai.instructions[2].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (thai.instructions[3].op != ScriptShapingIROp::GsubFeatureStage)
            return false;

        if (thai.instructions[4].op != ScriptShapingIROp::GposFeatureStage)
            return false;


        // ------------------------------------------------------------
        // Thai SARA AM replacement.
        // ------------------------------------------------------------

        const ScriptShapingIRScalarReplace* saraAm =
            thai.scalarReplace(thai.instructions[0].payloadIndex);

        if (!saraAm)
            return false;

        if (saraAm->inputValue != 0x0E33)
            return false;

        if (saraAm->replacementOffset != 0)
            return false;

        if (saraAm->replacementCount != 2)
            return false;

        if (thai.scalarReplacementValue(*saraAm, 0) != 0x0E4D)
            return false;

        if (thai.scalarReplacementValue(*saraAm, 1) != 0x0E32)
            return false;

        
        const ScriptShapingIRScalarMoveLeftAcrossRange* move =
            thai.scalarMoveLeftAcrossRange(thai.instructions[1].payloadIndex);

        if (!move)
            return false;

        if (move->targetValue != 0x0E4D)
            return false;

        if (move->requiredFlags != ScriptShapingItemFlagGenerated)
            return false;

        if (move->acrossFirst != 0x0E48)
            return false;

        if (move->acrossLast != 0x0E4B)
            return false;

        // ------------------------------------------------------------
        // Thai feature stages follow the scalar operation.
        // ------------------------------------------------------------

        const ScriptShapingIRFeatureStage* stage0 =
            thai.featureStage(thai.instructions[2].payloadIndex);

        const ScriptShapingIRFeatureStage* stage1 =
            thai.featureStage(thai.instructions[3].payloadIndex);

        const ScriptShapingIRFeatureStage* stage2 =
            thai.featureStage(thai.instructions[4].payloadIndex);

        if (!stage0 || !stage1 || !stage2)
            return false;

        if (stage0->featureCount != 2 ||
            stage0->includeRequiredFeature != 1)
        {
            return false;
        }

        if (stage1->featureCount != 1 ||
            stage1->includeRequiredFeature != 0)
        {
            return false;
        }

        if (stage2->featureCount != 3 ||
            stage2->includeRequiredFeature != 1)
        {
            return false;
        }

        if (thai.featureTag(*stage0, 0) != OTAG("ccmp"))
            return false;

        if (thai.featureTag(*stage0, 1) != OTAG("locl"))
            return false;

        if (thai.featureTag(*stage1, 0) != OTAG("rlig"))
            return false;

        if (thai.featureTag(*stage2, 0) != OTAG("kern"))
            return false;

        if (thai.featureTag(*stage2, 1) != OTAG("mark"))
            return false;

        if (thai.featureTag(*stage2, 2) != OTAG("mkmk"))
            return false;


        // ------------------------------------------------------------
        // Invalid script tag.
        // ------------------------------------------------------------

        ScriptShapingIR invalid;

        if (compileScriptShapingIRForScript(0, invalid))
            return false;

        return true;
    }


    static inline bool runScriptShapingPolicyCompiler()
    {
        if (!runScriptShapingPolicyCompilerGeneric())
            return false;

        if (!runScriptShapingPolicyCompilerLatin())
            return false;

        if (!runScriptShapingPolicyCompilerForScript())
            return false;

        return true;
    }


    static inline void testScriptShapingPolicyCompiler()
    {
        const bool passed = runScriptShapingPolicyCompiler();

        printf(
            "Script shaping policy compiler: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs