// test_svg_bidi_gallery.h
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
    struct SVGBidiGalleryLine
    {
        const char* text{ nullptr };
        const char* name{ nullptr };
        size_t minimumShapingRuns{ 1 };
    };


    static bool testSVGBidiGallery(const ByteSpan& databaseData,
        const ByteSpan& fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf("SVG bidi gallery: FAIL: %s\n", message);
                return false;
            };


        // ================================================================
        // Gallery text
        //
        // Source file must be saved as UTF-8.
        //
        // The actual gallery strings intentionally contain UTF-8 Hebrew.
        // Diagnostics and comments remain ASCII.
        // ================================================================

        static constexpr SVGBidiGalleryLine kLines[] =
        {
            {
                u8"AVATAR To Waavs is fine",
                "LTR reference",
                1
            },

            {
                u8"Hello שלום world",
                "basic mixed",
                2
            },

            {
                u8"ABC אבג DEF",
                "simple Hebrew",
                2
            },

            {
                u8"שלום עולם",
                "pure Hebrew",
                1
            },

            {
                u8"שלום 123 עולם",
                "RTL with numbers",
                2
            },

            {
                u8"ABC אבג 123 דהו XYZ",
                "nested numbers",
                3
            },

            {
                u8"English עברית English עברית",
                "multiple switches",
                3
            },

            {
                u8"2026 שלום 12345 עולם",
                "RTL paragraph numbers",
                2
            },

            {
                u8"מחיר 123.45 דולר",
                "decimal in RTL",
                2
            },

            {
                u8"one אחד two שני three שלוש",
                "many direction changes",
                3
            },

            {
                u8"שָׁלוֹם עולם",
                "Hebrew marks",
                1
            }
        };


        // ================================================================
        // Renderer
        // ================================================================

        constexpr double fontSize = 58.0;
        constexpr double leftMargin = 48.0;
        constexpr double topBaseline = 78.0;
        constexpr double lineAdvance = 82.0;
        constexpr double rightMargin = 48.0;
        constexpr double bottomMargin = 48.0;

        SVGTextDrawer drawer;

        if (!drawer.load(databaseData, fontData, fontSize))
            return fail("unable to initialize SVGTextDrawer");


        // ================================================================
        // Draw gallery
        // ================================================================

        double baseline = topBaseline;
        double maxPenX = leftMargin;

        size_t totalScalars = 0;
        size_t totalGraphemes = 0;
        size_t totalShapingRuns = 0;
        size_t totalFontRuns = 0;
        size_t totalGlyphs = 0;

        constexpr size_t lineCount =
            sizeof(kLines) / sizeof(kLines[0]);

        for (size_t i = 0; i < lineCount; ++i)
        {
            const SVGBidiGalleryLine& line = kLines[i];

            if (!drawer.drawText(line.text, leftMargin, baseline))
            {
                std::printf(
                    "SVG bidi gallery: FAIL: drawText failed\n"
                    "  Line: %zu\n"
                    "  Test: %s\n",
                    i + 1,
                    line.name);

                return false;
            }

            const SVGTextDrawStats stats =
                drawer.lastDrawStats();

            if (stats.paragraphCount != 1)
                return fail("gallery line did not produce exactly one paragraph");

            if (stats.shapingRunCount < line.minimumShapingRuns)
            {
                std::printf(
                    "SVG bidi gallery: FAIL: too few shaping runs\n"
                    "  Line:     %zu\n"
                    "  Test:     %s\n"
                    "  Minimum:  %zu\n"
                    "  Actual:   %zu\n",
                    i + 1,
                    line.name,
                    line.minimumShapingRuns,
                    stats.shapingRunCount);

                return false;
            }

            if (stats.fontRunCount == 0)
                return fail("gallery line produced no font runs");

            if (stats.shapedGlyphCount == 0)
                return fail("gallery line produced no glyphs");

            if (!(stats.finalPenX > stats.originX))
                return fail("gallery line produced no visual width");

            const double width =
                stats.finalPenX - stats.originX;

            maxPenX =
                std::max(maxPenX, stats.finalPenX);

            totalScalars += stats.normalizedScalars;
            totalGraphemes += stats.graphemes;
            totalShapingRuns += stats.shapingRunCount;
            totalFontRuns += stats.fontRunCount;
            totalGlyphs += stats.shapedGlyphCount;

            std::printf(
                "  %2zu  %-22s  scalars=%3zu  graphemes=%3zu"
                "  shaping=%2zu  fonts=%2zu  glyphs=%3zu  width=%8.2f\n",
                i + 1,
                line.name,
                stats.normalizedScalars,
                stats.graphemes,
                stats.shapingRunCount,
                stats.fontRunCount,
                stats.shapedGlyphCount,
                width);

            baseline += lineAdvance;
        }


        // ================================================================
        // SVG document
        // ================================================================

        const double width =
            maxPenX + rightMargin;

        const double height =
            topBaseline +
            static_cast<double>(lineCount - 1) * lineAdvance +
            bottomMargin;

        const std::string svg =
            drawer.document(
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
            return fail("generated SVG is missing expected content");
        }

        if (svgOutput)
            *svgOutput = svg;


        // ================================================================
        // Summary
        // ================================================================

        const char* faceName =
            drawer.fontName();

        if (!faceName)
            faceName = "(unnamed)";

        std::printf(
            "\n"
            "SVG bidi gallery: PASS\n"
            "  Face:                  %s\n"
            "  Font size:             %.2f\n"
            "  Gallery lines:         %zu\n"
            "  Normalized scalars:    %zu\n"
            "  Graphemes:             %zu\n"
            "  Shaping runs:          %zu\n"
            "  Font runs:             %zu\n"
            "  Shaped glyphs:         %zu\n"
            "  Glyph definitions:     %zu\n"
            "  Canvas width:          %.2f\n"
            "  Canvas height:         %.2f\n"
            "  SVG bytes:             %zu\n",
            faceName,
            drawer.fontSize(),
            lineCount,
            totalScalars,
            totalGraphemes,
            totalShapingRuns,
            totalFontRuns,
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

    static bool testSVGBidiGallery(const char* databaseFilename,
        const char* fontFilename,
        const char* svgFilename = "test_svg_bidi_gallery.svg")
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "SVG bidi gallery: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "SVG bidi gallery: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        const ByteSpan databaseData(
            databaseBytes.data(),
            databaseBytes.size());

        const ByteSpan fontData(
            fontBytes.data(),
            fontBytes.size());

        std::string svg;

        if (!testSVGBidiGallery(
            databaseData,
            fontData,
            &svg))
        {
            return false;
        }

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(
            svgFilename,
            std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG bidi gallery: FAIL: unable to create SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        output.write(
            svg.data(),
            static_cast<std::streamsize>(svg.size()));

        if (!output)
        {
            std::printf(
                "SVG bidi gallery: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf(
            "  SVG file:              %s\n",
            svgFilename);

        return true;
    }

} // namespace waavs