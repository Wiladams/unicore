// test_find_hebrew_font.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "opentype_cmap_view.h"
#include "opentype_name_view.h"
#include "opentype_maxp_view.h"
#include "unicode_database.h"
#include "unicode_coverage.h"

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


        // ------------------------------------------------------------
        // Locate Script=Hebrew by ISO 15924 tag.
        // ------------------------------------------------------------

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


        const UnicodeCoverage hebrewCoverage = database.scriptCoverage(hebrewScript);

        if (!hebrewCoverage)
        {
            std::printf("Hebrew font search: FAIL: Hebrew coverage unavailable\n");
            return false;
        }


        // ------------------------------------------------------------
        // Scan font faces structurally.
        //
        // No FontFace promotion is required. CmapView is sufficient to
        // determine Unicode coverage; NameView and MaxpView provide the
        // reporting metadata.
        // ------------------------------------------------------------

        FontDirectoryView fonts(fontDirectory);
        FontFaceView view;

        // In this case, we'll report all the fonts 
        int fontsFound = 0;

        while (fonts(view))
        {
            CmapView cmap(view);

            if (!cmap)
                continue;

            if (!cmap.unicodeCoverage().containsAll(hebrewCoverage))
                continue;

            ++fontsFound;
            
            NameView name(view);
            MaxpView maxp(view);


            std::printf(
                "Hebrew font search: PASS\n"
                "  Face:     %s\n"
                "  Family:   %s\n"
                "  Glyphs:   %u\n"
                "  Location: %s\n",
                name && name.fullName()
                ? name.fullName()
                : "(unnamed)",
                name && name.familyName()
                ? name.familyName()
                : "(unnamed)",
                maxp
                ? static_cast<unsigned>(maxp.glyphCount())
                : 0u,
                view.sourceLocation()
                ? view.sourceLocation()
                : "(unknown)");

        }

        if (fontsFound < 1)
        {
            std::printf(
                "Hebrew font search: FAIL: no full Hebrew font found\n"
                "  Directory: %s\n",
                fontDirectory);
            return false;
        }

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