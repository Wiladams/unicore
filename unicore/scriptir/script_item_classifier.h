// script_item_classifier.h
#pragma once

#include <cstdint>

#include "script_recognition_types.h"

namespace waavs
{
    struct UnicodeItem;


    using ScriptItemClassifierFn =
        ScriptItemKindId(*)(const UnicodeItem& item) noexcept;


    struct ScriptItemClassifier
    {
        ScriptItemClassifierFn classifyFn{ nullptr };

        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return classifyFn != nullptr;
        }

        [[nodiscard]]
        ScriptItemKindId classify(const UnicodeItem& item) const noexcept
        {
            return classifyFn
                ? classifyFn(item)
                : kScriptItemKindInvalid;
        }
    };

} // namespace waavs