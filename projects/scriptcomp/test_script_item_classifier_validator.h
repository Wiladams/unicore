// test_script_item_classifier_validator.h
#pragma once

#include "test_core.h"

#include "script_item_classifier_dsl.h"
#include "script_item_classifier_validator.h"

namespace waavs
{
    static inline bool runScriptItemClassifierValidator()
    {
        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;

        const ScriptItemKindRef ra =
            grammar.kind("Ra");

        const ScriptItemKindRef consonant =
            grammar.kind("Consonant");

        const ScriptItemKindRef matraPre =
            grammar.kind("MatraPre");

        const ScriptItemKindRef other =
            grammar.kind("Other");

        if (!ra || !consonant || !matraPre || !other)
            return false;


        if (!classifier.rule(
            ra,
            classifier.cp(0x0930)))
        {
            return false;
        }

        if (!classifier.rule(
            matraPre,
            classifier.ALL({
                classifier.property(
                    ScriptItemClassifierProperty::IndicSyllabicCategory,
                    12u),

                classifier.property(
                    ScriptItemClassifierProperty::IndicPositionalCategory,
                    4u)
                })))
        {
            return false;
        }

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


        // ------------------------------------------------------------
        // Valid description.
        // ------------------------------------------------------------

        {
            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    classifier.description(),
                    grammar.description());

            if (!result)
                return false;
        }


        // ------------------------------------------------------------
        // Invalid default kind.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDescription broken =
                classifier.description();

            broken.defaultKind =
                static_cast<ScriptItemKindId>(999);

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidDefaultKind)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid rule kind.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDescription broken =
                classifier.description();

            broken.rules[0].kind =
                static_cast<ScriptItemKindId>(999);

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidRuleKind)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid rule predicate.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDescription broken =
                classifier.description();

            broken.rules[0].predicate =
                kScriptItemClassifierPredicateInvalid;

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidRulePredicate)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid property.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDescription broken =
                classifier.description();

            const ScriptItemClassifierPredicateId predicateId =
                broken.rules[2].predicate;

            broken.predicates[predicateId].property =
                ScriptItemClassifierProperty::Invalid;

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidProperty)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid code point.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDescription broken =
                classifier.description();

            const ScriptItemClassifierPredicateId predicateId =
                broken.rules[0].predicate;

            broken.predicates[predicateId].value =
                0x110000u;

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidCodePoint)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid code-point range.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDSL rangeClassifier;

            if (!rangeClassifier.rule(
                consonant,
                rangeClassifier.range(
                    0x0900,
                    0x097F)))
            {
                return false;
            }

            if (!rangeClassifier.defaultKind(other))
                return false;

            ScriptItemClassifierDescription broken =
                rangeClassifier.description();

            const ScriptItemClassifierPredicateId predicateId =
                broken.rules[0].predicate;

            broken.predicates[predicateId].value =
                0x0980;

            broken.predicates[predicateId].value2 =
                0x0900;

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidCodePointRange)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid NOT arity.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDSL logic;

            const ScriptItemClassifierPredicateRef child =
                logic.cp(0x0930);

            const ScriptItemClassifierPredicateRef predicate =
                logic.NOT(child);

            if (!predicate)
                return false;

            if (!logic.rule(ra, predicate))
                return false;

            if (!logic.defaultKind(other))
                return false;

            ScriptItemClassifierDescription broken =
                logic.description();

            broken.predicates[predicate.id].childCount = 0;

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidPredicateArity)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Invalid ALL arity.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDescription broken =
                classifier.description();

            const ScriptItemClassifierPredicateId predicateId =
                broken.rules[1].predicate;

            broken.predicates[predicateId].childCount = 0;

            const ScriptItemClassifierValidationResult result =
                validateScriptItemClassifierDescription(
                    broken,
                    grammar.description());

            if (result)
                return false;

            if (result.error !=
                ScriptItemClassifierValidationError::InvalidPredicateArity)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Predicate cycle.
        // ------------------------------------------------------------

        {
            ScriptItemClassifierDSL logic;

            const ScriptItemClassifierPredicateRef child =
                logic.cp(0x0930);

            const ScriptItemClassifierPredicateRef predicate =
                logic.NOT(child);

            if (!predicate)
                return false;

            if (!logic.rule(ra, predicate))
                return false;

            if (!logic.defaultKind(other))
                return false;

            ScriptItemClassifierDescription broken =
                logic.description();

            ScriptItemClassifierPredicate& notPredicate =
                broken.predicates[predicate.id];

            if (notPredicate.childCount != 1)
                return false;

            broken.predicateChildren[
                notPredicate.childOffset] =
                predicate.id;

                const ScriptItemClassifierValidationResult result =
                    validateScriptItemClassifierDescription(
                        broken,
                        grammar.description());

                if (result)
                    return false;

                if (result.error !=
                    ScriptItemClassifierValidationError::PredicateCycle)
                {
                    return false;
                }
        }


        printf(
            "Script item classifier validator: PASS\n"
            "  Valid description\n"
            "  Invalid default kind\n"
            "  Invalid rule kind\n"
            "  Invalid rule predicate\n"
            "  Invalid property\n"
            "  Invalid code point\n"
            "  Invalid code-point range\n"
            "  Invalid NOT arity\n"
            "  Invalid ALL arity\n"
            "  Predicate cycle\n");

        return true;
    }


    static inline void testScriptItemClassifierValidator()
    {
        runScriptItemClassifierValidator();
    }

} // namespace waavs