// test_svg_text_drawer.h
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
    // testSVGTextDrawer
    //
    // Full real text pipeline:
    //
    //     UTF-8
    //       -> NFC
    //       -> graphemes
    //       -> Script
    //       -> bidi
    //       -> shaping runs
    //       -> font runs
    //       -> cmap / GSUB / hmtx / GPOS
    //       -> positioning
    //       -> SVG
    //
    // Draw 1 exercises normal Latin shaping and kerning.
    //
    // Draw 2 feeds decomposed UTF-8:
    //
    //     C a f e U+0301
    //
    // NFC should reduce the five input Unicode scalars to four normalized
    // scalars before the rest of the pipeline sees them.
    // ====================================================================

    static bool testSVGTextDrawer(const ByteSpan& databaseData,
        const ByteSpan& fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf("SVG text drawer: FAIL: %s\n", message);
                return false;
            };

        constexpr double fontSize = 96.0;

        SVGTextDrawer drawer;

        if (!drawer.load(databaseData, fontData, fontSize))
            return fail("unable to initialize SVGTextDrawer");


        // ================================================================
        // Normal Latin shaping / GPOS
        // ================================================================

        static constexpr char kLatinText[] =
            "AVATAR To Waavs is fine";

        if (!drawer.drawText(kLatinText, 40.0, 125.0))
            return fail("unable to draw Latin text");

        const SVGTextDrawStats latin = drawer.lastDrawStats();

        if (latin.utf8Bytes != 23)
            return fail("unexpected Latin UTF-8 byte count");

        if (latin.normalizedScalars != 23)
            return fail("unexpected Latin normalized scalar count");

        if (latin.paragraphCount != 1)
            return fail("expected one Latin paragraph");

        if (latin.shapingRunCount != 1)
            return fail("expected one Latin shaping run");

        if (latin.fontRunCount != 1)
            return fail("expected one Latin font run");

        if (latin.shapedGlyphCount == 0)
            return fail("Latin shaping produced no glyphs");

        if (!(latin.finalPenX > latin.originX))
            return fail("Latin text did not advance the pen");

        const size_t definitionsAfterLatin =
            drawer.glyphDefinitionCount();

        if (definitionsAfterLatin == 0)
            return fail("Latin draw produced no glyph definitions");


        // ================================================================
        // Real UTF-8 + NFC probe
        //
        // Source:
        //
        //     C a f e U+0301
        //
        // UTF-8 bytes:
        //
        //     43 61 66 65 CC 81
        //
        // NFC:
        //
        //     C a f U+00E9
        // ================================================================

        static constexpr char kNfcText[] =
            "Cafe" "\xCC\x81";

        if (!drawer.drawText(kNfcText, 40.0, 285.0))
            return fail("unable to draw decomposed NFC probe");

        const SVGTextDrawStats nfc = drawer.lastDrawStats();

        if (nfc.utf8Bytes != 6)
            return fail("unexpected NFC probe UTF-8 byte count");

        if (nfc.normalizedScalars != 4)
            return fail("NFC did not compose decomposed e + acute");

        if (nfc.graphemes != 4)
            return fail("unexpected NFC probe grapheme count");

        if (nfc.paragraphCount != 1)
            return fail("expected one NFC probe paragraph");

        if (nfc.shapingRunCount != 1)
            return fail("expected one NFC probe shaping run");

        if (nfc.fontRunCount != 1)
            return fail("expected one NFC probe font run");

        if (nfc.shapedGlyphCount == 0)
            return fail("NFC probe shaping produced no glyphs");

        if (!(nfc.finalPenX > nfc.originX))
            return fail("NFC probe did not advance the pen");


        // ================================================================
        // SVG document
        // ================================================================

        const std::string svg =
            drawer.document(
                0.0f,
                0.0f,
                1400.0f,
                350.0f);

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
        // Diagnostics
        // ================================================================

        const char* faceName = drawer.fontName();

        if (!faceName)
            faceName = "(unnamed)";

        std::printf(
            "SVG text drawer: PASS\n"
            "  Face:                  %s\n"
            "  Font size:             %.2f\n"
            "  Latin UTF-8 bytes:     %zu\n"
            "  Latin scalars:         %zu\n"
            "  Latin graphemes:       %zu\n"
            "  Latin shaping runs:    %zu\n"
            "  Latin font runs:       %zu\n"
            "  Latin glyphs:          %zu\n"
            "  Latin final pen:       %.4f\n"
            "  NFC UTF-8 bytes:       %zu\n"
            "  NFC scalars:           %zu\n"
            "  NFC graphemes:         %zu\n"
            "  NFC glyphs:            %zu\n"
            "  NFC final pen:         %.4f\n"
            "  Glyph definitions:     %zu\n"
            "  SVG bytes:             %zu\n",
            faceName,
            drawer.fontSize(),
            latin.utf8Bytes,
            latin.normalizedScalars,
            latin.graphemes,
            latin.shapingRunCount,
            latin.fontRunCount,
            latin.shapedGlyphCount,
            latin.finalPenX,
            nfc.utf8Bytes,
            nfc.normalizedScalars,
            nfc.graphemes,
            nfc.shapedGlyphCount,
            nfc.finalPenX,
            drawer.glyphDefinitionCount(),
            svg.size());

        return true;
    }


    // ====================================================================
    // Filename convenience overload
    // ====================================================================

    static bool testSVGTextDrawer(const char* databaseFilename,
        const char* fontFilename,
        const char* svgFilename = "test_svg_text_drawer.svg")
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "SVG text drawer: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "SVG text drawer: FAIL: unable to read font\n"
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

        if (!testSVGTextDrawer(databaseData, fontData, &svg))
            return false;

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG text drawer: FAIL: unable to create SVG\n"
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
                "SVG text drawer: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf("  SVG file:              %s\n", svgFilename);
        return true;
    }

} // namespace waavs