// test_script_shaping_indic_reordering.h
#pragma once

#include "test_core.h"

#include <cstdio>

#include "script_shaping_ir_executor.h"

namespace waavs
{
    static inline OpenTypeShapingGlyph makeIndicReorderGlyph(uint32_t glyphId, uint32_t scalarOffset, uint32_t scalarCount = 1)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;
        return glyph;
    }


    static bool testScriptShapingIndicReordering()
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping Indic reordering: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        static constexpr ScriptItemKindId kRa = 1;
        static constexpr ScriptItemKindId kConsonant = 2;
        static constexpr ScriptItemKindId kNukta = 3;
        static constexpr ScriptItemKindId kHalant = 4;
        static constexpr ScriptItemKindId kZWJ = 5;
        static constexpr ScriptItemKindId kZWNJ = 6;
        static constexpr ScriptItemKindId kMatra = 7;

        static constexpr ScriptRoleId kConsonantSequence = 1;
        static constexpr ScriptRoleId kBaseCandidate = 2;

        static constexpr ScriptShapingSelectionId kHalfCandidates = 1;
        static constexpr ScriptShapingSelectionId kBase = 2;
        static constexpr ScriptShapingSelectionId kInitialAnchor = 3;
        static constexpr ScriptShapingSelectionId kFinalAnchor = 4;
        static constexpr ScriptShapingSelectionId kReph = 5;
        static constexpr ScriptShapingSelectionId kPost = 6;
        static constexpr ScriptShapingSelectionId kRephAnchor = 7;


        // ------------------------------------------------------------
        // Half candidates: C H C H ZWNJ C
        //
        // First C is eligible. Second C is blocked by ZWNJ.
        // ------------------------------------------------------------

        {
            ScriptRecognitionResult recognition;
            recognition.kinds = { kConsonant, kHalant, kConsonant, kHalant, kZWNJ, kConsonant };
            recognition.roles.push_back({ kConsonantSequence, 0, { 0, 5 } });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 6 };
            unit.roleOffset = 0;
            unit.roleCount = 1;

            ScriptShapingSelectionState state;
            if (!state.reset(7)) return fail("unable to initialize selection state");

            ScriptShapingIRResolveIndicHalfCandidates resolver{};
            resolver.consonantSequence = scriptShapingRoleSelection(kConsonantSequence);
            resolver.outputSelection = kHalfCandidates;
            resolver.raKind = kRa;
            resolver.consonantKind = kConsonant;
            resolver.nuktaKind = kNukta;
            resolver.halantKind = kHalant;
            resolver.zwjKind = kZWJ;
            resolver.zwnjKind = kZWNJ;

            if (!applyScriptShapingIRResolveIndicHalfCandidates(resolver, recognition, unit, state))
                return fail("half-candidate resolution failed");

            const ScriptShapingDerivedSelection* half = state.selection(kHalfCandidates);

            if (!half || half->sourceSpans.size() != 1 ||
                half->sourceSpans[0].first != 0 || half->sourceSpans[0].count != 1)
            {
                return fail("ZWNJ did not block second half candidate");
            }
        }


        // ------------------------------------------------------------
        // Initial pre-base anchor uses first consonant in the sequence.
        // ------------------------------------------------------------

        {
            ScriptRecognitionResult recognition;
            recognition.kinds = { kConsonant, kHalant, kConsonant };
            recognition.roles.push_back({ kConsonantSequence, 0, { 0, 2 } });
            recognition.roles.push_back({ kBaseCandidate, 0, { 2, 1 } });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 3 };
            unit.roleOffset = 0;
            unit.roleCount = 2;

            ScriptShapingSelectionState state;
            if (!state.reset(7)) return fail("unable to initialize initial-anchor state");

            ScriptShapingIRResolveIndicPreBaseInitialAnchor resolver{};
            resolver.consonantSequence = scriptShapingRoleSelection(kConsonantSequence);
            resolver.baseCandidate = scriptShapingRoleSelection(kBaseCandidate);
            resolver.outputAnchorSelection = kInitialAnchor;

            if (!applyScriptShapingIRResolveIndicPreBaseInitialAnchor(resolver, recognition, unit, state))
                return fail("initial pre-base anchor failed");

            const ScriptShapingDerivedSelection* anchor = state.selection(kInitialAnchor);

            if (!anchor || anchor->sourceSpans.size() != 1 || anchor->sourceSpans[0].first != 0)
                return fail("initial pre-base anchor is not first consonant");
        }


        // ------------------------------------------------------------
        // Final pre-base anchor:
        //
        //     MATRA C H ZWJ BASE
        //
        // Matra target is after H+ZWJ, represented as Before BASE.
        // ------------------------------------------------------------

        {
            ScriptRecognitionResult recognition;
            recognition.kinds = { kConsonant, kHalant, kZWJ, kConsonant, kMatra };

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 5 };

            ScriptShapingSelectionState state;
            if (!state.reset(7)) return fail("unable to initialize final-anchor state");

            const ScriptSpan baseSpan{ 3, 1 };
            if (!assignScriptShapingDerivedSelection(state, kBase, &baseSpan, 1))
                return fail("unable to assign Base selection");

            OpenTypeShapingBuffer buffer;
            buffer.pushBack(makeIndicReorderGlyph(50, 4));
            buffer.pushBack(makeIndicReorderGlyph(10, 0));
            buffer.pushBack(makeIndicReorderGlyph(20, 1));
            buffer.pushBack(makeIndicReorderGlyph(21, 2));
            buffer.pushBack(makeIndicReorderGlyph(30, 3));

            ScriptShapingIRResolveIndicPreBaseAnchor resolver{};
            resolver.base = scriptShapingDerivedSelection(kBase);
            resolver.outputAnchorSelection = kFinalAnchor;
            resolver.halantKind = kHalant;
            resolver.zwjKind = kZWJ;
            resolver.zwnjKind = kZWNJ;

            if (!applyScriptShapingIRResolveIndicPreBaseAnchor(
                resolver, recognition, unit, state, state, buffer))
            {
                return fail("final pre-base anchor failed");
            }

            const ScriptShapingDerivedSelection* anchor = state.selection(kFinalAnchor);

            if (!anchor || anchor->sourceSpans.size() != 1 || anchor->sourceSpans[0].first != 3)
                return fail("final pre-base anchor did not move after H+ZWJ");
        }


        // ------------------------------------------------------------
        // Reph anchor for BeforePostscript:
        //
        //     REPH BASE POST MATRA
        //
        // Reph should land before POST, therefore its After-anchor is BASE.
        // ------------------------------------------------------------

        {
            ScriptRecognitionResult recognition;
            recognition.kinds = { kRa, kHalant, kConsonant, kConsonant, kMatra };
            ScriptRecognitionUnit unit{};
            unit.span = { 0, 5 };

            ScriptShapingSelectionState state;
            if (!state.reset(7)) return fail("unable to initialize Reph-anchor state");

            const ScriptSpan rephSpan{ 0, 2 };
            const ScriptSpan baseSpan{ 2, 1 };
            const ScriptSpan postSpan{ 3, 1 };

            if (!assignScriptShapingDerivedSelection(state, kReph, &rephSpan, 1) ||
                !assignScriptShapingDerivedSelection(state, kBase, &baseSpan, 1) ||
                !assignScriptShapingDerivedSelection(state, kPost, &postSpan, 1))
            {
                return fail("unable to assign Reph test selections");
            }

            OpenTypeShapingBuffer buffer;
            buffer.pushBack(makeIndicReorderGlyph(90, 0, 2));
            buffer.pushBack(makeIndicReorderGlyph(30, 2));
            buffer.pushBack(makeIndicReorderGlyph(40, 3));
            buffer.pushBack(makeIndicReorderGlyph(50, 4));

            ScriptShapingIRResolveIndicRephAnchor resolver{};
            resolver.reph = scriptShapingDerivedSelection(kReph);
            resolver.base = scriptShapingDerivedSelection(kBase);
            resolver.postBaseForms = scriptShapingDerivedSelection(kPost);
            resolver.outputAnchorSelection = kRephAnchor;
            resolver.raKind = kRa;
            resolver.consonantKind = kConsonant;
            resolver.nuktaKind = kNukta;
            resolver.halantKind = kHalant;
            resolver.zwjKind = kZWJ;
            resolver.zwnjKind = kZWNJ;

            if (!applyScriptShapingIRResolveIndicRephAnchor(
                resolver, recognition, unit, state, state, buffer))
            {
                return fail("Reph anchor resolution failed");
            }

            const ScriptShapingDerivedSelection* anchor = state.selection(kRephAnchor);

            if (!anchor || anchor->sourceSpans.size() != 1 || anchor->sourceSpans[0].first != 2)
                return fail("Reph anchor is not immediately before post-base form");
        }


        std::printf(
            "Script shaping Indic reordering: PASS\n"
            "  ZWNJ half blocking:        PASS\n"
            "  Initial pre-base anchor:  PASS\n"
            "  Final pre-base anchor:    PASS\n"
            "  Reph BeforePostscript:    PASS\n");

        return true;
    }

} // namespace waavs
