// test_script_item_classifier_dsl.h
#pragma once

#include "test_core.h"

#include "script_item_classifier_dsl.h"

namespace waavs
{
    static inline bool runScriptItemClassifierDSL()
    {
        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;

        const ScriptItemKindRef ra = grammar.kind("Ra");
        const ScriptItemKindRef halant = grammar.kind("Halant");
        const ScriptItemKindRef consonant = grammar.kind("Consonant");
        const ScriptItemKindRef matraPre = grammar.kind("MatraPre");
        const ScriptItemKindRef other = grammar.kind("Other");

        if (!ra || !halant || !consonant || !matraPre || !other)
            return false;


        const ScriptItemClassifierPredicateRef raPredicate =
            classifier.cp(0x0930);

        const ScriptItemClassifierPredicateRef halantPredicate =
            classifier.property(
                ScriptItemClassifierProperty::IndicSyllabicCategory,
                7u);

        const ScriptItemClassifierPredicateRef matraPrePredicate =
            classifier.ALL({
                classifier.property(
                    ScriptItemClassifierProperty::IndicSyllabicCategory,
                    12u),
                classifier.property(
                    ScriptItemClassifierProperty::IndicPositionalCategory,
                    4u)
                });

        const ScriptItemClassifierPredicateRef notRaPredicate =
            classifier.NOT(
                classifier.cp(0x0930));

        const ScriptItemClassifierPredicateRef anyPredicate =
            classifier.ANY({
                classifier.cp(0x0930),
                classifier.cp(0x0931)
                });

        if (!raPredicate ||
            !halantPredicate ||
            !matraPrePredicate ||
            !notRaPredicate ||
            !anyPredicate)
        {
            return false;
        }


        if (!classifier.rule(ra, raPredicate))
            return false;

        if (!classifier.rule(halant, halantPredicate))
            return false;

        if (!classifier.rule(matraPre, matraPrePredicate))
            return false;

        if (!classifier.rule(
            consonant,
            classifier.property(
                ScriptItemClassifierProperty::IndicSyllabicCategory,
                3u)))
        {
            return false;
        }

        if (!classifier.defaultKind(other))
            return false;


        const ScriptItemClassifierDescription& description =
            classifier.description();

        if (description.rules.size() != 4)
            return false;

        if (description.defaultKind != other.id)
            return false;


        if (description.rules[0].kind != ra.id ||
            description.rules[0].predicate != raPredicate.id)
        {
            return false;
        }

        if (description.rules[1].kind != halant.id ||
            description.rules[1].predicate != halantPredicate.id)
        {
            return false;
        }

        if (description.rules[2].kind != matraPre.id ||
            description.rules[2].predicate != matraPrePredicate.id)
        {
            return false;
        }


        // ------------------------------------------------------------
        // ALL
        // ------------------------------------------------------------

        const ScriptItemClassifierPredicate* predicate =
            description.predicate(matraPrePredicate.id);

        if (!predicate)
            return false;

        if (predicate->kind != ScriptItemClassifierPredicateKind::All)
            return false;

        if (predicate->childCount != 2)
            return false;

        const ScriptItemClassifierPredicateId* children =
            description.children(*predicate);

        if (!children)
            return false;

        const ScriptItemClassifierPredicate* child0 =
            description.predicate(children[0]);

        const ScriptItemClassifierPredicate* child1 =
            description.predicate(children[1]);

        if (!child0 || !child1)
            return false;

        if (child0->kind != ScriptItemClassifierPredicateKind::PropertyEqual ||
            child0->property != ScriptItemClassifierProperty::IndicSyllabicCategory ||
            child0->value != 12u)
        {
            return false;
        }

        if (child1->kind != ScriptItemClassifierPredicateKind::PropertyEqual ||
            child1->property != ScriptItemClassifierProperty::IndicPositionalCategory ||
            child1->value != 4u)
        {
            return false;
        }


        // ------------------------------------------------------------
        // NOT
        // ------------------------------------------------------------

        const ScriptItemClassifierPredicate* notPredicate =
            description.predicate(notRaPredicate.id);

        if (!notPredicate)
            return false;

        if (notPredicate->kind != ScriptItemClassifierPredicateKind::Not)
            return false;

        if (notPredicate->childCount != 1)
            return false;

        const ScriptItemClassifierPredicateId* notChildren =
            description.children(*notPredicate);

        if (!notChildren)
            return false;

        const ScriptItemClassifierPredicate* notChild =
            description.predicate(notChildren[0]);

        if (!notChild)
            return false;

        if (notChild->kind != ScriptItemClassifierPredicateKind::CodePointEqual ||
            notChild->value != 0x0930)
        {
            return false;
        }


        // ------------------------------------------------------------
        // ANY
        // ------------------------------------------------------------

        const ScriptItemClassifierPredicate* any =
            description.predicate(anyPredicate.id);

        if (!any)
            return false;

        if (any->kind != ScriptItemClassifierPredicateKind::Any)
            return false;

        if (any->childCount != 2)
            return false;

        const ScriptItemClassifierPredicateId* anyChildren =
            description.children(*any);

        if (!anyChildren)
            return false;

        const ScriptItemClassifierPredicate* anyChild0 =
            description.predicate(anyChildren[0]);

        const ScriptItemClassifierPredicate* anyChild1 =
            description.predicate(anyChildren[1]);

        if (!anyChild0 || !anyChild1)
            return false;

        if (anyChild0->kind != ScriptItemClassifierPredicateKind::CodePointEqual ||
            anyChild0->value != 0x0930)
        {
            return false;
        }

        if (anyChild1->kind != ScriptItemClassifierPredicateKind::CodePointEqual ||
            anyChild1->value != 0x0931)
        {
            return false;
        }


        printf(
            "Script item classifier DSL: PASS\n"
            "  Kinds: %zu\n"
            "  Rules: %zu\n"
            "  Predicates: %zu\n"
            "  Logic: ALL ANY NOT\n",
            grammar.description().kinds.size(),
            description.rules.size(),
            description.predicates.size());

        return true;
    }


    static inline void testScriptItemClassifierDSL()
    {
        runScriptItemClassifierDSL();
    }

} // namespace waavs