// item_classifier_devanagari.h
#pragma once

#include "script_item_classifier_dsl.h"
#include "script_recognition_dsl.h"

#include "unicode_general_category.h"
#include "unicode_indic_positional_category.h"
#include "unicode_indic_syllabic_category.h"

namespace waavs
{
    struct DevanagariItemKinds
    {
        ScriptItemKindRef ra{};

        ScriptItemKindRef consonant{};
        ScriptItemKindRef vowelIndependent{};

        ScriptItemKindRef halant{};
        ScriptItemKindRef nukta{};

        ScriptItemKindRef matraPre{};
        ScriptItemKindRef matraAbove{};
        ScriptItemKindRef matraBelow{};
        ScriptItemKindRef matraPost{};
        ScriptItemKindRef matraOther{};

        ScriptItemKindRef bindu{};
        ScriptItemKindRef visarga{};
        ScriptItemKindRef avagraha{};
        ScriptItemKindRef anudatta{};
        ScriptItemKindRef vedic{};
        ScriptItemKindRef nbsp{};

        ScriptItemKindRef zwj{};
        ScriptItemKindRef zwnj{};

        ScriptItemKindRef mark{};
        ScriptItemKindRef other{};

        [[nodiscard]]
        bool valid() const noexcept
        {
            return ra &&
                consonant &&
                vowelIndependent &&
                halant &&
                nukta &&
                matraPre &&
                matraAbove &&
                matraBelow &&
                matraPost &&
                matraOther &&
                bindu &&
                visarga &&
                avagraha &&
                anudatta &&
                vedic &&
                nbsp &&
                zwj &&
                zwnj &&
                mark &&
                other;
        }
    };


    [[nodiscard]]
    static inline bool defineDevanagariItemKinds(
        ScriptRecognitionDSL& grammar,
        DevanagariItemKinds& kinds)
    {
        DevanagariItemKinds working{};

        working.ra = grammar.kind("Ra");

        working.consonant = grammar.kind("Consonant");
        working.vowelIndependent = grammar.kind("VowelIndependent");

        working.halant = grammar.kind("Halant");
        working.nukta = grammar.kind("Nukta");

        working.matraPre = grammar.kind("MatraPre");
        working.matraAbove = grammar.kind("MatraAbove");
        working.matraBelow = grammar.kind("MatraBelow");
        working.matraPost = grammar.kind("MatraPost");
        working.matraOther = grammar.kind("MatraOther");

        working.bindu = grammar.kind("Bindu");
        working.visarga = grammar.kind("Visarga");
        working.avagraha = grammar.kind("Avagraha");
        working.anudatta = grammar.kind("Anudatta");
        working.vedic = grammar.kind("Vedic");
        working.nbsp = grammar.kind("NBSP");

        working.zwj = grammar.kind("ZWJ");
        working.zwnj = grammar.kind("ZWNJ");

        working.mark = grammar.kind("Mark");
        working.other = grammar.kind("Other");

        if (!working.valid())
            return false;

        kinds = working;
        return true;
    }


    [[nodiscard]]
    static inline bool defineDevanagariItemClassifier(
        ScriptItemClassifierDSL& classifier,
        const DevanagariItemKinds& kinds)
    {
        if (!kinds.valid())
            return false;


        // ------------------------------------------------------------
        // Exact code-point overrides.
        //
        // These must precede broader property rules.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.ra,
            classifier.cp(0x0930)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.zwj,
            classifier.cp(0x200D)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.zwnj,
            classifier.cp(0x200C)))
        {
            return false;
        }


        if (!classifier.rule(kinds.nbsp, classifier.cp(0x00A0)))
            return false;

        if (!classifier.rule(kinds.anudatta, classifier.cp(0x0952)))
            return false;

        const ScriptItemClassifierPredicateRef markCategory =
            classifier.ANY({
                classifier.generalCategory(UnicodeGeneralCategory::NonspacingMark),
                classifier.generalCategory(UnicodeGeneralCategory::SpacingMark),
                classifier.generalCategory(UnicodeGeneralCategory::EnclosingMark)
                });

        if (!markCategory)
            return false;

        if (!classifier.rule(
            kinds.vedic,
            classifier.ANY({
                classifier.cp(0x0951),
                classifier.range(0x0953, 0x0954),
                classifier.ALL({ classifier.range(0x1CD0, 0x1CFF), markCategory }),
                classifier.ALL({ classifier.range(0xA8E0, 0xA8F1), markCategory })
                })))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Structural Indic items.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.halant,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Virama)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.nukta,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Nukta)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.avagraha,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Avagraha)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.bindu,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Bindu)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.visarga,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Visarga)))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Dependent vowels.
        //
        // Specific positional forms precede the generic VowelDependent
        // fallback. This lets recognition reason in shaping vocabulary
        // while retaining unusual/composite positional forms.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.matraPre,
            classifier.ALL({
                classifier.isc(
                    UnicodeIndicSyllabicCategory::VowelDependent),

                classifier.ipc(
                    UnicodeIndicPositionalCategory::Left)
                })))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.matraAbove,
            classifier.ALL({
                classifier.isc(
                    UnicodeIndicSyllabicCategory::VowelDependent),

                classifier.ipc(
                    UnicodeIndicPositionalCategory::Top)
                })))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.matraBelow,
            classifier.ALL({
                classifier.isc(
                    UnicodeIndicSyllabicCategory::VowelDependent),

                classifier.ipc(
                    UnicodeIndicPositionalCategory::Bottom)
                })))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.matraPost,
            classifier.ALL({
                classifier.isc(
                    UnicodeIndicSyllabicCategory::VowelDependent),

                classifier.ipc(
                    UnicodeIndicPositionalCategory::Right)
                })))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.matraOther,
            classifier.isc(
                UnicodeIndicSyllabicCategory::VowelDependent)))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Main letters.
        //
        // Ra has already been captured by the exact code-point override.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.vowelIndependent,
            classifier.isc(
                UnicodeIndicSyllabicCategory::VowelIndependent)))
        {
            return false;
        }


        if (!classifier.rule(
            kinds.consonant,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Consonant)))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Remaining marks.
        //
        // This intentionally comes after virama, nukta, dependent vowels,
        // bindu, and visarga so that those retain stronger semantic kinds.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.mark,
            classifier.ANY({
                classifier.generalCategory(
                    UnicodeGeneralCategory::NonspacingMark),

                classifier.generalCategory(
                    UnicodeGeneralCategory::SpacingMark),

                classifier.generalCategory(
                    UnicodeGeneralCategory::EnclosingMark)
                })))
        {
            return false;
        }


        if (!classifier.defaultKind(kinds.other))
            return false;

        return true;
    }

} // namespace waavs