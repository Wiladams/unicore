// test_script_shaping_indic_base.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <vector>

#include "script_shaping_ir_executor.h"

namespace waavs
{
    static bool testScriptShapingIndicBase()
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping Indic base: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ------------------------------------------------------------
        // Minimal synthetic vocabulary.
        // ------------------------------------------------------------

        static constexpr ScriptItemKindId kRa = 1;
        static constexpr ScriptItemKindId kConsonant = 2;
        static constexpr ScriptItemKindId kNukta = 3;
        static constexpr ScriptItemKindId kHalant = 4;
        static constexpr ScriptItemKindId kZWJ = 5;
        static constexpr ScriptItemKindId kZWNJ = 6;

        static constexpr ScriptRoleId kConsonantSequence = 1;
        static constexpr ScriptRoleId kBaseCandidate = 2;

        static constexpr ScriptShapingSelectionId kBaseSelection = 1;


        ScriptShapingIRResolveIndicBase resolver{};

        resolver.consonantSequence =
            scriptShapingRoleSelection(kConsonantSequence);

        resolver.baseCandidate =
            scriptShapingRoleSelection(kBaseCandidate);

        resolver.outputBaseSelection =
            kBaseSelection;

        resolver.raKind = kRa;
        resolver.consonantKind = kConsonant;
        resolver.nuktaKind = kNukta;
        resolver.halantKind = kHalant;
        resolver.zwjKind = kZWJ;
        resolver.zwnjKind = kZWNJ;
        resolver.model = ScriptShapingIndicModel::New;


        // ============================================================
        // Case 1:
        //
        //     C
        //
        // A simple consonant syllable has exactly one Base candidate.
        // No font-dependent probing is necessary.
        // ============================================================

        {
            ScriptRecognitionResult recognition;

            recognition.kinds =
            {
                kConsonant
            };

            recognition.roles.push_back({
                kBaseCandidate,
                0,
                { 0, 1 }
                });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 1 };
            unit.roleOffset = 0;
            unit.roleCount = 1;

            ScriptShapingSelectionState selectionState;

            if (!selectionState.reset(1))
                return fail("unable to initialize selection state");

            OpenTypeShapingBuffer buffer;

            if (!applyScriptShapingIRResolveIndicBase(
                {},
                OTAG("dev2"),
                0,
                resolver,
                true,
                true,
                {},
                recognition,
                unit,
                selectionState,
                buffer))
            {
                return fail("simple Base resolution failed");
            }

            const ScriptShapingDerivedSelection* base =
                selectionState.selection(kBaseSelection);

            if (!base)
                return fail("simple Base selection missing");

            if (base->sourceSpans.size() != 1)
                return fail("simple Base selection has wrong span count");

            if (base->sourceSpans[0].first != 0 ||
                base->sourceSpans[0].count != 1)
            {
                return fail("simple Base selection has wrong source span");
            }
        }


        // ============================================================
        // Case 2:
        //
        //     C Nukta
        //
        // Nukta belongs to the selected Base source span.
        // ============================================================

        {
            ScriptRecognitionResult recognition;

            recognition.kinds =
            {
                kConsonant,
                kNukta
            };

            recognition.roles.push_back({
                kBaseCandidate,
                0,
                { 0, 2 }
                });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 2 };
            unit.roleOffset = 0;
            unit.roleCount = 1;

            ScriptShapingSelectionState selectionState;

            if (!selectionState.reset(1))
                return fail("unable to initialize Nukta selection state");

            OpenTypeShapingBuffer buffer;

            if (!applyScriptShapingIRResolveIndicBase(
                {},
                OTAG("dev2"),
                0,
                resolver,
                true,
                true,
                {},
                recognition,
                unit,
                selectionState,
                buffer))
            {
                return fail("Nukta Base resolution failed");
            }

            const ScriptShapingDerivedSelection* base =
                selectionState.selection(kBaseSelection);

            if (!base)
                return fail("Nukta Base selection missing");

            if (base->sourceSpans.size() != 1)
                return fail("Nukta Base selection has wrong span count");

            if (base->sourceSpans[0].first != 0 ||
                base->sourceSpans[0].count != 2)
            {
                return fail("Nukta was not retained with Base");
            }
        }


        // ============================================================
        // Case 3:
        //
        //     C Halant C
        //     |------| |
        //      seq     BaseCandidate
        //
        // Candidate extraction must discover:
        //
        //     candidate 0: C
        //     candidate 1: C, preceded by Halant at source 1
        //
        // This stops before font probing; it verifies the structural
        // information the resolver will use for blwf/pstf/pref probes.
        // ============================================================

        {
            ScriptRecognitionResult recognition;

            recognition.kinds =
            {
                kConsonant,
                kHalant,
                kConsonant
            };

            recognition.roles.push_back({
                kConsonantSequence,
                0,
                { 0, 2 }
                });

            recognition.roles.push_back({
                kBaseCandidate,
                0,
                { 2, 1 }
                });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 3 };
            unit.roleOffset = 0;
            unit.roleCount = 2;

            std::vector<ScriptShapingIndicBaseCandidate> candidates;

            if (!resolveScriptShapingIndicCandidates(
                resolver,
                recognition,
                unit,
                candidates))
            {
                return fail("candidate extraction failed");
            }

            if (candidates.size() != 2)
                return fail("expected two Indic Base candidates");

            if (candidates[0].span.first != 0 ||
                candidates[0].span.count != 1)
            {
                return fail("first candidate span is incorrect");
            }

            if (candidates[0].hasPrecedingHalant)
                return fail("first candidate unexpectedly has preceding Halant");

            if (candidates[1].span.first != 2 ||
                candidates[1].span.count != 1)
            {
                return fail("second candidate span is incorrect");
            }

            if (!candidates[1].hasPrecedingHalant)
                return fail("second candidate did not detect preceding Halant");

            if (candidates[1].precedingHalantOffset != 1)
                return fail("second candidate has wrong preceding Halant offset");

            if (!candidates[0].hasFollowingHalant ||
                candidates[0].followingHalantOffset != 1)
            {
                return fail("first candidate has wrong following Halant context");
            }
        }


        // ============================================================
        // Case 4:
        //
        //     C Nukta Halant C Nukta
        //
        // Make sure both candidates retain their complete consonant
        // source spans while the Halant remains structural context.
        // ============================================================

        {
            ScriptRecognitionResult recognition;

            recognition.kinds =
            {
                kConsonant,
                kNukta,
                kHalant,
                kConsonant,
                kNukta
            };

            recognition.roles.push_back({
                kConsonantSequence,
                0,
                { 0, 3 }
                });

            recognition.roles.push_back({
                kBaseCandidate,
                0,
                { 3, 2 }
                });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 5 };
            unit.roleOffset = 0;
            unit.roleCount = 2;

            std::vector<ScriptShapingIndicBaseCandidate> candidates;

            if (!resolveScriptShapingIndicCandidates(
                resolver,
                recognition,
                unit,
                candidates))
            {
                return fail("Nukta candidate extraction failed");
            }

            if (candidates.size() != 2)
                return fail("expected two Nukta candidates");

            if (candidates[0].span.first != 0 ||
                candidates[0].span.count != 2)
            {
                return fail("first Nukta candidate span is incorrect");
            }

            if (candidates[1].span.first != 3 ||
                candidates[1].span.count != 2)
            {
                return fail("second Nukta candidate span is incorrect");
            }

            if (!candidates[1].hasPrecedingHalant ||
                candidates[1].precedingHalantOffset != 2)
            {
                return fail("Nukta candidate Halant context is incorrect");
            }
        }


        // ============================================================
        // Case 5: ZWNJ after the preceding Halant blocks form probing.
        // ============================================================

        {
            ScriptRecognitionResult recognition;
            recognition.kinds = { kConsonant, kHalant, kZWNJ, kConsonant };
            recognition.roles.push_back({ kConsonantSequence, 0, { 0, 3 } });
            recognition.roles.push_back({ kBaseCandidate, 0, { 3, 1 } });

            ScriptRecognitionUnit unit{};
            unit.span = { 0, 4 };
            unit.roleOffset = 0;
            unit.roleCount = 2;

            std::vector<ScriptShapingIndicBaseCandidate> candidates;

            if (!resolveScriptShapingIndicCandidates(resolver, recognition, unit, candidates))
                return fail("ZWNJ candidate extraction failed");

            if (candidates.size() != 2 || !candidates[1].precededByZwnj)
                return fail("ZWNJ context was not retained on Base candidate");
        }


        std::printf(
            "Script shaping Indic base: PASS\n"
            "  Simple Base:              PASS\n"
            "  Base + Nukta:             PASS\n"
            "  Candidate extraction:     PASS\n"
            "  Preceding Halant:         PASS\n"
            "  Candidate provenance:     PASS\n"
            "  Bidirectional Halant ctx: PASS\n"
            "  ZWNJ Base context:        PASS\n");

        return true;
    }

} // namespace waavs