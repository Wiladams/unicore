// script_recognition_types.h
#pragma once

#include <cstdint>

namespace waavs
{
    using ScriptItemKindId = uint16_t;
    using ScriptUnitTypeId = uint16_t;
    using ScriptRoleId = uint16_t;

    static constexpr ScriptItemKindId kScriptItemKindInvalid = 0;
    static constexpr ScriptUnitTypeId kScriptUnitTypeInvalid = 0;
    static constexpr ScriptRoleId kScriptRoleInvalid = 0;


    struct ScriptSpan
    {
        uint32_t first{ 0 };
        uint32_t count{ 0 };

        [[nodiscard]]
        constexpr uint32_t end() const noexcept
        {
            return first + count;
        }

        [[nodiscard]]
        constexpr bool empty() const noexcept
        {
            return count == 0;
        }

        [[nodiscard]]
        constexpr bool contains(uint32_t index) const noexcept
        {
            return index >= first && index - first < count;
        }
    };


    struct ScriptRoleBinding
    {
        ScriptRoleId role{ kScriptRoleInvalid };
        uint16_t reserved{ 0 };
        ScriptSpan span{};
    };


    struct ScriptRecognitionUnit
    {
        ScriptSpan span{};

        uint32_t roleOffset{ 0 };

        ScriptUnitTypeId type{ kScriptUnitTypeInvalid };
        uint16_t roleCount{ 0 };

        uint32_t flags{ 0 };
    };


    static_assert(sizeof(ScriptSpan) == 8, "ScriptSpan must remain 8 bytes");
    static_assert(sizeof(ScriptRoleBinding) == 12, "ScriptRoleBinding must remain 12 bytes");
    static_assert(sizeof(ScriptRecognitionUnit) == 20, "ScriptRecognitionUnit must remain 20 bytes");

} // namespace waavs