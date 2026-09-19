// script_recognition_dump.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "script_recognition_description.h"

namespace waavs
{
    static inline const char* scriptRecognitionKindName(const ScriptRecognitionDescription& description, ScriptItemKindId id) noexcept
    {
        if (id == kScriptItemKindInvalid || id > description.kinds.size())
            return nullptr;

        return description.kinds[id - 1].name.c_str();
    }


    static inline const char* scriptRecognitionRoleName(const ScriptRecognitionDescription& description, ScriptRoleId id) noexcept
    {
        if (id == kScriptRoleInvalid || id > description.roles.size())
            return nullptr;

        return description.roles[id - 1].name.c_str();
    }


    static inline const char* scriptRecognitionUnitTypeName(const ScriptRecognitionDescription& description, ScriptUnitTypeId id) noexcept
    {
        if (id == kScriptUnitTypeInvalid || id > description.unitTypes.size())
            return nullptr;

        return description.unitTypes[id - 1].name.c_str();
    }


    static inline void appendScriptRecognitionIndent(std::string& output, uint32_t depth)
    {
        for (uint32_t i = 0; i < depth; ++i)
            output += "    ";
    }


    static inline bool appendScriptRecognitionExpression(
        const ScriptRecognitionDescription& description,
        ScriptRecognitionExprId id,
        std::string& output,
        uint32_t depth)
    {
        const ScriptRecognitionExpr* expr = description.expression(id);

        if (!expr)
            return false;

        const ScriptRecognitionExprId* children = description.children(*expr);

        if (expr->childCount != 0 && !children)
            return false;

        switch (expr->kind)
        {
        case ScriptRecognitionExprKind::Kind:
        {
            const char* name = scriptRecognitionKindName(description, expr->kindId);

            if (!name)
                return false;

            output += name;
            return true;
        }

        case ScriptRecognitionExprKind::Sequence:
        case ScriptRecognitionExprKind::Choice:
        {
            if (expr->childCount == 0)
                return false;

            output += expr->kind == ScriptRecognitionExprKind::Sequence
                ? "seq(\n"
                : "choice(\n";

            for (uint32_t i = 0; i < expr->childCount; ++i)
            {
                appendScriptRecognitionIndent(output, depth + 1);

                if (!appendScriptRecognitionExpression(description, children[i], output, depth + 1))
                    return false;

                if (i + 1 != expr->childCount)
                    output += ",";

                output += "\n";
            }

            appendScriptRecognitionIndent(output, depth);
            output += ")";
            return true;
        }

        case ScriptRecognitionExprKind::Optional:
        case ScriptRecognitionExprKind::ZeroOrMore:
        case ScriptRecognitionExprKind::OneOrMore:
        {
            if (expr->childCount != 1)
                return false;

            switch (expr->kind)
            {
            case ScriptRecognitionExprKind::Optional:
                output += "opt(";
                break;

            case ScriptRecognitionExprKind::ZeroOrMore:
                output += "zeroOrMore(";
                break;

            case ScriptRecognitionExprKind::OneOrMore:
                output += "oneOrMore(";
                break;

            default:
                return false;
            }

            if (!appendScriptRecognitionExpression(description, children[0], output, depth))
                return false;

            output += ")";
            return true;
        }

        case ScriptRecognitionExprKind::Capture:
        {
            if (expr->childCount != 1)
                return false;

            const char* roleName = scriptRecognitionRoleName(description, expr->roleId);

            if (!roleName)
                return false;

            output += "capture(";
            output += roleName;
            output += ", ";

            if (!appendScriptRecognitionExpression(description, children[0], output, depth))
                return false;

            output += ")";
            return true;
        }

        default:
            return false;
        }
    }


    [[nodiscard]]
    static inline bool dumpScriptRecognitionDescription(const ScriptRecognitionDescription& description, std::string& output)
    {
        output.clear();

        for (const ScriptRecognitionItemKind& kind : description.kinds)
        {
            output += "kind ";
            output += kind.name;
            output += "\n";
        }

        if (!description.kinds.empty() &&
            (!description.roles.empty() || !description.units.empty()))
        {
            output += "\n";
        }

        for (const ScriptRecognitionRole& role : description.roles)
        {
            output += "role ";
            output += role.name;
            output += "\n";
        }

        if (!description.roles.empty() && !description.units.empty())
            output += "\n";

        for (size_t i = 0; i < description.units.size(); ++i)
        {
            const ScriptRecognitionUnitDescription& unit = description.units[i];
            const char* unitName = scriptRecognitionUnitTypeName(description, unit.type);

            if (!unitName)
                return false;

            output += "unit ";
            output += unitName;
            output += " =\n    ";

            if (!appendScriptRecognitionExpression(description, unit.expression, output, 1))
                return false;

            output += "\n";

            if (i + 1 != description.units.size())
                output += "\n";
        }

        return true;
    }


    [[nodiscard]]
    static inline std::string dumpScriptRecognitionDescription(const ScriptRecognitionDescription& description)
    {
        std::string output;

        if (!dumpScriptRecognitionDescription(description, output))
            output.clear();

        return output;
    }

} // namespace waavs