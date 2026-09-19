// script_recognition_description.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "script_recognition_types.h"

namespace waavs
{
    using ScriptRecognitionExprId = uint32_t;

    static constexpr ScriptRecognitionExprId kScriptRecognitionExprInvalid = 0xFFFFFFFFu;


    enum class ScriptRecognitionExprKind : uint8_t
    {
        Invalid = 0,

        Kind,
        Sequence,
        Choice,
        Optional,
        ZeroOrMore,
        OneOrMore,
        Capture
    };


    struct ScriptRecognitionItemKind
    {
        ScriptItemKindId id{ kScriptItemKindInvalid };
        std::string name{};
    };


    struct ScriptRecognitionUnitType
    {
        ScriptUnitTypeId id{ kScriptUnitTypeInvalid };
        std::string name{};
    };


    struct ScriptRecognitionRole
    {
        ScriptRoleId id{ kScriptRoleInvalid };
        std::string name{};
    };


    struct ScriptRecognitionExpr
    {
        ScriptRecognitionExprKind kind{ ScriptRecognitionExprKind::Invalid };

        ScriptItemKindId kindId{ kScriptItemKindInvalid  };
        ScriptRoleId roleId{ kScriptRoleInvalid };

        uint32_t childOffset{ 0 };
        uint32_t childCount{ 0 };
    };


    struct ScriptRecognitionUnitDescription
    {
        ScriptUnitTypeId type{ kScriptUnitTypeInvalid };
        ScriptRecognitionExprId expression{ kScriptRecognitionExprInvalid };
    };


    struct ScriptRecognitionDescription
    {
        std::vector<ScriptRecognitionItemKind> kinds{};
        std::vector<ScriptRecognitionUnitType> unitTypes{};
        std::vector<ScriptRecognitionRole> roles{};

        std::vector<ScriptRecognitionExpr> expressions{};
        std::vector<ScriptRecognitionExprId> expressionChildren{};

        std::vector<ScriptRecognitionUnitDescription> units{};

        void clear()
        {
            kinds.clear();
            unitTypes.clear();
            roles.clear();
            expressions.clear();
            expressionChildren.clear();
            units.clear();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return units.empty();
        }

        [[nodiscard]]
        const ScriptRecognitionExpr* expression(ScriptRecognitionExprId id) const noexcept
        {
            return id < expressions.size()
                ? &expressions[id]
                : nullptr;
        }

        [[nodiscard]]
        const ScriptRecognitionExprId* children(const ScriptRecognitionExpr& expr) const noexcept
        {
            if (expr.childCount == 0)
                return nullptr;

            if (expr.childOffset >= expressionChildren.size())
                return nullptr;

            if (expr.childCount > expressionChildren.size() - expr.childOffset)
                return nullptr;

            return expressionChildren.data() + expr.childOffset;
        }
    };

} // namespace waavs