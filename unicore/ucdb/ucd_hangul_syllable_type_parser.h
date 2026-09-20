// ucd_hangul_syllable_type_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_hangul_syllable_type.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    enum class UCDHangulSyllableTypeParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingValue,
        UnexpectedField,
        UnknownValue,
        OverlappingRange,
        NoPropertyData
    };


    struct UCDHangulSyllableTypeParseResult
    {
        UCDHangulSyllableTypeParseError error{
            UCDHangulSyllableTypeParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };
    };


    static inline const char* ucdHangulSyllableTypeParseErrorString(
        UCDHangulSyllableTypeParseError error) noexcept
    {
        switch (error)
        {
        case UCDHangulSyllableTypeParseError::None:
            return "none";

        case UCDHangulSyllableTypeParseError::InvalidRange:
            return "invalid Hangul_Syllable_Type code-point range";

        case UCDHangulSyllableTypeParseError::MissingValue:
            return "missing Hangul_Syllable_Type value";

        case UCDHangulSyllableTypeParseError::UnexpectedField:
            return "unexpected extra field in Hangul_Syllable_Type record";

        case UCDHangulSyllableTypeParseError::UnknownValue:
            return "unknown Hangul_Syllable_Type value";

        case UCDHangulSyllableTypeParseError::OverlappingRange:
            return "Hangul_Syllable_Type ranges overlap";

        case UCDHangulSyllableTypeParseError::NoPropertyData:
            return "no Hangul_Syllable_Type data found";
        }

        return "unknown Hangul_Syllable_Type parser error";
    }


    namespace ucd_hangul_syllable_type_detail
    {
        static inline bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


        static inline bool valueFromField(
            const ByteSpan& field,
            UnicodeHangulSyllableType& outValue) noexcept
        {
            if (field.size() == 1)
            {
                switch (field.data()[0])
                {
                case 'L':
                    outValue = UnicodeHangulSyllableType::LeadingJamo;
                    return true;

                case 'V':
                    outValue = UnicodeHangulSyllableType::VowelJamo;
                    return true;

                case 'T':
                    outValue = UnicodeHangulSyllableType::TrailingJamo;
                    return true;
                }

                return false;
            }

            if (field.size() == 2 &&
                field.data()[0] == 'L' &&
                field.data()[1] == 'V')
            {
                outValue = UnicodeHangulSyllableType::LVSyllable;
                return true;
            }

            if (field.size() == 3 &&
                field.data()[0] == 'L' &&
                field.data()[1] == 'V' &&
                field.data()[2] == 'T')
            {
                outValue = UnicodeHangulSyllableType::LVTSyllable;
                return true;
            }

            return false;
        }


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

    } // namespace ucd_hangul_syllable_type_detail


    // ========================================================================
    // ucdParseHangulSyllableType
    //
    // Parse HangulSyllableType.txt.
    //
    // Records:
    //
    //      code-point-range ; value
    //
    // Unlisted code points default to Not_Applicable.
    // ========================================================================

    static inline bool ucdParseHangulSyllableType(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDHangulSyllableTypeParseResult& outResult)
    {
        outResult = {};

        values.clear(
            static_cast<uint8_t>(
                UnicodeHangulSyllableType::NotApplicable));


        UnicodeCoverageBuilder assignedCoverage;

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            ByteSpan rangeField;

            if (!ucdReadField(fields, rangeField))
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            UCDCodePointRange range;

            if (!ucdParseCodePointRange(
                rangeField,
                range))
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            ByteSpan valueField;

            if (!ucdReadField(fields, valueField))
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            bspan_trim_spaces(valueField);

            if (!valueField)
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            UnicodeHangulSyllableType value;

            if (!ucd_hangul_syllable_type_detail::valueFromField(
                valueField,
                value))
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::UnknownValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            if (ucd_hangul_syllable_type_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDHangulSyllableTypeParseError::OverlappingRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


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


        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDHangulSyllableTypeParseError::NoPropertyData;

            return false;
        }


        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    static inline bool ucdParseHangulSyllableType(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDHangulSyllableTypeParseResult result;

        return ucdParseHangulSyllableType(
            source,
            values,
            result);
    }

} // namespace waavs