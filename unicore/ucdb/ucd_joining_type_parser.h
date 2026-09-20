// ucd_joining_type_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_joining_type.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    enum class UCDJoiningTypeParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingValue,
        UnexpectedField,
        UnknownValue,
        OverlappingRange,
        NoPropertyData
    };


    struct UCDJoiningTypeParseResult
    {
        UCDJoiningTypeParseError error{
            UCDJoiningTypeParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };
    };


    static inline const char* ucdJoiningTypeParseErrorString(
        UCDJoiningTypeParseError error) noexcept
    {
        switch (error)
        {
        case UCDJoiningTypeParseError::None:
            return "none";

        case UCDJoiningTypeParseError::InvalidRange:
            return "invalid Joining_Type code-point range";

        case UCDJoiningTypeParseError::MissingValue:
            return "missing Joining_Type value";

        case UCDJoiningTypeParseError::UnexpectedField:
            return "unexpected extra field in Joining_Type record";

        case UCDJoiningTypeParseError::UnknownValue:
            return "unknown Joining_Type value";

        case UCDJoiningTypeParseError::OverlappingRange:
            return "Joining_Type ranges overlap";

        case UCDJoiningTypeParseError::NoPropertyData:
            return "no Joining_Type data found";
        }

        return "unknown Joining_Type parser error";
    }


    namespace ucd_joining_type_detail
    {
        static inline bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


        static inline bool valueFromField(
            const ByteSpan& field,
            UnicodeJoiningType& outValue) noexcept
        {
            if (field.size() != 1)
                return false;

            switch (field.data()[0])
            {
            case 'C':
                outValue = UnicodeJoiningType::JoinCausing;
                return true;

            case 'D':
                outValue = UnicodeJoiningType::DualJoining;
                return true;

            case 'L':
                outValue = UnicodeJoiningType::LeftJoining;
                return true;

            case 'R':
                outValue = UnicodeJoiningType::RightJoining;
                return true;

            case 'T':
                outValue = UnicodeJoiningType::Transparent;
                return true;

            case 'U':
                outValue = UnicodeJoiningType::NonJoining;
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

    } // namespace ucd_joining_type_detail


    // ========================================================================
    // ucdParseJoiningType
    //
    // Parse extracted/DerivedJoiningType.txt.
    //
    // Records:
    //
    //      code-point-range ; value
    //
    // Unlisted code points default to Non_Joining.
    // ========================================================================

    static inline bool ucdParseJoiningType(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDJoiningTypeParseResult& outResult)
    {
        outResult = {};

        values.clear(
            static_cast<uint8_t>(
                UnicodeJoiningType::NonJoining));


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
                    UCDJoiningTypeParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            UCDCodePointRange range;

            if (!ucdParseCodePointRange(
                rangeField,
                range))
            {
                outResult.error =
                    UCDJoiningTypeParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            ByteSpan valueField;

            if (!ucdReadField(fields, valueField))
            {
                outResult.error =
                    UCDJoiningTypeParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            bspan_trim_spaces(valueField);

            if (!valueField)
            {
                outResult.error =
                    UCDJoiningTypeParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDJoiningTypeParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            UnicodeJoiningType value;

            if (!ucd_joining_type_detail::valueFromField(
                valueField,
                value))
            {
                outResult.error =
                    UCDJoiningTypeParseError::UnknownValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            if (ucd_joining_type_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDJoiningTypeParseError::OverlappingRange;

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
                UCDJoiningTypeParseError::NoPropertyData;

            return false;
        }


        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    static inline bool ucdParseJoiningType(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDJoiningTypeParseResult result;

        return ucdParseJoiningType(
            source,
            values,
            result);
    }

} // namespace waavs