// test_unicode_nfc_stream.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

//#include "test_core.h"
#include "unicode_database.h"
#include "unicode_nfc_stream.h"
#include "unicode_scalar_stream.h"


namespace waavs
{
    struct UnicodeNfcStreamExpected
    {
        uint32_t value;
        TextOffset begin;
        TextOffset end;
    };


    static bool testUnicodeNfcStreamCase(
        const char* name,
        const UnicodeDatabase& database,
        const uint8_t* sourceBytes,
        size_t sourceSize,
        const UnicodeNfcStreamExpected* expected,
        size_t expectedCount)
    {
        ByteSpan source(sourceBytes, sourceSize);

        Utf8ScalarStream utf8(source);
        UnicodeNfcStream<Utf8ScalarStream> nfc(utf8, database);

        if (!nfc.valid())
        {
            std::printf("  %s: FAIL - NFC stream is invalid\n", name);
            return false;
        }

        UnicodeScalar scalar;


        for (size_t i = 0; i < expectedCount; ++i)
        {
            if (!nfc(scalar))
            {
                std::printf(
                    "  %s: FAIL - premature end at scalar %zu\n",
                    name,
                    i);

                return false;
            }


            if (scalar.value != expected[i].value)
            {
                std::printf(
                    "  %s: FAIL - scalar %zu value U+%04X expected U+%04X\n",
                    name,
                    i,
                    scalar.value,
                    expected[i].value);

                return false;
            }


            if (scalar.source.begin != expected[i].begin ||
                scalar.source.end != expected[i].end)
            {
                std::printf(
                    "  %s: FAIL - scalar %zu range [%u,%u) expected [%u,%u)\n",
                    name,
                    i,
                    scalar.source.begin,
                    scalar.source.end,
                    expected[i].begin,
                    expected[i].end);

                return false;
            }
        }


        if (nfc(scalar))
        {
            std::printf(
                "  %s: FAIL - unexpected additional scalar U+%04X\n",
                name,
                scalar.value);

            return false;
        }


        if (nfc.status() != TextStreamStatus::End)
        {
            std::printf(
                "  %s: FAIL - final stream status is not End\n",
                name);

            return false;
        }


        std::printf(
            "  %-24s PASS  input=%zu output=%zu\n",
            name,
            sourceSize,
            expectedCount);

        return true;
    }


    static bool testUnicodeNfcStream(const ByteSpan& databaseData)
    {
        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "UnicodeNfcStream: FAIL - unable to open Unicode database\n");

            return false;
        }


        // ====================================================================
        // Case 1: ASCII passthrough
        //
        //      A B C
        //
        // No normalization changes occur. Source provenance remains exact.
        // ====================================================================

        static const uint8_t asciiSource[] = {
            0x41,
            0x42,
            0x43
        };

        static const UnicodeNfcStreamExpected asciiExpected[] = {
            { 0x0041u, 0u, 1u },
            { 0x0042u, 1u, 2u },
            { 0x0043u, 2u, 3u }
        };


        if (!testUnicodeNfcStreamCase(
            "ASCII passthrough",
            database,
            asciiSource,
            sizeof(asciiSource),
            asciiExpected,
            sizeof(asciiExpected) / sizeof(asciiExpected[0])))
        {
            return false;
        }


        // ====================================================================
        // Case 2: Canonical composition
        //
        //      U+0041 U+030A
        //
        //          A + COMBINING RING ABOVE
        //
        //      NFC -> U+00C5
        //
        // UTF-8:
        //
        //      U+0041      41          [0,1)
        //      U+030A      CC 8A       [1,3)
        //
        // The composed scalar inherits the union:
        //
        //      U+00C5                  [0,3)
        // ====================================================================

        static const uint8_t composeSource[] = {
            0x41,
            0xCC, 0x8A
        };

        static const UnicodeNfcStreamExpected composeExpected[] = {
            { 0x00C5u, 0u, 3u }
        };


        if (!testUnicodeNfcStreamCase(
            "Canonical composition",
            database,
            composeSource,
            sizeof(composeSource),
            composeExpected,
            sizeof(composeExpected) / sizeof(composeExpected[0])))
        {
            return false;
        }


        // ====================================================================
        // Case 3: Canonical reordering
        //
        // Input:
        //
        //      U+0071      LATIN SMALL LETTER Q
        //      U+0315      COMBINING COMMA ABOVE RIGHT   CCC 232
        //      U+0300      COMBINING GRAVE ACCENT       CCC 230
        //
        // Canonical ordering produces:
        //
        //      U+0071
        //      U+0300
        //      U+0315
        //
        // Notice that source ranges are now deliberately non-monotonic:
        //
        //      U+0071      [0,1)
        //      U+0300      [3,5)
        //      U+0315      [1,3)
        //
        // This demonstrates that provenance moves with the scalar.
        // ====================================================================

        static const uint8_t reorderSource[] = {
            0x71,
            0xCC, 0x95,
            0xCC, 0x80
        };

        static const UnicodeNfcStreamExpected reorderExpected[] = {
            { 0x0071u, 0u, 1u },
            { 0x0300u, 3u, 5u },
            { 0x0315u, 1u, 3u }
        };


        if (!testUnicodeNfcStreamCase(
            "Canonical reordering",
            database,
            reorderSource,
            sizeof(reorderSource),
            reorderExpected,
            sizeof(reorderExpected) / sizeof(reorderExpected[0])))
        {
            return false;
        }


        // ====================================================================
        // Case 4: Algorithmic Hangul composition
        //
        // Input Jamo:
        //
        //      U+1100      HANGUL CHOSEONG KIYEOK
        //      U+1161      HANGUL JUNGSEONG A
        //      U+11A8      HANGUL JONGSEONG KIYEOK
        //
        // NFC:
        //
        //      U+AC01
        //
        // UTF-8 input occupies nine bytes:
        //
        //      U+1100      [0,3)
        //      U+1161      [3,6)
        //      U+11A8      [6,9)
        //
        // The final Hangul syllable therefore carries:
        //
        //      U+AC01      [0,9)
        // ====================================================================

        static const uint8_t hangulSource[] = {
            0xE1, 0x84, 0x80,
            0xE1, 0x85, 0xA1,
            0xE1, 0x86, 0xA8
        };

        static const UnicodeNfcStreamExpected hangulExpected[] = {
            { 0xAC01u, 0u, 9u }
        };


        if (!testUnicodeNfcStreamCase(
            "Hangul composition",
            database,
            hangulSource,
            sizeof(hangulSource),
            hangulExpected,
            sizeof(hangulExpected) / sizeof(hangulExpected[0])))
        {
            return false;
        }


        std::printf("UnicodeNfcStream: PASS\n");
        std::printf("  ASCII passthrough\n");
        std::printf("  Canonical composition\n");
        std::printf("  Canonical reordering with provenance\n");
        std::printf("  Algorithmic Hangul composition\n");

        return true;
    }


    static bool testUnicodeNfcStream(const char* databaseFilename)
    {
        std::ifstream input(
            databaseFilename,
            std::ios::binary |
            std::ios::ate);

        if (!input)
        {
            std::printf(
                "UnicodeNfcStream: FAIL - unable to open database: %s\n",
                databaseFilename);

            return false;
        }


        const std::streampos end =
            input.tellg();

        if (end <= 0)
        {
            std::printf(
                "UnicodeNfcStream: FAIL - database is empty\n");

            return false;
        }


        std::vector<uint8_t> bytes(
            static_cast<size_t>(end));

        input.seekg(
            0,
            std::ios::beg);

        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));


        if (!input)
        {
            std::printf(
                "UnicodeNfcStream: FAIL - unable to read database\n");

            return false;
        }


        return testUnicodeNfcStream(
            ByteSpan(
                bytes.data(),
                bytes.size()));
    }

} // namespace waavs