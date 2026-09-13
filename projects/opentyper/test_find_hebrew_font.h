// test_find_hebrew_font.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "font_filter.h"
#include "font_predicates.h"
#include "unicode_database.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace waavs
{
    static bool testFindHebrewFont(const ByteSpan& databaseData, const char* fontDirectory)
    {
        if (!databaseData || !fontDirectory || !*fontDirectory)
            return false;

        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf("Hebrew font search: FAIL: invalid Unicode database\n");
            return false;
        }

        // Locate Script=Hebrew by ISO 15924 tag.

        UnicodeScriptIndex hebrewScript = kUnicodeScriptIndexInvalid;

        for (uint32_t i = 0; i < database.scriptCount(); ++i)
        {
            InternedKey tag = database.scriptISO15924(i);

            if (tag && std::strcmp(tag, "Hebr") == 0)
            {
                hebrewScript = static_cast<UnicodeScriptIndex>(i);
                break;
            }
        }

        if (hebrewScript == kUnicodeScriptIndexInvalid)
        {
            std::printf("Hebrew font search: FAIL: Hebr script not found\n");
            return false;
        }

        const UnicodeCoverage hebrewCoverage =
            database.scriptCoverage(hebrewScript);

        if (!hebrewCoverage)
        {
            std::printf("Hebrew font search: FAIL: Hebrew coverage unavailable\n");
            return false;
        }

        FontFace face = FontDirectoryView(fontDirectory) | covers(hebrewCoverage) | first;

        if (!face)
        {
            std::printf(
                "Hebrew font search: FAIL: no full Hebrew font found\n"
                "  Directory: %s\n",
                fontDirectory);

            return false;
        }

        std::printf(
            "Hebrew font search: PASS\n"
            "  Face:     %s\n"
            "  Family:   %s\n"
            "  Glyphs:   %u\n"
            "  Location: %s\n",
            face.fullName() ? face.fullName() : "(unnamed)",
            face.familyName() ? face.familyName() : "(unnamed)",
            static_cast<unsigned>(face.glyphCount()),
            face.sourceLocation() ? face.sourceLocation() : "(unknown)");

        return true;
    }


    static bool testFindHebrewFont(const char* databaseFilename, const char* fontDirectory)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Hebrew font search: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        return testFindHebrewFont(
            ByteSpan(fileData.data(), fileData.size()),
            fontDirectory);
    }
}