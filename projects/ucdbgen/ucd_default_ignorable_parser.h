// ucd_default_ignorable_parser.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"


namespace waavs
{
    enum class UCDDefaultIgnorableCodePointParseError : uint8_t
    {
        None = 0,
        MissingRange,
        MissingProperty,
        InvalidRange,
        UnexpectedField,
        OverlappingRange,
        MissingPropertyData
    };


    struct UCDDefaultIgnorableCodePointParseResult
    {
        UCDDefaultIgnorableCodePointParseError error{
            UCDDefaultIgnorableCodePointParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };
        size_t codePoints{ 0 };
    };


    static inline const char* ucdDefaultIgnorableCodePointParseErrorString(
        UCDDefaultIgnorableCodePointParseError error) noexcept
    {
        switch (error)
        {
        case UCDDefaultIgnorableCodePointParseError::None:
            return "none";

        case UCDDefaultIgnorableCodePointParseError::MissingRange:
            return "missing code-point range";

        case UCDDefaultIgnorableCodePointParseError::MissingProperty:
            return "missing property name";

        case UCDDefaultIgnorableCodePointParseError::InvalidRange:
            return "invalid code-point range";

        case UCDDefaultIgnorableCodePointParseError::UnexpectedField:
            return "unexpected field";

        case UCDDefaultIgnorableCodePointParseError::OverlappingRange:
            return "overlapping Default_Ignorable_Code_Point range";

        case UCDDefaultIgnorableCodePointParseError::MissingPropertyData:
            return "Default_Ignorable_Code_Point property not found";
        }

        return "unknown error";
    }


    namespace ucd_default_ignorable_detail
    {
        static inline bool coverageIntersectsRange(
            const UnicodeCoverageBuilder& coverage,
            uint32_t first, uint32_t last) noexcept
        {
            uint32_t cp = first;

            for (;;)
            {
                if (coverage.contains(cp))
                    return true;

                if (cp == last)
                    break;

                ++cp;
            }

            return false;
        }
    }


    // ========================================================================
    // ucdParseDefaultIgnorableCodePoint
    //
    // Selectively parse:
    //
    //      DerivedCoreProperties.txt
    //
    // extracting only:
    //
    //      Default_Ignorable_Code_Point
    //
    // Other properties in the file are ignored.
    // ========================================================================

    static inline bool ucdParseDefaultIgnorableCodePoint(
        const ByteSpan& source,
        UnicodeCoverageBuilder& coverage,
        UCDDefaultIgnorableCodePointParseResult& outResult)
    {
        outResult = {};

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            // ---------------------------------------------------------------
            // Field 1: code-point range
            // ---------------------------------------------------------------

            ByteSpan rangeField;

            if (!ucdReadField(fields, rangeField) || !rangeField)
            {
                outResult.error =
                    UCDDefaultIgnorableCodePointParseError::MissingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Field 2: property name
            // ---------------------------------------------------------------

            ByteSpan propertyField;

            if (!ucdReadField(fields, propertyField) || !propertyField)
            {
                outResult.error =
                    UCDDefaultIgnorableCodePointParseError::MissingProperty;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // DerivedCoreProperties.txt contains many properties.
            // Ignore everything except Default_Ignorable_Code_Point.
            // ---------------------------------------------------------------

            if (!ucdFieldEquals(
                propertyField,
                "Default_Ignorable_Code_Point"))
            {
                continue;
            }


            // ---------------------------------------------------------------
            // Selected records contain exactly two fields.
            // ---------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDDefaultIgnorableCodePointParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Parse range.
            // ---------------------------------------------------------------

            UCDCodePointRange range{};

            if (!ucdParseCodePointRange(
                rangeField,
                range))
            {
                outResult.error =
                    UCDDefaultIgnorableCodePointParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Binary-property ranges should not overlap each other.
            // ---------------------------------------------------------------

            if (ucd_default_ignorable_detail::coverageIntersectsRange(
                coverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDDefaultIgnorableCodePointParseError::OverlappingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            coverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;

            outResult.codePoints +=
                static_cast<size_t>(
                    range.last - range.first + 1u);
        }


        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDDefaultIgnorableCodePointParseError::MissingPropertyData;

            return false;
        }


        return true;
    }


    static inline bool ucdParseDefaultIgnorableCodePoint(
        const ByteSpan& source,
        UnicodeCoverageBuilder& coverage)
    {
        UCDDefaultIgnorableCodePointParseResult result;

        return ucdParseDefaultIgnorableCodePoint(
            source,
            coverage,
            result);
    }

} // namespace waavs