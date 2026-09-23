// script_item_classifier_description.h
#pragma once

#include <vector>

#include "script_item_classifier_types.h"

namespace waavs
{
    struct ScriptItemClassifierDescription
    {
        std::vector<ScriptItemClassifierPredicate> predicates{};
        std::vector<ScriptItemClassifierPredicateId> predicateChildren{};
        std::vector<ScriptItemClassifierRule> rules{};

        ScriptItemKindId defaultKind{ kScriptItemKindInvalid };


        void clear()
        {
            predicates.clear();
            predicateChildren.clear();
            rules.clear();
            defaultKind = kScriptItemKindInvalid;
        }


        [[nodiscard]]
        bool empty() const noexcept
        {
            return rules.empty();
        }


        [[nodiscard]]
        const ScriptItemClassifierPredicate* predicate(ScriptItemClassifierPredicateId id) const noexcept
        {
            return id < predicates.size()
                ? &predicates[id]
                : nullptr;
        }


        [[nodiscard]]
        const ScriptItemClassifierPredicateId* children(const ScriptItemClassifierPredicate& predicate) const noexcept
        {
            if (predicate.childCount == 0)
                return nullptr;

            if (predicate.childOffset > predicateChildren.size())
                return nullptr;

            if (predicate.childCount > predicateChildren.size() - predicate.childOffset)
                return nullptr;

            return predicateChildren.data() + predicate.childOffset;
        }
    };

} // namespace waavs