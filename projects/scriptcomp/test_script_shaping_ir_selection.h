
// test_script_shaping_ir_selection.h
#pragma once

#include "test_core.h"

#include "script_shaping_ir_builder.h"

#include <cstdio>

namespace waavs
{
    static bool testScriptShapingIRSelection()
    {
        ScriptShapingIRBuilder builder;


        // ------------------------------------------------------------
        // Allocate shaping-derived semantic selections.
        //
        // Selection ids are owned by the finalized ScriptShapingIR.
        // ------------------------------------------------------------

        const ScriptShapingSelectionId unused =
            builder.addDerivedSelection();

        const ScriptShapingSelectionId reph =
            builder.addDerivedSelection();

        if (unused == kScriptShapingSelectionInvalid ||
            reph == kScriptShapingSelectionInvalid)
        {
            return false;
        }

        if (unused != 1 ||
            reph != 2)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Selection-aware GSUB stage.
        //
        // Apply rphf to recognition role 3 and record changed
        // descendants into derived selection "reph".
        // ------------------------------------------------------------

        static constexpr uint32_t rphf[] =
        {
            OTAG("rphf")
        };

        const ScriptShapingSelectionRef rephCandidate =
            scriptShapingRoleSelection(
                ScriptRoleId(3));

        if (!builder.addGsubFeatureStage(
            rphf,
            rephCandidate,
            reph))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Existing whole-buffer GSUB stages must continue to work.
        // ------------------------------------------------------------

        static constexpr uint32_t liga[] =
        {
            OTAG("liga")
        };

        if (!builder.addGsubFeatureStage(liga))
            return false;


        // ------------------------------------------------------------
        // Finalize.
        // ------------------------------------------------------------

        ScriptShapingIR ir;

        if (!builder.finalize(ir))
            return false;


        // ------------------------------------------------------------
        // Derived selection ownership.
        // ------------------------------------------------------------

        if (ir.derivedSelectionCount != 2)
            return false;

        if (!ir.hasDerivedSelection(unused) ||
            !ir.hasDerivedSelection(reph))
        {
            return false;
        }

        if (ir.hasDerivedSelection(
            ScriptShapingSelectionId(3)))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Two feature-stage instructions should have been emitted.
        // ------------------------------------------------------------

        if (ir.instructions.size() != 2 ||
            ir.featureStages.size() != 2)
        {
            return false;
        }


        // ------------------------------------------------------------
        // First stage:
        //
        //     rphf
        //     input  = Role(3)
        //     output = Derived(2)
        // ------------------------------------------------------------

        const ScriptShapingIRFeatureStage& selected =
            ir.featureStages[0];

        if (!selected.hasInputSelection())
            return false;

        if (selected.inputSelection.kind !=
            ScriptShapingSelectionKind::Role)
        {
            return false;
        }

        if (selected.inputSelection.id != 3)
            return false;

        if (!selected.hasOutputChangedSelection())
            return false;

        if (selected.outputChangedSelection != reph)
            return false;

        if (selected.featureCount != 1)
            return false;

        if (ir.featureTag(selected, 0) != OTAG("rphf"))
            return false;


        // ------------------------------------------------------------
        // Second stage:
        //
        //     liga
        //     input  = entire shaping buffer
        //     output = none
        // ------------------------------------------------------------

        const ScriptShapingIRFeatureStage& ordinary =
            ir.featureStages[1];

        if (ordinary.hasInputSelection())
            return false;

        if (ordinary.hasOutputChangedSelection())
            return false;

        if (ordinary.featureCount != 1)
            return false;

        if (ir.featureTag(ordinary, 0) != OTAG("liga"))
            return false;


        std::printf(
            "Script shaping IR selection: PASS\n"
            "  Derived selection allocation: PASS\n"
            "  Selection-aware GSUB input:   PASS\n"
            "  Changed-output selection:     PASS\n"
            "  Legacy whole-buffer GSUB:     PASS\n"
            "  Finalized selection refs:     PASS\n");

        return true;
    }

} // namespace waavs
