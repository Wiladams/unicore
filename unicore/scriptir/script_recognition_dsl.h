// script_recognition_dsl.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <utility>

#include "script_recognition_description.h"

namespace waavs
{
    struct ScriptItemKindRef
    {
        ScriptItemKindId id{ kScriptItemKindInvalid };

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return id != kScriptItemKindInvalid;
        }
    };


    struct ScriptRoleRef
    {
        ScriptRoleId id{ kScriptRoleInvalid };

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return id != kScriptRoleInvalid;
        }
    };


    struct ScriptExprRef
    {
        ScriptRecognitionExprId id{ kScriptRecognitionExprInvalid };

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return id != kScriptRecognitionExprInvalid;
        }
    };


    struct ScriptUnitTypeRef
    {
        ScriptUnitTypeId id{ kScriptUnitTypeInvalid };

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return id != kScriptUnitTypeInvalid;
        }
    };


    class ScriptRecognitionDSL
    {
        ScriptRecognitionDescription mDescription{};


        [[nodiscard]]
        ScriptExprRef addExpr(
            ScriptRecognitionExprKind kind,
            std::initializer_list<ScriptExprRef> children = {})
        {
            if (mDescription.expressions.size() >= std::numeric_limits<uint32_t>::max())
                return {};

            if (mDescription.expressionChildren.size() >
                std::numeric_limits<uint32_t>::max() - children.size())
            {
                return {};
            }

            ScriptRecognitionExpr expr{};

            expr.kind = kind;
            expr.childOffset = static_cast<uint32_t>(mDescription.expressionChildren.size());
            expr.childCount = static_cast<uint32_t>(children.size());

            for (const ScriptExprRef child : children)
            {
                if (!child)
                    return {};

                mDescription.expressionChildren.push_back(child.id);
            }

            const ScriptRecognitionExprId id =
                static_cast<ScriptRecognitionExprId>(mDescription.expressions.size());

            mDescription.expressions.push_back(expr);

            return { id };
        }


    public:
        void clear()
        {
            mDescription.clear();
        }


        [[nodiscard]]
        const ScriptRecognitionDescription& description() const noexcept
        {
            return mDescription;
        }


        [[nodiscard]]
        ScriptRecognitionDescription& description() noexcept
        {
            return mDescription;
        }


        // ------------------------------------------------------------
        // Vocabulary.
        // ------------------------------------------------------------

        [[nodiscard]]
        ScriptItemKindRef kind(const char* name)
        {
            if (!name || !*name)
                return {};

            if (mDescription.kinds.size() >= std::numeric_limits<ScriptItemKindId>::max())
                return {};

            const ScriptItemKindId id =
                static_cast<ScriptItemKindId>(mDescription.kinds.size() + 1);

            ScriptRecognitionItemKind value{};
            value.id = id;
            value.name = name;

            mDescription.kinds.push_back(std::move(value));

            return { id };
        }


        [[nodiscard]]
        ScriptRoleRef role(const char* name)
        {
            if (!name || !*name)
                return {};

            if (mDescription.roles.size() >= std::numeric_limits<ScriptRoleId>::max())
                return {};

            const ScriptRoleId id =
                static_cast<ScriptRoleId>(mDescription.roles.size() + 1);

            ScriptRecognitionRole value{};
            value.id = id;
            value.name = name;

            mDescription.roles.push_back(std::move(value));

            return { id };
        }


        [[nodiscard]]
        ScriptUnitTypeRef unitType(const char* name)
        {
            if (!name || !*name)
                return {};

            if (mDescription.unitTypes.size() >= std::numeric_limits<ScriptUnitTypeId>::max())
                return {};

            const ScriptUnitTypeId id =
                static_cast<ScriptUnitTypeId>(mDescription.unitTypes.size() + 1);

            ScriptRecognitionUnitType value{};
            value.id = id;
            value.name = name;

            mDescription.unitTypes.push_back(std::move(value));

            return { id };
        }


        // ------------------------------------------------------------
        // Expressions.
        // ------------------------------------------------------------

        [[nodiscard]]
        ScriptExprRef match(ScriptItemKindRef value)
        {
            if (!value)
                return {};

            if (mDescription.expressions.size() >= std::numeric_limits<uint32_t>::max())
                return {};

            ScriptRecognitionExpr expr{};
            expr.kind = ScriptRecognitionExprKind::Kind;
            expr.kindId = value.id;

            const ScriptRecognitionExprId id =
                static_cast<ScriptRecognitionExprId>(mDescription.expressions.size());

            mDescription.expressions.push_back(expr);

            return { id };
        }


        [[nodiscard]]
        ScriptExprRef seq(std::initializer_list<ScriptExprRef> children)
        {
            if (children.size() == 0)
                return {};

            return addExpr(ScriptRecognitionExprKind::Sequence, children);
        }


        [[nodiscard]]
        ScriptExprRef choice(std::initializer_list<ScriptExprRef> children)
        {
            if (children.size() == 0)
                return {};

            return addExpr(ScriptRecognitionExprKind::Choice, children);
        }


        [[nodiscard]]
        ScriptExprRef opt(ScriptExprRef child)
        {
            if (!child)
                return {};

            return addExpr(ScriptRecognitionExprKind::Optional, { child });
        }


        [[nodiscard]]
        ScriptExprRef zeroOrMore(ScriptExprRef child)
        {
            if (!child)
                return {};

            return addExpr(ScriptRecognitionExprKind::ZeroOrMore, { child });
        }


        [[nodiscard]]
        ScriptExprRef oneOrMore(ScriptExprRef child)
        {
            if (!child)
                return {};

            return addExpr(ScriptRecognitionExprKind::OneOrMore, { child });
        }


        [[nodiscard]]
        ScriptExprRef capture(ScriptRoleRef role, ScriptExprRef child)
        {
            if (!role || !child)
                return {};

            ScriptExprRef result =
                addExpr(ScriptRecognitionExprKind::Capture, { child });

            if (!result)
                return {};

            ScriptRecognitionExpr& expr =
                mDescription.expressions[result.id];

            expr.roleId = role.id;

            return result;
        }


        [[nodiscard]]
        ScriptExprRef expr(ScriptItemKindRef value)
        {
            return match(value);
        }


        // ------------------------------------------------------------
        // Units.
        // ------------------------------------------------------------

        [[nodiscard]]
        bool unit(ScriptUnitTypeRef type, ScriptExprRef expression)
        {
            if (!type || !expression)
                return false;

            ScriptRecognitionUnitDescription unit{};
            unit.type = type.id;
            unit.expression = expression.id;

            mDescription.units.push_back(unit);

            return true;
        }


        [[nodiscard]]
        bool unit(const char* name, ScriptExprRef expression)
        {
            const ScriptUnitTypeRef type = unitType(name);

            if (!type)
                return false;

            return unit(type, expression);
        }
    };

} // namespace waavs