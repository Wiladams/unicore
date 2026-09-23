// script_item_classifier_ucdb.h
#pragma once

#include <cstdint>

#include "script_item_classifier_eval.h"
#include "unicode_database.h"

namespace waavs
{
    [[nodiscard]]
    static inline ScriptItemClassifierFacts scriptItemClassifierFacts(
        const UnicodeDatabase& database,
        uint32_t codePoint) noexcept
    {
        ScriptItemClassifierFacts facts{};

        facts.codePoint = codePoint;

        facts.generalCategory =
            database.generalCategory(codePoint);

        facts.combiningClass =
            database.combiningClass(codePoint);

        facts.indicSyllabicCategory =
            database.indicSyllabicCategory(codePoint);

        facts.indicPositionalCategory =
            database.indicPositionalCategory(codePoint);

        facts.indicConjunctBreak =
            database.indicConjunctBreak(codePoint);

        facts.joiningType =
            database.joiningType(codePoint);

        facts.joiningGroup =
            database.joiningGroup(codePoint);

        facts.hangulSyllableType =
            database.hangulSyllableType(codePoint);

        facts.graphemeClusterBreak =
            database.graphemeClusterBreak(codePoint);

        facts.bidiClass =
            database.bidiClass(codePoint);

        facts.script =
            static_cast<uint32_t>(
                database.script(codePoint));

        facts.defaultIgnorableCodePoint =
            database.isDefaultIgnorableCodePoint(codePoint);

        facts.extendedPictographic =
            database.isExtendedPictographic(codePoint);

        return facts;
    }


    [[nodiscard]]
    static inline ScriptItemKindId classifyScriptItem(
        const ScriptItemClassifierDescription& description,
        const UnicodeDatabase& database,
        uint32_t codePoint) noexcept
    {
        const ScriptItemClassifierFacts facts =
            scriptItemClassifierFacts(
                database,
                codePoint);

        return classifyScriptItem(
            description,
            facts);
    }

} // namespace waavs