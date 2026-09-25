
// test_report_font_script_coverage.h
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
    static bool testReportFontScriptCoverage(const ByteSpan& databaseData, const char* fontDirectory, double minimumCoverage = 50.0)
    {
        if (!databaseData || !fontDirectory || !*fontDirectory)
            return false;

        if (minimumCoverage < 0.0 || minimumCoverage > 100.0)
            return false;


        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Font script coverage: FAIL: invalid Unicode database\n");

            return false;
        }


        // ------------------------------------------------------------
        // Precompute the number of code points assigned to each script.
        //
        // Empty script coverages are ignored. This is important for
        // Script values such as Hrkt which may exist as metadata but
        // have no Script-property code points of their own.
        // ------------------------------------------------------------

        const uint32_t scriptCount = database.scriptCount();

        std::vector<size_t> scriptTotals(scriptCount, 0);

        for (uint32_t i = 0; i < scriptCount; ++i)
        {
            const UnicodeCoverage coverage = database.scriptCoverage(static_cast<UnicodeScriptIndex>(i));

            if (!coverage || coverage.empty())
                continue;

            scriptTotals[i] =
                coverage.stats().coveredCodePoints;
        }


        // ------------------------------------------------------------
        // Scan every font face recursively.
        // ------------------------------------------------------------

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

            UnicodeCoverageStorage coverageStorage;
            if (!cmap.buildCoverage(coverageStorage))
                continue;

            const UnicodeCoverage& fontCoverage = coverageStorage.coverage();

            NameView name(view);
            MaxpView maxp(view);


            // --------------------------------------------------------
            // Count covered code points by Unicode Script.
            //
            // We scan Unicode once per font rather than once per
            // font/script combination.
            // --------------------------------------------------------

            std::vector<size_t> scriptCovered(scriptCount, 0);

            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                if (!fontCoverage.contains(cp))
                    continue;

                const UnicodeScriptIndex script =
                    database.script(cp);

                if (script == kUnicodeScriptIndexInvalid ||
                    static_cast<uint32_t>(script) >= scriptCount)
                {
                    continue;
                }

                ++scriptCovered[script];
            }


            std::printf(
                "Font\n"
                "  Face:       %s\n"
                "  Family:     %s\n"
                "  Glyphs:     %u\n"
                "  Location:   %s\n"
                "  Threshold:  > %.1f%%\n"
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
                : "(unknown)",
                minimumCoverage);


            size_t matchingScripts = 0;


            for (uint32_t i = 0; i < scriptCount; ++i)
            {
                const size_t total =
                    scriptTotals[i];

                if (total == 0)
                    continue;


                const size_t covered =
                    scriptCovered[i];

                const double percentage =
                    100.0 *
                    static_cast<double>(covered) /
                    static_cast<double>(total);


                if (percentage <= minimumCoverage)
                    continue;


                const InternedKey tag =
                    database.scriptISO15924(i);

                const InternedKey scriptName =
                    database.scriptName(i);


                std::printf(
                    "    %-4s  %-28s  %6.2f%%  (%zu / %zu)%s\n",
                    tag
                    ? tag
                    : "(unknown)",
                    scriptName
                    ? scriptName
                    : "(unknown)",
                    percentage,
                    covered,
                    total,
                    percentage == 100.0
                    ? "  FULL"
                    : "");


                ++matchingScripts;
            }


            if (matchingScripts == 0)
            {
                std::printf(
                    "    (none above threshold)\n");
            }


            std::printf(
                "  Matching scripts: %zu\n\n",
                matchingScripts);


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
            "  Directory:       %s\n"
            "  Threshold:       > %.1f%%\n"
            "  Faces scanned:   %zu\n"
            "  Faces reported:  %zu\n",
            fontDirectory,
            minimumCoverage,
            faceCount,
            reportedFaces);


        return true;
    }


    static bool testReportFontScriptCoverage(const char* databaseFilename, const char* fontDirectory, double minimumCoverage = 50.0)
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


        return testReportFontScriptCoverage(
            ByteSpan(fileData.data(), fileData.size()),
            fontDirectory,
            minimumCoverage);
    }
}
