// test_svg_ligature_gallery.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "svg_text_drawer.h"

namespace waavs
{
    // ====================================================================
    // writeSVGLigatureGalleryFile
    // ====================================================================

    static bool writeSVGLigatureGalleryFile(const char* filename, const std::string& svg)
    {
        if (!filename || !*filename || svg.empty())
            return false;

        std::ofstream output(filename, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!output)
            return false;

        output.write(svg.data(), static_cast<std::streamsize>(svg.size()));

        if (!output)
            return false;

        output.close();
        return bool(output);
    }


    // ====================================================================
    // testSVGLigatureGallery
    //
    // Visual layout:
    //
    //     Standard Latin Ligatures
    //
    //     components          separated          ligature
    //
    //     f + i               f i                fi
    //     f + l               f l                fl
    //     f + f + i           f f i              ffi
    //     f + f + l           f f l              ffl
    //
    //     Ligatures in words
    //
    //     office affinity efficient difficult
    //     waffle offline shuffle affluent
    //
    // The compact ligature examples are also structurally checked:
    //
    //     fi   2 scalars -> 1 glyph
    //     fl   2 scalars -> 1 glyph
    //     ffi  3 scalars -> 1 glyph
    //     ffl  3 scalars -> 1 glyph
    //
    // These are ordinary ASCII input sequences. No Unicode compatibility
    // ligature characters such as U+FB01 are used.
    // ====================================================================

    static bool testSVGLigatureGallery(const ByteSpan& databaseData,
        const ByteSpan& fontData, const char* outputFilename)
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "SVG ligature gallery: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        if (databaseData.empty())
            return fail("empty Unicode database");

        if (fontData.empty())
            return fail("empty font data");

        if (!outputFilename || !*outputFilename)
            return fail("invalid output filename");


        // ================================================================
        // Load complete text pipeline.
        // ================================================================

        constexpr double fontSize = 58.0;

        SVGTextDrawer drawer;

        if (!drawer.load(databaseData, fontData, fontSize))
            return fail("unable to load SVG text drawer");

        if (!drawer)
            return fail("SVG text drawer is invalid");


        // ================================================================
        // Small drawing helpers.
        // ================================================================

        auto draw =
            [&](const char* text, double x, double y) -> bool
            {
                if (!drawer.drawText(text, x, y))
                {
                    std::printf(
                        "SVG ligature gallery: FAIL\n"
                        "  drawText failed\n"
                        "  Text: %s\n",
                        text ? text : "(null)");

                    return false;
                }

                return true;
            };


        size_t ligatureCases = 0;
        size_t ligaturePassed = 0;

        auto drawLigature =
            [&](const char* text, double x, double y,
                size_t expectedScalars, const char* description) -> bool
            {
                ++ligatureCases;

                if (!drawer.drawText(text, x, y))
                {
                    std::printf(
                        "SVG ligature gallery: FAIL\n"
                        "  Ligature draw failed: %s\n",
                        description);

                    return false;
                }

                const SVGTextDrawStats& stats = drawer.lastDrawStats();

                if (stats.normalizedScalars != expectedScalars)
                {
                    std::printf(
                        "SVG ligature gallery: FAIL\n"
                        "  Ligature: %s\n"
                        "  Expected scalars: %zu\n"
                        "  Actual scalars:   %zu\n",
                        description,
                        expectedScalars,
                        stats.normalizedScalars);

                    return false;
                }

                if (stats.shapedGlyphCount != 1)
                {
                    std::printf(
                        "SVG ligature gallery: FAIL\n"
                        "  Ligature: %s\n"
                        "  Input scalars: %zu\n"
                        "  Shaped glyphs: %zu\n"
                        "  Expected:      1\n",
                        description,
                        stats.normalizedScalars,
                        stats.shapedGlyphCount);

                    return false;
                }

                if (stats.shapingRunCount != 1 || stats.fontRunCount != 1)
                {
                    std::printf(
                        "SVG ligature gallery: FAIL\n"
                        "  Ligature: %s\n"
                        "  Shaping runs: %zu\n"
                        "  Font runs:    %zu\n",
                        description,
                        stats.shapingRunCount,
                        stats.fontRunCount);

                    return false;
                }

                ++ligaturePassed;
                return true;
            };


        // ================================================================
        // Gallery geometry.
        // ================================================================

        constexpr float documentWidth = 1500.0f;
        constexpr float documentHeight = 900.0f;

        constexpr double labelX = 60.0;
        constexpr double separatedX = 430.0;
        constexpr double ligatureX = 820.0;

        constexpr double titleY = 90.0;
        constexpr double headerY = 180.0;

        constexpr double row1Y = 270.0;
        constexpr double row2Y = 350.0;
        constexpr double row3Y = 430.0;
        constexpr double row4Y = 510.0;

        constexpr double wordsTitleY = 620.0;
        constexpr double words1Y = 710.0;
        constexpr double words2Y = 790.0;


        // ================================================================
        // Title and headings.
        // ================================================================

        if (!draw("Standard Latin Ligatures", labelX, titleY))
            return false;

        if (!draw("components", labelX, headerY))
            return false;

        if (!draw("separated", separatedX, headerY))
            return false;

        if (!draw("ligature", ligatureX, headerY))
            return false;


        // ================================================================
        // fi
        // ================================================================

        if (!draw("f + i", labelX, row1Y))
            return false;

        if (!draw("f i", separatedX, row1Y))
            return false;

        if (!drawLigature("fi", ligatureX, row1Y, 2, "fi"))
            return false;


        // ================================================================
        // fl
        // ================================================================

        if (!draw("f + l", labelX, row2Y))
            return false;

        if (!draw("f l", separatedX, row2Y))
            return false;

        if (!drawLigature("fl", ligatureX, row2Y, 2, "fl"))
            return false;


        // ================================================================
        // ffi
        // ================================================================

        if (!draw("f + f + i", labelX, row3Y))
            return false;

        if (!draw("f f i", separatedX, row3Y))
            return false;

        if (!drawLigature("ffi", ligatureX, row3Y, 3, "ffi"))
            return false;


        // ================================================================
        // ffl
        // ================================================================

        if (!draw("f + f + l", labelX, row4Y))
            return false;

        if (!draw("f f l", separatedX, row4Y))
            return false;

        if (!drawLigature("ffl", ligatureX, row4Y, 3, "ffl"))
            return false;


        // ================================================================
        // Ligatures in ordinary words.
        //
        // These go through exactly the same normal Latin shaping policy as
        // ordinary SVG text.
        // ================================================================

        if (!draw("Ligatures in words", labelX, wordsTitleY))
            return false;

        if (!draw("office affinity efficient difficult", labelX, words1Y))
            return false;

        if (!draw("waffle offline shuffle affluent", labelX, words2Y))
            return false;


        // ================================================================
        // Produce SVG document.
        // ================================================================

        const std::string svg = drawer.document(
            0.0f,
            0.0f,
            documentWidth,
            documentHeight);

        if (svg.empty())
            return fail("SVG document generation failed");

        if (!writeSVGLigatureGalleryFile(outputFilename, svg))
            return fail("unable to write SVG output file");


        // ================================================================
        // Summary.
        // ================================================================

        std::printf(
            "SVG ligature gallery: PASS\n"
            "  Font:             %s\n"
            "  Ligature cases:   %zu\n"
            "  Ligatures passed: %zu\n"
            "  fi:               2 -> 1\n"
            "  fl:               2 -> 1\n"
            "  ffi:              3 -> 1\n"
            "  ffl:              3 -> 1\n"
            "  Glyph definitions:%zu\n"
            "  Output:           %s\n",
            drawer.fontName() ? drawer.fontName() : "(unnamed)",
            ligatureCases,
            ligaturePassed,
            drawer.glyphDefinitionCount(),
            outputFilename);

        return ligaturePassed == ligatureCases;
    }


    // ====================================================================
    // Filename convenience overload.
    // ====================================================================

    static bool testSVGLigatureGallery(const char* databaseFilename,
        const char* fontFilename, const char* outputFilename)
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "SVG ligature gallery: FAIL\n"
                "  Unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "SVG ligature gallery: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return testSVGLigatureGallery(
            ByteSpan(databaseBytes.data(), databaseBytes.size()),
            ByteSpan(fontBytes.data(), fontBytes.size()),
            outputFilename);
    }

} // namespace waavs