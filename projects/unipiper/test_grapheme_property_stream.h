// test_grapheme_property_stream.h

#pragma once


#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "unicode_database.h"
#include "unicode_grapheme_property_stream.h"
#include "unicode_nfc_stream.h"
#include "unicode_scalar_stream.h"


namespace waavs
{
    struct GraphemePropertyStreamExpected
    {
        uint32_t value;
        TextOffset begin;
        TextOffset end;

        UnicodeGraphemeClusterBreak gcb;
        UnicodeIndicConjunctBreak incb;

        bool extendedPictographic;
    };


    static bool testGraphemePropertyStream(const ByteSpan& databaseData)
    {
        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "GraphemePropertyStream: FAIL - invalid Unicode database\n");

            return false;
        }


        // ====================================================================
        // Test sequence
        //
        // The input deliberately exercises several materially different
        // grapheme properties.
        //
        //      U+0071      LATIN SMALL LETTER Q
        //      U+0301      COMBINING ACUTE ACCENT
        //      U+200D      ZERO WIDTH JOINER
        //      U+1F1E6     REGIONAL INDICATOR SYMBOL LETTER A
        //      U+1F600     GRINNING FACE
        //      U+0915      DEVANAGARI LETTER KA
        //      U+094D      DEVANAGARI SIGN VIRAMA
        //
        // UTF-8 source ranges:
        //
        //      U+0071      [ 0,  1)
        //      U+0301      [ 1,  3)
        //      U+200D      [ 3,  6)
        //      U+1F1E6     [ 6, 10)
        //      U+1F600     [10, 14)
        //      U+0915      [14, 17)
        //      U+094D      [17, 20)
        //
        // No NFC transformation changes this particular sequence, allowing the
        // test to verify that provenance passes unchanged through both NFC and
        // grapheme-property annotation.
        // ====================================================================

        static const uint8_t sourceBytes[] = {
            0x71,

            0xCC, 0x81,

            0xE2, 0x80, 0x8D,

            0xF0, 0x9F, 0x87, 0xA6,

            0xF0, 0x9F, 0x98, 0x80,

            0xE0, 0xA4, 0x95,

            0xE0, 0xA5, 0x8D
        };


        static const GraphemePropertyStreamExpected expected[] = {
            {
                0x0071u,
                0u, 1u,
                UnicodeGraphemeClusterBreak::Other,
                UnicodeIndicConjunctBreak::None,
                false
            },

            {
                0x0301u,
                1u, 3u,
                UnicodeGraphemeClusterBreak::Extend,
                UnicodeIndicConjunctBreak::Extend,
                false
            },

            {
                0x200Du,
                3u, 6u,
                UnicodeGraphemeClusterBreak::ZWJ,
                UnicodeIndicConjunctBreak::Extend,
                false
            },

            {
                0x1F1E6u,
                6u, 10u,
                UnicodeGraphemeClusterBreak::RegionalIndicator,
                UnicodeIndicConjunctBreak::None,
                false
            },

            {
                0x1F600u,
                10u, 14u,
                UnicodeGraphemeClusterBreak::Other,
                UnicodeIndicConjunctBreak::None,
                true
            },

            {
                0x0915u,
                14u, 17u,
                UnicodeGraphemeClusterBreak::Other,
                UnicodeIndicConjunctBreak::Consonant,
                false
            },

            {
                0x094Du,
                17u, 20u,
                UnicodeGraphemeClusterBreak::Extend,
                UnicodeIndicConjunctBreak::Linker,
                false
            }
        };


        // ====================================================================
        // Build the complete pipeline developed so far.
        //
        //      UTF-8
        //          ->
        //      UnicodeScalar
        //          ->
        //      NFC UnicodeScalar
        //          ->
        //      GraphemeScalar
        // ====================================================================

        ByteSpan source(
            sourceBytes,
            sizeof(sourceBytes));

        Utf8ScalarStream utf8(source);

        UnicodeNfcStream<Utf8ScalarStream> nfc(
            utf8,
            database);

        GraphemePropertyStream<
            UnicodeNfcStream<Utf8ScalarStream>>
            properties(
                nfc,
                database);


        if (!nfc.valid())
        {
            std::printf(
                "GraphemePropertyStream: FAIL - NFC stream invalid\n");

            return false;
        }


        if (!properties.valid())
        {
            std::printf(
                "GraphemePropertyStream: FAIL - property stream invalid\n");

            return false;
        }


        // ====================================================================
        // Verify annotated output.
        // ====================================================================

        GraphemeScalar scalar;

        constexpr size_t expectedCount =
            sizeof(expected) / sizeof(expected[0]);


        for (size_t i = 0; i < expectedCount; ++i)
        {
            if (!properties(scalar))
            {
                std::printf(
                    "GraphemePropertyStream: FAIL - premature end at scalar %zu\n",
                    i);

                return false;
            }


            const GraphemePropertyStreamExpected& e =
                expected[i];


            if (scalar.scalar.value != e.value)
            {
                std::printf(
                    "GraphemePropertyStream: FAIL - scalar %zu value "
                    "U+%04X expected U+%04X\n",
                    i,
                    scalar.scalar.value,
                    e.value);

                return false;
            }


            if (scalar.scalar.source.begin != e.begin ||
                scalar.scalar.source.end != e.end)
            {
                std::printf(
                    "GraphemePropertyStream: FAIL - scalar %zu range "
                    "[%u,%u) expected [%u,%u)\n",
                    i,
                    scalar.scalar.source.begin,
                    scalar.scalar.source.end,
                    e.begin,
                    e.end);

                return false;
            }


            if (scalar.gcb != e.gcb)
            {
                std::printf(
                    "GraphemePropertyStream: FAIL - scalar %zu "
                    "GCB=%u expected %u\n",
                    i,
                    static_cast<unsigned>(scalar.gcb),
                    static_cast<unsigned>(e.gcb));

                return false;
            }


            if (scalar.incb != e.incb)
            {
                std::printf(
                    "GraphemePropertyStream: FAIL - scalar %zu "
                    "InCB=%u expected %u\n",
                    i,
                    static_cast<unsigned>(scalar.incb),
                    static_cast<unsigned>(e.incb));

                return false;
            }


            if (scalar.extendedPictographic !=
                e.extendedPictographic)
            {
                std::printf(
                    "GraphemePropertyStream: FAIL - scalar %zu "
                    "Extended_Pictographic=%u expected %u\n",
                    i,
                    scalar.extendedPictographic ? 1u : 0u,
                    e.extendedPictographic ? 1u : 0u);

                return false;
            }
        }


        // ====================================================================
        // Verify clean end-of-stream propagation.
        // ====================================================================

        if (properties(scalar))
        {
            std::printf(
                "GraphemePropertyStream: FAIL - unexpected extra scalar "
                "U+%04X\n",
                scalar.scalar.value);

            return false;
        }


        if (properties.status() != TextStreamStatus::End)
        {
            std::printf(
                "GraphemePropertyStream: FAIL - property stream did not end\n");

            return false;
        }


        if (nfc.status() != TextStreamStatus::End)
        {
            std::printf(
                "GraphemePropertyStream: FAIL - NFC stream did not end\n");

            return false;
        }


        if (utf8.status() != TextStreamStatus::End)
        {
            std::printf(
                "GraphemePropertyStream: FAIL - UTF-8 stream did not end\n");

            return false;
        }


        std::printf("GraphemePropertyStream: PASS\n");
        std::printf("  Scalars annotated:       %zu\n", expectedCount);
        std::printf("  Source bytes:            %zu\n", sizeof(sourceBytes));
        std::printf("  GCB Other:               PASS\n");
        std::printf("  GCB Extend:              PASS\n");
        std::printf("  GCB ZWJ:                 PASS\n");
        std::printf("  GCB RegionalIndicator:   PASS\n");
        std::printf("  InCB Extend:             PASS\n");
        std::printf("  InCB Consonant:          PASS\n");
        std::printf("  InCB Linker:             PASS\n");
        std::printf("  Extended_Pictographic:   PASS\n");
        std::printf("  Provenance passthrough:  PASS\n");
        std::printf("  Stream status propagation: PASS\n");

        return true;
    }


    static bool testGraphemePropertyStream(
        const char* databaseFilename)
    {
        std::ifstream input(
            databaseFilename,
            std::ios::binary |
            std::ios::ate);

        if (!input)
        {
            std::printf(
                "GraphemePropertyStream: FAIL - unable to open database: %s\n",
                databaseFilename);

            return false;
        }


        const std::streampos end =
            input.tellg();

        if (end <= 0)
        {
            std::printf(
                "GraphemePropertyStream: FAIL - database is empty\n");

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
                "GraphemePropertyStream: FAIL - unable to read database\n");

            return false;
        }


        return testGraphemePropertyStream(
            ByteSpan(
                bytes.data(),
                bytes.size()));
    }

} // namespace waavs
