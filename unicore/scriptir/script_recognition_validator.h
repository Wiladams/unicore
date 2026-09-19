// script_recognition_validator.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "script_recognition_description.h"

namespace waavs
{
    enum class ScriptRecognitionValidationError : uint8_t
    {
        None = 0,

        InvalidKind,
        InvalidUnitType,
        InvalidRole,

        InvalidExpressionKind,
        InvalidExpressionReference,
        InvalidExpressionArity,
        InvalidKindReference,
        InvalidRoleReference,

        ExpressionCycle,
        NullableRepetition,
        NullableCapture,

        InvalidUnitReference,
        NullableUnit
    };


    struct ScriptRecognitionValidationResult
    {
        ScriptRecognitionValidationError error{ ScriptRecognitionValidationError::None };
        uint32_t index{ 0 };

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return error == ScriptRecognitionValidationError::None;
        }
    };


    enum class ScriptRecognitionVisitState : uint8_t
    {
        Unvisited = 0,
        Visiting,
        Complete
    };


    struct ScriptRecognitionValidationContext
    {
        const ScriptRecognitionDescription* description{ nullptr };

        std::vector<ScriptRecognitionVisitState> visit{};
        std::vector<uint8_t> nullable{};

        ScriptRecognitionValidationResult result{};
    };


    static inline bool validateScriptRecognitionVocabulary(
        const ScriptRecognitionDescription& description,
        ScriptRecognitionValidationResult& result)
    {
        for (size_t i = 0; i < description.kinds.size(); ++i)
        {
            const ScriptRecognitionItemKind& value = description.kinds[i];

            if (value.id == kScriptItemKindInvalid ||
                value.id != static_cast<ScriptItemKindId>(i + 1) ||
                value.name.empty())
            {
                result.error = ScriptRecognitionValidationError::InvalidKind;
                result.index = static_cast<uint32_t>(i);
                return false;
            }
        }

        for (size_t i = 0; i < description.unitTypes.size(); ++i)
        {
            const ScriptRecognitionUnitType& value = description.unitTypes[i];

            if (value.id == kScriptUnitTypeInvalid ||
                value.id != static_cast<ScriptUnitTypeId>(i + 1) ||
                value.name.empty())
            {
                result.error = ScriptRecognitionValidationError::InvalidUnitType;
                result.index = static_cast<uint32_t>(i);
                return false;
            }
        }

        for (size_t i = 0; i < description.roles.size(); ++i)
        {
            const ScriptRecognitionRole& value = description.roles[i];

            if (value.id == kScriptRoleInvalid ||
                value.id != static_cast<ScriptRoleId>(i + 1) ||
                value.name.empty())
            {
                result.error = ScriptRecognitionValidationError::InvalidRole;
                result.index = static_cast<uint32_t>(i);
                return false;
            }
        }

        return true;
    }


    static inline bool validateScriptRecognitionExpression(
        ScriptRecognitionValidationContext& context,
        ScriptRecognitionExprId id)
    {
        const ScriptRecognitionDescription& description = *context.description;

        if (id >= description.expressions.size())
        {
            context.result.error = ScriptRecognitionValidationError::InvalidExpressionReference;
            context.result.index = id;
            return false;
        }

        if (context.visit[id] == ScriptRecognitionVisitState::Complete)
            return true;

        if (context.visit[id] == ScriptRecognitionVisitState::Visiting)
        {
            context.result.error = ScriptRecognitionValidationError::ExpressionCycle;
            context.result.index = id;
            return false;
        }

        context.visit[id] = ScriptRecognitionVisitState::Visiting;

        const ScriptRecognitionExpr& expr = description.expressions[id];

        const ScriptRecognitionExprId* children = description.children(expr);

        if (expr.childCount != 0 && !children)
        {
            context.result.error = ScriptRecognitionValidationError::InvalidExpressionReference;
            context.result.index = id;
            return false;
        }


        switch (expr.kind)
        {
        case ScriptRecognitionExprKind::Kind:
            if (expr.childCount != 0)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            if (expr.kindId == kScriptItemKindInvalid ||
                expr.kindId > description.kinds.size())
            {
                context.result.error = ScriptRecognitionValidationError::InvalidKindReference;
                context.result.index = id;
                return false;
            }

            context.nullable[id] = 0;
            break;


        case ScriptRecognitionExprKind::Sequence:
            if (expr.childCount == 0)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            context.nullable[id] = 1;

            for (uint32_t i = 0; i < expr.childCount; ++i)
            {
                if (!validateScriptRecognitionExpression(context, children[i]))
                    return false;

                if (!context.nullable[children[i]])
                    context.nullable[id] = 0;
            }

            break;


        case ScriptRecognitionExprKind::Choice:
            if (expr.childCount == 0)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            context.nullable[id] = 0;

            for (uint32_t i = 0; i < expr.childCount; ++i)
            {
                if (!validateScriptRecognitionExpression(context, children[i]))
                    return false;

                if (context.nullable[children[i]])
                    context.nullable[id] = 1;
            }

            break;


        case ScriptRecognitionExprKind::Optional:
            if (expr.childCount != 1)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            if (!validateScriptRecognitionExpression(context, children[0]))
                return false;

            context.nullable[id] = 1;
            break;


        case ScriptRecognitionExprKind::ZeroOrMore:
            if (expr.childCount != 1)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            if (!validateScriptRecognitionExpression(context, children[0]))
                return false;

            if (context.nullable[children[0]])
            {
                context.result.error = ScriptRecognitionValidationError::NullableRepetition;
                context.result.index = id;
                return false;
            }

            context.nullable[id] = 1;
            break;


        case ScriptRecognitionExprKind::OneOrMore:
            if (expr.childCount != 1)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            if (!validateScriptRecognitionExpression(context, children[0]))
                return false;

            if (context.nullable[children[0]])
            {
                context.result.error = ScriptRecognitionValidationError::NullableRepetition;
                context.result.index = id;
                return false;
            }

            context.nullable[id] = 0;
            break;


        case ScriptRecognitionExprKind::Capture:
            if (expr.childCount != 1)
            {
                context.result.error = ScriptRecognitionValidationError::InvalidExpressionArity;
                context.result.index = id;
                return false;
            }

            if (expr.roleId == kScriptRoleInvalid ||
                expr.roleId > description.roles.size())
            {
                context.result.error = ScriptRecognitionValidationError::InvalidRoleReference;
                context.result.index = id;
                return false;
            }

            if (!validateScriptRecognitionExpression(context, children[0]))
                return false;

            if (context.nullable[children[0]])
            {
                context.result.error = ScriptRecognitionValidationError::NullableCapture;
                context.result.index = id;
                return false;
            }

            context.nullable[id] = 0;
            break;


        default:
            context.result.error = ScriptRecognitionValidationError::InvalidExpressionKind;
            context.result.index = id;
            return false;
        }


        context.visit[id] = ScriptRecognitionVisitState::Complete;
        return true;
    }


    [[nodiscard]]
    static inline ScriptRecognitionValidationResult validateScriptRecognitionDescription(
        const ScriptRecognitionDescription& description)
    {
        ScriptRecognitionValidationContext context;

        context.description = &description;

        if (!validateScriptRecognitionVocabulary(description, context.result))
            return context.result;

        context.visit.resize(
            description.expressions.size(),
            ScriptRecognitionVisitState::Unvisited);

        context.nullable.resize(
            description.expressions.size(),
            0);


        // Validate every expression, including expressions that are not
        // currently referenced by a Unit. This keeps the entire description
        // internally well-formed.

        for (ScriptRecognitionExprId id = 0;
            id < description.expressions.size();
            ++id)
        {
            if (!validateScriptRecognitionExpression(context, id))
                return context.result;
        }


        // Unit roots must reference valid, consuming expressions.

        for (size_t i = 0; i < description.units.size(); ++i)
        {
            const ScriptRecognitionUnitDescription& unit = description.units[i];

            if (unit.type == kScriptUnitTypeInvalid ||
                unit.type > description.unitTypes.size() ||
                unit.expression >= description.expressions.size())
            {
                context.result.error = ScriptRecognitionValidationError::InvalidUnitReference;
                context.result.index = static_cast<uint32_t>(i);
                return context.result;
            }

            if (context.nullable[unit.expression])
            {
                context.result.error = ScriptRecognitionValidationError::NullableUnit;
                context.result.index = static_cast<uint32_t>(i);
                return context.result;
            }
        }


        return {};
    }

} // namespace waavs