// unicode_database_demo.cpp

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <vector>

#include "unicode_database.h"
#include "test_ucd_script_extensions.h"
#include "test_unicode_database_script_extensions.h"

using namespace waavs;

    static bool loadFile(const char* filename, std::vector<uint8_t>& data)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (!file)
            return false;

        const std::streamsize size = file.tellg();

        if (size <= 0)
            return false;

        data.resize(static_cast<size_t>(size));

        file.seekg(0, std::ios::beg);

        return static_cast<bool>(
            file.read(
                reinterpret_cast<char*>(data.data()),
                size));
    }


    static void inspectCodePoint(const UnicodeDatabase& database, uint32_t cp)
    {
        const UnicodeGeneralCategory gc =
            database.generalCategory(cp);

        const UnicodeCombiningClass ccc =
            database.combiningClass(cp);

        const UnicodeBidiClass bidi =
            database.bidiClass(cp);

        const UnicodeGraphemeClusterBreak gcb =
            database.graphemeClusterBreak(cp);

        const UnicodeIndicConjunctBreak incb =
            database.indicConjunctBreak(cp);

        const bool extendedPictographic =
            database.isExtendedPictographic(cp);


        std::printf(
            "U+%04X\n"
            "  General_Category:       %u\n"
            "  Combining_Class:        %u\n"
            "  Bidi_Class:             %u\n"
            "  Grapheme_Cluster_Break: %u\n"
            "  Indic_Conjunct_Break:   %u\n"
            "  Extended_Pictographic:  %s\n",
            cp,
            static_cast<unsigned>(gc),
            static_cast<unsigned>(ccc),
            static_cast<unsigned>(bidi),
            static_cast<unsigned>(gcb),
            static_cast<unsigned>(incb),
            extendedPictographic ? "yes" : "no");
    }


    static bool unicodeDatabaseDemo(const char* filename)
    {
        std::vector<uint8_t> fileData;

        if (!loadFile(filename, fileData))
        {
            std::printf("Unable to read database: %s\n", filename);
            return false;
        }


        const ByteSpan data(
            fileData.data(),
            fileData.size());

        UnicodeDatabase database;

        if (!database.reset(data))
        {
            std::printf("Invalid Unicode database\n");
            return false;
        }


        std::printf(
            "Unicode database loaded\n"
            "  Unicode version:       %u.%u.%u\n"
            "  Database bytes:        %zu\n"
            "  Blocks:                %u\n"
            "  Scripts:               %u\n"
            "  Binary properties:     %u\n"
            "  VALUE8 properties:     %u\n"
            "  VALUE8 tables:         %u\n"
            "\n",
            database.unicodeMajor(),
            database.unicodeMinor(),
            database.unicodePatch(),
            fileData.size(),
            database.blockCount(),
            database.scriptCount(),
            database.propertyCount(),
            database.valueProperty8Count(),
            database.valueTable8Count());


        // ASCII capital A.
        inspectCodePoint(database, 0x0041);

        // Combining acute accent.
        inspectCodePoint(database, 0x0301);

        // Devanagari virama - interesting for Indic conjunct handling.
        inspectCodePoint(database, 0x094D);

        // Emoji.
        inspectCodePoint(database, 0x1F600);


        return true;
    }

    void runTests()
    {
        //testUCDScriptExtensions(
        //    "../ucdbgen/ucd/PropertyValueAliases.txt",
        //    "../ucdbgen/ucd/Scripts.txt",
        //    "../ucdbgen/ucd/ScriptExtensions.txt");

        testUnicodeDatabaseScriptExtensions("../../release/unicode-17.0.0.ucdb");
    }

int main()
{
    //unicodeDatabaseDemo("../../release/unicode-17.0.0.ucdb");
    runTests();
}