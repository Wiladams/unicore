// test_script_shaping_ir_gsub_selection.h
#pragma once

#include "test_core.h"

#include <cstdio>

#include "script_shaping_ir_executor.h"

namespace waavs
{
    static bool testScriptShapingIRGsubSelection()
    {
        static constexpr ScriptRoleId kSelectedRole = 1;


        // ------------------------------------------------------------
        // Build a recognition result:
        //
        //     source scalars:   0   1   2
        //                           ^
        //                      selected role
        //
        // The unit owns all three source scalars, but Role(1) owns only
        // source scalar 1.
        // ------------------------------------------------------------

        ScriptRecognitionResult recognition;

        recognition.roles.push_back({
            kSelectedRole,
            0,
            { 1, 1 }
            });

        ScriptRecognitionUnit unit{};
        unit.span = { 0, 3 };
        unit.roleOffset = 0;
        unit.roleCount = 1;


        // ------------------------------------------------------------
        // No derived selections are required for this test.
        // ------------------------------------------------------------

        ScriptShapingSelectionState selectionState;

        if (!selectionState.reset(0))
            return false;


        // ------------------------------------------------------------
        // Build the Script feature stage.
        //
        // Its only semantic constraint is:
        //
        //     inputSelection = Role(1)
        //
        // We call applyScriptShapingIRGsubPlan() directly, so feature tags
        // are irrelevant here. Feature selection itself is already tested
        // independently; this test targets the Script -> GSUB execution seam.
        // ------------------------------------------------------------

        ScriptShapingIRFeatureStage stage{};
        stage.inputSelection =
            scriptShapingRoleSelection(kSelectedRole);


        // ------------------------------------------------------------
        // Build a two-root GSUB plan.
        //
        // Root 0:
        //
        //     10 -> 11 12
        //
        // Root 1:
        //
        //     11 -> 21
        //     12 -> 22
        //
        // Initial buffer:
        //
        //     10 10 10
        //        ^
        //        selected by Role(1)
        //
        // After root 0:
        //
        //     10 11 12 10
        //        \___/
        //          |
        //       same source provenance
        //
        // Before root 1 the semantic selection must be resolved again,
        // yielding BOTH descendants 11 and 12.
        //
        // Final:
        //
        //     10 21 22 10
        // ------------------------------------------------------------

        OpenTypeShapingIRPlan plan;


        // ------------------------------------------------------------
        // Root lookup 0: MultipleSubst 10 -> {11, 12}
        // ------------------------------------------------------------

        plan.ir.gsubMultipleGlyphs.push_back(11);
        plan.ir.gsubMultipleGlyphs.push_back(12);

        plan.ir.gsubMultipleSequences.push_back({
            0,
            2
            });

        OpenTypeShapingIRGsubMultiplePair multiplePair{};
        multiplePair.input = 10;
        multiplePair.sequenceIndex = 0;

        plan.ir.gsubMultiplePairs.push_back(multiplePair);

        plan.ir.gsubMultipleSubtables.push_back({
            0,
            1
            });

        OpenTypeShapingIRLookup multipleLookup{};
        multipleLookup.op = OpenTypeShapingIROp::GsubMultiple;
        multipleLookup.payloadOffset = 0;
        multipleLookup.payloadCount = 1;

        const OpenTypeShapingIRLookupId multipleLookupId =
            static_cast<OpenTypeShapingIRLookupId>(
                plan.ir.lookups.size());

        plan.ir.lookups.push_back(multipleLookup);
        plan.lookups.push_back(multipleLookupId);


        // ------------------------------------------------------------
        // Root lookup 1: SingleSubst
        //
        //     11 -> 21
        //     12 -> 22
        // ------------------------------------------------------------

        plan.ir.gsubSinglePairs.push_back({
            11,
            21
            });

        plan.ir.gsubSinglePairs.push_back({
            12,
            22
            });

        plan.ir.gsubSingleSubtables.push_back({
            0,
            2
            });

        OpenTypeShapingIRLookup singleLookup{};
        singleLookup.op = OpenTypeShapingIROp::GsubSingle;
        singleLookup.payloadOffset = 0;
        singleLookup.payloadCount = 1;

        const OpenTypeShapingIRLookupId singleLookupId =
            static_cast<OpenTypeShapingIRLookupId>(
                plan.ir.lookups.size());

        plan.ir.lookups.push_back(singleLookup);
        plan.lookups.push_back(singleLookupId);


        // ------------------------------------------------------------
        // Initial glyph buffer.
        //
        // All three glyphs are substitution candidates. Selection, not glyph
        // identity, must determine which one is allowed to start GSUB.
        // ------------------------------------------------------------

        OpenTypeShapingBuffer buffer;

        OpenTypeShapingGlyph glyph{};

        glyph.glyphId = 10;
        glyph.scalarOffset = 0;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);

        glyph = {};
        glyph.glyphId = 10;
        glyph.scalarOffset = 1;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);

        glyph = {};
        glyph.glyphId = 10;
        glyph.scalarOffset = 2;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);


        // ------------------------------------------------------------
        // Execute.
        // ------------------------------------------------------------

        if (!applyScriptShapingIRGsubPlan(
            plan,
            stage,
            recognition,
            unit,
            selectionState,
            buffer))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Validate final topology and glyph identity.
        // ------------------------------------------------------------

        if (buffer.size() != 4)
            return false;

        if (buffer[0].glyphId != 10 ||
            buffer[1].glyphId != 21 ||
            buffer[2].glyphId != 22 ||
            buffer[3].glyphId != 10)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Validate provenance.
        //
        // Both descendants of the selected original glyph must still map to
        // source scalar 1. The unselected neighbors retain their own source.
        // ------------------------------------------------------------

        if (buffer[0].scalarOffset != 0 ||
            buffer[0].scalarCount != 1)
        {
            return false;
        }

        if (buffer[1].scalarOffset != 1 ||
            buffer[1].scalarCount != 1 ||
            buffer[2].scalarOffset != 1 ||
            buffer[2].scalarCount != 1)
        {
            return false;
        }

        if (buffer[3].scalarOffset != 2 ||
            buffer[3].scalarCount != 1)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Resolve the semantic role one more time after all substitutions.
        //
        // It must still identify the two current descendants.
        // ------------------------------------------------------------

        ScriptShapingResolvedGlyphSelection resolved;

        if (!resolveScriptShapingGlyphSelection(
            stage.inputSelection,
            recognition,
            unit,
            selectionState,
            buffer,
            resolved))
        {
            return false;
        }

        if (resolved.size() != 2 ||
            resolved.glyphIndices[0] != 1 ||
            resolved.glyphIndices[1] != 2)
        {
            return false;
        }


        std::printf(
            "Script shaping IR GSUB selection: PASS\n"
            "  Role selection:             PASS\n"
            "  Selected GSUB start:        PASS\n"
            "  Unselected glyphs preserved: PASS\n"
            "  Topology change:            PASS\n"
            "  Selection re-resolution:    PASS\n"
            "  Provenance preserved:       PASS\n");

        return true;
    }

} // namespace waavs