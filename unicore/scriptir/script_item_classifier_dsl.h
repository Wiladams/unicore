// script_item_classifier_dsl.h
#pragma once

#include <cstdint>
#include <initializer_list>
#include <type_traits>

#include "script_item_classifier_description.h"
#include "script_recognition_dsl.h"

namespace waavs
{
    struct ScriptItemClassifierPredicateRef
    {
        ScriptItemClassifierPredicateId id{ kScriptItemClassifierPredicateInvalid };

        constexpr explicit operator bool() const noexcept
        {
            return id != kScriptItemClassifierPredicateInvalid;
        }
    };


    class ScriptItemClassifierDSL
    {
        ScriptItemClassifierDescription mDescription{};


        ScriptItemClassifierPredicateRef addPredicate(
            ScriptItemClassifierPredicateKind kind,
            std::initializer_list<ScriptItemClassifierPredicateRef> children = {})
        {
            ScriptItemClassifierPredicate predicate{};
            predicate.kind = kind;

            if (!children.size())
            {
                predicate.childOffset = 0;
                predicate.childCount = 0;
            }
            else
            {
                predicate.childOffset = static_cast<uint32_t>(mDescription.predicateChildren.size());
                predicate.childCount = static_cast<uint32_t>(children.size());

                for (const ScriptItemClassifierPredicateRef child : children)
                {
                    if (!child)
                        return {};

                    mDescription.predicateChildren.push_back(child.id);
                }
            }

            const ScriptItemClassifierPredicateId id =
                static_cast<ScriptItemClassifierPredicateId>(mDescription.predicates.size());

            mDescription.predicates.push_back(predicate);
            return { id };
        }


    public:
        void clear()
        {
            mDescription.clear();
        }


        [[nodiscard]]
        const ScriptItemClassifierDescription& description() const noexcept
        {
            return mDescription;
        }


        [[nodiscard]]
        ScriptItemClassifierDescription& description() noexcept
        {
            return mDescription;
        }


        // ------------------------------------------------------------
        // Code point predicates.
        // ------------------------------------------------------------

        ScriptItemClassifierPredicateRef cp(uint32_t value)
        {
            ScriptItemClassifierPredicateRef ref =
                addPredicate(ScriptItemClassifierPredicateKind::CodePointEqual);

            if (!ref)
                return {};

            mDescription.predicates[ref.id].value = value;
            return ref;
        }


        ScriptItemClassifierPredicateRef range(uint32_t first, uint32_t last)
        {
            if (first > last)
                return {};

            ScriptItemClassifierPredicateRef ref =
                addPredicate(ScriptItemClassifierPredicateKind::CodePointRange);

            if (!ref)
                return {};

            ScriptItemClassifierPredicate& predicate = mDescription.predicates[ref.id];
            predicate.value = first;
            predicate.value2 = last;
            return ref;
        }


        // ------------------------------------------------------------
        // Generic property predicate.
        // ------------------------------------------------------------

        ScriptItemClassifierPredicateRef property(
            ScriptItemClassifierProperty property,
            uint32_t value)
        {
            if (property == ScriptItemClassifierProperty::Invalid)
                return {};

            ScriptItemClassifierPredicateRef ref =
                addPredicate(ScriptItemClassifierPredicateKind::PropertyEqual);

            if (!ref)
                return {};

            ScriptItemClassifierPredicate& predicate = mDescription.predicates[ref.id];
            predicate.property = property;
            predicate.value = value;
            return ref;
        }


        template<typename T>
        ScriptItemClassifierPredicateRef property(
            ScriptItemClassifierProperty property,
            T value)
        {
            static_assert(std::is_enum<T>::value, "Classifier property value must be an enum");
            return this->property(property, static_cast<uint32_t>(value));
        }


        // ------------------------------------------------------------
        // Property conveniences.
        // ------------------------------------------------------------

        template<typename T>
        ScriptItemClassifierPredicateRef generalCategory(T value)
        {
            return property(ScriptItemClassifierProperty::GeneralCategory, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef combiningClass(T value)
        {
            return property(ScriptItemClassifierProperty::CombiningClass, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef script(T value)
        {
            return property(ScriptItemClassifierProperty::Script, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef isc(T value)
        {
            return property(ScriptItemClassifierProperty::IndicSyllabicCategory, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef ipc(T value)
        {
            return property(ScriptItemClassifierProperty::IndicPositionalCategory, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef incb(T value)
        {
            return property(ScriptItemClassifierProperty::IndicConjunctBreak, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef joiningType(T value)
        {
            return property(ScriptItemClassifierProperty::JoiningType, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef joiningGroup(T value)
        {
            return property(ScriptItemClassifierProperty::JoiningGroup, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef hangulSyllableType(T value)
        {
            return property(ScriptItemClassifierProperty::HangulSyllableType, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef graphemeClusterBreak(T value)
        {
            return property(ScriptItemClassifierProperty::GraphemeClusterBreak, value);
        }


        template<typename T>
        ScriptItemClassifierPredicateRef bidiClass(T value)
        {
            return property(ScriptItemClassifierProperty::BidiClass, value);
        }


        ScriptItemClassifierPredicateRef defaultIgnorable(bool value = true)
        {
            return property(
                ScriptItemClassifierProperty::DefaultIgnorableCodePoint,
                value ? 1u : 0u);
        }


        ScriptItemClassifierPredicateRef extendedPictographic(bool value = true)
        {
            return property(
                ScriptItemClassifierProperty::ExtendedPictographic,
                value ? 1u : 0u);
        }


        // ------------------------------------------------------------
        // Predicate composition.
        // ------------------------------------------------------------

        ScriptItemClassifierPredicateRef ALL(
            std::initializer_list<ScriptItemClassifierPredicateRef> children)
        {
            if (!children.size())
                return {};

            return addPredicate(
                ScriptItemClassifierPredicateKind::All,
                children);
        }


        ScriptItemClassifierPredicateRef ANY(
            std::initializer_list<ScriptItemClassifierPredicateRef> children)
        {
            if (!children.size())
                return {};

            return addPredicate(
                ScriptItemClassifierPredicateKind::Any,
                children);
        }


        ScriptItemClassifierPredicateRef NOT(ScriptItemClassifierPredicateRef child)
        {
            if (!child)
                return {};

            return addPredicate(
                ScriptItemClassifierPredicateKind::Not,
                { child });
        }


        // ------------------------------------------------------------
        // Rules.
        //
        // Rules are ordered. First matching rule wins.
        // ------------------------------------------------------------

        bool rule(ScriptItemKindRef kind, ScriptItemClassifierPredicateRef predicate)
        {
            if (!kind || !predicate)
                return false;

            ScriptItemClassifierRule rule{};
            rule.kind = kind.id;
            rule.predicate = predicate.id;

            mDescription.rules.push_back(rule);
            return true;
        }


        bool defaultKind(ScriptItemKindRef kind)
        {
            if (!kind)
                return false;

            mDescription.defaultKind = kind.id;
            return true;
        }
    };

} // namespace waavs