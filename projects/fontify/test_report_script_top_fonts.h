
// test_report_script_top_fonts.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "opentype_cmap_view.h"
#include "opentype_name_view.h"
#include "opentype_maxp_view.h"
#include "unicode_database.h"
#include "unicode_coverage.h"
#include "unicode_coverage_storage.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>


namespace waavs
{
    struct ScriptFontCoverageResult
    {
        std::string face;
        std::string family;
        std::string location;

        size_t covered{ 0 };
        size_t total{ 0 };

        uint32_t glyphCount{ 0 };

        double percentage() const noexcept
        {
            return total != 0
                ? 100.0 * static_cast<double>(covered) / static_cast<double>(total)
                : 0.0;
        }
    };


    static bool scriptFontCoverageBetter(
        const ScriptFontCoverageResult& a,
        const ScriptFontCoverageResult& b) noexcept
    {
        if (a.covered * b.total != b.covered * a.total)
            return a.covered * b.total > b.covered * a.total;

        if (a.covered != b.covered)
            return a.covered > b.covered;

        return a.glyphCount > b.glyphCount;
    }


    static void insertTopScriptFontFamily(
        std::vector<ScriptFontCoverageResult>& results,
        ScriptFontCoverageResult result,
        size_t maximumResults = 3)
    {
        // ------------------------------------------------------------
        // One result per family.
        //
        // If the family already exists, retain whichever face provides
        // the better script coverage.
        // ------------------------------------------------------------

        for (ScriptFontCoverageResult& existing : results)
        {
            if (existing.family != result.family)
                continue;

            if (scriptFontCoverageBetter(result, existing))
                existing = std::move(result);

            std::sort(
                results.begin(),
                results.end(),
                scriptFontCoverageBetter);

            return;
        }


        results.push_back(std::move(result));

        std::sort(
            results.begin(),
            results.end(),
            scriptFontCoverageBetter);

        if (results.size() > maximumResults)
            results.resize(maximumResults);
    }


    static bool testReportScriptTopFonts(const ByteSpan& databaseData, const char* fontDirectory)
    {
        if (!databaseData || !fontDirectory || !*fontDirectory)
            return false;


        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Script top-font report: FAIL: invalid Unicode database\n");

            return false;
        }


        // ------------------------------------------------------------
        // Precompute the size of every non-empty Script coverage.
        // ------------------------------------------------------------

        const uint32_t scriptCount = database.scriptCount();

        std::vector<size_t> scriptTotals(scriptCount, 0);

        for (uint32_t i = 0; i < scriptCount; ++i)
        {
            const UnicodeCoverage coverage =
                database.scriptCoverage(
                    static_cast<UnicodeScriptIndex>(i));

            if (!coverage || coverage.empty())
                continue;

            scriptTotals[i] =
                coverage.stats().coveredCodePoints;
        }


        // ------------------------------------------------------------
        // Keep only the best three distinct font families for each
        // script.
        // ------------------------------------------------------------

        std::vector<std::vector<ScriptFontCoverageResult>>
            topFonts(scriptCount);


        FontDirectoryView fonts(fontDirectory, true);
        FontFaceView view;

        size_t facesScanned = 0;
        size_t cmapFaces = 0;


        while (fonts(view))
        {
            ++facesScanned;


            CmapView cmap(view);

            if (!cmap)
                continue;

            UnicodeCoverageStorage coverageStorage;
            if (!cmap.buildCoverage(coverageStorage))
                continue;

            ++cmapFaces;

            const UnicodeCoverage& fontCoverage = coverageStorage.coverage();



            // --------------------------------------------------------
            // Count this font's covered characters by Script.
            //
            // One Unicode pass per font.
            // --------------------------------------------------------

            std::vector<size_t> scriptCovered(scriptCount, 0);

            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                if (!fontCoverage.contains(cp))
                    continue;

                const UnicodeScriptIndex script =
                    database.script(cp);

                if (script == kUnicodeScriptIndexInvalid)
                    continue;

                const uint32_t index =
                    static_cast<uint32_t>(script);

                if (index >= scriptCount)
                    continue;

                ++scriptCovered[index];
            }


            NameView name(view);
            MaxpView maxp(view);

            const char* faceName =
                name && name.fullName()
                ? name.fullName()
                : "(unnamed)";

            const char* familyName =
                name && name.familyName()
                ? name.familyName()
                : faceName;

            const char* location =
                view.sourceLocation()
                ? view.sourceLocation()
                : "(unknown)";

            const uint32_t glyphCount =
                maxp
                ? maxp.glyphCount()
                : 0u;


            // --------------------------------------------------------
            // Feed this font into each script's top-family ranking.
            // --------------------------------------------------------

            for (uint32_t i = 0; i < scriptCount; ++i)
            {
                const size_t total =
                    scriptTotals[i];

                const size_t covered =
                    scriptCovered[i];

                if (total == 0 || covered == 0)
                    continue;


                ScriptFontCoverageResult result;

                result.face = faceName;
                result.family = familyName;
                result.location = location;
                result.covered = covered;
                result.total = total;
                result.glyphCount = glyphCount;


                insertTopScriptFontFamily(
                    topFonts[i],
                    std::move(result));
            }
        }


        if (cmapFaces == 0)
        {
            std::printf(
                "Script top-font report: FAIL: no readable cmap faces found\n"
                "  Directory: %s\n",
                fontDirectory);

            return false;
        }


        // ------------------------------------------------------------
        // Report every non-empty Unicode script.
        // ------------------------------------------------------------

        size_t scriptsReported = 0;


        std::printf(
            "Script coverage: top font families\n"
            "  Directory:    %s\n"
            "  Faces:        %zu\n"
            "  Cmap faces:   %zu\n\n",
            fontDirectory,
            facesScanned,
            cmapFaces);


        for (uint32_t i = 0; i < scriptCount; ++i)
        {
            const size_t total =
                scriptTotals[i];

            if (total == 0)
                continue;


            const InternedKey tag =
                database.scriptISO15924(i);

            const InternedKey scriptName =
                database.scriptName(i);


            std::printf(
                "%-4s  %s\n"
                "  Script code points: %zu\n",
                tag
                ? tag
                : "(unknown)",
                scriptName
                ? scriptName
                : "(unknown)",
                total);


            const auto& results =
                topFonts[i];


            if (results.empty())
            {
                std::printf(
                    "  (no font coverage)\n\n");

                ++scriptsReported;
                continue;
            }


            for (size_t rank = 0; rank < results.size(); ++rank)
            {
                const ScriptFontCoverageResult& result =
                    results[rank];


                std::printf(
                    "  %zu. %-32s %6.2f%%  (%zu / %zu)\n"
                    "     Representative face: %s\n"
                    "     Glyphs:              %u\n"
                    "     Location:            %s\n",
                    rank + 1,
                    result.family.c_str(),
                    result.percentage(),
                    result.covered,
                    result.total,
                    result.face.c_str(),
                    static_cast<unsigned>(result.glyphCount),
                    result.location.c_str());
            }


            std::printf("\n");

            ++scriptsReported;
        }


        std::printf(
            "Script top-font report: PASS\n"
            "  Scripts reported: %zu\n"
            "  Faces scanned:    %zu\n"
            "  Cmap faces:       %zu\n",
            scriptsReported,
            facesScanned,
            cmapFaces);


        return true;
    }


    static bool testReportScriptTopFonts(const char* databaseFilename, const char* fontDirectory)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Script top-font report: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename
                ? databaseFilename
                : "(null)");

            return false;
        }


        return testReportScriptTopFonts(
            ByteSpan(fileData.data(), fileData.size()),
            fontDirectory);
    }
}
