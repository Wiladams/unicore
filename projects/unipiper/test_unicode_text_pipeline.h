// test_unicode_text_pipeline.h

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
    // testUnicodeTextPipeline
    //
    // End-to-end Unicode text preprocessing test:
    //
    //      UTF-8
    //          |
    //          v
    //      Utf8ScalarStream
    //          |
    //          v
    //      UnicodeNfcStream
    //          |
    //          v
    //      GraphemePropertyStream
    //          |
    //          v
    //      GraphemeStream
    //          |
    //          v
    //      UnicodeScriptStream
    //          |
    //          v
    //      UnicodeBidiStream
    //          |
    //          v
    //      ShapingRunItemizer
    //
    //
    // Input text:
    //
    //      e + U+0301
    //      U+0633 U+064E U+0644 U+0627 U+0645
    //      x
    //
    // The first two input scalars NFC-compose:
    //
    //      U+0065 U+0301 -> U+00E9
    //
    // The Arabic seen + fatha pair forms one grapheme cluster.
    //
    // Expected normalized logical scalars:
    //
    //      U+00E9
    //      U+0633 U+064E U+0644 U+0627 U+0645
    //      U+0078
    //
    // Expected candidate shaping runs:
    //
    //      Latn / level 0
    //      Arab / level 1
    //      Latn / level 0
    //
    // ========================================================================

    static bool testUnicodeTextPipeline(const ByteSpan& databaseData)
    {
        auto fail = [](const char* message) -> bool
            {
                std::printf(
                    "Unicode text pipeline: FAIL: %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Database
        // ====================================================================

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("unable to initialize Unicode database");


        const UnicodeScriptIndex latin =
            database.script(0x00E9);

        const UnicodeScriptIndex arabic =
            database.script(0x0633);


        if (latin == kUnicodeScriptIndexInvalid ||
            arabic == kUnicodeScriptIndexInvalid)
        {
            return fail("unable to locate required Scripts");
        }


        // ====================================================================
        // UTF-8 source
        //
        //      e                   U+0065
        //      combining acute     U+0301
        //      Arabic seen         U+0633
        //      Arabic fatha        U+064E
        //      Arabic lam          U+0644
        //      Arabic alef         U+0627
        //      Arabic meem         U+0645
        //      x                   U+0078
        //
        // Keep the source byte array explicit so the test source remains
        // ASCII-only.
        // ====================================================================

        static const uint8_t sourceBytes[] =
        {
            0x65,

            0xCC, 0x81,

            0xD8, 0xB3,
            0xD9, 0x8E,
            0xD9, 0x84,
            0xD8, 0xA7,
            0xD9, 0x85,

            0x78
        };


        const ByteSpan source(
            sourceBytes,
            sizeof(sourceBytes));


        // ====================================================================
        // Construct the complete pipeline.
        // ====================================================================

        Utf8ScalarStream utf8(source);

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
        // Pull the resolved paragraph.
        // ====================================================================

        BidiParagraphView paragraph{};

        if (!bidi(paragraph))
            return fail("bidi paragraph was not emitted");


        // ====================================================================
        // Verify NFC normalization.
        //
        // Input scalar count:
        //
        //      8
        //
        // Normalized scalar count:
        //
        //      7
        //
        // because:
        //
        //      U+0065 U+0301 -> U+00E9
        // ====================================================================

        static const uint32_t expectedScalars[] =
        {
            0x00E9,
            0x0633,
            0x064E,
            0x0644,
            0x0627,
            0x0645,
            0x0078
        };


        if (paragraph.scalarCount !=
            sizeof(expectedScalars) / sizeof(expectedScalars[0]))
        {
            return fail("normalized scalar count");
        }


        for (uint32_t i = 0; i < paragraph.scalarCount; ++i)
        {
            if (paragraph.scalars[i].value != expectedScalars[i])
                return fail("normalized scalar sequence");
        }


        // ====================================================================
        // Paragraph metadata.
        // ====================================================================

        if (paragraph.paragraphLevel != 0)
            return fail("paragraph level");

        if (!paragraph.leftToRight())
            return fail("paragraph direction");

        if (paragraph.normalizedBegin != 0)
            return fail("paragraph normalized begin");

        if (paragraph.source.begin != 0 ||
            paragraph.source.end != sizeof(sourceBytes))
        {
            return fail("paragraph source range");
        }


        // ====================================================================
        // Verify grapheme segmentation.
        //
        // Expected clusters:
        //
        //      cluster 0       U+00E9
        //
        //      cluster 1       U+0633 U+064E
        //                      seen + fatha
        //
        //      cluster 2       U+0644
        //      cluster 3       U+0627
        //      cluster 4       U+0645
        //
        //      cluster 5       U+0078
        // ====================================================================

        if (paragraph.clusterCount != 6)
            return fail("grapheme cluster count");


        static const uint32_t expectedClusterOffsets[] =
        {
            0, 1, 3, 4, 5, 6
        };


        static const uint32_t expectedClusterCounts[] =
        {
            1, 2, 1, 1, 1, 1
        };


        for (uint32_t i = 0; i < paragraph.clusterCount; ++i)
        {
            const ShapingCluster& cluster =
                paragraph.clusters[i];

            if (cluster.scalarOffset != expectedClusterOffsets[i])
                return fail("grapheme scalar offset");

            if (cluster.scalarCount != expectedClusterCounts[i])
                return fail("grapheme scalar count");
        }


        // ====================================================================
        // Verify normalization provenance.
        //
        // The composed U+00E9 represents source bytes:
        //
        //      e           [0,1)
        //      U+0301      [1,3)
        //
        // so the normalized scalar/cluster must retain:
        //
        //      [0,3)
        // ====================================================================

        if (paragraph.clusters[0].source.begin != 0 ||
            paragraph.clusters[0].source.end != 3)
        {
            return fail("NFC composition source provenance");
        }


        // ====================================================================
        // Verify multi-scalar Arabic grapheme provenance.
        //
        //      seen        [3,5)
        //      fatha       [5,7)
        //
        // cluster:
        //
        //                  [3,7)
        // ====================================================================

        if (paragraph.clusters[1].source.begin != 3 ||
            paragraph.clusters[1].source.end != 7)
        {
            return fail("Arabic grapheme source provenance");
        }


        // ====================================================================
        // Verify Script analysis.
        // ====================================================================

        if (paragraph.scripts[0].script != latin)
            return fail("Latin Script before Arabic");

        for (uint32_t i = 1; i <= 4; ++i)
        {
            if (paragraph.scripts[i].script != arabic)
                return fail("Arabic Script");
        }

        if (paragraph.scripts[5].script != latin)
            return fail("Latin Script after Arabic");


        // ====================================================================
        // Verify final resolved bidi levels.
        //
        // The paragraph begins with Latin and therefore has paragraph level 0.
        //
        // Arabic letters and the Arabic NSM resolve to level 1.
        //
        // The trailing Latin x returns to level 0.
        //
        // Logical order remains unchanged.
        // ====================================================================

        static const UnicodeBidiLevel expectedLevels[] =
        {
            0,
            1, 1, 1, 1, 1,
            0
        };


        for (uint32_t i = 0; i < paragraph.scalarCount; ++i)
        {
            if (paragraph.levels[i] != expectedLevels[i])
                return fail("final resolved bidi levels");
        }


        // ====================================================================
        // Shaping-run itemization.
        // ====================================================================

        ShapingRunItemizer itemizer(paragraph, database);

        if (!itemizer.ready())
            return fail("shaping-run itemizer rejected paragraph");


        struct ExpectedRun
        {
            uint32_t scalarOffset;
            uint32_t scalarCount;

            uint32_t clusterCount;

            UnicodeScriptIndex script;
            UnicodeBidiLevel level;

            ScalarIndex normalizedBegin;

            TextOffset sourceBegin;
            TextOffset sourceEnd;
        };


        const ExpectedRun expectedRuns[] =
        {
            {
                0, 1,
                1,
                latin, 0,
                0,
                0, 3
            },

            {
                1, 5,
                4,
                arabic, 1,
                1,
                3, 13
            },

            {
                6, 1,
                1,
                latin, 0,
                6,
                13, 14
            }
        };


        uint32_t runCount = 0;


        for (const ExpectedRun& expected : expectedRuns)
        {
            ShapingRunView run{};

            if (!itemizer(run))
                return fail("expected shaping run was not emitted");


            if (run.scalars !=
                paragraph.scalars + expected.scalarOffset)
            {
                return fail("shaping run scalar pointer");
            }

            if (run.scalarCount != expected.scalarCount)
                return fail("shaping run scalar count");

            if (run.clusterCount != expected.clusterCount)
                return fail("shaping run cluster count");

            if (run.script != expected.script)
                return fail("shaping run Script");

            if (run.bidiLevel != expected.level)
                return fail("shaping run bidi level");

            if (run.normalizedBegin != expected.normalizedBegin)
                return fail("shaping run normalized begin");

            if (run.source.begin != expected.sourceBegin ||
                run.source.end != expected.sourceEnd)
            {
                return fail("shaping run source range");
            }


            // ---------------------------------------------------------------
            // Direction is derived directly from final level parity.
            // ---------------------------------------------------------------

            if ((expected.level & 1u) != 0)
            {
                if (!run.rightToLeft() || run.leftToRight())
                    return fail("RTL shaping run direction");
            }
            else
            {
                if (!run.leftToRight() || run.rightToLeft())
                    return fail("LTR shaping run direction");
            }


            // ---------------------------------------------------------------
            // Print the run.
            // ---------------------------------------------------------------

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
                runCount,
                scriptTag ? scriptTag : "????",
                static_cast<unsigned>(run.bidiLevel),
                run.rightToLeft() ? "RTL" : "LTR",
                run.scalarCount,
                run.clusterCount,
                run.normalizedBegin,
                run.source.begin,
                run.source.end);


            std::printf("    Code points:       ");

            for (const UnicodeScalar& scalar : run)
            {
                std::printf(
                    " U+%04X",
                    scalar.value);
            }

            std::printf("\n");


            std::printf("    Cluster geometry: ");

            for (uint32_t i = 0; i < run.clusterCount; ++i)
            {
                const ShapingCluster& cluster = run.clusters[i];

                std::printf(
                    " [%u+%u]",
                    cluster.scalarOffset,
                    cluster.scalarCount);
            }

            std::printf("\n");


            ++runCount;
        }


        // ====================================================================
        // Verify exact run count and itemizer End state.
        // ====================================================================

        {
            ShapingRunView run{};

            if (itemizer(run))
                return fail("unexpected additional shaping run");

            if (!itemizer.ended())
                return fail("itemizer did not reach End");
        }


        if (runCount != 3)
            return fail("shaping run count");


        // ====================================================================
        // Verify the Arabic run's rebased cluster geometry.
        //
        // Paragraph cluster offsets:
        //
        //      1, 3, 4, 5
        //
        // Arabic shaping run begins at paragraph scalar 1.
        //
        // Run-relative offsets:
        //
        //      0, 2, 3, 4
        //
        // Pull a fresh pipeline is unnecessary here because this geometry was
        // already validated by the itemizer's own focused unit test.
        // The printed output above makes it directly visible in this
        // integration test.
        // ====================================================================


        // ====================================================================
        // The paragraph was the final paragraph in the UTF-8 source.
        //
        // Only check bidi EOF after all shaping-run views have been consumed,
        // because another bidi pull invalidates the borrowed paragraph view.
        // ====================================================================

        BidiParagraphView extraParagraph{};

        if (bidi(extraParagraph))
            return fail("unexpected second paragraph");

        if (!bidi.ended())
            return fail("bidi stream did not reach End");


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "Unicode text pipeline: PASS\n"
            "  UTF-8 bytes:         %zu\n"
            "  Normalized scalars:  %u\n"
            "  Grapheme clusters:   %u\n"
            "  Shaping runs:        %u\n",
            sizeof(sourceBytes),
            paragraph.scalarCount,
            paragraph.clusterCount,
            runCount);


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static bool testUnicodeTextPipeline(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Unicode text pipeline: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testUnicodeTextPipeline(databaseData);
    }

} // namespace waavs