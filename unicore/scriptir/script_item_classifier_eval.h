// script_item_classifier_eval.h
#pragma once

#include <cstdint>

#include "script_item_classifier_description.h"

#include "unicode_bidi_class.h"
#include "unicode_general_category.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_hangul_syllable_type.h"
#include "unicode_indic_conjunct_break.h"
#include "unicode_indic_positional_category.h"
#include "unicode_indic_syllabic_category.h"
#include "unicode_joining_group.h"
#include "unicode_joining_type.h"

namespace waavs
{
    // Transient factual input for classification. This is not intended to be
    // stored permanently on Unicode pipeline items.
    struct ScriptItemClassifierFacts
    {
        uint32_t codePoint{ 0 };
        uint32_t script{ 0 };

        UnicodeGeneralCategory generalCategory{ UnicodeGeneralCategory::Unassigned };
        uint32_t combiningClass{ 0 };

        UnicodeIndicSyllabicCategory indicSyllabicCategory{ UnicodeIndicSyllabicCategory::Other };
        UnicodeIndicPositionalCategory indicPositionalCategory{ UnicodeIndicPositionalCategory::NotApplicable };
        UnicodeIndicConjunctBreak indicConjunctBreak{ UnicodeIndicConjunctBreak::None };

        UnicodeJoiningType joiningType{ UnicodeJoiningType::NonJoining };
        UnicodeJoiningGroup joiningGroup{ UnicodeJoiningGroup::NoJoiningGroup };

        UnicodeHangulSyllableType hangulSyllableType{ UnicodeHangulSyllableType::NotApplicable };

        UnicodeGraphemeClusterBreak graphemeClusterBreak{ UnicodeGraphemeClusterBreak::Other };
        UnicodeBidiClass bidiClass{ UnicodeBidiClass::LeftToRight };

        bool defaultIgnorableCodePoint{ false };
        bool extendedPictographic{ false };
    };


    static inline bool scriptItemClassifierPropertyValue(
        const ScriptItemClassifierFacts& facts,
        ScriptItemClassifierProperty property,
        uint32_t& value) noexcept
    {
        switch (property)
        {
        case ScriptItemClassifierProperty::GeneralCategory:
            value = static_cast<uint32_t>(facts.generalCategory);
            return true;

        case ScriptItemClassifierProperty::CombiningClass:
            value = facts.combiningClass;
            return true;

        case ScriptItemClassifierProperty::Script:
            value = facts.script;
            return true;

        case ScriptItemClassifierProperty::IndicSyllabicCategory:
            value = static_cast<uint32_t>(facts.indicSyllabicCategory);
            return true;

        case ScriptItemClassifierProperty::IndicPositionalCategory:
            value = static_cast<uint32_t>(facts.indicPositionalCategory);
            return true;

        case ScriptItemClassifierProperty::IndicConjunctBreak:
            value = static_cast<uint32_t>(facts.indicConjunctBreak);
            return true;

        case ScriptItemClassifierProperty::JoiningType:
            value = static_cast<uint32_t>(facts.joiningType);
            return true;

        case ScriptItemClassifierProperty::JoiningGroup:
            value = static_cast<uint32_t>(facts.joiningGroup);
            return true;

        case ScriptItemClassifierProperty::HangulSyllableType:
            value = static_cast<uint32_t>(facts.hangulSyllableType);
            return true;

        case ScriptItemClassifierProperty::GraphemeClusterBreak:
            value = static_cast<uint32_t>(facts.graphemeClusterBreak);
            return true;

        case ScriptItemClassifierProperty::BidiClass:
            value = static_cast<uint32_t>(facts.bidiClass);
            return true;

        case ScriptItemClassifierProperty::DefaultIgnorableCodePoint:
            value = facts.defaultIgnorableCodePoint ? 1u : 0u;
            return true;

        case ScriptItemClassifierProperty::ExtendedPictographic:
            value = facts.extendedPictographic ? 1u : 0u;
            return true;

        default:
            return false;
        }
    }


    static inline bool evaluateScriptItemClassifierPredicate(
        const ScriptItemClassifierDescription& description,
        ScriptItemClassifierPredicateId id,
        const ScriptItemClassifierFacts& facts,
        bool& matches)
    {
        const ScriptItemClassifierPredicate* predicate =
            description.predicate(id);

        if (!predicate)
            return false;

        const ScriptItemClassifierPredicateId* children =
            description.children(*predicate);

        if (predicate->childCount != 0 && !children)
            return false;


        switch (predicate->kind)
        {
        case ScriptItemClassifierPredicateKind::CodePointEqual:
            matches = facts.codePoint == predicate->value;
            return true;


        case ScriptItemClassifierPredicateKind::CodePointRange:
            matches =
                facts.codePoint >= predicate->value &&
                facts.codePoint <= predicate->value2;
            return true;


        case ScriptItemClassifierPredicateKind::PropertyEqual:
        {
            uint32_t value = 0;

            if (!scriptItemClassifierPropertyValue(
                facts,
                predicate->property,
                value))
            {
                return false;
            }

            matches = value == predicate->value;
            return true;
        }


        case ScriptItemClassifierPredicateKind::All:
            for (uint32_t i = 0; i < predicate->childCount; ++i)
            {
                bool childMatches = false;

                if (!evaluateScriptItemClassifierPredicate(
                    description,
                    children[i],
                    facts,
                    childMatches))
                {
                    return false;
                }

                if (!childMatches)
                {
                    matches = false;
                    return true;
                }
            }

            matches = true;
            return true;


        case ScriptItemClassifierPredicateKind::Any:
            for (uint32_t i = 0; i < predicate->childCount; ++i)
            {
                bool childMatches = false;

                if (!evaluateScriptItemClassifierPredicate(
                    description,
                    children[i],
                    facts,
                    childMatches))
                {
                    return false;
                }

                if (childMatches)
                {
                    matches = true;
                    return true;
                }
            }

            matches = false;
            return true;


        case ScriptItemClassifierPredicateKind::Not:
        {
            if (predicate->childCount != 1)
                return false;

            bool childMatches = false;

            if (!evaluateScriptItemClassifierPredicate(
                description,
                children[0],
                facts,
                childMatches))
            {
                return false;
            }

            matches = !childMatches;
            return true;
        }


        default:
            return false;
        }
    }


    [[nodiscard]]
    static inline ScriptItemKindId classifyScriptItem(
        const ScriptItemClassifierDescription& description,
        const ScriptItemClassifierFacts& facts)
    {
        for (const ScriptItemClassifierRule& rule : description.rules)
        {
            bool matches = false;

            if (!evaluateScriptItemClassifierPredicate(
                description,
                rule.predicate,
                facts,
                matches))
            {
                return kScriptItemKindInvalid;
            }

            if (matches)
                return rule.kind;
        }

        return description.defaultKind;
    }

} // namespace waavs