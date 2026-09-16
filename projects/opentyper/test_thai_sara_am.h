// test_thai_sara_am.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "opentype_horizontal_shaper.h"
#include "svg_text_drawer.h"

namespace waavs
{
    static constexpr char kThaiSaraAmText[] =
        u8"\u0E01\u0E48\u0E33";

    static constexpr char kThaiSaraAmReferenceText[] =
        u8"\u0E01\u0E4D\u0E48\u0E32";


    // ====================================================================
    // dumpThaiSaraAmBuffer
    // ====================================================================

    static void dumpThaiSaraAmBuffer(const char* name, const ShapedGlyphBuffer& buffer)
    {
        std::printf("  %s\n", name);

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const ShapedGlyph& glyph = buffer[i];

            std::printf(
                "    [%zu] gid=%u source=[%u,%u) adv=(%d,%d) off=(%d,%d)\n",
                i,
                static_cast<unsigned>(glyph.shaping.glyphId),
                static_cast<unsigned>(glyph.shaping.scalarOffset),
                static_cast<unsigned>(
                    glyph.shaping.scalarOffset +
                    glyph.shaping.scalarCount),
                glyph.placement.advanceX,
                glyph.placement.advanceY,
                glyph.placement.offsetX,
                glyph.placement.offsetY);
        }
    }


    // ====================================================================
    // thaiSaraAmPlacementEqual
    // ====================================================================

    static bool thaiSaraAmPlacementEqual(const GlyphPlacement& a,
        const GlyphPlacement& b) noexcept
    {
        return a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    // ====================================================================
    // thaiSaraAmVisualEquivalent
    //
    // Provenance is deliberately ignored here.
    //
    // The source run contains three Unicode scalars:
    //
    //     KO KAI + MAI EK + SARA AM
    //
    // The reference shaping run contains four:
    //
    //     KO KAI + NIKHAHIT + MAI EK + SARA AA
    //
    // A correct Thai preprocessor therefore changes the shaping sequence
    // while preserving provenance back to the original source.
    //
    // At this checkpoint we compare only the resulting glyph identities
    // and placements.
    // ====================================================================

    static bool thaiSaraAmVisualEquivalent(const ShapedGlyphBuffer& actual,
        const ShapedGlyphBuffer& reference) noexcept
    {
        if (actual.size() != reference.size())
            return false;

        for (size_t i = 0; i < actual.size(); ++i)
        {
            if (actual[i].shaping.glyphId != reference[i].shaping.glyphId)
                return false;

            if (!thaiSaraAmPlacementEqual(
                actual[i].placement,
                reference[i].placement))
            {
                return false;
            }
        }

        return true;
    }


    // ====================================================================
    // makeThaiSaraAmReferenceRun
    //
    // Construct the shaping-time sequence that SARA AM should produce:
    //
    //     input:
    //
    //         0E01 0E48 0E33
    //
    //     shaping reference:
    //
    //         0E01 0E4D 0E48 0E32
    //
    // NIKHAHIT and SARA AA both inherit the UnicodeScalar metadata from
    // the original SARA AM scalar. MAI EK retains its original metadata.
    //
    // This is only a test oracle. It is not the production implementation.
    // ====================================================================

    static bool makeThaiSaraAmReferenceRun(const FontRunView& source,
        UnicodeScalar(&scalars)[4], ShapingCluster& cluster,
        FontRunView& reference)
    {
        reference = {};

        if (!source.face ||
            !source.scalars ||
            source.scalarCount != 3 ||
            source.clusterCount != 1)
        {
            return false;
        }

        if (source.scalars[0].value != 0x0E01 ||
            source.scalars[1].value != 0x0E48 ||
            source.scalars[2].value != 0x0E33)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Preserve scalar metadata/provenance while changing only the
        // shaping-time code point sequence.
        // ------------------------------------------------------------

        scalars[0] = source.scalars[0];

        scalars[1] = source.scalars[2];
        scalars[1].value = 0x0E4D;

        scalars[2] = source.scalars[1];

        scalars[3] = source.scalars[2];
        scalars[3].value = 0x0E32;


        // ------------------------------------------------------------
        // Preserve the original cluster envelope.
        // ------------------------------------------------------------

        cluster = source.clusters[0];
        cluster.scalarOffset = 0;
        cluster.scalarCount = 4;


        // ------------------------------------------------------------
        // Reference font run.
        // ------------------------------------------------------------

        reference.scalars = scalars;
        reference.scalarCount = 4;

        reference.clusters = &cluster;
        reference.clusterCount = 1;

        reference.face = source.face;
        reference.script = source.script;
        reference.bidiLevel = source.bidiLevel;

        reference.normalizedBegin = source.normalizedBegin;
        reference.source = source.source;

        reference.completeCoverage = true;

        return true;
    }


    // ====================================================================
    // testThaiSaraAm
    //
    // Checkpoint:
    //
    //     UTF-8
    //       -> NFC
    //       -> grapheme segmentation
    //       -> Script
    //       -> bidi
    //       -> font selection
    //       -> current Thai shaping
    //
    // and compare that result against the explicit Thai SARA AM shaping
    // sequence:
    //
    //     U+0E01 U+0E4D U+0E48 U+0E32
    //
    // The SVG contains:
    //
    //     top:    original SARA AM source
    //     bottom: explicit decomposed/reordered reference
    //
    // The first run of this test may legitimately FAIL. That would isolate
    // the missing Thai preprocessing rule rather than an OpenType lookup or
    // SVG rendering failure.
    // ====================================================================

    static bool testThaiSaraAm(const ByteSpan& databaseData,
        const ByteSpan& fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "Thai SARA AM: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        if (databaseData.empty())
            return fail("empty Unicode database");

        if (fontData.empty())
            return fail("empty font data");


        // ================================================================
        // Unicode database and SVG facade.
        // ================================================================

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("unable to load Unicode database");

        constexpr double fontSize = 96.0;

        SVGTextDrawer drawer;

        if (!drawer.load(databaseData, fontData, fontSize))
            return fail("unable to initialize SVGTextDrawer");

        FontFace face = drawer.face();

        if (!face)
            return fail("SVGTextDrawer produced no FontFace");


        // ------------------------------------------------------------
        // The selected face must also contain the decomposition products.
        //
        // Font fallback currently sees U+0E33 in the original grapheme.
        // Once Thai preprocessing is introduced, U+0E4D and U+0E32 will
        // also be required by shaping.
        // ------------------------------------------------------------

        if (!face.hasGlyph(0x0E4D))
            return fail("font lacks U+0E4D NIKHAHIT");

        if (!face.hasGlyph(0x0E32))
            return fail("font lacks U+0E32 SARA AA");


        // ================================================================
        // Original source:
        //
        //     KO KAI + MAI EK + SARA AM
        // ================================================================

        const ByteSpan text(
            reinterpret_cast<const uint8_t*>(kThaiSaraAmText),
            sizeof(kThaiSaraAmText) - 1);

        Utf8ScalarStream utf8(text);
        UnicodeNfcStream<Utf8ScalarStream> nfc(utf8, database);
        GraphemePropertyStream<decltype(nfc)> properties(nfc, database);
        GraphemeStream<decltype(properties)> graphemes(properties);
        UnicodeScriptStream<decltype(graphemes)> scripts(graphemes, database);
        UnicodeBidiStream<decltype(scripts)> bidi(scripts, database);

        BidiParagraphView paragraph{};

        if (!bidi(paragraph))
            return fail("Unicode pipeline produced no paragraph");


        // ------------------------------------------------------------
        // NFC should not perform the Thai shaping-time SARA AM expansion.
        // ------------------------------------------------------------

        if (paragraph.scalarCount != 3)
            return fail("expected three normalized source scalars");

        if (paragraph.scalars[0].value != 0x0E01 ||
            paragraph.scalars[1].value != 0x0E48 ||
            paragraph.scalars[2].value != 0x0E33)
        {
            return fail("NFC unexpectedly changed the SARA AM source sequence");
        }

        if (paragraph.clusterCount != 1)
            return fail("expected one grapheme cluster");

        if (paragraph.paragraphLevel != 0)
            return fail("expected LTR paragraph");


        // ================================================================
        // Shaping run.
        // ================================================================

        ShapingRunItemizer shapingRuns(paragraph, database);

        if (shapingRuns.failed())
            return fail("shaping-run itemizer initialization failed");

        ShapingRunView shapingRun{};

        if (!shapingRuns(shapingRun))
            return fail("unable to obtain Thai shaping run");

        if (shapingRun.scalarCount != 3 ||
            shapingRun.clusterCount != 1)
        {
            return fail("unexpected Thai shaping-run extent");
        }

        if (shapingRun.bidiLevel != 0)
            return fail("Thai SARA AM run is not LTR");

        const InternedKey scriptName =
            database.scriptISO15924(shapingRun.script);

        if (!scriptName || std::strcmp(scriptName, "Thai") != 0)
            return fail("shaping run was not resolved as Thai");


        // ================================================================
        // Font run.
        // ================================================================

        FontRunItemizer fontRuns(shapingRun, drawer, database);

        if (fontRuns.failed())
            return fail("font-run itemizer initialization failed");

        FontRunView fontRun{};

        if (!fontRuns(fontRun))
            return fail("unable to obtain Thai font run");

        if (!fontRun.face || fontRun.face != face)
            return fail("unexpected Thai FontFace");

        if (!fontRun.completeCoverage)
            return fail("font does not completely cover the source cluster");

        if (fontRun.scalarCount != 3 ||
            fontRun.clusterCount != 1)
        {
            return fail("unexpected Thai font-run extent");
        }


        // ================================================================
        // Shape the source exactly as the production pipeline currently
        // presents it to OpenType.
        // ================================================================

        ShapedGlyphBuffer actual;

        if (!shapeOpenTypeHorizontalRun(
            fontRun,
            OTAG("thai"),
            0,
            actual,
            false,
            true))
        {
            return fail("unable to shape original SARA AM run");
        }


        // ================================================================
        // Build the expected Thai shaping-time sequence.
        //
        //     source:
        //
        //         0E01 0E48 0E33
        //
        //     reference:
        //
        //         0E01 0E4D 0E48 0E32
        //
        // ================================================================

        UnicodeScalar referenceScalars[4]{};
        ShapingCluster referenceCluster{};
        FontRunView referenceRun{};

        if (!makeThaiSaraAmReferenceRun(
            fontRun,
            referenceScalars,
            referenceCluster,
            referenceRun))
        {
            return fail("unable to construct SARA AM reference run");
        }


        ShapedGlyphBuffer reference;

        if (!shapeOpenTypeHorizontalRun(
            referenceRun,
            OTAG("thai"),
            0,
            reference,
            false,
            true))
        {
            return fail("unable to shape SARA AM reference run");
        }


        // ================================================================
        // Diagnostics.
        // ================================================================

        std::printf(
            "Thai SARA AM shaping\n"
            "  Face:       %s\n"
            "  Script:     %s\n"
            "  Scalars:    %u\n"
            "  Graphemes:  %u\n"
            "  Bidi level: %u\n"
            "\n"
            "  Source sequence:\n"
            "    U+0E01 U+0E48 U+0E33\n"
            "\n"
            "  Reference shaping sequence:\n"
            "    U+0E01 U+0E4D U+0E48 U+0E32\n"
            "\n",
            face.fullName() ? face.fullName() : "(unnamed)",
            scriptName,
            static_cast<unsigned>(paragraph.scalarCount),
            static_cast<unsigned>(paragraph.clusterCount),
            static_cast<unsigned>(shapingRun.bidiLevel));

        dumpThaiSaraAmBuffer("current source shaping", actual);
        dumpThaiSaraAmBuffer("reference shaping", reference);


        // ================================================================
        // Compare the visual shaping result.
        //
        // Ignore scalarOffset/scalarCount here because the reference run has
        // deliberately expanded the three-scalar source into four shaping
        // scalars.
        // ================================================================

        const bool equivalent =
            thaiSaraAmVisualEquivalent(actual, reference);

        std::printf(
            "\n"
            "  Source glyphs:    %zu\n"
            "  Reference glyphs: %zu\n"
            "  Visual match:     %s\n",
            actual.size(),
            reference.size(),
            equivalent ? "PASS" : "FAIL");


        // ================================================================
        // Finish itemizer validation before SVG rendering.
        // ================================================================

        FontRunView extraFontRun{};

        if (fontRuns(extraFontRun))
            return fail("expected exactly one font run");

        if (!fontRuns.ended())
            return fail("font-run itemizer did not end cleanly");

        ShapingRunView extraShapingRun{};

        if (shapingRuns(extraShapingRun))
            return fail("expected exactly one shaping run");

        if (!shapingRuns.ended())
            return fail("shaping-run itemizer did not end cleanly");

        BidiParagraphView extraParagraph{};

        if (bidi(extraParagraph))
            return fail("expected exactly one bidi paragraph");

        if (!bidi.ended())
            return fail("bidi stream did not end cleanly");


        // ================================================================
        // SVG comparison.
        //
        // Top:
        //
        //     original U+0E01 U+0E48 U+0E33
        //
        // Bottom:
        //
        //     reference U+0E01 U+0E4D U+0E48 U+0E32
        // ================================================================

        constexpr double originX = 48.0;
        constexpr double sourceBaseline = 120.0;
        constexpr double referenceBaseline = 270.0;

        if (!drawer.drawText(
            kThaiSaraAmText,
            originX,
            sourceBaseline))
        {
            return fail("SVG source rendering failed");
        }

        const SVGTextDrawStats sourceStats =
            drawer.lastDrawStats();

        if (!drawer.drawText(
            kThaiSaraAmReferenceText,
            originX,
            referenceBaseline))
        {
            return fail("SVG reference rendering failed");
        }

        const SVGTextDrawStats referenceStats =
            drawer.lastDrawStats();


        if (sourceStats.normalizedScalars != 3)
            return fail("SVG source normalized-scalar count mismatch");

        if (referenceStats.normalizedScalars != 4)
            return fail("SVG reference normalized-scalar count mismatch");

        if (sourceStats.graphemes != 1)
            return fail("SVG source grapheme count mismatch");

        if (referenceStats.graphemes != 1)
            return fail("SVG reference grapheme count mismatch");

        if (sourceStats.shapingRunCount != 1 ||
            referenceStats.shapingRunCount != 1)
        {
            return fail("unexpected SVG shaping-run count");
        }

        if (sourceStats.fontRunCount != 1 ||
            referenceStats.fontRunCount != 1)
        {
            return fail("unexpected SVG font-run count");
        }


        const std::string svg =
            drawer.document(
                0.0f,
                0.0f,
                480.0f,
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
        // Final result.
        //
        // A failure here is intentionally different from infrastructure
        // failure above. It means the pipeline is healthy but the source
        // SARA AM sequence does not yet receive the Thai preprocessing that
        // makes it equivalent to the reference shaping sequence.
        // ================================================================

        if (!equivalent)
        {
            std::printf(
                "\n"
                "Thai SARA AM: FAIL\n"
                "  Unicode front end:       PASS\n"
                "  Thai script itemization: PASS\n"
                "  Font selection:          PASS\n"
                "  OpenType shaping:        PASS\n"
                "  SVG rendering:           PASS\n"
                "  SARA AM equivalence:     FAIL\n"
                "\n"
                "  This isolates the Thai shaping-time transformation:\n"
                "\n"
                "    U+0E01 U+0E48 U+0E33\n"
                "             |\n"
                "             v\n"
                "    U+0E01 U+0E4D U+0E48 U+0E32\n");

            return false;
        }


        std::printf(
            "\n"
            "Thai SARA AM: PASS\n"
            "  Unicode front end:       PASS\n"
            "  Thai script itemization: PASS\n"
            "  Font selection:          PASS\n"
            "  OpenType shaping:        PASS\n"
            "  SARA AM equivalence:     PASS\n"
            "  SVG rendering:           PASS\n"
            "  SVG definitions:         %zu\n"
            "  SVG bytes:               %zu\n",
            drawer.glyphDefinitionCount(),
            svg.size());

        return true;
    }


    // ====================================================================
    // Filename convenience overload.
    //
    // The SVG is written even when the final SARA AM equivalence check
    // fails, provided both rendering paths themselves succeeded. This makes
    // the expected first failure visually inspectable.
    // ====================================================================

    static bool testThaiSaraAm(const char* databaseFilename,
        const char* fontFilename,
        const char* svgFilename = "test_thai_sara_am.svg")
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "Thai SARA AM: FAIL\n"
                "  Unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "Thai SARA AM: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        std::string svg;

        const bool result =
            testThaiSaraAm(
                ByteSpan(databaseBytes.data(), databaseBytes.size()),
                ByteSpan(fontBytes.data(), fontBytes.size()),
                &svg);


        // ------------------------------------------------------------
        // Write the diagnostic SVG even when the final equivalence test
        // fails.
        // ------------------------------------------------------------

        if (svgFilename && *svgFilename && !svg.empty())
        {
            std::ofstream output(
                svgFilename,
                std::ios::binary |
                std::ios::out |
                std::ios::trunc);

            if (!output)
            {
                std::printf(
                    "Thai SARA AM: FAIL\n"
                    "  Unable to create SVG\n"
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
                    "Thai SARA AM: FAIL\n"
                    "  Unable to write SVG\n"
                    "  File: %s\n",
                    svgFilename);

                return false;
            }

            std::printf(
                "  SVG file: %s\n",
                svgFilename);
        }

        return result;
    }

} // namespace waavs