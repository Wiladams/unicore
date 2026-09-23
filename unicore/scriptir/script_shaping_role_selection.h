// script_shaping_role_selection.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "script_recognition_result.h"
#include "script_shaping_provenance.h"

namespace waavs
{
    // A transient resolution of one recognition role against the current
    // ScriptShapingBuffer.
    //
    // itemIndices are valid only until the shaping buffer is structurally
    // modified. Re-resolve the role after replacement, insertion, removal,
    // contraction, or reordering.

    struct ScriptShapingRoleSelection
    {
        ScriptRoleId role{ kScriptRoleInvalid };
        ScriptSpan sourceSpan{};
        std::vector<uint32_t> itemIndices{};

        void clear()
        {
            role = kScriptRoleInvalid;
            sourceSpan = {};
            itemIndices.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return itemIndices.empty(); }
        [[nodiscard]] size_t size() const noexcept { return itemIndices.size(); }
    };


    [[nodiscard]]
    static inline bool selectScriptShapingRole(
        const ScriptRecognitionResult& recognition,
        const ScriptRecognitionUnit& unit,
        ScriptRoleId role,
        const ScriptShapingBuffer& buffer,
        ScriptShapingRoleSelection& result)
    {
        if (role == kScriptRoleInvalid)
            return false;

        ScriptShapingRoleSelection working{};
        working.role = role;

        const ScriptRoleBinding* binding =
            recognition.roleFor(unit, role);

        // An absent optional role is a valid empty selection.
        if (!binding)
        {
            result = std::move(working);
            return true;
        }

        working.sourceSpan = binding->span;

        if (!collectScriptShapingItemIndicesForSourceSpan(
            buffer,
            binding->span,
            working.itemIndices))
        {
            return false;
        }

        // Zero current descendants is also a valid empty selection.
        //
        // A later shaping operation may have consumed or removed every item
        // descended from this source role.

        result = std::move(working);
        return true;
    }

} // namespace waavs