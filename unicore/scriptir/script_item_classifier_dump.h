// script_item_classifier_dump.h
#pragma once

#include <cstdio>
#include <string>

#include "script_item_classifier_description.h"
#include "script_item_classifier_validator.h"

#include "unicode_bidi_class.h"
#include "unicode_general_category.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_hangul_syllable_type.h"
#include "unicode_indic_conjunct_break.h"
#include "unicode_indic_positional_category.h"
#include "unicode_indic_syllabic_category.h"
#include "unicode_joining_group.h"
#include "unicode_joining_type.h"

namespace waavs
{
    static inline const char* scriptItemKindName(
        const ScriptRecognitionDescription& recognition,
        ScriptItemKindId id) noexcept
    {
        if (id == kScriptItemKindInvalid || id > recognition.kinds.size())
            return nullptr;

        return recognition.kinds[id - 1].name.c_str();
    }


    static inline void appendScriptItemClassifierIndent(std::string& output, uint32_t depth)
    {
        for (uint32_t i = 0; i < depth; ++i)
            output += "    ";
    }


    static inline bool appendScriptItemClassifierPropertyValue(
        ScriptItemClassifierProperty property,
        uint32_t value,
        std::string& output)
    {
        const char* name = nullptr;

        switch (property)
        {
        case ScriptItemClassifierProperty::GeneralCategory:
            if (value >= kUnicodeGeneralCategoryCount)
                return false;

            name = unicodeGeneralCategoryName(
                static_cast<UnicodeGeneralCategory>(value));
            break;

        case ScriptItemClassifierProperty::CombiningClass:
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%u", value);
            output += buffer;
            return true;
        }

        case ScriptItemClassifierProperty::Script:
        {
            // Script naming is deferred until UnicodeScript has a generated
            // enum/name table. Keep this deterministic in the meantime.
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%u", value);
            output += buffer;
            return true;
        }

        case ScriptItemClassifierProperty::IndicSyllabicCategory:
            if (value >= kUnicodeIndicSyllabicCategoryCount)
                return false;

            name = unicodeIndicSyllabicCategoryName(
                static_cast<UnicodeIndicSyllabicCategory>(value));
            break;

        case ScriptItemClassifierProperty::IndicPositionalCategory:
            if (value >= kUnicodeIndicPositionalCategoryCount)
                return false;

            name = unicodeIndicPositionalCategoryName(
                static_cast<UnicodeIndicPositionalCategory>(value));
            break;

        case ScriptItemClassifierProperty::IndicConjunctBreak:
            if (value >= kUnicodeIndicConjunctBreakCount)
                return false;

            name = unicodeIndicConjunctBreakName(
                static_cast<UnicodeIndicConjunctBreak>(value));
            break;

        case ScriptItemClassifierProperty::JoiningType:
            if (value >= kUnicodeJoiningTypeCount)
                return false;

            name = unicodeJoiningTypeName(
                static_cast<UnicodeJoiningType>(value));
            break;

        case ScriptItemClassifierProperty::JoiningGroup:
            if (value >= kUnicodeJoiningGroupCount)
                return false;

            name = unicodeJoiningGroupName(
                static_cast<UnicodeJoiningGroup>(value));
            break;

        case ScriptItemClassifierProperty::HangulSyllableType:
            if (value >= kUnicodeHangulSyllableTypeCount)
                return false;

            name = unicodeHangulSyllableTypeName(
                static_cast<UnicodeHangulSyllableType>(value));
            break;

        case ScriptItemClassifierProperty::GraphemeClusterBreak:
            if (value >= kUnicodeGraphemeClusterBreakCount)
                return false;

            name = unicodeGraphemeClusterBreakName(
                static_cast<UnicodeGraphemeClusterBreak>(value));
            break;

        case ScriptItemClassifierProperty::BidiClass:
            if (value >= kUnicodeBidiClassCount)
                return false;

            name = unicodeBidiClassName(
                static_cast<UnicodeBidiClass>(value));
            break;

        case ScriptItemClassifierProperty::DefaultIgnorableCodePoint:
        case ScriptItemClassifierProperty::ExtendedPictographic:
            output += value ? "true" : "false";
            return true;

        default:
            return false;
        }

        if (!name)
            return false;

        output += name;
        return true;
    }


    static inline const char* scriptItemClassifierPropertyName(
        ScriptItemClassifierProperty property) noexcept
    {
        switch (property)
        {
        case ScriptItemClassifierProperty::GeneralCategory:
            return "generalCategory";

        case ScriptItemClassifierProperty::CombiningClass:
            return "combiningClass";

        case ScriptItemClassifierProperty::Script:
            return "script";

        case ScriptItemClassifierProperty::IndicSyllabicCategory:
            return "isc";

        case ScriptItemClassifierProperty::IndicPositionalCategory:
            return "ipc";

        case ScriptItemClassifierProperty::IndicConjunctBreak:
            return "incb";

        case ScriptItemClassifierProperty::JoiningType:
            return "joiningType";

        case ScriptItemClassifierProperty::JoiningGroup:
            return "joiningGroup";

        case ScriptItemClassifierProperty::HangulSyllableType:
            return "hangulSyllableType";

        case ScriptItemClassifierProperty::GraphemeClusterBreak:
            return "graphemeClusterBreak";

        case ScriptItemClassifierProperty::BidiClass:
            return "bidiClass";

        case ScriptItemClassifierProperty::DefaultIgnorableCodePoint:
            return "defaultIgnorable";

        case ScriptItemClassifierProperty::ExtendedPictographic:
            return "extendedPictographic";

        default:
            return nullptr;
        }
    }


    static inline bool appendScriptItemClassifierPredicate(
        const ScriptItemClassifierDescription& classifier,
        ScriptItemClassifierPredicateId id,
        std::string& output,
        uint32_t depth)
    {
        const ScriptItemClassifierPredicate* predicate =
            classifier.predicate(id);

        if (!predicate)
            return false;

        const ScriptItemClassifierPredicateId* children =
            classifier.children(*predicate);

        if (predicate->childCount != 0 && !children)
            return false;


        switch (predicate->kind)
        {
        case ScriptItemClassifierPredicateKind::CodePointEqual:
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "cp(U+%04X)", predicate->value);
            output += buffer;
            return true;
        }


        case ScriptItemClassifierPredicateKind::CodePointRange:
        {
            char buffer[32];

            std::snprintf(
                buffer,
                sizeof(buffer),
                "range(U+%04X, U+%04X)",
                predicate->value,
                predicate->value2);

            output += buffer;
            return true;
        }


        case ScriptItemClassifierPredicateKind::PropertyEqual:
        {
            const char* propertyName =
                scriptItemClassifierPropertyName(
                    predicate->property);

            if (!propertyName)
                return false;

            output += propertyName;
            output += "(";

            if (!appendScriptItemClassifierPropertyValue(
                predicate->property,
                predicate->value,
                output))
            {
                return false;
            }

            output += ")";
            return true;
        }


        case ScriptItemClassifierPredicateKind::All:
        case ScriptItemClassifierPredicateKind::Any:
        {
            if (predicate->childCount == 0)
                return false;

            output += predicate->kind == ScriptItemClassifierPredicateKind::All
                ? "ALL(\n"
                : "ANY(\n";

            for (uint32_t i = 0; i < predicate->childCount; ++i)
            {
                appendScriptItemClassifierIndent(output, depth + 1);

                if (!appendScriptItemClassifierPredicate(
                    classifier,
                    children[i],
                    output,
                    depth + 1))
                {
                    return false;
                }

                if (i + 1 != predicate->childCount)
                    output += ",";

                output += "\n";
            }

            appendScriptItemClassifierIndent(output, depth);
            output += ")";
            return true;
        }


        case ScriptItemClassifierPredicateKind::Not:
            if (predicate->childCount != 1)
                return false;

            output += "NOT(";

            if (!appendScriptItemClassifierPredicate(
                classifier,
                children[0],
                output,
                depth))
            {
                return false;
            }

            output += ")";
            return true;


        default:
            return false;
        }
    }


    [[nodiscard]]
    static inline bool dumpScriptItemClassifierDescription(
        const ScriptItemClassifierDescription& classifier,
        const ScriptRecognitionDescription& recognition,
        std::string& output)
    {
        output.clear();

        const ScriptItemClassifierValidationResult validation =
            validateScriptItemClassifierDescription(
                classifier,
                recognition);

        if (!validation)
            return false;


        for (size_t i = 0; i < classifier.rules.size(); ++i)
        {
            const ScriptItemClassifierRule& rule =
                classifier.rules[i];

            const char* kindName =
                scriptItemKindName(
                    recognition,
                    rule.kind);

            if (!kindName)
                return false;

            output += "rule ";
            output += kindName;
            output += " =\n    ";

            if (!appendScriptItemClassifierPredicate(
                classifier,
                rule.predicate,
                output,
                1))
            {
                return false;
            }

            output += "\n\n";
        }


        const char* defaultKindName =
            scriptItemKindName(
                recognition,
                classifier.defaultKind);

        if (!defaultKindName)
            return false;

        output += "default ";
        output += defaultKindName;
        output += "\n";

        return true;
    }


    [[nodiscard]]
    static inline std::string dumpScriptItemClassifierDescription(
        const ScriptItemClassifierDescription& classifier,
        const ScriptRecognitionDescription& recognition)
    {
        std::string output;

        if (!dumpScriptItemClassifierDescription(
            classifier,
            recognition,
            output))
        {
            output.clear();
        }

        return output;
    }

} // namespace waavs