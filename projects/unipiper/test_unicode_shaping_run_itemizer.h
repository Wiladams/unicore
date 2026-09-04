// test_unicode_text_file_itemization.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_database.h"
#include "unicode_scalar_stream.h"
#include "unicode_nfc_stream.h"
#include "unicode_grapheme_property_stream.h"
#include "unicode_grapheme_stream.h"
#include "unicode_script_analysis.h"
#include "unicode_bidi_analysis.h"
#include "unicode_shaping_run_itemizer.h"


namespace waavs
{
    // ========================================================================
    // dumpUnicodeTextFileItemization
    //
    // Run an arbitrary UTF-8 file through the complete Unicode preprocessing
    // pipeline and print the resulting shaping runs.
    //
    // This is intentionally an observational integration test. It does not
    // verify expected scripts, levels, clusters, or run counts.
    //
    // Mixed-level grapheme clusters are reported explicitly. Such a cluster
    // is not inherently invalid because X9-removed scalars may retain a
    // bookkeeping level different from the effective shaping level.
    // ========================================================================

    static bool dumpUnicodeTextFileItemization(const ByteSpan& databaseData,
        const ByteSpan& textData)
    {
        auto fail = [](const char* message) -> bool
            {
                std::printf(
                    "Unicode text file itemization: FAIL: %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Unicode database
        // ====================================================================

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("unable to initialize Unicode database");


        // ====================================================================
        // UTF-8
        //   ->
        // NFC
        //   ->
        // grapheme properties
        //   ->
        // grapheme clusters
        //   ->
        // Script analysis
        //   ->
        // bidi analysis
        // ====================================================================

        Utf8ScalarStream utf8(textData);


        UnicodeNfcStream<Utf8ScalarStream> nfc(
            utf8,
            database);


        GraphemePropertyStream<
            UnicodeNfcStream<Utf8ScalarStream>> properties(
                nfc,
                database);


        GraphemeStream<
            GraphemePropertyStream<
            UnicodeNfcStream<Utf8ScalarStream>>> graphemes(
                properties);


        UnicodeScriptStream<
            GraphemeStream<
            GraphemePropertyStream<
            UnicodeNfcStream<Utf8ScalarStream>>>> scripts(
                graphemes,
                database);


        UnicodeBidiStream<
            UnicodeScriptStream<
            GraphemeStream<
            GraphemePropertyStream<
            UnicodeNfcStream<Utf8ScalarStream>>>>> bidi(
                scripts,
                database);


        // ====================================================================
        // Pull paragraphs
        //
        // BidiParagraphView is borrowed from UnicodeBidiStream and remains
        // valid only until the next bidi pull. Therefore each paragraph must
        // be completely inspected and itemized before requesting the next
        // paragraph.
        // ====================================================================

        uint32_t paragraphIndex = 0;
        uint32_t totalRuns = 0;
        uint32_t totalMixedLevelClusters = 0;

        BidiParagraphView paragraph{};


        while (bidi(paragraph))
        {
            if (paragraph.scalarCount != 0 &&
                !paragraph.originalTypes)
            {
                return fail("bidi paragraph is missing original bidi types");
            }


            std::printf(
                "\n"
                "Paragraph %u\n"
                "  Scalars:          %u\n"
                "  Clusters:         %u\n"
                "  Paragraph level:  %u\n"
                "  Direction:        %s\n"
                "  Normalized begin: %u\n"
                "  Source:           [%u,%u)\n",
                paragraphIndex,
                paragraph.scalarCount,
                paragraph.clusterCount,
                static_cast<unsigned>(paragraph.paragraphLevel),
                paragraph.rightToLeft() ? "RTL" : "LTR",
                paragraph.normalizedBegin,
                paragraph.source.begin,
                paragraph.source.end);


            // ================================================================
            // Inspect grapheme bidi-level homogeneity.
            //
            // A mixed-level cluster is informational here. X9-removed scalars,
            // such as ZWJ with bidi class BN, may legitimately retain a stored
            // level different from the participating scalars in the cluster.
            // ================================================================

            uint32_t paragraphMixedLevelClusters = 0;


            for (uint32_t clusterIndex = 0;
                clusterIndex < paragraph.clusterCount;
                ++clusterIndex)
            {
                const ShapingCluster& cluster =
                    paragraph.clusters[clusterIndex];


                const UnicodeBidiLevel firstLevel =
                    paragraph.levels[cluster.scalarOffset];


                bool mixedLevel = false;


                for (uint32_t scalar = 1;
                    scalar < cluster.scalarCount;
                    ++scalar)
                {
                    if (paragraph.levels[
                        cluster.scalarOffset + scalar] != firstLevel)
                    {
                        mixedLevel = true;
                        break;
                    }
                }


                if (!mixedLevel)
                    continue;


                ++paragraphMixedLevelClusters;
                ++totalMixedLevelClusters;


                InternedKey scriptTag =
                    database.scriptISO15924(
                        paragraph.scripts[clusterIndex].script);


                std::printf(
                    "  Mixed-level cluster %u\n"
                    "    Script:  %s\n"
                    "    Offset:  %u\n"
                    "    Scalars: %u\n"
                    "    Source:  [%u,%u)\n"
                    "    Contents:\n",
                    clusterIndex,
                    scriptTag ? scriptTag : "????",
                    cluster.scalarOffset,
                    cluster.scalarCount,
                    cluster.source.begin,
                    cluster.source.end);


                for (uint32_t scalar = 0;
                    scalar < cluster.scalarCount;
                    ++scalar)
                {
                    const uint32_t scalarIndex =
                        cluster.scalarOffset + scalar;


                    const UnicodeScalar& value =
                        paragraph.scalars[scalarIndex];


                    const UnicodeBidiClass originalType =
                        paragraph.originalTypes[scalarIndex];


                    const UnicodeBidiLevel level =
                        paragraph.levels[scalarIndex];


                    std::printf(
                        "      [%u] U+%04X level=%u X9=%s\n",
                        scalar,
                        value.value,
                        static_cast<unsigned>(level),
                        isBidiRemovedByX9(originalType)
                        ? "YES"
                        : "NO");
                }
            }


            std::printf(
                "  Mixed-level clusters: %u\n",
                paragraphMixedLevelClusters);


            // ================================================================
            // Candidate shaping runs
            //
            // The itemizer receives the paragraph and Unicode database.
            //
            // It uses originalTypes to ignore X9-removed scalars when
            // determining the effective shaping bidi level of a grapheme.
            // ================================================================

            ShapingRunItemizer itemizer(
                paragraph,
                database);


            if (itemizer.failed())
                return fail("shaping-run itemizer rejected paragraph");


            uint32_t runIndex = 0;
            ShapingRunView run{};


            while (itemizer(run))
            {
                InternedKey scriptTag =
                    database.scriptISO15924(run.script);


                std::printf(
                    "  Run %u\n"
                    "    Script:           %s\n"
                    "    Bidi level:       %u\n"
                    "    Direction:        %s\n"
                    "    Scalars:          %u\n"
                    "    Clusters:         %u\n"
                    "    Normalized begin: %u\n"
                    "    Source:           [%u,%u)\n",
                    runIndex,
                    scriptTag ? scriptTag : "????",
                    static_cast<unsigned>(run.bidiLevel),
                    run.rightToLeft() ? "RTL" : "LTR",
                    run.scalarCount,
                    run.clusterCount,
                    run.normalizedBegin,
                    run.source.begin,
                    run.source.end);


                // ============================================================
                // Scalars
                //
                // ShapingRunView::begin()/end() iterate UnicodeScalar.
                // ============================================================

                std::printf(
                    "    Code points:      ");


                for (const UnicodeScalar& scalar : run)
                {
                    std::printf(
                        " U+%04X",
                        scalar.value);
                }


                std::printf("\n");


                // ============================================================
                // Grapheme geometry
                //
                // ShapingCluster offsets are relative to the run.
                // ============================================================

                std::printf(
                    "    Cluster geometry:");


                for (uint32_t i = 0;
                    i < run.clusterCount;
                    ++i)
                {
                    const ShapingCluster& cluster =
                        run.clusters[i];


                    std::printf(
                        " [%u+%u]",
                        cluster.scalarOffset,
                        cluster.scalarCount);
                }


                std::printf("\n");


                ++runIndex;
                ++totalRuns;
            }


            if (!itemizer.ended())
                return fail("shaping-run itemizer did not reach End");


            std::printf(
                "  Paragraph runs:      %u\n",
                runIndex);


            ++paragraphIndex;
        }


        // ====================================================================
        // Final stream state
        // ====================================================================

        if (!bidi.ended())
            return fail("bidi stream did not reach End");


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "\n"
            "Unicode text file itemization: PASS\n"
            "  UTF-8 bytes:          %zu\n"
            "  Paragraphs:           %u\n"
            "  Runs:                 %u\n"
            "  Mixed-level clusters: %u\n",
            textData.size(),
            paragraphIndex,
            totalRuns,
            totalMixedLevelClusters);


        return true;
    }


    // ========================================================================
    // Filename convenience overload
    // ========================================================================

    static bool testUnicodeTextFileItemization(const char* databaseFilename,
        const char* textFilename)
    {
        std::vector<uint8_t> databaseFileData;
        std::vector<uint8_t> textFileData;


        if (!readFileData(databaseFilename, databaseFileData))
        {
            std::printf(
                "Unicode text file itemization: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        if (!readFileData(textFilename, textFileData))
        {
            std::printf(
                "Unicode text file itemization: FAIL: unable to read text file\n"
                "  File: %s\n",
                textFilename);

            return false;
        }


        const ByteSpan databaseData(
            databaseFileData.data(),
            databaseFileData.size());


        const ByteSpan textData(
            textFileData.data(),
            textFileData.size());


        return dumpUnicodeTextFileItemization(
            databaseData,
            textData);
    }

} // namespace waavs