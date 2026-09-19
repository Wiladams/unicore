// ucd_indic_syllabic_category_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_indic_syllabic_category.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDIndicSyllabicCategoryParseError
    // ========================================================================

    enum class UCDIndicSyllabicCategoryParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingValue,
        UnexpectedField,
        UnknownValue,
        OverlappingRange,
        NoPropertyData
    };


    // ========================================================================
    // UCDIndicSyllabicCategoryParseResult
    // ========================================================================

    struct UCDIndicSyllabicCategoryParseResult
    {
        UCDIndicSyllabicCategoryParseError error{
            UCDIndicSyllabicCategoryParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UCDIndicSyllabicCategoryParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // ucdIndicSyllabicCategoryParseErrorString
    // ========================================================================

    static inline const char* ucdIndicSyllabicCategoryParseErrorString(
        UCDIndicSyllabicCategoryParseError error) noexcept
    {
        switch (error)
        {
        case UCDIndicSyllabicCategoryParseError::None:
            return "no error";

        case UCDIndicSyllabicCategoryParseError::InvalidRange:
            return "invalid Indic_Syllabic_Category code-point range";

        case UCDIndicSyllabicCategoryParseError::MissingValue:
            return "missing Indic_Syllabic_Category value";

        case UCDIndicSyllabicCategoryParseError::UnexpectedField:
            return "unexpected extra field in Indic_Syllabic_Category record";

        case UCDIndicSyllabicCategoryParseError::UnknownValue:
            return "unknown Indic_Syllabic_Category value";

        case UCDIndicSyllabicCategoryParseError::OverlappingRange:
            return "Indic_Syllabic_Category ranges overlap";

        case UCDIndicSyllabicCategoryParseError::NoPropertyData:
            return "no Indic_Syllabic_Category data found";
        }

        return "unknown Indic_Syllabic_Category parser error";
    }


    namespace ucd_indic_syllabic_category_detail
    {
        // ====================================================================
        // spanEquals
        // ====================================================================

        static inline bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


        // ====================================================================
        // valueFromField
        // ====================================================================

        static inline bool valueFromField(
            const ByteSpan& field,
            UnicodeIndicSyllabicCategory& outValue) noexcept
        {
            struct NameRecord
            {
                const char* name;
                UnicodeIndicSyllabicCategory value;
            };


            static constexpr NameRecord names[] =
            {
                { "Other",                      UnicodeIndicSyllabicCategory::Other },

                { "Avagraha",                   UnicodeIndicSyllabicCategory::Avagraha },
                { "Bindu",                      UnicodeIndicSyllabicCategory::Bindu },
                { "Brahmi_Joining_Number",      UnicodeIndicSyllabicCategory::BrahmiJoiningNumber },
                { "Cantillation_Mark",          UnicodeIndicSyllabicCategory::CantillationMark },

                { "Consonant",                  UnicodeIndicSyllabicCategory::Consonant },
                { "Consonant_Dead",             UnicodeIndicSyllabicCategory::ConsonantDead },
                { "Consonant_Final",            UnicodeIndicSyllabicCategory::ConsonantFinal },
                { "Consonant_Head_Letter",      UnicodeIndicSyllabicCategory::ConsonantHeadLetter },
                { "Consonant_Initial_Postfixed",UnicodeIndicSyllabicCategory::ConsonantInitialPostfixed },
                { "Consonant_Killer",           UnicodeIndicSyllabicCategory::ConsonantKiller },
                { "Consonant_Medial",           UnicodeIndicSyllabicCategory::ConsonantMedial },
                { "Consonant_Placeholder",      UnicodeIndicSyllabicCategory::ConsonantPlaceholder },
                { "Consonant_Preceding_Repha",  UnicodeIndicSyllabicCategory::ConsonantPrecedingRepha },
                { "Consonant_Prefixed",         UnicodeIndicSyllabicCategory::ConsonantPrefixed },
                { "Consonant_Subjoined",        UnicodeIndicSyllabicCategory::ConsonantSubjoined },
                { "Consonant_Succeeding_Repha", UnicodeIndicSyllabicCategory::ConsonantSucceedingRepha },
                { "Consonant_With_Stacker",     UnicodeIndicSyllabicCategory::ConsonantWithStacker },

                { "Gemination_Mark",             UnicodeIndicSyllabicCategory::GeminationMark },
                { "Invisible_Stacker",           UnicodeIndicSyllabicCategory::InvisibleStacker },
                { "Joiner",                      UnicodeIndicSyllabicCategory::Joiner },
                { "Modifying_Letter",            UnicodeIndicSyllabicCategory::ModifyingLetter },
                { "Non_Joiner",                  UnicodeIndicSyllabicCategory::NonJoiner },
                { "Nukta",                       UnicodeIndicSyllabicCategory::Nukta },

                { "Number",                      UnicodeIndicSyllabicCategory::Number },
                { "Number_Joiner",               UnicodeIndicSyllabicCategory::NumberJoiner },

                { "Pure_Killer",                 UnicodeIndicSyllabicCategory::PureKiller },
                { "Register_Shifter",            UnicodeIndicSyllabicCategory::RegisterShifter },
                { "Reordering_Killer",           UnicodeIndicSyllabicCategory::ReorderingKiller },
                { "Syllable_Modifier",           UnicodeIndicSyllabicCategory::SyllableModifier },

                { "Tone_Letter",                 UnicodeIndicSyllabicCategory::ToneLetter },
                { "Tone_Mark",                   UnicodeIndicSyllabicCategory::ToneMark },

                { "Virama",                      UnicodeIndicSyllabicCategory::Virama },
                { "Visarga",                     UnicodeIndicSyllabicCategory::Visarga },

                { "Vowel",                       UnicodeIndicSyllabicCategory::Vowel },
                { "Vowel_Dependent",             UnicodeIndicSyllabicCategory::VowelDependent },
                { "Vowel_Independent",           UnicodeIndicSyllabicCategory::VowelIndependent }
            };


            for (const NameRecord& name : names)
            {
                if (spanEquals(field, name.name))
                {
                    outValue = name.value;
                    return true;
                }
            }


            return false;
        }


        // ====================================================================
        // coverageIntersectsRange
        // ====================================================================

        static inline bool coverageIntersectsRange(
            const UnicodeCoverageBuilder& coverage,
            uint32_t first, uint32_t last) noexcept
        {
            for (uint32_t cp = first;; ++cp)
            {
                if (coverage.contains(cp))
                    return true;

                if (cp == last)
                    break;
            }

            return false;
        }

    } // namespace ucd_indic_syllabic_category_detail


    // ========================================================================
    // ucdParseIndicSyllabicCategory
    //
    // Parse IndicSyllabicCategory.txt into a mutable VALUE8 table.
    //
    // Expected meaningful line syntax:
    //
    //      code-point-range ; Indic_Syllabic_Category
    //
    // Unicode 17.0.0 defines:
    //
    //      @missing: 0000..10FFFF; Other
    //
    // UCDParser intentionally ignores comment-only @missing records, so seed
    // the complete table with Other and apply explicit assignments afterward.
    //
    // Explicit ranges are single-valued and therefore may not overlap.
    // ========================================================================

    static inline bool ucdParseIndicSyllabicCategory(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDIndicSyllabicCategoryParseResult& outResult)
    {
        outResult = {};


        // --------------------------------------------------------------------
        // Documented default.
        // --------------------------------------------------------------------

        values.clear(
            static_cast<uint8_t>(
                UnicodeIndicSyllabicCategory::Other));


        // --------------------------------------------------------------------
        // Explicit assignment coverage.
        // --------------------------------------------------------------------

        UnicodeCoverageBuilder assignedCoverage;

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            // ----------------------------------------------------------------

            UCDCodePointRange range;

            if (!ucdReadCodePointRange(fields, range))
            {
                outResult.error =
                    UCDIndicSyllabicCategoryParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: Indic_Syllabic_Category
            // ----------------------------------------------------------------

            ByteSpan valueField;

            if (!ucdReadField(fields, valueField) || !valueField)
            {
                outResult.error =
                    UCDIndicSyllabicCategoryParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // IndicSyllabicCategory.txt records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDIndicSyllabicCategoryParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Decode property value.
            // ----------------------------------------------------------------

            UnicodeIndicSyllabicCategory value;

            if (!ucd_indic_syllabic_category_detail::valueFromField(
                valueField, value))
            {
                outResult.error =
                    UCDIndicSyllabicCategoryParseError::UnknownValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Indic_Syllabic_Category is single-valued.
            // ----------------------------------------------------------------

            if (ucd_indic_syllabic_category_detail::coverageIntersectsRange(
                assignedCoverage, range.first, range.last))
            {
                outResult.error =
                    UCDIndicSyllabicCategoryParseError::OverlappingRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Store explicit assignment.
            // ----------------------------------------------------------------

            values.setRange(
                range.first,
                range.last,
                static_cast<uint8_t>(value));

            assignedCoverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                static_cast<size_t>(
                    range.last - range.first + 1u);
        }


        // --------------------------------------------------------------------
        // Make sure the expected property data was actually present.
        // --------------------------------------------------------------------

        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDIndicSyllabicCategoryParseError::NoPropertyData;

            return false;
        }


        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseIndicSyllabicCategory(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDIndicSyllabicCategoryParseResult result;

        return ucdParseIndicSyllabicCategory(
            source,
            values,
            result);
    }

} // namespace waavs