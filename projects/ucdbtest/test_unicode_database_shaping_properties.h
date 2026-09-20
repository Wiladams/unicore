// test_unicode_database_shaping_properties.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "test_core.h"

#include "ucd_hangul_syllable_type_parser.h"
#include "ucd_indic_positional_category_parser.h"
#include "ucd_joining_group_parser.h"
#include "ucd_joining_type_parser.h"

#include "unicode_database.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeDatabaseShapingProperties
    //
    // Independently parse the four recently-added shaping properties from
    // their Unicode source files and compare every Unicode code point against
    // the typed runtime accessors in the generated database.
    //
    // Properties:
    //
    //      Indic_Positional_Category
    //      Joining_Type
    //      Joining_Group
    //      Hangul_Syllable_Type
    //
    // ========================================================================

    static bool testUnicodeDatabaseShapingProperties(
        const ByteSpan& databaseData,
        const ByteSpan& indicPositionalCategorySource,
        const ByteSpan& joiningTypeSource,
        const ByteSpan& joiningGroupSource,
        const ByteSpan& hangulSyllableTypeSource)
    {
        UnicodeDatabase database;

        if (!database.reset(databaseData))
        {
            std::printf(
                "Unicode database shaping properties: FAIL\n"
                "  Unable to attach Unicode database\n");

            return false;
        }


        auto fail = [](const char* message) -> bool
            {
                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Presence
        // ====================================================================

        if (!database.hasIndicPositionalCategory())
            return fail("missing Indic_Positional_Category");

        if (!database.hasJoiningType())
            return fail("missing Joining_Type");

        if (!database.hasJoiningGroup())
            return fail("missing Joining_Group");

        if (!database.hasHangulSyllableType())
            return fail("missing Hangul_Syllable_Type");


        // ====================================================================
        // Indic_Positional_Category
        // ====================================================================

        UCDIndicPositionalCategoryParseResult ipcResult;

        {
            auto expected =
                std::make_unique<UnicodeValueTable8Builder>();


            if (!ucdParseIndicPositionalCategory(
                indicPositionalCategorySource,
                *expected,
                ipcResult))
            {
                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Indic_Positional_Category source parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdIndicPositionalCategoryParseErrorString(
                        ipcResult.error),
                    ipcResult.lineNumber);

                return false;
            }


            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                const uint8_t expectedValue =
                    expected->value(cp);

                const uint8_t actualValue =
                    static_cast<uint8_t>(
                        database.indicPositionalCategory(cp));


                if (expectedValue == actualValue)
                    continue;


                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Indic_Positional_Category mismatch\n"
                    "  Code point: U+%04X\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    cp,
                    static_cast<unsigned>(expectedValue),
                    static_cast<unsigned>(actualValue));

                return false;
            }
        }


        // ====================================================================
        // Joining_Type
        // ====================================================================

        UCDJoiningTypeParseResult joiningTypeResult;

        {
            auto expected =
                std::make_unique<UnicodeValueTable8Builder>();


            if (!ucdParseJoiningType(
                joiningTypeSource,
                *expected,
                joiningTypeResult))
            {
                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Joining_Type source parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdJoiningTypeParseErrorString(
                        joiningTypeResult.error),
                    joiningTypeResult.lineNumber);

                return false;
            }


            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                const uint8_t expectedValue =
                    expected->value(cp);

                const uint8_t actualValue =
                    static_cast<uint8_t>(
                        database.joiningType(cp));


                if (expectedValue == actualValue)
                    continue;


                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Joining_Type mismatch\n"
                    "  Code point: U+%04X\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    cp,
                    static_cast<unsigned>(expectedValue),
                    static_cast<unsigned>(actualValue));

                return false;
            }
        }


        // ====================================================================
        // Joining_Group
        // ====================================================================

        UCDJoiningGroupParseResult joiningGroupResult;

        {
            auto expected =
                std::make_unique<UnicodeValueTable8Builder>();


            if (!ucdParseJoiningGroup(
                joiningGroupSource,
                *expected,
                joiningGroupResult))
            {
                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Joining_Group source parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdJoiningGroupParseErrorString(
                        joiningGroupResult.error),
                    joiningGroupResult.lineNumber);

                return false;
            }


            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                const uint8_t expectedValue =
                    expected->value(cp);

                const uint8_t actualValue =
                    static_cast<uint8_t>(
                        database.joiningGroup(cp));


                if (expectedValue == actualValue)
                    continue;


                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Joining_Group mismatch\n"
                    "  Code point: U+%04X\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    cp,
                    static_cast<unsigned>(expectedValue),
                    static_cast<unsigned>(actualValue));

                return false;
            }
        }


        // ====================================================================
        // Hangul_Syllable_Type
        // ====================================================================

        UCDHangulSyllableTypeParseResult hangulResult;

        {
            auto expected =
                std::make_unique<UnicodeValueTable8Builder>();


            if (!ucdParseHangulSyllableType(
                hangulSyllableTypeSource,
                *expected,
                hangulResult))
            {
                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Hangul_Syllable_Type source parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdHangulSyllableTypeParseErrorString(
                        hangulResult.error),
                    hangulResult.lineNumber);

                return false;
            }


            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                const uint8_t expectedValue =
                    expected->value(cp);

                const uint8_t actualValue =
                    static_cast<uint8_t>(
                        database.hangulSyllableType(cp));


                if (expectedValue == actualValue)
                    continue;


                std::printf(
                    "Unicode database shaping properties: FAIL\n"
                    "  Hangul_Syllable_Type mismatch\n"
                    "  Code point: U+%04X\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    cp,
                    static_cast<unsigned>(expectedValue),
                    static_cast<unsigned>(actualValue));

                return false;
            }
        }


        // ====================================================================
        // Out-of-range behavior
        // ====================================================================

        if (database.indicPositionalCategory(kUnicodeLimit) !=
            UnicodeIndicPositionalCategory::NotApplicable)
        {
            return fail(
                "out-of-range Indic_Positional_Category lookup");
        }


        if (database.joiningType(kUnicodeLimit) !=
            UnicodeJoiningType::NonJoining)
        {
            return fail(
                "out-of-range Joining_Type lookup");
        }


        if (database.joiningGroup(kUnicodeLimit) !=
            UnicodeJoiningGroup::NoJoiningGroup)
        {
            return fail(
                "out-of-range Joining_Group lookup");
        }


        if (database.hangulSyllableType(kUnicodeLimit) !=
            UnicodeHangulSyllableType::NotApplicable)
        {
            return fail(
                "out-of-range Hangul_Syllable_Type lookup");
        }


        // ====================================================================
        // Diagnostics
        // ====================================================================

        std::printf(
            "Unicode database shaping properties: PASS\n"
            "  Indic_Positional_Category\n"
            "    Source ranges:           %u\n"
            "    Explicit code points:    %zu\n"
            "    Defaulted code points:   %zu\n"
            "  Joining_Type\n"
            "    Source ranges:           %u\n"
            "    Explicit code points:    %zu\n"
            "    Defaulted code points:   %zu\n"
            "  Joining_Group\n"
            "    Source ranges:           %u\n"
            "    Explicit code points:    %zu\n"
            "    Defaulted code points:   %zu\n"
            "  Hangul_Syllable_Type\n"
            "    Source ranges:           %u\n"
            "    Explicit code points:    %zu\n"
            "    Defaulted code points:   %zu\n"
            "  Unicode code points:       %u\n"
            "  Full table comparisons:    PASS\n",
            ipcResult.rangeCount,
            ipcResult.explicitCodePoints,
            ipcResult.defaultedCodePoints,
            joiningTypeResult.rangeCount,
            joiningTypeResult.explicitCodePoints,
            joiningTypeResult.defaultedCodePoints,
            joiningGroupResult.rangeCount,
            joiningGroupResult.explicitCodePoints,
            joiningGroupResult.defaultedCodePoints,
            hangulResult.rangeCount,
            hangulResult.explicitCodePoints,
            hangulResult.defaultedCodePoints,
            kUnicodeLimit);


        return true;
    }


    // ========================================================================
    // Convenience filename overload
    // ========================================================================

    static bool testUnicodeDatabaseShapingProperties(
        const char* databaseFilename,
        const char* indicPositionalCategoryFilename,
        const char* joiningTypeFilename,
        const char* joiningGroupFilename,
        const char* hangulSyllableTypeFilename)
    {
        std::vector<uint8_t> databaseData;
        std::vector<uint8_t> indicPositionalCategoryData;
        std::vector<uint8_t> joiningTypeData;
        std::vector<uint8_t> joiningGroupData;
        std::vector<uint8_t> hangulSyllableTypeData;


        if (!readFileData(databaseFilename, databaseData))
        {
            std::printf(
                "Unicode database shaping properties: FAIL\n"
                "  Unable to read database: %s\n",
                databaseFilename);

            return false;
        }


        if (!readFileData(
            indicPositionalCategoryFilename,
            indicPositionalCategoryData))
        {
            std::printf(
                "Unicode database shaping properties: FAIL\n"
                "  Unable to read Indic_Positional_Category source: %s\n",
                indicPositionalCategoryFilename);

            return false;
        }


        if (!readFileData(joiningTypeFilename, joiningTypeData))
        {
            std::printf(
                "Unicode database shaping properties: FAIL\n"
                "  Unable to read Joining_Type source: %s\n",
                joiningTypeFilename);

            return false;
        }


        if (!readFileData(joiningGroupFilename, joiningGroupData))
        {
            std::printf(
                "Unicode database shaping properties: FAIL\n"
                "  Unable to read Joining_Group source: %s\n",
                joiningGroupFilename);

            return false;
        }


        if (!readFileData(
            hangulSyllableTypeFilename,
            hangulSyllableTypeData))
        {
            std::printf(
                "Unicode database shaping properties: FAIL\n"
                "  Unable to read Hangul_Syllable_Type source: %s\n",
                hangulSyllableTypeFilename);

            return false;
        }


        return testUnicodeDatabaseShapingProperties(
            ByteSpan(
                databaseData.data(),
                databaseData.size()),
            ByteSpan(
                indicPositionalCategoryData.data(),
                indicPositionalCategoryData.size()),
            ByteSpan(
                joiningTypeData.data(),
                joiningTypeData.size()),
            ByteSpan(
                joiningGroupData.data(),
                joiningGroupData.size()),
            ByteSpan(
                hangulSyllableTypeData.data(),
                hangulSyllableTypeData.size()));
    }

} // namespace waavs