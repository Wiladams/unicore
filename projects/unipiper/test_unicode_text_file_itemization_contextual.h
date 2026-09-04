// test_unicode_text_file_itemization_contextual.h

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
    static bool dumpUnicodeTextFileItemizationContextual(
        const ByteSpan& databaseData,
        const ByteSpan& textData)
    {
        auto fail = [](const char* message) -> bool
        {
            std::printf("Unicode contextual itemization: FAIL: %s\n", message);
            return false;
        };

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("unable to initialize Unicode database");

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

        uint32_t paragraphIndex = 0;
        uint32_t totalRuns = 0;

        BidiParagraphView paragraph{};

        while (bidi(paragraph))
        {
            std::printf(
                "\nParagraph %u\n"
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

                std::printf("    Code points:       ");

                for (const UnicodeScalar& scalar : run)
                    std::printf(" U+%04X", scalar.value);

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

                ++runIndex;
                ++totalRuns;
            }

            if (!itemizer.ended())
                return fail("shaping-run itemizer did not reach End");

            std::printf(
                "  Paragraph runs:   %u\n",
                runIndex);

            ++paragraphIndex;
        }

        if (!bidi.ended())
            return fail("bidi stream did not reach End");

        std::printf(
            "\nUnicode contextual itemization: PASS\n"
            "  UTF-8 bytes: %zu\n"
            "  Paragraphs:  %u\n"
            "  Runs:        %u\n",
            textData.size(),
            paragraphIndex,
            totalRuns);

        return true;
    }

    static bool testUnicodeTextFileItemizationContextual(
        const char* databaseFilename,
        const char* textFilename)
    {
        std::vector<uint8_t> databaseFileData;
        std::vector<uint8_t> textFileData;

        if (!readFileData(databaseFilename, databaseFileData))
        {
            std::printf(
                "Unicode contextual itemization: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }

        if (!readFileData(textFilename, textFileData))
        {
            std::printf(
                "Unicode contextual itemization: FAIL: unable to read text file\n"
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

        return dumpUnicodeTextFileItemizationContextual(
            databaseData,
            textData);
    }

} // namespace waavs
