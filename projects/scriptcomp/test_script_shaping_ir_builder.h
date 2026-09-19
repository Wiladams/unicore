// test_script_shaping_ir_builder.h
#pragma once

#include "test_core.h"

#include "script_shaping_ir_builder.h"
#include "opentype_types.h"

namespace waavs
{
    static inline bool runScriptShapingIRBuilder()
    {
        static constexpr uint32_t gsubStage0[] =
        {
            OTAG("ccmp"),
            OTAG("locl")
        };

        static constexpr uint32_t gsubStage1[] =
        {
            OTAG("rlig")
        };

        static constexpr uint32_t gposStage0[] =
        {
            OTAG("kern"),
            OTAG("mark"),
            OTAG("mkmk")
        };

        ScriptShapingIRBuilder builder;

        if (!builder.empty())
            return false;

        if (builder.size() != 0)
            return false;

        if (!builder.addGsubFeatureStage(gsubStage0, true))
            return false;

        if (!builder.addGsubFeatureStage(gsubStage1))
            return false;

        if (!builder.addGposFeatureStage(gposStage0, true))
            return false;

        if (builder.empty())
            return false;

        if (builder.size() != 3)
            return false;


        ScriptShapingIR ir;

        if (!builder.finalize(ir))
            return false;

        if (ir.empty())
            return false;

        if (ir.size() != 3)
            return false;

        if (ir.instructions.size() != 3)
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

        if (ir.instructions[0].payloadIndex != 0)
            return false;

        if (ir.instructions[1].payloadIndex != 1)
            return false;

        if (ir.instructions[2].payloadIndex != 2)
            return false;


        // ------------------------------------------------------------
        // Feature-stage packing.
        // ------------------------------------------------------------

        const ScriptShapingIRFeatureStage* stage0 = ir.featureStage(0);
        const ScriptShapingIRFeatureStage* stage1 = ir.featureStage(1);
        const ScriptShapingIRFeatureStage* stage2 = ir.featureStage(2);

        if (!stage0 || !stage1 || !stage2)
            return false;

        if (stage0->featureOffset != 0 || stage0->featureCount != 2 || stage0->includeRequiredFeature != 1)
            return false;

        if (stage1->featureOffset != 2 || stage1->featureCount != 1 || stage1->includeRequiredFeature != 0)
            return false;

        if (stage2->featureOffset != 3 || stage2->featureCount != 3 || stage2->includeRequiredFeature != 1)
            return false;


        // ------------------------------------------------------------
        // Feature-tag contents.
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


        // ------------------------------------------------------------
        // Bounds behavior.
        // ------------------------------------------------------------

        if (ir.featureStage(3) != nullptr)
            return false;

        if (ir.featureTag(*stage0, 2) != 0)
            return false;


        // ------------------------------------------------------------
        // Finalization is repeatable and does not consume the builder.
        // ------------------------------------------------------------

        ScriptShapingIR second;

        if (!builder.finalize(second))
            return false;

        if (second.instructions.size() != ir.instructions.size())
            return false;

        if (second.featureStages.size() != ir.featureStages.size())
            return false;

        if (second.featureTags != ir.featureTags)
            return false;


        // ------------------------------------------------------------
        // clear().
        // ------------------------------------------------------------

        builder.clear();

        if (!builder.empty())
            return false;

        if (builder.size() != 0)
            return false;

        ScriptShapingIR emptyIR;

        if (!builder.finalize(emptyIR))
            return false;

        if (!emptyIR.empty())
            return false;


        // ------------------------------------------------------------
        // Failure-path checks.
        // ------------------------------------------------------------

        ScriptShapingIRBuilder invalidBuilder;

        if (invalidBuilder.addGsubFeatureStage(nullptr, 1))
            return false;

        if (invalidBuilder.addGposFeatureStage(nullptr, 1))
            return false;


        return true;
    }


    static inline void testScriptShapingIRBuilder()
    {
        const bool passed = runScriptShapingIRBuilder();

        printf(
            "Script shaping IR builder: %s\n",
            passed ? "PASS" : "FAIL");

    }

} // namespace waavs