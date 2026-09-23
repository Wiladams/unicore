// test_devanagari_script_edge_cases.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <cstring>
#include <vector>

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
    struct DevanagariScriptEdgeCase
    {
        const char* name{ nullptr };
        const char* text{ nullptr };
    };

    static bool inspectDevanagariScriptEdgeCase(const UnicodeDatabase& database, const DevanagariScriptEdgeCase& testCase)
    {
        auto fail = [&testCase](const char* stage)
        {
            std::printf("  %-30s FAIL: %s\n", testCase.name ? testCase.name : "(unnamed)", stage);
            return false;
        };

        if (!testCase.name || !testCase.text)
            return fail("invalid test case");

        const ByteSpan text(reinterpret_cast<const uint8_t*>(testCase.text), std::strlen(testCase.text));

        Utf8ScalarStream utf8(text);
        UnicodeNfcStream<Utf8ScalarStream> nfc(utf8, database);
        GraphemePropertyStream<decltype(nfc)> properties(nfc, database);
        GraphemeStream<decltype(properties)> graphemes(properties);
        UnicodeScriptStream<decltype(graphemes)> scripts(graphemes, database);
        UnicodeBidiStream<decltype(scripts)> bidi(scripts, database);

        BidiParagraphView paragraph{};

        size_t paragraphCount = 0;
        size_t shapingRunCount = 0;

        while (bidi(paragraph))
        {
            ++paragraphCount;

            std::printf(
                "\nCASE: %s\n"
                "  paragraph %zu\n"
                "  scalars:  %u\n"
                "  clusters: %u\n",
                testCase.name,
                paragraphCount,
                static_cast<unsigned>(paragraph.scalarCount),
                static_cast<unsigned>(paragraph.clusterCount));

            ShapingRunItemizer itemizer(paragraph, database);

            if (itemizer.failed())
                return fail("ShapingRunItemizer constructor");

            ShapingRunView run{};
            size_t localRunCount = 0;

            while (itemizer(run))
            {
                ++localRunCount;
                ++shapingRunCount;

                const InternedKey scriptName = database.scriptISO15924(run.script);

                std::printf(
                    "    run %zu\n"
                    "      scalars:  %u\n"
                    "      clusters: %u\n"
                    "      level:    %u\n"
                    "      script:   %s\n",
                    localRunCount,
                    static_cast<unsigned>(run.scalarCount),
                    static_cast<unsigned>(run.clusterCount),
                    static_cast<unsigned>(run.bidiLevel),
                    scriptName ? scriptName : "(null)");
            }

            if (!itemizer.ended())
                return fail("ShapingRunItemizer did not end cleanly");

            std::printf("  shaping runs: %zu\n", localRunCount);
        }

        if (!bidi.ended())
            return fail("bidi stream did not end cleanly");

        if (paragraphCount != 1)
            return fail("expected exactly one paragraph");

        if (shapingRunCount == 0)
            return fail("no shaping runs produced");

        std::printf("  %-30s PASS\n", testCase.name);
        return true;
    }

    static bool testDevanagariScriptEdgeCases(const ByteSpan& databaseData)
    {
        auto fail = [](const char* message)
        {
            std::printf("Devanagari script edge cases: FAIL\n  %s\n", message);
            return false;
        };

        if (!databaseData)
            return fail("empty Unicode database");

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("invalid Unicode database");

        static constexpr DevanagariScriptEdgeCase kCases[] =
        {
            { "Devanagari digits compact", "०१२३४५६७८९" },
            { "Devanagari digits spaced", "० १ २ ३ ४ ५ ६ ७ ८ ९" },
            { "ASCII digits compact", "0123456789" },
            { "ASCII digits spaced", "0 1 2 3 4 5 6 7 8 9" },
            { "Deva then ASCII digits", "०१२३ 0123" },
            { "Deva spaced then ASCII", "० १ २   0 1 2" },
            { "ASCII then Deva digits", "0123 ०१२३" },
            { "Devanagari word + digits", "भारत १२३" },
            { "Devanagari word + ASCII", "भारत 123" },
            { "sentence with danda", "यह एक देवनागरी परीक्षण है।" },
            { "sentence with period", "यह एक देवनागरी परीक्षण है." },
            { "Vedic basic", "अ॑" },
            { "Vedic spaced", "अ॑ अ॒" },
            { "Vedic extended 1CD0", "अ᳐" },
            { "Vedic extended 1CD2", "अ᳒" },
            { "Vedic extended 1CDA", "अ᳚" },
            { "Vedic mixed", "अ॑   अ॒   अ᳐   अ᳒   अ᳚" },
            { "stress sentence", "श्रेणी संयोजक ज्ञानी प्रार्थना संस्कृत कृपया ब्रह्म" }
        };

        size_t passed = 0;

        for (const DevanagariScriptEdgeCase& testCase : kCases)
        {
            if (!inspectDevanagariScriptEdgeCase(database, testCase))
                return false;

            ++passed;
        }

        std::printf("\nDevanagari script edge cases: PASS\n  Cases: %zu\n", passed);
        return true;
    }

    static bool testDevanagariScriptEdgeCases(const char* databaseFilename)
    {
        std::vector<uint8_t> databaseBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "Devanagari script edge cases: FAIL\n"
                "  Unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        return testDevanagariScriptEdgeCases(ByteSpan(databaseBytes.data(), databaseBytes.size()));
    }

} // namespace waavs
