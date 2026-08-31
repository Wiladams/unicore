// test_ucd_script_extensions.h

#pragma once


#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "ucd_property_value_aliases_parser.h"
#include "ucd_scripts_parser.h"
#include "ucd_script_extensions_parser.h"
#include "unicode_database_builder.h"
#include "unicode_script_extensions_data.h"
#include "unicode_script_set.h"


namespace waavs
{
    // ========================================================================
    // testUCDScriptExtensions
    // ========================================================================

    static bool testUCDScriptExtensions(
        const ByteSpan& propertyValueAliasesSource,
        const ByteSpan& scriptsSource,
        const ByteSpan& scriptExtensionsSource)
    {
        // ====================================================================
        // Parse Script aliases.
        // ====================================================================

        UCDScriptValueAliases aliases;
        UCDPropertyValueAliasesParseResult aliasResult;


        if (!ucdParseScriptValueAliases(
            propertyValueAliasesSource,
            aliases,
            aliasResult))
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  PropertyValueAliases.txt parse failed\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdPropertyValueAliasesParseErrorString(
                    aliasResult.error),
                aliasResult.lineNumber);

            return false;
        }


        // ====================================================================
        // Parse Scripts.txt.
        //
        // This establishes the Script record order used by
        // UnicodeScriptIndex.
        // ====================================================================

        UnicodeDatabaseBuilder database;

        UCDScriptsParseResult scriptsResult;


        if (!ucdParseScripts(
            scriptsSource,
            aliases,
            database,
            scriptsResult))
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Scripts.txt parse failed\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdScriptsParseErrorString(
                    scriptsResult.error),
                scriptsResult.lineNumber);

            return false;
        }


        // ====================================================================
        // Parse ScriptExtensions.txt.
        // ====================================================================

        UCDScriptExtensionsParseResult extensionsResult;


        if (!ucdParseScriptExtensions(
            scriptExtensionsSource,
            aliases,
            database,
            extensionsResult))
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  ScriptExtensions.txt parse failed\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdScriptExtensionsParseErrorString(
                    extensionsResult.error),
                extensionsResult.lineNumber);

            return false;
        }


        const std::vector<UnicodeScriptRecord>& scripts =
            database.scripts();

        const std::vector<UnicodeScriptSet>& sets =
            database.scriptSets();

        const std::vector<UnicodeScriptExtensionRange>& ranges =
            database.scriptExtensionRanges();


        // ====================================================================
        // Basic count relationships.
        // ====================================================================

        if (scripts.size() != aliases.size())
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Script record count mismatch\n"
                "  Aliases: %zu\n"
                "  Scripts: %zu\n",
                aliases.size(),
                scripts.size());

            return false;
        }


        if (extensionsResult.rangeCount != ranges.size())
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Range count mismatch\n"
                "  Parser:  %u\n"
                "  Builder: %zu\n",
                extensionsResult.rangeCount,
                ranges.size());

            return false;
        }


        if (extensionsResult.uniqueSetCount != sets.size())
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Script set count mismatch\n"
                "  Parser:  %u\n"
                "  Builder: %zu\n",
                extensionsResult.uniqueSetCount,
                sets.size());

            return false;
        }


        if (sets.empty() || ranges.empty())
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Script_Extensions data is empty\n");

            return false;
        }


        // ====================================================================
        // Validate every stored Script set.
        //
        // Each set must:
        //
        //      - be non-empty
        //      - contain only valid Script indices
        // ====================================================================

        for (size_t setIndex = 0; setIndex < sets.size(); ++setIndex)
        {
            const UnicodeScriptSet& set =
                sets[setIndex];


            if (set.empty())
            {
                std::printf(
                    "UCD Script_Extensions: FAIL\n"
                    "  Script set %zu is empty\n",
                    setIndex);

                return false;
            }


            for (uint32_t scriptIndex =
                static_cast<uint32_t>(scripts.size());
                scriptIndex < kUnicodeScriptIndexInvalid;
                ++scriptIndex)
            {
                if (set.contains(
                    static_cast<UnicodeScriptIndex>(
                        scriptIndex)))
                {
                    std::printf(
                        "UCD Script_Extensions: FAIL\n"
                        "  Script set %zu contains invalid Script index %u\n"
                        "  Script records: %zu\n",
                        setIndex,
                        scriptIndex,
                        scripts.size());

                    return false;
                }
            }
        }


        // ====================================================================
        // Verify Script-set deduplication.
        //
        // No two physical Script sets should be identical.
        // ====================================================================

        for (size_t i = 0; i < sets.size(); ++i)
        {
            for (size_t j = i + 1; j < sets.size(); ++j)
            {
                if (sets[i] == sets[j])
                {
                    std::printf(
                        "UCD Script_Extensions: FAIL\n"
                        "  Duplicate physical Script sets\n"
                        "  Set A: %zu\n"
                        "  Set B: %zu\n",
                        i,
                        j);

                    return false;
                }
            }
        }


        // ====================================================================
        // Validate ranges and collect set references.
        // ====================================================================

        std::vector<uint8_t> referencedSets(
            sets.size(),
            0);

        size_t explicitCodePoints = 0;

        uint32_t previousLast = 0;
        bool havePrevious = false;


        for (size_t i = 0; i < ranges.size(); ++i)
        {
            const UnicodeScriptExtensionRange& range =
                ranges[i];


            if (range.first > range.last ||
                range.last >= kUnicodeLimit)
            {
                std::printf(
                    "UCD Script_Extensions: FAIL\n"
                    "  Invalid range %zu\n"
                    "  First: U+%04X\n"
                    "  Last:  U+%04X\n",
                    i,
                    range.first,
                    range.last);

                return false;
            }


            if (havePrevious &&
                range.first <= previousLast)
            {
                std::printf(
                    "UCD Script_Extensions: FAIL\n"
                    "  Ranges are unordered or overlap\n"
                    "  Range:          %zu\n"
                    "  Previous last:  U+%04X\n"
                    "  Current first:  U+%04X\n",
                    i,
                    previousLast,
                    range.first);

                return false;
            }


            if (range.setIndex >= sets.size())
            {
                std::printf(
                    "UCD Script_Extensions: FAIL\n"
                    "  Range %zu has invalid Script set index %u\n"
                    "  Script sets: %zu\n",
                    i,
                    static_cast<unsigned>(
                        range.setIndex),
                    sets.size());

                return false;
            }


            if (range.reserved != 0)
            {
                std::printf(
                    "UCD Script_Extensions: FAIL\n"
                    "  Range %zu has non-zero reserved field\n",
                    i);

                return false;
            }


            referencedSets[range.setIndex] = 1;


            explicitCodePoints +=
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);


            previousLast =
                range.last;

            havePrevious =
                true;
        }


        // ====================================================================
        // Every physical Script set should be referenced by at least one
        // explicit range.
        // ====================================================================

        for (size_t i = 0; i < referencedSets.size(); ++i)
        {
            if (!referencedSets[i])
            {
                std::printf(
                    "UCD Script_Extensions: FAIL\n"
                    "  Unreferenced Script set: %zu\n",
                    i);

                return false;
            }
        }


        // ====================================================================
        // Parser statistics must agree with the generated ranges.
        // ====================================================================

        if (explicitCodePoints !=
            extensionsResult.explicitCodePoints)
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Explicit code-point count mismatch\n"
                "  Parser: %zu\n"
                "  Range:  %zu\n",
                extensionsResult.explicitCodePoints,
                explicitCodePoints);

            return false;
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "UCD Script_Extensions: PASS\n"
            "  Script aliases:          %zu\n"
            "  Script records:          %zu\n"
            "  Explicit ranges:         %zu\n"
            "  Explicit code points:    %zu\n"
            "  Unique Script sets:      %zu\n"
            "  Range ordering:          PASS\n"
            "  Set references:          PASS\n"
            "  Set deduplication:        PASS\n"
            "  Script index validation: PASS\n",
            aliases.size(),
            scripts.size(),
            ranges.size(),
            explicitCodePoints,
            sets.size());


        return true;
    }


    // ========================================================================
    // testUCDScriptExtensionsReadFile
    // ========================================================================

    static bool testUCDScriptExtensionsReadFile(
        const char* filename,
        std::vector<uint8_t>& outData)
    {
        outData.clear();


        if (!filename)
            return false;


        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);


        if (!input)
            return false;


        const std::streampos end =
            input.tellg();


        if (end <= 0)
            return false;


        outData.resize(
            static_cast<size_t>(
                end));


        input.seekg(
            0,
            std::ios::beg);


        input.read(
            reinterpret_cast<char*>(
                outData.data()),
            static_cast<std::streamsize>(
                outData.size()));


        return !!input;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static bool testUCDScriptExtensions(
        const char* propertyValueAliasesFilename,
        const char* scriptsFilename,
        const char* scriptExtensionsFilename)
    {
        std::vector<uint8_t> aliasData;
        std::vector<uint8_t> scriptsData;
        std::vector<uint8_t> extensionsData;


        if (!testUCDScriptExtensionsReadFile(
            propertyValueAliasesFilename,
            aliasData))
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Unable to read PropertyValueAliases.txt\n");

            return false;
        }


        if (!testUCDScriptExtensionsReadFile(
            scriptsFilename,
            scriptsData))
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Unable to read Scripts.txt\n");

            return false;
        }


        if (!testUCDScriptExtensionsReadFile(
            scriptExtensionsFilename,
            extensionsData))
        {
            std::printf(
                "UCD Script_Extensions: FAIL\n"
                "  Unable to read ScriptExtensions.txt\n");

            return false;
        }


        return testUCDScriptExtensions(
            ByteSpan(
                aliasData.data(),
                aliasData.size()),
            ByteSpan(
                scriptsData.data(),
                scriptsData.size()),
            ByteSpan(
                extensionsData.data(),
                extensionsData.size()));
    }

} // namespace waavs