// test_svg_devanagari_gallery.h
#pragma once

#include "test_core.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "svg_text_drawer.h"

namespace waavs
{
    struct SVGDevanagariGalleryLine
    {
        const char* text{ nullptr };
        const char* name{ nullptr };
    };


    static bool testSVGDevanagariGallery(
        const ByteSpan& databaseData,
        const ByteSpan& fontData,
        std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf("SVG Devanagari text gallery: FAIL: %s\n", message);
                return false;
            };


        // ================================================================
        // Gallery text
        //
        // Source is UTF-8 and the project is compiled as C++20.
        //
        // The lines intentionally exercise:
        //
        //   independent vowels
        //   consonants
        //   dependent vowel signs
        //   explicit halant
        //   conjuncts / half forms
        //   Reph
        //   pre-base matra reordering
        //   ZWJ / ZWNJ behavior
        //   marks and modifiers
        //   Vedic signs
        //   real Hindi / Sanskrit text
        // ================================================================

        static constexpr SVGDevanagariGalleryLine kLines[] =
        {
            {
                "अ आ इ ई उ ऊ ऋ ए ऐ ओ औ अं अः",
                "independent vowels"
            },

            {
                "क ख ग घ ङ   च छ ज झ ञ   ट ठ ड ढ ण",
                "consonants 1"
            },

            {
                "त थ द ध न   प फ ब भ म   य र ल व श ष स ह",
                "consonants 2"
            },

            {
                "क का कि की कु कू कृ के कै को कौ कं कः",
                "matras"
            },

            {
                "क् ख् ग् त् न् प् म् र्",
                "explicit halant"
            },

            {
                "क्ष त्र ज्ञ श्र स्त स्थ द्व द्य न्त म्प",
                "common conjuncts"
            },

            {
                "क्क क्त ग्द त्म न्द न्त प्र ब्र म्र",
                "half forms"
            },

            {
                "र्क र्ग र्त र्म र्व र्कि र्क्ष",
                "reph"
            },

            {
                "कि गि ति मि क्गि क्षि र्कि र्क्षि",
                "pre-base matra"
            },

            {
                "क्‍ग   क्‌ग   र्‍क   र्‌क",
                "ZWJ and ZWNJ"
            },

            {
                "कँ कं कः क़ ड़ ढ़ फ़",
                "marks and nukta"
            },

            {
                "भारत   हिन्दी   देवनागरी   संस्कृत   विद्यालय",
                "Hindi words"
            },

            {
                "कर्म धर्म प्रकाश श्रेणी प्रार्थना ब्रह्म",
                "Sanskrit forms"
            },

            
            {
                "० १ २ ३ ४ ५ ६ ७ ८ ९   0 1 2 3 4 5 6 7 8 9",
                "Devanagari numerals"
            },

            {
                "अ॑   अ॒   अ᳐   अ᳒   अ᳚",
                "Vedic accents"
            },

            {
                "यह एक देवनागरी परीक्षण है।",
                "sentence 1"
            },

            {
                "कृपया सही आकार और संयोजन देखें।",
                "sentence 2"
            },
            
            {
                "श्रेणी संयोजक ज्ञानी प्रार्थना संस्कृत कृपया ब्रह्म",
                "stress line"
            }
            
        };


        // ================================================================
        // Renderer
        // ================================================================

        constexpr double fontSize = 52.0;
        constexpr double leftMargin = 48.0;
        constexpr double topMargin = 72.0;
        constexpr double lineAdvance = 76.0;
        constexpr double rightMargin = 48.0;
        constexpr double bottomMargin = 48.0;

        SVGTextDrawer drawer;

        if (!drawer.load(databaseData, fontData, fontSize))
            return fail("unable to initialize SVGTextDrawer");


        // ================================================================
        // Draw
        // ================================================================

        double baseline = topMargin;
        double maxPenX = leftMargin;

        size_t totalGlyphs = 0;
        size_t totalRuns = 0;
        size_t totalScalars = 0;
        size_t totalGraphemes = 0;

        for (size_t i = 0; i < sizeof(kLines) / sizeof(kLines[0]); ++i)
        {
            const SVGDevanagariGalleryLine& line = kLines[i];

            if (!drawer.drawText(line.text, leftMargin, baseline))
            {
                std::printf(
                    "SVG Devanagari text gallery: FAIL: drawText failed\n"
                    "  Line: %zu\n"
                    "  Test: %s\n",
                    i,
                    line.name);

                return false;
            }

            const SVGTextDrawStats& stats = drawer.lastDrawStats();

            if (stats.paragraphCount != 1)
                return fail("gallery line did not produce exactly one paragraph");

            if (stats.shapedGlyphCount == 0)
                return fail("gallery line produced no glyphs");

            if (!(stats.finalPenX > leftMargin))
                return fail("gallery line did not advance");

            maxPenX = std::max(maxPenX, stats.finalPenX);

            totalGlyphs += stats.shapedGlyphCount;
            totalRuns += stats.shapingRunCount;
            totalScalars += stats.normalizedScalars;
            totalGraphemes += stats.graphemes;

            std::printf(
                "  %2zu  %-22s  scalars=%3zu  graphemes=%3zu  runs=%2zu  glyphs=%3zu  width=%8.2f\n",
                i + 1,
                line.name,
                stats.normalizedScalars,
                stats.graphemes,
                stats.shapingRunCount,
                stats.shapedGlyphCount,
                stats.finalPenX - leftMargin);

            baseline += lineAdvance;
        }


        // ================================================================
        // Produce SVG
        // ================================================================

        const double width = maxPenX + rightMargin;
        const double height = baseline - lineAdvance + bottomMargin;

        const std::string svg = drawer.document(
            0.0f,
            0.0f,
            static_cast<float>(width),
            static_cast<float>(height));

        if (svg.empty())
            return fail("generated SVG is empty");

        if (svg.find("<svg") == std::string::npos ||
            svg.find("<path") == std::string::npos ||
            svg.find("<use") == std::string::npos)
        {
            return fail("generated SVG is missing expected SVG content");
        }

        if (svgOutput)
            *svgOutput = svg;


        // ================================================================
        // Summary
        // ================================================================

        const char* faceName = drawer.fontName();

        if (!faceName)
            faceName = "(unnamed)";

        std::printf(
            "\n"
            "SVG Devanagari text gallery: PASS\n"
            "  Face:                  %s\n"
            "  Font size:             %.2f\n"
            "  Gallery lines:         %zu\n"
            "  Normalized scalars:    %zu\n"
            "  Graphemes:             %zu\n"
            "  Shaping runs:          %zu\n"
            "  Shaped glyphs:         %zu\n"
            "  Glyph definitions:     %zu\n"
            "  Canvas width:          %.2f\n"
            "  Canvas height:         %.2f\n"
            "  SVG bytes:             %zu\n",
            faceName,
            drawer.fontSize(),
            sizeof(kLines) / sizeof(kLines[0]),
            totalScalars,
            totalGraphemes,
            totalRuns,
            totalGlyphs,
            drawer.glyphDefinitionCount(),
            width,
            height,
            svg.size());

        return true;
    }


    // ====================================================================
    // Filename convenience overload
    // ====================================================================

    static bool testSVGDevanagariGallery(
        const char* databaseFilename,
        const char* fontFilename,
        const char* svgFilename = "test_svg_devanagari_gallery.svg")
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "SVG Devanagari text gallery: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "SVG Devanagari text gallery: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        const ByteSpan databaseData(databaseBytes.data(), databaseBytes.size());
        const ByteSpan fontData(fontBytes.data(), fontBytes.size());

        std::string svg;

        if (!testSVGDevanagariGallery(databaseData, fontData, &svg))
            return false;

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG Devanagari text gallery: FAIL: unable to create SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        output.write(svg.data(), static_cast<std::streamsize>(svg.size()));

        if (!output)
        {
            std::printf(
                "SVG Devanagari text gallery: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf("  SVG file:              %s\n", svgFilename);
        return true;
    }

} // namespace waavs
