// script_item_classifier_validator.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "script_item_classifier_description.h"
#include "script_recognition_description.h"

namespace waavs
{
    enum class ScriptItemClassifierValidationError : uint8_t
    {
        None = 0,

        InvalidDefaultKind,
        InvalidRuleKind,
        InvalidRulePredicate,

        InvalidPredicateKind,
        InvalidPredicateReference,
        InvalidPredicateArity,

        InvalidProperty,
        InvalidCodePoint,
        InvalidCodePointRange,

        PredicateCycle
    };


    struct ScriptItemClassifierValidationResult
    {
        ScriptItemClassifierValidationError error{ ScriptItemClassifierValidationError::None };
        uint32_t index{ 0 };

        constexpr explicit operator bool() const noexcept
        {
            return error == ScriptItemClassifierValidationError::None;
        }
    };


    enum class ScriptItemClassifierVisitState : uint8_t
    {
        Unvisited = 0,
        Visiting,
        Complete
    };


    struct ScriptItemClassifierValidationContext
    {
        const ScriptItemClassifierDescription* classifier{ nullptr };
        const ScriptRecognitionDescription* recognition{ nullptr };

        std::vector<ScriptItemClassifierVisitState> visit{};

        ScriptItemClassifierValidationResult result{};
    };


    static inline bool isValidScriptItemKind(
        const ScriptRecognitionDescription& description,
        ScriptItemKindId kind) noexcept
    {
        return kind != kScriptItemKindInvalid &&
            kind <= description.kinds.size() &&
            description.kinds[kind - 1].id == kind;
    }


    static inline bool validateScriptItemClassifierPredicate(
        ScriptItemClassifierValidationContext& context,
        ScriptItemClassifierPredicateId id)
    {
        if (id >= context.classifier->predicates.size())
        {
            context.result.error =
                ScriptItemClassifierValidationError::InvalidPredicateReference;

            context.result.index = id;
            return false;
        }


        ScriptItemClassifierVisitState& state =
            context.visit[id];

        if (state == ScriptItemClassifierVisitState::Visiting)
        {
            context.result.error =
                ScriptItemClassifierValidationError::PredicateCycle;

            context.result.index = id;
            return false;
        }

        if (state == ScriptItemClassifierVisitState::Complete)
            return true;

        state = ScriptItemClassifierVisitState::Visiting;


        const ScriptItemClassifierPredicate& predicate =
            context.classifier->predicates[id];

        const ScriptItemClassifierPredicateId* children =
            context.classifier->children(predicate);

        if (predicate.childCount != 0 && !children)
        {
            context.result.error =
                ScriptItemClassifierValidationError::InvalidPredicateReference;

            context.result.index = id;
            return false;
        }


        switch (predicate.kind)
        {
        case ScriptItemClassifierPredicateKind::CodePointEqual:
            if (predicate.childCount != 0)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidPredicateArity;

                context.result.index = id;
                return false;
            }

            if (predicate.value > 0x10FFFFu)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidCodePoint;

                context.result.index = id;
                return false;
            }

            break;


        case ScriptItemClassifierPredicateKind::CodePointRange:
            if (predicate.childCount != 0)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidPredicateArity;

                context.result.index = id;
                return false;
            }

            if (predicate.value > 0x10FFFFu ||
                predicate.value2 > 0x10FFFFu ||
                predicate.value > predicate.value2)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidCodePointRange;

                context.result.index = id;
                return false;
            }

            break;


        case ScriptItemClassifierPredicateKind::PropertyEqual:
            if (predicate.childCount != 0)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidPredicateArity;

                context.result.index = id;
                return false;
            }

            if (predicate.property == ScriptItemClassifierProperty::Invalid)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidProperty;

                context.result.index = id;
                return false;
            }

            break;


        case ScriptItemClassifierPredicateKind::All:
        case ScriptItemClassifierPredicateKind::Any:
            if (predicate.childCount == 0)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidPredicateArity;

                context.result.index = id;
                return false;
            }

            for (uint32_t i = 0; i < predicate.childCount; ++i)
            {
                if (!validateScriptItemClassifierPredicate(
                    context,
                    children[i]))
                {
                    return false;
                }
            }

            break;


        case ScriptItemClassifierPredicateKind::Not:
            if (predicate.childCount != 1)
            {
                context.result.error =
                    ScriptItemClassifierValidationError::InvalidPredicateArity;

                context.result.index = id;
                return false;
            }

            if (!validateScriptItemClassifierPredicate(
                context,
                children[0]))
            {
                return false;
            }

            break;


        default:
            context.result.error =
                ScriptItemClassifierValidationError::InvalidPredicateKind;

            context.result.index = id;
            return false;
        }


        state = ScriptItemClassifierVisitState::Complete;
        return true;
    }


    [[nodiscard]]
    static inline ScriptItemClassifierValidationResult
        validateScriptItemClassifierDescription(
            const ScriptItemClassifierDescription& classifier,
            const ScriptRecognitionDescription& recognition)
    {
        ScriptItemClassifierValidationContext context{};
        context.classifier = &classifier;
        context.recognition = &recognition;
        context.visit.resize(
            classifier.predicates.size(),
            ScriptItemClassifierVisitState::Unvisited);


        // ------------------------------------------------------------
        // Default kind.
        // ------------------------------------------------------------

        if (!isValidScriptItemKind(
            recognition,
            classifier.defaultKind))
        {
            return {
                ScriptItemClassifierValidationError::InvalidDefaultKind,
                classifier.defaultKind
            };
        }


        // ------------------------------------------------------------
        // Rules.
        // ------------------------------------------------------------

        for (uint32_t i = 0; i < classifier.rules.size(); ++i)
        {
            const ScriptItemClassifierRule& rule =
                classifier.rules[i];

            if (!isValidScriptItemKind(
                recognition,
                rule.kind))
            {
                return {
                    ScriptItemClassifierValidationError::InvalidRuleKind,
                    i
                };
            }

            if (rule.predicate >= classifier.predicates.size())
            {
                return {
                    ScriptItemClassifierValidationError::InvalidRulePredicate,
                    i
                };
            }

            if (!validateScriptItemClassifierPredicate(
                context,
                rule.predicate))
            {
                return context.result;
            }
        }


        // ------------------------------------------------------------
        // Validate unreferenced predicates too.
        // ------------------------------------------------------------

        for (uint32_t i = 0; i < classifier.predicates.size(); ++i)
        {
            if (!validateScriptItemClassifierPredicate(
                context,
                i))
            {
                return context.result;
            }
        }


        return {};
    }

} // namespace waavs