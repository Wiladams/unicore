// test_unicode_database_indic_syllabic_category.h

#pragma once

#include "test_core.h"

#include <cstdio>
#include <memory>
#include <vector>

#include "ucd_indic_syllabic_category_parser.h"
#include "unicode_database.h"
#include "unicode_indic_syllabic_category.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeDatabaseIndicSyllabicCategory
    //
    // Validate the persisted Indic_Syllabic_Category table against the
    // independently parsed Unicode source data.
    //
    // This exercises:
    //
    //      IndicSyllabicCategory.txt
    //          -> parser
    //          -> UnicodeValueTable8Builder
    //
    // against:
    //
    //      generated .ucdb
    //          -> UnicodeDatabase
    //          -> indicSyllabicCategory()
    //
    // Every Unicode code point is compared.
    // ========================================================================

    static bool testUnicodeDatabaseIndicSyllabicCategory(
        const ByteSpan& databaseData, const ByteSpan& sourceData)
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "Unicode database Indic_Syllabic_Category: FAIL: %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Parse source data independently.
        // ====================================================================

        auto expected =
            std::make_unique<UnicodeValueTable8Builder>();

        UCDIndicSyllabicCategoryParseResult parseResult;

        if (!ucdParseIndicSyllabicCategory(
            sourceData, *expected, parseResult))
        {
            std::printf(
                "Unicode database Indic_Syllabic_Category: FAIL: source parse\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdIndicSyllabicCategoryParseErrorString(parseResult.error),
                parseResult.lineNumber);

            return false;
        }


        // ====================================================================
        // Attach generated database.
        // ====================================================================

        UnicodeDatabase database;

        if (!database.reset(databaseData))
            return fail("unable to load Unicode database");

        if (!database.hasIndicSyllabicCategory())
            return fail("database has no Indic_Syllabic_Category table");


        // ====================================================================
        // Semantic spot checks.
        // ====================================================================

        struct Check
        {
            uint32_t cp;
            UnicodeIndicSyllabicCategory expected;
            const char* description;
        };


        static constexpr Check checks[] =
        {
            {
                0x0915,
                UnicodeIndicSyllabicCategory::Consonant,
                "DEVANAGARI LETTER KA"
            },
            {
                0x093C,
                UnicodeIndicSyllabicCategory::Nukta,
                "DEVANAGARI SIGN NUKTA"
            },
            {
                0x094D,
                UnicodeIndicSyllabicCategory::Virama,
                "DEVANAGARI SIGN VIRAMA"
            },
            {
                0x0905,
                UnicodeIndicSyllabicCategory::VowelIndependent,
                "DEVANAGARI LETTER A"
            },
            {
                0x093F,
                UnicodeIndicSyllabicCategory::VowelDependent,
                "DEVANAGARI VOWEL SIGN I"
            },
            {
                0x0041,
                UnicodeIndicSyllabicCategory::Other,
                "LATIN CAPITAL LETTER A"
            }
        };


        for (const Check& check : checks)
        {
            const UnicodeIndicSyllabicCategory actual =
                database.indicSyllabicCategory(check.cp);

            if (actual != check.expected)
            {
                std::printf(
                    "Unicode database Indic_Syllabic_Category: FAIL: spot check\n"
                    "  Code point: U+%04X\n"
                    "  Name:       %s\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    check.cp,
                    check.description,
                    static_cast<unsigned>(check.expected),
                    static_cast<unsigned>(actual));

                return false;
            }
        }


        // ====================================================================
        // Exhaustive comparison.
        // ====================================================================

        size_t comparedCodePoints = 0;

        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const uint8_t expectedValue =
                expected->value(cp);

            const UnicodeIndicSyllabicCategory actual =
                database.indicSyllabicCategory(cp);

            const uint8_t actualValue =
                static_cast<uint8_t>(actual);

            if (actualValue != expectedValue)
            {
                std::printf(
                    "Unicode database Indic_Syllabic_Category: FAIL: "
                    "full comparison\n"
                    "  Code point: U+%04X\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    cp,
                    static_cast<unsigned>(expectedValue),
                    static_cast<unsigned>(actualValue));

                return false;
            }

            ++comparedCodePoints;
        }


        // ====================================================================
        // Runtime default / out-of-range behavior.
        //
        // UnicodeValueTable8::value() returns zero outside Unicode, and ISC
        // value zero is Other.
        // ====================================================================

        if (database.indicSyllabicCategory(kUnicodeLimit) !=
            UnicodeIndicSyllabicCategory::Other)
        {
            return fail(
                "out-of-range lookup did not return Other");
        }


        // ====================================================================
        // Diagnostics.
        // ====================================================================

        std::printf(
            "Unicode database Indic_Syllabic_Category: PASS\n"
            "  Source ranges:           %u\n"
            "  Explicit code points:    %zu\n"
            "  Defaulted code points:   %zu\n"
            "  Unicode code points:     %zu\n"
            "  Semantic spot checks:    %zu\n"
            "  Full table comparison:   PASS\n",
            parseResult.rangeCount,
            parseResult.explicitCodePoints,
            parseResult.defaultedCodePoints,
            comparedCodePoints,
            sizeof(checks) / sizeof(checks[0]));


        return true;
    }


    // ========================================================================
    // Convenience filename overload
    // ========================================================================

    static bool testUnicodeDatabaseIndicSyllabicCategory(
        const char* databaseFilename,
        const char* sourceFilename)
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> sourceBytes;


        if (!readFileData(
            databaseFilename,
            databaseBytes))
        {
            std::printf(
                "Unicode database Indic_Syllabic_Category: FAIL: "
                "unable to read database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }


        if (!readFileData(
            sourceFilename,
            sourceBytes))
        {
            std::printf(
                "Unicode database Indic_Syllabic_Category: FAIL: "
                "unable to read source\n"
                "  File: %s\n",
                sourceFilename ? sourceFilename : "(null)");

            return false;
        }


        const ByteSpan databaseData(
            databaseBytes.data(),
            databaseBytes.size());

        const ByteSpan sourceData(
            sourceBytes.data(),
            sourceBytes.size());


        return testUnicodeDatabaseIndicSyllabicCategory(
            databaseData,
            sourceData);
    }

} // namespace waavs