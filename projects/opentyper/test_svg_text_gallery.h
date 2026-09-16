// test_svg_text_gallery.h
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
    struct SVGTextGalleryLine
    {
        const char* text{ nullptr };
        const char* name{ nullptr };
    };


    static bool testSVGTextGallery(const ByteSpan& databaseData, const ByteSpan& fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf("SVG Latin text gallery: FAIL: %s\n", message);
                return false;
            };


        // ================================================================
        // Gallery text
        //
        // Keep the source itself ASCII-only. Non-ASCII UTF-8 is expressed
        // explicitly as byte escapes.
        // ================================================================

        static constexpr SVGTextGalleryLine kLines[] =
        {
            {
                "The quick brown fox jumps over the lazy dog.",
                "pangram"
            },

            {
                "AVATAR WA To Yo Ta Te VA Vo AWAY WATER",
                "kerning"
            },

            {
                "office affine efficient difficult fluff waffle final",
                "ligatures"
            },

            {
                "Hamburgefontsiv AVATAR Typography Waavs",
                "type specimen"
            },

            {
                "0123456789   $123.45   50%   (2026)   + - = /",
                "numbers"
            },

            {
                "Hello, world!  \"Quoted text\"; commas, periods... and more.",
                "punctuation"
            },

            {
                "Café  déjà vu  façade  naïve  coöperate",
                "western accents"
            },

            {
                "Ångström  smörgåsbord  piñata  São Paulo",
                "extended Latin"
            },

            {
                "Dvořák  Łódź  Žižkov  Reykjavík",
                "central European"
            },

            {
                "Straße  français  español  português",
                "European languages"
            },

            {
                "Æsir  Øresund  Œuvre  Þingvellir",
                "Latin extensions"
            },

            {
                "Cafe\u0301   A\u030A   n\u0303   o\u0308   c\u0327",
                "decomposed NFC"
            },

            {
                "minimum maximum momentum rhythm typography",
                "repeated shapes"
            },

            {
                "A beautiful text pipeline should make this line look boring.",
                "final sentence"
            }
        };


        // ================================================================
        // Renderer
        // ================================================================

        constexpr double fontSize = 54.0;
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
            const SVGTextGalleryLine& line = kLines[i];

            if (!drawer.drawText(line.text, leftMargin, baseline))
            {
                std::printf(
                    "SVG Latin text gallery: FAIL: drawText failed\n"
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
                "  %2zu  %-20s  scalars=%3zu  graphemes=%3zu  runs=%2zu  glyphs=%3zu  width=%8.2f\n",
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
            "SVG Latin text gallery: PASS\n"
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

    static bool testSVGTextGallery(const char* databaseFilename, const char* fontFilename,
        const char* svgFilename = "test_svg_text_gallery.svg")
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "SVG Latin text gallery: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "SVG Latin text gallery: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        const ByteSpan databaseData(databaseBytes.data(), databaseBytes.size());
        const ByteSpan fontData(fontBytes.data(), fontBytes.size());

        std::string svg;

        if (!testSVGTextGallery(databaseData, fontData, &svg))
            return false;

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG Latin text gallery: FAIL: unable to create SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        output.write(svg.data(), static_cast<std::streamsize>(svg.size()));

        if (!output)
        {
            std::printf(
                "SVG Latin text gallery: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf("  SVG file:              %s\n", svgFilename);
        return true;
    }

} // namespace waavs