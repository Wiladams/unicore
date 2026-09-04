// test_unicode_grapheme_bidi_levels.h

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
    static bool testUnicodeGraphemeBidiLevels(const ByteSpan& databaseData)
    {
        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Unicode grapheme bidi levels: FAIL\n"
                "  Unable to initialize Unicode database\n");

            return false;
        }


        // ====================================================================
        // Probe cases
        //
        // All source arrays are explicit UTF-8 so the C++ source remains
        // ASCII-only.
        // ====================================================================


        // --------------------------------------------------------------------
        // Arabic base + combining marks
        //
        //   a [SEEN FATHA SHADDA] b
        // --------------------------------------------------------------------

        static const uint8_t arabicMarks[] =
        {
            0x61,

            0xD8, 0xB3,             // U+0633 ARABIC LETTER SEEN
            0xD9, 0x8E,             // U+064E ARABIC FATHA
            0xD9, 0x91,             // U+0651 ARABIC SHADDA

            0x62
        };


        // --------------------------------------------------------------------
        // Hebrew base + combining mark
        //
        //   a [SHIN QAMATS] b
        // --------------------------------------------------------------------

        static const uint8_t hebrewMark[] =
        {
            0x61,

            0xD7, 0xA9,             // U+05E9 HEBREW LETTER SHIN
            0xD6, 0xB8,             // U+05B8 HEBREW POINT QAMATS

            0x62
        };


        // --------------------------------------------------------------------
        // Emoji ZWJ sequence
        //
        //   a [WOMAN ZWJ LAPTOP] b
        //
        // U+1F469 U+200D U+1F4BB
        // --------------------------------------------------------------------

        static const uint8_t womanTechnologist[] =
        {
            0x61,

            0xF0, 0x9F, 0x91, 0xA9, // U+1F469 WOMAN
            0xE2, 0x80, 0x8D,       // U+200D ZERO WIDTH JOINER
            0xF0, 0x9F, 0x92, 0xBB, // U+1F4BB PERSONAL COMPUTER

            0x62
        };


        // --------------------------------------------------------------------
        // Longer emoji ZWJ sequence
        //
        //   a [MAN ZWJ WOMAN ZWJ GIRL] b
        // --------------------------------------------------------------------

        static const uint8_t familyEmoji[] =
        {
            0x61,

            0xF0, 0x9F, 0x91, 0xA8, // U+1F468 MAN
            0xE2, 0x80, 0x8D,       // U+200D
            0xF0, 0x9F, 0x91, 0xA9, // U+1F469 WOMAN
            0xE2, 0x80, 0x8D,       // U+200D
            0xF0, 0x9F, 0x91, 0xA7, // U+1F467 GIRL

            0x62
        };


        // --------------------------------------------------------------------
        // Variation selector
        //
        //   x [HEAVY BLACK HEART VS16] y
        //
        // U+2764 U+FE0F
        // --------------------------------------------------------------------

        static const uint8_t variationSelector[] =
        {
            0x78,

            0xE2, 0x9D, 0xA4,       // U+2764 HEAVY BLACK HEART
            0xEF, 0xB8, 0x8F,       // U+FE0F VARIATION SELECTOR-16

            0x79
        };


        // --------------------------------------------------------------------
        // Arabic with ZWJ
        //
        // This is particularly interesting because ZWJ is internal to text
        // that otherwise has strong RTL bidi behavior.
        //
        //   a ALEF ZWJ LAM b
        // --------------------------------------------------------------------

        static const uint8_t arabicZwj[] =
        {
            0x61,

            0xD8, 0xA7,             // U+0627 ARABIC LETTER ALEF
            0xE2, 0x80, 0x8D,       // U+200D ZERO WIDTH JOINER
            0xD9, 0x84,             // U+0644 ARABIC LETTER LAM

            0x62
        };


        // --------------------------------------------------------------------
        // RTL paragraph containing an emoji ZWJ cluster
        //
        // Paragraph begins with Arabic, so paragraph level should naturally
        // become RTL under Auto direction.
        //
        //   ALEF SPACE [WOMAN ZWJ LAPTOP] SPACE LAM
        // --------------------------------------------------------------------

        static const uint8_t rtlEmojiZwj[] =
        {
            0xD8, 0xA7,             // U+0627 ALEF
            0x20,

            0xF0, 0x9F, 0x91, 0xA9, // U+1F469 WOMAN
            0xE2, 0x80, 0x8D,       // U+200D
            0xF0, 0x9F, 0x92, 0xBB, // U+1F4BB PERSONAL COMPUTER

            0x20,
            0xD9, 0x84              // U+0644 LAM
        };


        // --------------------------------------------------------------------
        // Right-to-left isolate
        //
        //   a RLI ALEF LAM PDI b
        //
        // The isolate controls should normally form their own grapheme
        // boundaries, but this lets us inspect their final stored levels.
        // --------------------------------------------------------------------

        static const uint8_t rtlIsolate[] =
        {
            0x61,

            0xE2, 0x81, 0xA7,       // U+2067 RIGHT-TO-LEFT ISOLATE
            0xD8, 0xA7,             // U+0627 ALEF
            0xD9, 0x84,             // U+0644 LAM
            0xE2, 0x81, 0xA9,       // U+2069 POP DIRECTIONAL ISOLATE

            0x62
        };


        // --------------------------------------------------------------------
        // Explicit directional formatting
        //
        //   a RLO 1 2 PDF b
        //
        // U+202E RIGHT-TO-LEFT OVERRIDE
        // U+202C POP DIRECTIONAL FORMATTING
        // --------------------------------------------------------------------

        static const uint8_t rtlOverride[] =
        {
            0x61,

            0xE2, 0x80, 0xAE,       // U+202E RLO
            0x31,
            0x32,
            0xE2, 0x80, 0xAC,       // U+202C PDF

            0x62
        };


        struct ProbeCase
        {
            const char* description;
            const uint8_t* bytes;
            uint32_t byteCount;
        };


        const ProbeCase cases[] =
        {
            {
                "Arabic base + combining marks",
                arabicMarks,
                static_cast<uint32_t>(sizeof(arabicMarks))
            },

            {
                "Hebrew base + combining mark",
                hebrewMark,
                static_cast<uint32_t>(sizeof(hebrewMark))
            },

            {
                "Emoji ZWJ: woman technologist",
                womanTechnologist,
                static_cast<uint32_t>(sizeof(womanTechnologist))
            },

            {
                "Emoji ZWJ: family",
                familyEmoji,
                static_cast<uint32_t>(sizeof(familyEmoji))
            },

            {
                "Variation selector",
                variationSelector,
                static_cast<uint32_t>(sizeof(variationSelector))
            },

            {
                "Arabic with ZWJ",
                arabicZwj,
                static_cast<uint32_t>(sizeof(arabicZwj))
            },

            {
                "RTL paragraph with emoji ZWJ",
                rtlEmojiZwj,
                static_cast<uint32_t>(sizeof(rtlEmojiZwj))
            },

            {
                "RTL isolate",
                rtlIsolate,
                static_cast<uint32_t>(sizeof(rtlIsolate))
            },

            {
                "RTL override",
                rtlOverride,
                static_cast<uint32_t>(sizeof(rtlOverride))
            }
        };


        uint32_t caseCount = 0;
        uint32_t paragraphCount = 0;
        uint32_t clusterCount = 0;
        uint32_t multiScalarClusterCount = 0;
        uint32_t mixedLevelClusterCount = 0;
        uint32_t itemizerAcceptedCount = 0;
        uint32_t itemizerRejectedCount = 0;


        // ====================================================================
        // Execute probes
        // ====================================================================

        for (const ProbeCase& probe : cases)
        {
            ++caseCount;


            std::printf(
                "\n"
                "============================================================\n"
                "%s\n"
                "============================================================\n",
                probe.description);


            const ByteSpan source(
                probe.bytes,
                probe.byteCount);


            Utf8ScalarStream utf8(
                source);


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


            BidiParagraphView paragraph{};

            uint32_t caseParagraphCount = 0;


            while (bidi(paragraph))
            {
                ++paragraphCount;
                ++caseParagraphCount;


                std::printf(
                    "Paragraph %u\n"
                    "  Scalars:         %u\n"
                    "  Clusters:        %u\n"
                    "  Paragraph level: %u\n"
                    "  Direction:       %s\n",
                    caseParagraphCount - 1,
                    paragraph.scalarCount,
                    paragraph.clusterCount,
                    static_cast<unsigned>(paragraph.paragraphLevel),
                    paragraph.rightToLeft() ? "RTL" : "LTR");


                // ============================================================
                // Inspect every grapheme cluster.
                // ============================================================

                for (uint32_t clusterIndex = 0;
                    clusterIndex < paragraph.clusterCount;
                    ++clusterIndex)
                {
                    ++clusterCount;


                    const ShapingCluster& cluster =
                        paragraph.clusters[clusterIndex];


                    const ScriptClusterInfo& scriptInfo =
                        paragraph.scripts[clusterIndex];


                    if (cluster.scalarCount > 1)
                        ++multiScalarClusterCount;


                    InternedKey scriptTag =
                        database.scriptISO15924(scriptInfo.script);


                    const UnicodeBidiLevel firstLevel =
                        paragraph.levels[cluster.scalarOffset];


                    bool mixedLevel = false;


                    for (uint32_t scalarIndex = 1;
                        scalarIndex < cluster.scalarCount;
                        ++scalarIndex)
                    {
                        const UnicodeBidiLevel level =
                            paragraph.levels[
                                cluster.scalarOffset +
                                    scalarIndex];


                        if (level != firstLevel)
                            mixedLevel = true;
                    }


                    if (mixedLevel)
                        ++mixedLevelClusterCount;


                    std::printf(
                        "  Cluster %u\n"
                        "    Script:      %s\n"
                        "    Scalars:     %u\n"
                        "    Offset:      %u\n"
                        "    Source:      [%u,%u)\n"
                        "    Mixed level: %s\n"
                        "    Contents:\n",
                        clusterIndex,
                        scriptTag ? scriptTag : "????",
                        cluster.scalarCount,
                        cluster.scalarOffset,
                        cluster.source.begin,
                        cluster.source.end,
                        mixedLevel ? "YES" : "NO");


                    for (uint32_t scalarIndex = 0;
                        scalarIndex < cluster.scalarCount;
                        ++scalarIndex)
                    {
                        const uint32_t paragraphScalarIndex =
                            cluster.scalarOffset +
                            scalarIndex;


                        const UnicodeScalar& scalar =
                            paragraph.scalars[
                                paragraphScalarIndex];


                        const UnicodeBidiLevel level =
                            paragraph.levels[
                                paragraphScalarIndex];


                        std::printf(
                            "      [%u] U+%04X  level=%u\n",
                            scalarIndex,
                            scalar.value,
                            static_cast<unsigned>(level));
                    }
                }


                // ============================================================
                // Ask the current shaping itemizer whether it accepts exactly
                // this paragraph.
                //
                // Rejection is observational here, not a test failure.
                // ============================================================

                ShapingRunItemizer itemizer(
                    paragraph,
                    database);


                if (itemizer.failed())
                {
                    ++itemizerRejectedCount;

                    std::printf(
                        "  Itemizer: REJECTED\n");
                }
                else
                {
                    uint32_t runCount = 0;
                    ShapingRunView run{};


                    while (itemizer(run))
                        ++runCount;


                    if (itemizer.ended())
                    {
                        ++itemizerAcceptedCount;

                        std::printf(
                            "  Itemizer: ACCEPTED (%u runs)\n",
                            runCount);
                    }
                    else
                    {
                        ++itemizerRejectedCount;

                        std::printf(
                            "  Itemizer: FAILED DURING ITEMIZATION\n");
                    }
                }
            }


            if (!bidi.ended())
            {
                std::printf(
                    "Unicode grapheme bidi levels: FAIL\n"
                    "  Case: %s\n"
                    "  Bidi stream did not reach End\n",
                    probe.description);

                return false;
            }


            if (caseParagraphCount == 0)
            {
                std::printf(
                    "Unicode grapheme bidi levels: FAIL\n"
                    "  Case: %s\n"
                    "  No paragraph emitted\n",
                    probe.description);

                return false;
            }
        }


        // ====================================================================
        // Summary
        //
        // A non-zero mixedLevelClusterCount is NOT considered failure here.
        // Discovering whether it is zero is the entire purpose of this probe.
        // ====================================================================

        std::printf(
            "\n"
            "Unicode grapheme bidi-level probe: PASS\n"
            "  Cases:                  %u\n"
            "  Paragraphs:             %u\n"
            "  Grapheme clusters:      %u\n"
            "  Multi-scalar clusters:  %u\n"
            "  Mixed-level clusters:   %u\n"
            "  Itemizer accepted:      %u\n"
            "  Itemizer rejected:      %u\n",
            caseCount,
            paragraphCount,
            clusterCount,
            multiScalarClusterCount,
            mixedLevelClusterCount,
            itemizerAcceptedCount,
            itemizerRejectedCount);


        return true;
    }


    static bool testUnicodeGraphemeBidiLevels(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;


        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Unicode grapheme bidi levels: FAIL\n"
                "  Unable to read database: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testUnicodeGraphemeBidiLevels(
            databaseData);
    }

} // namespace waavs