// script_item_classifier_types.h
#pragma once

#include <cstdint>

#include "script_recognition_types.h"

namespace waavs
{
    using ScriptItemClassifierPredicateId = uint32_t;

    static constexpr ScriptItemClassifierPredicateId kScriptItemClassifierPredicateInvalid = 0xFFFFFFFFu;


    enum class ScriptItemClassifierProperty : uint8_t
    {
        Invalid = 0,

        GeneralCategory,
        CombiningClass,
        Script,

        IndicSyllabicCategory,
        IndicPositionalCategory,
        IndicConjunctBreak,

        JoiningType,
        JoiningGroup,

        HangulSyllableType,

        GraphemeClusterBreak,
        BidiClass,

        DefaultIgnorableCodePoint,
        ExtendedPictographic
    };


    enum class ScriptItemClassifierPredicateKind : uint8_t
    {
        Invalid = 0,

        CodePointEqual,
        CodePointRange,

        PropertyEqual,

        All,
        Any,
        Not
    };


    struct ScriptItemClassifierPredicate
    {
        ScriptItemClassifierPredicateKind kind{ ScriptItemClassifierPredicateKind::Invalid };

        ScriptItemClassifierProperty property{ ScriptItemClassifierProperty::Invalid };

        uint32_t value{ 0 };
        uint32_t value2{ 0 };

        uint32_t childOffset{ 0 };
        uint32_t childCount{ 0 };
    };


    struct ScriptItemClassifierRule
    {
        ScriptItemKindId kind{ kScriptItemKindInvalid };
        ScriptItemClassifierPredicateId predicate{ kScriptItemClassifierPredicateInvalid };
    };

} // namespace waavs