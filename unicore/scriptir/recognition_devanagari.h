// recognition_devanagari.h
#pragma once

#include "item_classifier_devanagari.h"
#include "script_recognition_dsl.h"

namespace waavs
{
    struct DevanagariRecognition
    {
        ScriptRoleRef rephCandidate{};
        ScriptRoleRef consonantSequence{};
        ScriptRoleRef baseCandidate{};

        ScriptRoleRef preBaseMatras{};
        ScriptRoleRef aboveBaseMatras{};
        ScriptRoleRef belowBaseMatras{};
        ScriptRoleRef postBaseMatras{};
        ScriptRoleRef otherMatras{};

        ScriptRoleRef modifiers{};

        ScriptUnitTypeRef consonantSyllable{};
        ScriptUnitTypeRef vowelSyllable{};
        ScriptUnitTypeRef standAloneCluster{};

        [[nodiscard]]
        bool valid() const noexcept
        {
            return rephCandidate &&
                consonantSequence &&
                baseCandidate &&
                preBaseMatras &&
                aboveBaseMatras &&
                belowBaseMatras &&
                postBaseMatras &&
                otherMatras &&
                modifiers &&
                consonantSyllable &&
                vowelSyllable &&
                standAloneCluster;
        }
    };


    [[nodiscard]]
    static inline bool defineDevanagariRecognition(
        ScriptRecognitionDSL& grammar,
        const DevanagariItemKinds& kinds,
        DevanagariRecognition& recognition)
    {
        if (!kinds.valid())
            return false;

        DevanagariRecognition working{};


        // ------------------------------------------------------------
        // Roles.
        // ------------------------------------------------------------

        working.rephCandidate = grammar.role("RephCandidate");
        working.consonantSequence = grammar.role("ConsonantSequence");
        working.baseCandidate = grammar.role("BaseCandidate");

        working.preBaseMatras = grammar.role("PreBaseMatras");
        working.aboveBaseMatras = grammar.role("AboveBaseMatras");
        working.belowBaseMatras = grammar.role("BelowBaseMatras");
        working.postBaseMatras = grammar.role("PostBaseMatras");
        working.otherMatras = grammar.role("OtherMatras");

        working.modifiers = grammar.role("Modifiers");

        working.consonantSyllable = grammar.unitType("ConsonantSyllable");
        working.vowelSyllable = grammar.unitType("VowelSyllable");
        working.standAloneCluster = grammar.unitType("StandAloneCluster");

        if (!working.valid())
            return false;


        // ------------------------------------------------------------
        // Common expressions.
        //
        // Ra is classified separately because it has special behavior, but
        // everywhere else it remains an ordinary consonant candidate.
        // ------------------------------------------------------------

        const ScriptExprRef consonant =
            grammar.choice({
                grammar.match(kinds.ra),
                grammar.match(kinds.consonant)
                });

        if (!consonant)
            return false;


        const ScriptExprRef joinControl =
            grammar.choice({
                grammar.match(kinds.zwj),
                grammar.match(kinds.zwnj)
                });

        if (!joinControl)
            return false;


        // ------------------------------------------------------------
        // Reph candidate.
        //
        // Do not include ZWJ/ZWNJ here. Join controls following the virama
        // change the interpretation and should prevent this simple sequence
        // from being recognized as the normal reph candidate.
        //
        //     Ra [Nukta] Halant
        // ------------------------------------------------------------

        const ScriptExprRef rephSequence =
            grammar.seq({
                grammar.match(kinds.ra),
                grammar.opt(grammar.match(kinds.nukta)),
                grammar.match(kinds.halant)
                });

        if (!rephSequence)
            return false;

        const ScriptExprRef reph =
            grammar.capture(
                working.rephCandidate,
                rephSequence);

        if (!reph)
            return false;


        // ------------------------------------------------------------
        // Consonant prefix.
        //
        //     Consonant [Nukta] Halant [ZWJ|ZWNJ]
        //
        // One or more of these before the final consonant form the logical
        // consonant sequence preceding the base candidate.
        // ------------------------------------------------------------

        const ScriptExprRef halantJoin =
            grammar.choice({
                grammar.seq({ grammar.match(kinds.halant), grammar.opt(joinControl) }),
                grammar.seq({ joinControl, grammar.match(kinds.halant) })
                });

        if (!halantJoin)
            return false;

        const ScriptExprRef consonantPrefix =
            grammar.seq({
                consonant,
                grammar.opt(grammar.match(kinds.nukta)),
                halantJoin
                });

        if (!consonantPrefix)
            return false;


        const ScriptExprRef consonantSequence =
            grammar.capture(
                working.consonantSequence,
                grammar.oneOrMore(consonantPrefix));

        if (!consonantSequence)
            return false;


        // ------------------------------------------------------------
        // Base candidate.
        //
        // Recognition identifies the structural candidate. Later shaping
        // state may refine the actual base after OpenType substitutions.
        //
        //     Consonant [Nukta]
        // ------------------------------------------------------------

        const ScriptExprRef base =
            grammar.capture(
                working.baseCandidate,
                grammar.seq({
                    consonant,
                    grammar.opt(grammar.match(kinds.nukta))
                    }));

        if (!base)
            return false;


        // ------------------------------------------------------------
        // Positional matras.
        //
        // Each capture is non-null; the capture itself is optional.
        // ------------------------------------------------------------

        const ScriptExprRef preBaseMatras =
            grammar.capture(
                working.preBaseMatras,
                grammar.oneOrMore(
                    grammar.match(kinds.matraPre)));

        const ScriptExprRef aboveBaseMatras =
            grammar.capture(
                working.aboveBaseMatras,
                grammar.oneOrMore(
                    grammar.match(kinds.matraAbove)));

        const ScriptExprRef belowBaseMatras =
            grammar.capture(
                working.belowBaseMatras,
                grammar.oneOrMore(
                    grammar.match(kinds.matraBelow)));

        const ScriptExprRef postBaseMatras =
            grammar.capture(
                working.postBaseMatras,
                grammar.oneOrMore(
                    grammar.match(kinds.matraPost)));

        const ScriptExprRef otherMatras =
            grammar.capture(
                working.otherMatras,
                grammar.oneOrMore(
                    grammar.match(kinds.matraOther)));

        if (!preBaseMatras ||
            !aboveBaseMatras ||
            !belowBaseMatras ||
            !postBaseMatras ||
            !otherMatras)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Syllable modifiers and residual marks.
        // ------------------------------------------------------------

        const ScriptExprRef modifier =
            grammar.choice({
                grammar.match(kinds.bindu),
                grammar.match(kinds.visarga),
                grammar.match(kinds.mark)
                });

        if (!modifier)
            return false;

        const ScriptExprRef modifiers =
            grammar.capture(
                working.modifiers,
                grammar.oneOrMore(modifier));

        if (!modifiers)
            return false;

        const ScriptExprRef vedicSigns =
            grammar.oneOrMore(grammar.match(kinds.vedic));

        if (!vedicSigns)
            return false;

        const ScriptExprRef matrasAndSigns =
            grammar.seq({
                grammar.opt(preBaseMatras),
                grammar.opt(aboveBaseMatras),
                grammar.opt(belowBaseMatras),
                grammar.opt(postBaseMatras),
                grammar.opt(otherMatras),
                grammar.opt(modifiers),
                grammar.opt(vedicSigns)
                });

        if (!matrasAndSigns)
            return false;


        // ------------------------------------------------------------
        // Consonant syllable.
        //
        //     [RephCandidate]
        //     [ConsonantSequence]
        //     BaseCandidate
        //     [PreBaseMatras]
        //     [AboveBaseMatras]
        //     [BelowBaseMatras]
        //     [PostBaseMatras]
        //     [OtherMatras]
        //     [Modifiers]
        //
        // The consonant sequence is optional, but when present its capture
        // always contains at least one consuming consonant-prefix sequence.
        // ------------------------------------------------------------

        const ScriptExprRef consonantSyllable =
            grammar.seq({
                grammar.opt(reph),
                grammar.opt(consonantSequence),
                base,
                grammar.opt(grammar.match(kinds.anudatta)),
                grammar.opt(halantJoin),
                matrasAndSigns
                });

        if (!consonantSyllable)
            return false;

        if (!grammar.unit(
            working.consonantSyllable,
            consonantSyllable))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Independent-vowel and stand-alone clusters.
        // ------------------------------------------------------------

        const ScriptExprRef vowelTail =
            grammar.seq({
                grammar.opt(grammar.match(kinds.nukta)),
                grammar.opt(grammar.choice({
                    grammar.seq({ grammar.opt(joinControl), grammar.match(kinds.halant), consonant }),
                    grammar.seq({ grammar.match(kinds.zwj), consonant })
                    })),
                matrasAndSigns
                });

        if (!vowelTail)
            return false;

        const ScriptExprRef vowelSyllable =
            grammar.seq({
                grammar.opt(reph),
                grammar.match(kinds.vowelIndependent),
                vowelTail
                });

        if (!vowelSyllable)
            return false;

        if (!grammar.unit(working.vowelSyllable, vowelSyllable))
            return false;

        const ScriptExprRef standAlone =
            grammar.seq({
                grammar.opt(reph),
                grammar.match(kinds.nbsp),
                vowelTail
                });

        if (!standAlone)
            return false;

        if (!grammar.unit(working.standAloneCluster, standAlone))
            return false;


        recognition = working;
        return true;
    }

} // namespace waavs