// test_script_shaping_glyph_selection.h
#pragma once

#include "test_core.h"

#include "script_shaping_glyph_selection.h"

#include <cstdio>

namespace waavs
{
    static bool testScriptShapingGlyphSelection()
    {
        ScriptRecognitionResult recognition;

        recognition.roles.push_back({
            ScriptRoleId(1),
            0,
            { 1, 2 }
            });

        ScriptRecognitionUnit unit{};

        unit.span = { 0, 4 };
        unit.roleOffset = 0;
        unit.roleCount = 1;


        OpenTypeShapingBuffer buffer;

        OpenTypeShapingGlyph glyph{};

        glyph.glyphId = 10;
        glyph.scalarOffset = 0;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);

        glyph = {};
        glyph.glyphId = 11;
        glyph.scalarOffset = 1;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);

        glyph = {};
        glyph.glyphId = 12;
        glyph.scalarOffset = 3;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);

        glyph = {};
        glyph.glyphId = 13;
        glyph.scalarOffset = 2;
        glyph.scalarCount = 1;
        buffer.pushBack(glyph);


        ScriptShapingSelectionState state;

        if (!state.reset(1))
            return false;


        // ------------------------------------------------------------
        // Current unit [0,4) resolves all four glyphs despite glyph order.
        // ------------------------------------------------------------

        ScriptShapingResolvedGlyphSelection resolved;

        if (!resolveScriptShapingGlyphSelection(
            scriptShapingUnitSelection(),
            recognition, unit, state, buffer, resolved))
        {
            return false;
        }

        if (resolved.size() != 4 ||
            resolved.glyphIndices[0] != 0 ||
            resolved.glyphIndices[1] != 1 ||
            resolved.glyphIndices[2] != 2 ||
            resolved.glyphIndices[3] != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Role selection [1,3) should resolve to glyphs 1 and 3.
        // ------------------------------------------------------------

        const ScriptShapingSelectionRef role =
            scriptShapingRoleSelection(
                ScriptRoleId(1));

        if (!resolveScriptShapingGlyphSelection(
            role,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (resolved.size() != 2 ||
            resolved.glyphIndices[0] != 1 ||
            resolved.glyphIndices[1] != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Derived selection containing source scalars 0 and 3.
        // ------------------------------------------------------------

        ScriptShapingDerivedSelection* derived =
            state.selection(
                ScriptShapingSelectionId(1));

        if (!derived)
            return false;

        derived->sourceSpans.push_back({ 0, 1 });
        derived->sourceSpans.push_back({ 3, 1 });


        const ScriptShapingSelectionRef derivedRef =
            scriptShapingDerivedSelection(
                ScriptShapingSelectionId(1));

        if (!resolveScriptShapingGlyphSelection(
            derivedRef,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (resolved.size() != 2 ||
            resolved.glyphIndices[0] != 0 ||
            resolved.glyphIndices[1] != 2)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Missing optional role remains valid and empty.
        // ------------------------------------------------------------

        const ScriptShapingSelectionRef missing =
            scriptShapingRoleSelection(
                ScriptRoleId(2));

        if (!resolveScriptShapingGlyphSelection(
            missing,
            recognition,
            unit,
            state,
            buffer,
            resolved))
        {
            return false;
        }

        if (!resolved.empty())
            return false;


        std::printf(
            "Script shaping glyph selection: PASS\n"
            "  Unit resolution:       PASS\n"
            "  Role resolution:       PASS\n"
            "  Derived resolution:    PASS\n"
            "  Missing role:          PASS\n");

        return true;
    }

} // namespace waavs