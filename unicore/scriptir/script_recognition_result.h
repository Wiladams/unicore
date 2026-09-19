// script_recognition_result.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "script_recognition_types.h"

namespace waavs
{
    struct ScriptRecognitionResult
    {
        std::vector<ScriptItemKindId> kinds{};
        std::vector<ScriptRecognitionUnit> units{};
        std::vector<ScriptRoleBinding> roles{};

        void clear()
        {
            kinds.clear();
            units.clear();
            roles.clear();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return units.empty();
        }

        [[nodiscard]]
        size_t kindCount() const noexcept
        {
            return kinds.size();
        }

        [[nodiscard]]
        size_t unitCount() const noexcept
        {
            return units.size();
        }

        [[nodiscard]]
        size_t roleCount() const noexcept
        {
            return roles.size();
        }

        [[nodiscard]]
        ScriptItemKindId kindAt(size_t index) const noexcept
        {
            return index < kinds.size()
                ? kinds[index]
                : kScriptItemKindInvalid;
        }

        [[nodiscard]]
        const ScriptRecognitionUnit* unit(size_t index) const noexcept
        {
            return index < units.size()
                ? &units[index]
                : nullptr;
        }

        [[nodiscard]]
        const ScriptRoleBinding* role(size_t index) const noexcept
        {
            return index < roles.size()
                ? &roles[index]
                : nullptr;
        }

        [[nodiscard]]
        const ScriptRoleBinding* roleData(const ScriptRecognitionUnit& unit) const noexcept
        {
            if (unit.roleCount == 0)
                return nullptr;

            if (unit.roleOffset >= roles.size())
                return nullptr;

            if (unit.roleCount > roles.size() - unit.roleOffset)
                return nullptr;

            return roles.data() + unit.roleOffset;
        }

        [[nodiscard]]
        const ScriptRoleBinding* roleFor(
            const ScriptRecognitionUnit& unit,
            ScriptRoleId roleId) const noexcept
        {
            const ScriptRoleBinding* data = roleData(unit);

            if (!data)
                return nullptr;

            for (uint16_t i = 0; i < unit.roleCount; ++i)
            {
                if (data[i].role == roleId)
                    return &data[i];
            }

            return nullptr;
        }
    };

} // namespace waavs