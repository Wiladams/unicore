// script_shaping_selection_types.h
#pragma once

#include <cstdint>

#include "script_recognition_types.h"

namespace waavs
{
    using ScriptShapingSelectionId = uint16_t;

    static constexpr ScriptShapingSelectionId kScriptShapingSelectionInvalid = 0;


    enum class ScriptShapingSelectionKind : uint8_t
    {
        Invalid = 0,
        Unit,
        Role,
        Derived
    };


    struct ScriptShapingSelectionRef
    {
        ScriptShapingSelectionKind kind{ ScriptShapingSelectionKind::Invalid };

        uint8_t reserved{ 0 };
        uint16_t id{ 0 };

        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            switch (kind)
            {
            case ScriptShapingSelectionKind::Unit:
                return id == 0;

            case ScriptShapingSelectionKind::Role:
                return id != kScriptRoleInvalid;

            case ScriptShapingSelectionKind::Derived:
                return id != kScriptShapingSelectionInvalid;

            default:
                return false;
            }
        }
    };


    [[nodiscard]]
    static constexpr ScriptShapingSelectionRef scriptShapingUnitSelection() noexcept
    {
        return {
            ScriptShapingSelectionKind::Unit,
            0,
            0
        };
    }


    [[nodiscard]]
    static constexpr ScriptShapingSelectionRef scriptShapingRoleSelection(
        ScriptRoleId role) noexcept
    {
        return {
            ScriptShapingSelectionKind::Role,
            0,
            role
        };
    }


    [[nodiscard]]
    static constexpr ScriptShapingSelectionRef scriptShapingDerivedSelection(
        ScriptShapingSelectionId selection) noexcept
    {
        return {
            ScriptShapingSelectionKind::Derived,
            0,
            selection
        };
    }


    static_assert(
        sizeof(ScriptShapingSelectionRef) == 4,
        "ScriptShapingSelectionRef must remain 4 bytes");

} // namespace waavs
