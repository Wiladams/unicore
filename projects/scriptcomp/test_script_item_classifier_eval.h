// test_script_item_classifier_eval.h
#pragma once

#include "test_core.h"

#include "script_item_classifier_dsl.h"
#include "script_item_classifier_eval.h"

namespace waavs
{
    static inline bool runScriptItemClassifierEval()
    {
        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;

        const ScriptItemKindRef ra = grammar.kind("Ra");
        const ScriptItemKindRef halant = grammar.kind("Halant");
        const ScriptItemKindRef consonant = grammar.kind("Consonant");
        const ScriptItemKindRef matraPre = grammar.kind("MatraPre");
        const ScriptItemKindRef mark = grammar.kind("Mark");
        const ScriptItemKindRef other = grammar.kind("Other");

        if (!ra || !halant || !consonant || !matraPre || !mark || !other)
            return false;


        // Specific rule before the general consonant rule.
        if (!classifier.rule(
            ra,
            classifier.cp(0x0930)))
        {
            return false;
        }


        if (!classifier.rule(
            halant,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Virama)))
        {
            return false;
        }


        if (!classifier.rule(
            matraPre,
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
            consonant,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Consonant)))
        {
            return false;
        }


        if (!classifier.rule(
            mark,
            classifier.ANY({
                classifier.generalCategory(
                    UnicodeGeneralCategory::NonspacingMark),

                classifier.generalCategory(
                    UnicodeGeneralCategory::SpacingMark)
                })))
        {
            return false;
        }


        if (!classifier.defaultKind(other))
            return false;


        const ScriptItemClassifierDescription& description =
            classifier.description();


        // ------------------------------------------------------------
        // U+0930 DEVANAGARI LETTER RA.
        //
        // It is also a consonant, but the exact code-point rule must win.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x0930;
            facts.indicSyllabicCategory =
                UnicodeIndicSyllabicCategory::Consonant;

            if (classifyScriptItem(description, facts) != ra.id)
                return false;
        }


        // ------------------------------------------------------------
        // Virama.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x094D;
            facts.indicSyllabicCategory =
                UnicodeIndicSyllabicCategory::Virama;

            if (classifyScriptItem(description, facts) != halant.id)
                return false;
        }


        // ------------------------------------------------------------
        // Pre-base dependent vowel.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x093F;
            facts.indicSyllabicCategory =
                UnicodeIndicSyllabicCategory::VowelDependent;
            facts.indicPositionalCategory =
                UnicodeIndicPositionalCategory::Left;

            if (classifyScriptItem(description, facts) != matraPre.id)
                return false;
        }


        // ------------------------------------------------------------
        // Dependent vowel, but not left-positioned.
        //
        // ALL must fail, so this falls through.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x0940;
            facts.indicSyllabicCategory =
                UnicodeIndicSyllabicCategory::VowelDependent;
            facts.indicPositionalCategory =
                UnicodeIndicPositionalCategory::Right;

            if (classifyScriptItem(description, facts) != other.id)
                return false;
        }


        // ------------------------------------------------------------
        // Generic consonant.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x0915;
            facts.indicSyllabicCategory =
                UnicodeIndicSyllabicCategory::Consonant;

            if (classifyScriptItem(description, facts) != consonant.id)
                return false;
        }


        // ------------------------------------------------------------
        // ANY: mark categories.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x0951;
            facts.generalCategory =
                UnicodeGeneralCategory::NonspacingMark;

            if (classifyScriptItem(description, facts) != mark.id)
                return false;
        }


        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x0903;
            facts.generalCategory =
                UnicodeGeneralCategory::SpacingMark;

            if (classifyScriptItem(description, facts) != mark.id)
                return false;
        }


        // ------------------------------------------------------------
        // Default.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierFacts facts{};
            facts.codePoint = 0x0020;
            facts.generalCategory =
                UnicodeGeneralCategory::SpaceSeparator;

            if (classifyScriptItem(description, facts) != other.id)
                return false;
        }


        printf(
            "Script item classifier eval: PASS\n"
            "  Ordered first-match: PASS\n"
            "  Property match: PASS\n"
            "  ALL: PASS\n"
            "  ANY: PASS\n"
            "  Default: PASS\n");

        return true;
    }


    static inline void testScriptItemClassifierEval()
    {
        runScriptItemClassifierEval();
    }

} // namespace waavs