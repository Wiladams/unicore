// unicode_value_property8_info.h

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "unicode_bidi_class.h"
#include "unicode_combining_class.h"
#include "unicode_database_format.h"
#include "unicode_general_category.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_indic_conjunct_break.h"
#include "unicode_indic_syllabic_category.h"
#include "unicode_hangul_syllable_type.h"
#include "unicode_indic_positional_category.h"
#include "unicode_joining_group.h"
#include "unicode_joining_type.h"



namespace waavs
{
    // ========================================================================
    // UnicodeValueProperty8Validation
    //
    // StaticMaximum:
    //      Valid values are 0 .. maximumValue.
    //
    // ScriptIndex:
    //      Valid values depend on the Script table stored in the same database.
    //
    // Additional special validation modes can be added if a future property
    // cannot be described by a simple static maximum.
    // ========================================================================

    enum class UnicodeValueProperty8Validation : uint8_t
    {
        None = 0,
        StaticMaximum,
        ScriptIndex
    };


    // ========================================================================
    // UnicodeValueProperty8Info
    //
    // Runtime/validation description of one semantic VALUE8 property.
    //
    // This is compile-time metadata. It is not persisted in UCDB.
    // ========================================================================

    struct UnicodeValueProperty8Info
    {
        UnicodeValueProperty8 property{ UnicodeValueProperty8Unknown };
        bool required{ false };

        UnicodeValueProperty8Validation validation{
            UnicodeValueProperty8Validation::None
        };

        uint8_t maximumValue{ 0 };
    };


    // ========================================================================
    // UnicodeValueProperty8 metadata
    //
    // The array is indexed directly by UnicodeValueProperty8.
    //
    // Entry zero corresponds to Unknown and is intentionally invalid.
    // ========================================================================

    inline constexpr std::array<
        UnicodeValueProperty8Info,
        static_cast<size_t>(UnicodeValueProperty8MAX) + 1u>
        kUnicodeValueProperty8Info =
    { {
            // Unknown
            {
                .property = UnicodeValueProperty8Unknown,
                .required = false,
                .validation = UnicodeValueProperty8Validation::None,
                .maximumValue = 0
            },

        // General_Category
        {
            .property = UnicodeValueProperty8GeneralCategory,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(
                kUnicodeGeneralCategoryCount - 1u)
        },

        // Canonical_Combining_Class
        {
            .property = UnicodeValueProperty8CanonicalCombiningClass,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(
                kUnicodeCombiningClassMaximum)
        },

        // Bidi_Class
        {
            .property = UnicodeValueProperty8BidiClass,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(
                kUnicodeBidiClassCount - 1u)
        },

        // Grapheme_Cluster_Break
        {
            .property = UnicodeValueProperty8GraphemeClusterBreak,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(
                kUnicodeGraphemeClusterBreakCount - 1u)
        },

        // Indic_Conjunct_Break
        {
            .property = UnicodeValueProperty8IndicConjunctBreak,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(
                kUnicodeIndicConjunctBreakCount - 1u)
        },

        // Script
        {
            .property = UnicodeValueProperty8Script,
            .required = true,
            .validation = UnicodeValueProperty8Validation::ScriptIndex,
            .maximumValue = 0
        },

        // Indic_Syllabic_Category
        {
            .property = UnicodeValueProperty8IndicSyllabicCategory,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(
                kUnicodeIndicSyllabicCategoryCount - 1u)
        },

        // Indic_Positional_Category
        {
    .property = UnicodeValueProperty8IndicPositionalCategory,
    .required = true,
    .validation = UnicodeValueProperty8Validation::StaticMaximum,
    .maximumValue = static_cast<uint8_t>(
        kUnicodeIndicPositionalCategoryCount - 1u)
        },

        // Joining_Type
        {
    .property = UnicodeValueProperty8JoiningType,
    .required = true,
    .validation = UnicodeValueProperty8Validation::StaticMaximum,
    .maximumValue = static_cast<uint8_t>(
        kUnicodeJoiningTypeCount - 1u)
        },

        // Joining_Group
        {
            .property = UnicodeValueProperty8JoiningGroup,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(kUnicodeJoiningGroupCount - 1u)
        },

        // Hangul_Syllable_Type
        {
            .property = UnicodeValueProperty8HangulSyllableType,
            .required = true,
            .validation = UnicodeValueProperty8Validation::StaticMaximum,
            .maximumValue = static_cast<uint8_t>(kUnicodeHangulSyllableTypeCount - 1u)
        }
    }};


    // ========================================================================
    // Compile-time metadata validation
    //
    // Because this array is indexed by the persistent semantic property ID,
    // every entry must describe its own array index.
    // ========================================================================

    consteval bool validateUnicodeValueProperty8Info() noexcept
    {
        for (size_t i = 0; i < kUnicodeValueProperty8Info.size(); ++i)
        {
            if (static_cast<size_t>(
                kUnicodeValueProperty8Info[i].property) != i)
            {
                return false;
            }
        }

        return true;
    }


    static_assert(
        validateUnicodeValueProperty8Info(),
        "Unicode VALUE8 property metadata is inconsistent with property ids");


    // ========================================================================
    // unicodeValueProperty8Info
    // ========================================================================

    [[nodiscard]]
    constexpr const UnicodeValueProperty8Info* unicodeValueProperty8Info(UnicodeValueProperty8 property) noexcept
    {
        const size_t index = static_cast<size_t>(property);

        if (index == static_cast<size_t>(UnicodeValueProperty8Unknown) ||
            index >= kUnicodeValueProperty8Info.size())
        {
            return nullptr;
        }

        return &kUnicodeValueProperty8Info[index];
    }

} // namespace waavs