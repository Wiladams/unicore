
// test_report_font_scripts.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "opentype_cmap_view.h"
#include "opentype_name_view.h"
#include "opentype_maxp_view.h"
#include "unicode_database.h"
#include "unicode_coverage.h"

#include <cstdio>
#include <vector>


namespace waavs
{
    static bool testReportFontScripts(const ByteSpan& databaseData, const char* fontDirectory)
    {
        if (!databaseData || !fontDirectory || !*fontDirectory)
            return false;


        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Font script coverage: FAIL: invalid Unicode database\n");

            return false;
        }


        FontDirectoryView fonts(fontDirectory, true);
        FontFaceView view;

        size_t faceCount = 0;
        size_t reportedFaces = 0;


        while (fonts(view))
        {
            ++faceCount;


            CmapView cmap(view);

            if (!cmap)
                continue;


            NameView name(view);
            MaxpView maxp(view);


            std::printf(
                "Font\n"
                "  Face:     %s\n"
                "  Family:   %s\n"
                "  Glyphs:   %u\n"
                "  Location: %s\n"
                "  Scripts:\n",
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


            const UnicodeCoverage& fontCoverage =
                cmap.unicodeCoverage();

            size_t scriptCount = 0;


            for (uint32_t i = 0; i < database.scriptCount(); ++i)
            {
                const UnicodeCoverage scriptCoverage =
                    database.scriptCoverage(
                        static_cast<UnicodeScriptIndex>(i));

                if (!scriptCoverage || scriptCoverage.empty())
                    continue;


                if (!fontCoverage.containsAll(scriptCoverage))
                    continue;


                InternedKey tag =
                    database.scriptISO15924(i);

                InternedKey scriptName =
                    database.scriptName(i);


                std::printf(
                    "    %-4s  %s\n",
                    tag
                    ? tag
                    : "(unknown)",
                    scriptName
                    ? scriptName
                    : "(unknown)");

                ++scriptCount;
            }


            if (scriptCount == 0)
            {
                std::printf(
                    "    (none)\n");
            }


            std::printf(
                "  Full coverage scripts: %zu\n\n",
                scriptCount);


            ++reportedFaces;
        }


        if (reportedFaces == 0)
        {
            std::printf(
                "Font script coverage: FAIL: no readable font faces found\n"
                "  Directory: %s\n",
                fontDirectory);

            return false;
        }


        std::printf(
            "Font script coverage: PASS\n"
            "  Faces scanned:  %zu\n"
            "  Faces reported: %zu\n",
            faceCount,
            reportedFaces);

        return true;
    }


    static bool testReportFontScripts(const char* databaseFilename, const char* fontDirectory)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Font script coverage: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename
                ? databaseFilename
                : "(null)");

            return false;
        }


        return testReportFontScripts(
            ByteSpan(fileData.data(), fileData.size()),
            fontDirectory);
    }
}
