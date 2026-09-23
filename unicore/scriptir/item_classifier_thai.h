// item_classifier_thai.h
#pragma once

#include "script_item_classifier_dsl.h"
#include "script_recognition_dsl.h"

#include "unicode_general_category.h"
#include "unicode_indic_positional_category.h"
#include "unicode_indic_syllabic_category.h"

namespace waavs
{
    struct ThaiItemKinds
    {
        ScriptItemKindRef consonant{};
        ScriptItemKindRef vowelIndependent{};

        ScriptItemKindRef vowelPre{};
        ScriptItemKindRef vowelAbove{};
        ScriptItemKindRef vowelBelow{};
        ScriptItemKindRef vowelPost{};
        ScriptItemKindRef vowelOther{};

        ScriptItemKindRef toneMark{};
        ScriptItemKindRef mark{};

        ScriptItemKindRef other{};

        [[nodiscard]]
        bool valid() const noexcept
        {
            return consonant &&
                vowelIndependent &&
                vowelPre &&
                vowelAbove &&
                vowelBelow &&
                vowelPost &&
                vowelOther &&
                toneMark &&
                mark &&
                other;
        }
    };


    [[nodiscard]]
    static inline bool defineThaiItemKinds(
        ScriptRecognitionDSL& grammar,
        ThaiItemKinds& kinds)
    {
        ThaiItemKinds working{};

        working.consonant = grammar.kind("Consonant");
        working.vowelIndependent = grammar.kind("VowelIndependent");

        working.vowelPre = grammar.kind("VowelPre");
        working.vowelAbove = grammar.kind("VowelAbove");
        working.vowelBelow = grammar.kind("VowelBelow");
        working.vowelPost = grammar.kind("VowelPost");
        working.vowelOther = grammar.kind("VowelOther");

        working.toneMark = grammar.kind("ToneMark");
        working.mark = grammar.kind("Mark");

        working.other = grammar.kind("Other");

        if (!working.valid())
            return false;

        kinds = working;
        return true;
    }


    [[nodiscard]]
    static inline bool defineThaiItemClassifier(
        ScriptItemClassifierDSL& classifier,
        const ThaiItemKinds& kinds)
    {
        if (!kinds.valid())
            return false;


        // ------------------------------------------------------------
        // Tone marks.
        //
        // Keep these ahead of the residual General_Category mark rule.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.toneMark,
            classifier.isc(
                UnicodeIndicSyllabicCategory::ToneMark)))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Dependent vowels by logical position.
        // ------------------------------------------------------------

        if (!classifier.rule(
            kinds.vowelPre,
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
            kinds.vowelAbove,
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
            kinds.vowelBelow,
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
            kinds.vowelPost,
            classifier.ALL({
                classifier.isc(
                    UnicodeIndicSyllabicCategory::VowelDependent),

                classifier.ipc(
                    UnicodeIndicPositionalCategory::Right)
                })))
        {
            return false;
        }


        // Any dependent vowel not described by one of the simple positional
        // categories remains a vowel rather than falling into Other.

        if (!classifier.rule(
            kinds.vowelOther,
            classifier.isc(
                UnicodeIndicSyllabicCategory::VowelDependent)))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Letters.
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
        // Residual combining marks.
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