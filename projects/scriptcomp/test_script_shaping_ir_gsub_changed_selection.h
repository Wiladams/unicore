// test_script_shaping_ir_gsub_changed_selection.h
#pragma once

#include "test_core.h"

#include <cstdio>

#include "script_shaping_ir_executor.h"

namespace waavs
{
    static bool testScriptShapingIRGsubChangedSelection()
    {
        static constexpr ScriptRoleId kSelectedRole = 1;
        static constexpr ScriptShapingSelectionId kChangedSelection = 1;


        // ------------------------------------------------------------
        // Recognition:
        //
        //     source 0    source 1    source 2
        //       10          10          10
        //                    ^
        //                 Role(1)
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
        // One persistent derived selection.
        // ------------------------------------------------------------

        ScriptShapingSelectionState selectionState;

        if (!selectionState.reset(1))
            return false;


        // ------------------------------------------------------------
        // Initial glyph buffer.
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


        // ============================================================
        // Stage 1
        //
        //     Role(1)
        //         |
        //         v
        //       10 -> 11 12
        //
        // Capture the source provenance of changed glyphs into
        // DerivedSelection(1).
        // ============================================================

        OpenTypeShapingIRPlan stage1Plan;

        stage1Plan.ir.gsubMultipleGlyphs.push_back(11);
        stage1Plan.ir.gsubMultipleGlyphs.push_back(12);

        stage1Plan.ir.gsubMultipleSequences.push_back({
            0,
            2
            });

        OpenTypeShapingIRGsubMultiplePair multiplePair{};
        multiplePair.input = 10;
        multiplePair.sequenceIndex = 0;

        stage1Plan.ir.gsubMultiplePairs.push_back(multiplePair);

        stage1Plan.ir.gsubMultipleSubtables.push_back({
            0,
            1
            });

        OpenTypeShapingIRLookup multipleLookup{};
        multipleLookup.op = OpenTypeShapingIROp::GsubMultiple;
        multipleLookup.payloadOffset = 0;
        multipleLookup.payloadCount = 1;

        stage1Plan.ir.lookups.push_back(multipleLookup);
        stage1Plan.lookups.push_back(0);


        ScriptShapingIRFeatureStage stage1{};
        stage1.inputSelection =
            scriptShapingRoleSelection(kSelectedRole);

        stage1.outputChangedSelection =
            kChangedSelection;


        if (!applyScriptShapingIRGsubPlan(
            stage1Plan,
            stage1,
            recognition,
            unit,
            selectionState,
            buffer))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Stage 1 result:
        //
        //     [10] [11] [12] [10]
        //
        // Only source scalar 1 changed.
        // ------------------------------------------------------------

        if (buffer.size() != 4)
            return false;

        if (buffer[0].glyphId != 10 ||
            buffer[1].glyphId != 11 ||
            buffer[2].glyphId != 12 ||
            buffer[3].glyphId != 10)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Verify Derived(1) resolves to both descendants of source 1.
        // ------------------------------------------------------------

        ScriptShapingResolvedGlyphSelection changed;

        if (!resolveScriptShapingGlyphSelection(
            scriptShapingDerivedSelection(kChangedSelection),
            recognition,
            unit,
            selectionState,
            buffer,
            changed))
        {
            return false;
        }

        if (changed.size() != 2 ||
            changed.glyphIndices[0] != 1 ||
            changed.glyphIndices[1] != 2)
        {
            return false;
        }


        // ============================================================
        // Stage 2
        //
        // Input is no longer Role(1).
        //
        // It is the semantic result of Stage 1:
        //
        //     DerivedSelection(1)
        //
        //     11 -> 21
        //     12 -> 22
        // ============================================================

        OpenTypeShapingIRPlan stage2Plan;

        stage2Plan.ir.gsubSinglePairs.push_back({
            11,
            21
            });

        stage2Plan.ir.gsubSinglePairs.push_back({
            12,
            22
            });

        stage2Plan.ir.gsubSingleSubtables.push_back({
            0,
            2
            });

        OpenTypeShapingIRLookup singleLookup{};
        singleLookup.op = OpenTypeShapingIROp::GsubSingle;
        singleLookup.payloadOffset = 0;
        singleLookup.payloadCount = 1;

        stage2Plan.ir.lookups.push_back(singleLookup);
        stage2Plan.lookups.push_back(0);


        ScriptShapingIRFeatureStage stage2{};
        stage2.inputSelection =
            scriptShapingDerivedSelection(kChangedSelection);


        if (!applyScriptShapingIRGsubPlan(
            stage2Plan,
            stage2,
            recognition,
            unit,
            selectionState,
            buffer))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Final result:
        //
        //     source 0       source 1        source 2
        //
        //       10          21  22            10
        //
        // Only descendants of the changed source are eligible for
        // Stage 2.
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
        // Provenance must still be intact.
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
        // The derived selection remains semantic and resolves against the
        // final glyph topology as well.
        // ------------------------------------------------------------

        changed.clear();

        if (!resolveScriptShapingGlyphSelection(
            scriptShapingDerivedSelection(kChangedSelection),
            recognition,
            unit,
            selectionState,
            buffer,
            changed))
        {
            return false;
        }

        if (changed.size() != 2 ||
            changed.glyphIndices[0] != 1 ||
            changed.glyphIndices[1] != 2)
        {
            return false;
        }


        std::printf(
            "Script shaping IR GSUB changed selection: PASS\n"
            "  Role input selection:        PASS\n"
            "  Stage 1 GSUB:                PASS\n"
            "  Changed-output capture:      PASS\n"
            "  Derived selection resolve:   PASS\n"
            "  Stage 2 derived input:       PASS\n"
            "  Unselected glyphs preserved: PASS\n"
            "  Provenance preserved:        PASS\n");

        return true;
    }

} // namespace waavs