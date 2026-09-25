
// test_find_script_fonts.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "opentype_cmap_view.h"
#include "opentype_name_view.h"
#include "opentype_maxp_view.h"
#include "unicode_database.h"
#include "unicode_coverage.h"
#include "unicode_coverage_storage.h"

#include <cstdio>
#include <cstring>
#include <vector>


namespace waavs
{
    static bool testFindScriptFonts(const ByteSpan& databaseData, const char* fontDirectory, const char* scriptTag)
    {
        if (!databaseData ||
            !fontDirectory || !*fontDirectory ||
            !scriptTag || !*scriptTag)
        {
            return false;
        }


        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Script font search: FAIL: invalid Unicode database\n"
                "  Script: %s\n",
                scriptTag);

            return false;
        }


        // ------------------------------------------------------------
        // Locate script by ISO 15924 tag.
        // ------------------------------------------------------------

        UnicodeScriptIndex script = kUnicodeScriptIndexInvalid;

        for (uint32_t i = 0; i < database.scriptCount(); ++i)
        {
            InternedKey tag = database.scriptISO15924(i);

            if (tag && std::strcmp(tag, scriptTag) == 0)
            {
                script = static_cast<UnicodeScriptIndex>(i);
                break;
            }
        }

        if (script == kUnicodeScriptIndexInvalid)
        {
            std::printf(
                "Script font search: FAIL: script not found\n"
                "  Script: %s\n",
                scriptTag);

            return false;
        }


        const UnicodeCoverage scriptCoverage =
            database.scriptCoverage(script);

        if (!scriptCoverage)
        {
            std::printf(
                "Script font search: FAIL: coverage unavailable\n"
                "  Script: %s\n",
                scriptTag);

            return false;
        }


        const UnicodeCoverageStats scriptStats =
            scriptCoverage.stats();


        std::printf(
            "Script font search\n"
            "  Script:             %s\n"
            "  Script index:       %u\n"
            "  Script code points: %zu\n"
            "  Directory:          %s\n",
            scriptTag,
            static_cast<unsigned>(script),
            scriptStats.coveredCodePoints,
            fontDirectory);


        // ------------------------------------------------------------
        // Scan font faces structurally.
        // ------------------------------------------------------------

        FontDirectoryView fonts(fontDirectory, true);
        FontFaceView view;

        size_t facesScanned = 0;
        size_t cmapFaces = 0;
        size_t fontsFound = 0;

        constexpr size_t kCandidateDiagnostics = 8;


        while (fonts(view))
        {
            ++facesScanned;


            CmapView cmap(view);

            if (!cmap)
            {
                if (facesScanned <= kCandidateDiagnostics)
                {
                    std::printf(
                        "  Candidate %zu: no valid cmap\n"
                        "    Location: %s\n",
                        facesScanned,
                        view.sourceLocation()
                        ? view.sourceLocation()
                        : "(unknown)");
                }

                continue;
            }

            UnicodeCoverageStorage coverageStorage;
            if (!cmap.buildCoverage(coverageStorage))
                continue;

            ++cmapFaces;

            const UnicodeCoverage& fontCoverage = coverageStorage.coverage();


            const UnicodeCoverageStats fontStats =
                fontCoverage.stats();


            if (cmapFaces <= kCandidateDiagnostics)
            {
                NameView name(view);

                std::printf(
                    "  Candidate %zu\n"
                    "    Face:        %s\n"
                    "    Code points: %zu\n"
                    "    Full match:  %s\n"
                    "    Location:    %s\n",
                    cmapFaces,
                    name && name.fullName()
                    ? name.fullName()
                    : "(unnamed)",
                    fontStats.coveredCodePoints,
                    fontCoverage.containsAll(scriptCoverage)
                    ? "yes"
                    : "no",
                    view.sourceLocation()
                    ? view.sourceLocation()
                    : "(unknown)");
            }


            if (!fontCoverage.containsAll(scriptCoverage))
                continue;


            ++fontsFound;


            NameView name(view);
            MaxpView maxp(view);


            std::printf(
                "Script font match\n"
                "  Script:   %s\n"
                "  Face:     %s\n"
                "  Family:   %s\n"
                "  Glyphs:   %u\n"
                "  Coverage: %zu code points\n"
                "  Location: %s\n",
                scriptTag,
                name && name.fullName()
                ? name.fullName()
                : "(unnamed)",
                name && name.familyName()
                ? name.familyName()
                : "(unnamed)",
                maxp
                ? static_cast<unsigned>(maxp.glyphCount())
                : 0u,
                fontStats.coveredCodePoints,
                view.sourceLocation()
                ? view.sourceLocation()
                : "(unknown)");
        }


        std::printf(
            "Script font search summary\n"
            "  Script:       %s\n"
            "  Script index: %u\n"
            "  Script cps:   %zu\n"
            "  Faces:        %zu\n"
            "  Cmap faces:   %zu\n"
            "  Full matches: %zu\n",
            scriptTag,
            static_cast<unsigned>(script),
            scriptStats.coveredCodePoints,
            facesScanned,
            cmapFaces,
            fontsFound);


        if (fontsFound == 0)
        {
            std::printf(
                "Script font search: FAIL: no full-coverage font found\n"
                "  Script:    %s\n"
                "  Directory: %s\n",
                scriptTag,
                fontDirectory);

            return false;
        }


        std::printf(
            "Script font search: PASS\n"
            "  Script: %s\n"
            "  Fonts:  %zu\n",
            scriptTag,
            fontsFound);

        return true;
    }


    static bool testFindScriptFonts(const char* databaseFilename, const char* fontDirectory, const char* scriptTag)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Script font search: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename
                ? databaseFilename
                : "(null)");

            return false;
        }


        return testFindScriptFonts(
            ByteSpan(fileData.data(), fileData.size()),
            fontDirectory,
            scriptTag);
    }
}
