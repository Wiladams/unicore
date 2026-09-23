// test_script_shaping_move_selection.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <vector>

#include "script_shaping_ir_builder.h"
#include "script_shaping_ir_executor.h"
#include "script_shaping_selection.h"

namespace waavs
{
    static inline OpenTypeShapingGlyph makeMoveSelectionTestGlyph(
        uint32_t glyphId,
        uint32_t scalarOffset,
        uint32_t scalarCount = 1)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;
        return glyph;
    }


    static inline bool moveSelectionTestGlyphIds(
        const OpenTypeShapingBuffer& buffer,
        std::initializer_list<uint32_t> expected)
    {
        if (buffer.size() != expected.size())
            return false;

        size_t index = 0;

        for (uint32_t glyphId : expected)
        {
            if (buffer[index].glyphId != glyphId)
                return false;

            ++index;
        }

        return true;
    }


    static inline bool testScriptShapingMoveSelection()
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping MoveSelection: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ------------------------------------------------------------
        // Minimal recognition vocabulary.
        //
        // Source-domain layout:
        //
        //     0  A
        //     1  B
        //     2  C
        //     3  D
        //
        // Roles:
        //
        //     Move   -> B
        //     Anchor -> D
        // ------------------------------------------------------------

        static constexpr ScriptRoleId kMoveRole = 1;
        static constexpr ScriptRoleId kAnchorRole = 2;

        ScriptRecognitionResult recognition;
        recognition.kinds.resize(4, 1);

        recognition.roles.push_back({
            kMoveRole,
            0,
            { 1, 1 }
            });

        recognition.roles.push_back({
            kAnchorRole,
            0,
            { 3, 1 }
            });

        ScriptRecognitionUnit unit{};
        unit.span = { 0, 4 };
        unit.roleOffset = 0;
        unit.roleCount = 2;

        recognition.units.push_back(unit);


        // ------------------------------------------------------------
        // Before.
        //
        //     A B C D
        //       ^   ^
        //       |   |
        //      Move Anchor
        //
        // ->  A C B D
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(20, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));
            buffer.pushBack(makeMoveSelectionTestGlyph(40, 3));

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingRoleSelection(kMoveRole);
            move.anchor = scriptShapingRoleSelection(kAnchorRole);
            move.placement = ScriptShapingIRMovePlacement::Before;

            ScriptShapingSelectionState state;

            if (!applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("Before execution failed");
            }

            if (!moveSelectionTestGlyphIds(
                buffer,
                { 10, 30, 20, 40 }))
            {
                return fail("Before produced wrong glyph order");
            }
        }


        // ------------------------------------------------------------
        // After.
        //
        //     A B C D
        //
        // Move B after D:
        //
        // ->  A C D B
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(20, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));
            buffer.pushBack(makeMoveSelectionTestGlyph(40, 3));

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingRoleSelection(kMoveRole);
            move.anchor = scriptShapingRoleSelection(kAnchorRole);
            move.placement = ScriptShapingIRMovePlacement::After;

            ScriptShapingSelectionState state;

            if (!applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("After execution failed");
            }

            if (!moveSelectionTestGlyphIds(
                buffer,
                { 10, 30, 40, 20 }))
            {
                return fail("After produced wrong glyph order");
            }
        }


        // ------------------------------------------------------------
        // Multi-glyph semantic selection.
        //
        // Two glyphs share source scalar 1:
        //
        //     A B0 B1 C D
        //
        // Role Move still refers only to source scalar 1.
        //
        // Move before D:
        //
        // ->  A C B0 B1 D
        //
        // This proves source provenance expands naturally to multiple glyphs.
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(20, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(21, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));
            buffer.pushBack(makeMoveSelectionTestGlyph(40, 3));

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingRoleSelection(kMoveRole);
            move.anchor = scriptShapingRoleSelection(kAnchorRole);
            move.placement = ScriptShapingIRMovePlacement::Before;

            ScriptShapingSelectionState state;

            if (!applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("multi-glyph movement failed");
            }

            if (!moveSelectionTestGlyphIds(
                buffer,
                { 10, 30, 20, 21, 40 }))
            {
                return fail("multi-glyph movement lost order");
            }
        }


        // ------------------------------------------------------------
        // Derived selection.
        //
        // Derived selection 1 points at source scalar 1.
        //
        // Use that instead of the Move role.
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(20, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));
            buffer.pushBack(makeMoveSelectionTestGlyph(40, 3));

            ScriptShapingSelectionState state;
            state.selections.resize(1);

            const ScriptSpan span{ 1, 1 };

            if (!assignScriptShapingDerivedSelection(
                state,
                1,
                &span,
                1))
            {
                return fail("unable to assign derived selection");
            }

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingDerivedSelection(1);
            move.anchor = scriptShapingRoleSelection(kAnchorRole);
            move.placement = ScriptShapingIRMovePlacement::Before;

            if (!applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("derived movement failed");
            }

            if (!moveSelectionTestGlyphIds(
                buffer,
                { 10, 30, 20, 40 }))
            {
                return fail("derived movement produced wrong glyph order");
            }
        }


        // ------------------------------------------------------------
        // Post-GSUB provenance.
        //
        // Simulate:
        //
        //     source scalar 1
        //         ->
        //     replacement glyph spanning scalar 1
        //
        // Selection resolution must still find it after topology changes.
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(200, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));
            buffer.pushBack(makeMoveSelectionTestGlyph(40, 3));

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingRoleSelection(kMoveRole);
            move.anchor = scriptShapingRoleSelection(kAnchorRole);
            move.placement = ScriptShapingIRMovePlacement::After;

            ScriptShapingSelectionState state;

            if (!applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("post-GSUB provenance movement failed");
            }

            if (!moveSelectionTestGlyphIds(
                buffer,
                { 10, 30, 40, 200 }))
            {
                return fail("post-GSUB provenance was not preserved");
            }
        }


        // ------------------------------------------------------------
        // Empty derived selection is a legal no-op.
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(20, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));
            buffer.pushBack(makeMoveSelectionTestGlyph(40, 3));

            ScriptShapingSelectionState state;
            state.selections.resize(1);

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingDerivedSelection(1);
            move.anchor = scriptShapingRoleSelection(kAnchorRole);
            move.placement = ScriptShapingIRMovePlacement::Before;

            if (!applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("empty selection was not accepted as no-op");
            }

            if (!moveSelectionTestGlyphIds(
                buffer,
                { 10, 20, 30, 40 }))
            {
                return fail("empty selection changed glyph order");
            }
        }


        // ------------------------------------------------------------
        // Overlap rejection.
        //
        // Both selections resolve to source scalar 1.
        // ------------------------------------------------------------

        {
            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeMoveSelectionTestGlyph(10, 0));
            buffer.pushBack(makeMoveSelectionTestGlyph(20, 1));
            buffer.pushBack(makeMoveSelectionTestGlyph(30, 2));

            ScriptShapingSelectionState state;

            ScriptShapingIRMoveSelection move{};
            move.move = scriptShapingRoleSelection(kMoveRole);
            move.anchor = scriptShapingRoleSelection(kMoveRole);
            move.placement = ScriptShapingIRMovePlacement::Before;

            if (applyScriptShapingIRMoveSelection(
                move,
                recognition,
                recognition.units[0],
                state,
                buffer))
            {
                return fail("overlapping selections were accepted");
            }
        }


        std::printf(
            "Script shaping MoveSelection: PASS\n"
            "  Before:                   PASS\n"
            "  After:                    PASS\n"
            "  Multi-glyph selection:    PASS\n"
            "  Derived selection:        PASS\n"
            "  Post-GSUB provenance:     PASS\n"
            "  Empty selection no-op:    PASS\n"
            "  Overlap rejection:        PASS\n");

        return true;
    }

} // namespace waavs