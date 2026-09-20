// ucd_indic_positional_category_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_indic_positional_category.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDIndicPositionalCategoryParseError
    // ========================================================================

    enum class UCDIndicPositionalCategoryParseError : uint8_t
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
    // UCDIndicPositionalCategoryParseResult
    // ========================================================================

    struct UCDIndicPositionalCategoryParseResult
    {
        UCDIndicPositionalCategoryParseError error{
            UCDIndicPositionalCategoryParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };
    };


    // ========================================================================
    // ucdIndicPositionalCategoryParseErrorString
    // ========================================================================

    static inline const char* ucdIndicPositionalCategoryParseErrorString(
        UCDIndicPositionalCategoryParseError error) noexcept
    {
        switch (error)
        {
        case UCDIndicPositionalCategoryParseError::None:
            return "none";

        case UCDIndicPositionalCategoryParseError::InvalidRange:
            return "invalid Indic_Positional_Category code-point range";

        case UCDIndicPositionalCategoryParseError::MissingValue:
            return "missing Indic_Positional_Category value";

        case UCDIndicPositionalCategoryParseError::UnexpectedField:
            return "unexpected extra field in Indic_Positional_Category record";

        case UCDIndicPositionalCategoryParseError::UnknownValue:
            return "unknown Indic_Positional_Category value";

        case UCDIndicPositionalCategoryParseError::OverlappingRange:
            return "Indic_Positional_Category ranges overlap";

        case UCDIndicPositionalCategoryParseError::NoPropertyData:
            return "no Indic_Positional_Category data found";
        }

        return "unknown Indic_Positional_Category parser error";
    }


    namespace ucd_indic_positional_category_detail
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
            UnicodeIndicPositionalCategory& outValue) noexcept
        {
            struct NameRecord
            {
                const char* name;
                UnicodeIndicPositionalCategory value;
            };


            static constexpr NameRecord names[] =
            {
                {
                    "Not_Applicable",
                    UnicodeIndicPositionalCategory::NotApplicable
                },
                {
                    "Bottom",
                    UnicodeIndicPositionalCategory::Bottom
                },
                {
                    "Bottom_And_Left",
                    UnicodeIndicPositionalCategory::BottomAndLeft
                },
                {
                    "Bottom_And_Right",
                    UnicodeIndicPositionalCategory::BottomAndRight
                },
                {
                    "Left",
                    UnicodeIndicPositionalCategory::Left
                },
                {
                    "Left_And_Right",
                    UnicodeIndicPositionalCategory::LeftAndRight
                },
                {
                    "Overstruck",
                    UnicodeIndicPositionalCategory::Overstruck
                },
                {
                    "Right",
                    UnicodeIndicPositionalCategory::Right
                },
                {
                    "Top",
                    UnicodeIndicPositionalCategory::Top
                },
                {
                    "Top_And_Bottom",
                    UnicodeIndicPositionalCategory::TopAndBottom
                },
                {
                    "Top_And_Bottom_And_Left",
                    UnicodeIndicPositionalCategory::TopAndBottomAndLeft
                },
                {
                    "Top_And_Bottom_And_Right",
                    UnicodeIndicPositionalCategory::TopAndBottomAndRight
                },
                {
                    "Top_And_Left",
                    UnicodeIndicPositionalCategory::TopAndLeft
                },
                {
                    "Top_And_Left_And_Right",
                    UnicodeIndicPositionalCategory::TopAndLeftAndRight
                },
                {
                    "Top_And_Right",
                    UnicodeIndicPositionalCategory::TopAndRight
                },
                {
                    "Visual_Order_Left",
                    UnicodeIndicPositionalCategory::VisualOrderLeft
                }
            };


            for (const NameRecord& record : names)
            {
                if (spanEquals(field, record.name))
                {
                    outValue = record.value;
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
            uint32_t first,
            uint32_t last) noexcept
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

    } // namespace ucd_indic_positional_category_detail


    // ========================================================================
    // ucdParseIndicPositionalCategory
    //
    // Parse IndicPositionalCategory.txt.
    //
    // Records have the form:
    //
    //      code-point-range ; value
    //
    // All code points not explicitly listed have the documented default:
    //
    //      Not_Applicable
    //
    // ========================================================================

    static inline bool ucdParseIndicPositionalCategory(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDIndicPositionalCategoryParseResult& outResult)
    {
        outResult = {};

        values.clear(
            static_cast<uint8_t>(
                UnicodeIndicPositionalCategory::NotApplicable));


        UnicodeCoverageBuilder assignedCoverage;

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            // ----------------------------------------------------------------
            // Field 0: code point or code-point range.
            // ----------------------------------------------------------------

            ByteSpan rangeField;

            if (!ucdReadField(fields, rangeField))
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            UCDCodePointRange range;

            if (!ucdParseCodePointRange(
                rangeField,
                range))
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 1: Indic_Positional_Category.
            // ----------------------------------------------------------------

            ByteSpan valueField;

            if (!ucdReadField(fields, valueField))
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            bspan_trim_spaces(valueField);

            if (!valueField)
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // No further fields are allowed.
            //
            // UCDParser has already removed the trailing # comment.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Decode value.
            // ----------------------------------------------------------------

            UnicodeIndicPositionalCategory value;

            if (!ucd_indic_positional_category_detail::valueFromField(
                valueField,
                value))
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::UnknownValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // IPC is single-valued. Explicit ranges must not overlap.
            // ----------------------------------------------------------------

            if (ucd_indic_positional_category_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDIndicPositionalCategoryParseError::OverlappingRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Store explicit value.
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


        // ====================================================================
        // Ensure the property file actually contained IPC data.
        // ====================================================================

        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDIndicPositionalCategoryParseError::NoPropertyData;

            return false;
        }


        // ====================================================================
        // Remaining Unicode positions have the default Not_Applicable value.
        // ====================================================================

        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseIndicPositionalCategory(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDIndicPositionalCategoryParseResult result;

        return ucdParseIndicPositionalCategory(
            source,
            values,
            result);
    }

} // namespace waavs