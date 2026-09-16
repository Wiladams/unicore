// test_thai_mark_stack.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "font_directory_view.h"
#include "font_filter.h"
#include "font_predicates.h"
#include "opentype_horizontal_shaper.h"
#include "svg_text_drawer.h"

namespace waavs
{
    static constexpr char kThaiMarkStackText[] = u8"\u0E01\u0E35\u0E48";

    static constexpr uint32_t kThaiMarkStackCodePoints[] =
    {
        0x0E01, // KO KAI
        0x0E35, // SARA II
        0x0E48  // MAI EK
    };


    struct ThaiMarkStackProbeResult
    {
        ShapedGlyphBuffer noGpos{};
        ShapedGlyphBuffer markOnly{};
        ShapedGlyphBuffer markMkmk{};
        ShapedGlyphBuffer normal{};

        bool markChanged{ false };
        bool mkmkChanged{ false };
    };


    static bool thaiMarkStackPlacementEqual(const GlyphPlacement& a, const GlyphPlacement& b) noexcept
    {
        return a.advanceX == b.advanceX &&
            a.advanceY == b.advanceY &&
            a.offsetX == b.offsetX &&
            a.offsetY == b.offsetY;
    }


    static bool thaiMarkStackShapingEqual(const OpenTypeShapingGlyph& a,
        const OpenTypeShapingGlyph& b) noexcept
    {
        return a.glyphId == b.glyphId &&
            a.scalarOffset == b.scalarOffset &&
            a.scalarCount == b.scalarCount &&
            a.ligature.id == b.ligature.id &&
            a.ligature.component == b.ligature.component &&
            a.ligature.componentCount == b.ligature.componentCount;
    }


    static bool thaiMarkStackSameIdentity(const ShapedGlyphBuffer& a,
        const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!thaiMarkStackShapingEqual(a[i].shaping, b[i].shaping))
                return false;
        }

        return true;
    }


    static bool thaiMarkStackPlacementChanged(const ShapedGlyphBuffer& a,
        const ShapedGlyphBuffer& b, size_t index) noexcept
    {
        if (index >= a.size() || index >= b.size())
            return false;

        return !thaiMarkStackPlacementEqual(a[index].placement, b[index].placement);
    }


    static void dumpThaiMarkStackBuffer(const char* name, const ShapedGlyphBuffer& buffer)
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
    // probeThaiMarkStackRun
    //
    // Shape the same run four ways:
    //
    //   1. normal GSUB, no GPOS
    //   2. normal GSUB, mark
    //   3. normal GSUB, mark + mkmk
    //   4. normal policy
    //
    // This lets us observe the positioning contribution of mark and mkmk
    // without changing the GSUB side of the experiment.
    // ====================================================================

    static bool probeThaiMarkStackRun(const FontRunView& run, ThaiMarkStackProbeResult& result)
    {
        result = {};

        if (!run.face || run.scalarCount != 3 || run.rightToLeft())
            return false;

        for (uint32_t i = 0; i < 3; ++i)
        {
            if (run.scalars[i].value != kThaiMarkStackCodePoints[i])
                return false;
        }


        const uint32_t scriptTag = OTAG("thai");

        const OpenTypeShapingPolicy& normalPolicy =
            openTypeShapingPolicyForScript(scriptTag);


        // ------------------------------------------------------------
        // GSUB + nominal metrics only.
        // ------------------------------------------------------------

        OpenTypeShapingPolicy noGposPolicy = normalPolicy;

        noGposPolicy.gposStages = nullptr;
        noGposPolicy.gposStageCount = 0;

        if (!shapeOpenTypeHorizontalRun(
            run, scriptTag, 0, noGposPolicy, result.noGpos,
            false, true))
        {
            return false;
        }


        // ------------------------------------------------------------
        // mark only.
        // ------------------------------------------------------------

        static constexpr uint32_t markFeatures[] =
        {
            OTAG("mark")
        };

        static constexpr OpenTypeShapingFeatureStage markStage =
        {
            markFeatures,
            sizeof(markFeatures) / sizeof(markFeatures[0]),
            true
        };

        OpenTypeShapingPolicy markPolicy = normalPolicy;

        markPolicy.gposStages = &markStage;
        markPolicy.gposStageCount = 1;

        if (!shapeOpenTypeHorizontalRun(
            run, scriptTag, 0, markPolicy, result.markOnly,
            false, true))
        {
            return false;
        }


        // ------------------------------------------------------------
        // mark + mkmk.
        // ------------------------------------------------------------

        static constexpr uint32_t markMkmkFeatures[] =
        {
            OTAG("mark"),
            OTAG("mkmk")
        };

        static constexpr OpenTypeShapingFeatureStage markMkmkStage =
        {
            markMkmkFeatures,
            sizeof(markMkmkFeatures) / sizeof(markMkmkFeatures[0]),
            true
        };

        OpenTypeShapingPolicy markMkmkPolicy = normalPolicy;

        markMkmkPolicy.gposStages = &markMkmkStage;
        markMkmkPolicy.gposStageCount = 1;

        if (!shapeOpenTypeHorizontalRun(
            run, scriptTag, 0, markMkmkPolicy, result.markMkmk,
            false, true))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Normal current policy.
        // ------------------------------------------------------------

        if (!shapeOpenTypeHorizontalRun(
            run, scriptTag, 0, normalPolicy, result.normal,
            false, true))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Controlled test string should remain three glyphs.
        // ------------------------------------------------------------

        if (result.noGpos.size() != 3 ||
            result.markOnly.size() != 3 ||
            result.markMkmk.size() != 3 ||
            result.normal.size() != 3)
        {
            return false;
        }


        // ------------------------------------------------------------
        // GPOS must not change glyph identity or provenance.
        // ------------------------------------------------------------

        if (!thaiMarkStackSameIdentity(result.noGpos, result.markOnly) ||
            !thaiMarkStackSameIdentity(result.noGpos, result.markMkmk) ||
            !thaiMarkStackSameIdentity(result.noGpos, result.normal))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Each glyph should still represent exactly one input scalar.
        // ------------------------------------------------------------

        for (size_t i = 0; i < 3; ++i)
        {
            const OpenTypeShapingGlyph& glyph =
                result.normal[i].shaping;

            if (glyph.glyphId == 0 ||
                glyph.scalarOffset != i ||
                glyph.scalarCount != 1)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Behavioral observations.
        //
        // mark may affect either or both marks.
        //
        // mkmk is expected to further change MAI EK, the second mark.
        // ------------------------------------------------------------

        result.markChanged =
            thaiMarkStackPlacementChanged(result.noGpos, result.markOnly, 1) ||
            thaiMarkStackPlacementChanged(result.noGpos, result.markOnly, 2);

        result.mkmkChanged =
            thaiMarkStackPlacementChanged(result.markOnly, result.markMkmk, 2);

        return true;
    }


    static bool probeThaiMarkStackFace(const FontFace& face, ThaiMarkStackProbeResult& result)
    {
        result = {};

        if (!face)
            return false;

        UnicodeScalar scalars[3]{};

        for (uint32_t i = 0; i < 3; ++i)
        {
            if (!face.hasGlyph(kThaiMarkStackCodePoints[i]))
                return false;

            scalars[i].value = kThaiMarkStackCodePoints[i];
        }

        FontRunView run{};

        run.scalars = scalars;
        run.scalarCount = 3;
        run.face = face;
        run.bidiLevel = 0;
        run.completeCoverage = true;

        return probeThaiMarkStackRun(run, result);
    }


    // ====================================================================
    // hasThaiTrueTypeOutlines
    //
    // SVGTextDrawer currently requires TrueType glyf/loca outlines.
    // ====================================================================

    static FontFacePredFn hasThaiTrueTypeOutlines()
    {
        return [](const FontFace& face)
            {
                const IProvideOpenTypeTables* tables =
                    openTypeTableProvider(face);

                if (!tables)
                    return false;

                return tables->getTable(OTAG("glyf")) != nullptr &&
                    tables->getTable(OTAG("loca")) != nullptr &&
                    tables->getTable(OTAG("head")) != nullptr;
            };
    }


    // ====================================================================
    // testFindThaiMarkStackFont
    //
    // Optional first step:
    //
    // Search a font directory using cheap cmap coverage filters first, then
    // behaviorally require both mark and mkmk to affect the controlled stack.
    //
    // The printed source location can then be supplied to testThaiMarkStack().
    // ====================================================================

    static bool testFindThaiMarkStackFont(const char* fontDirectory)
    {
        if (!fontDirectory || !*fontDirectory)
        {
            std::printf(
                "Thai mark-stack font search: FAIL\n"
                "  Invalid font directory\n");

            return false;
        }

        auto fonts =
            FontDirectoryView(fontDirectory)
            | covers(0x0E01)
            | covers(0x0E35)
            | covers(0x0E48)
            | hasThaiTrueTypeOutlines();

        FontFace face{};
        size_t candidates = 0;

        while (fonts(face))
        {
            ++candidates;

            ThaiMarkStackProbeResult probe;

            if (!probeThaiMarkStackFace(face, probe))
                continue;

            if (!probe.markChanged || !probe.mkmkChanged)
                continue;

            std::printf(
                "Thai mark-stack font search: PASS\n"
                "  Candidates tested: %zu\n"
                "  Face:              %s\n"
                "  Family:            %s\n"
                "  Location:          %s\n"
                "  mark effect:       PASS\n"
                "  mkmk effect:       PASS\n",
                candidates,
                face.fullName() ? face.fullName() : "(unnamed)",
                face.familyName() ? face.familyName() : "(unnamed)",
                face.sourceLocation() ? face.sourceLocation() : "(unknown)");

            return true;
        }

        std::printf(
            "Thai mark-stack font search: FAIL\n"
            "  Candidates tested: %zu\n"
            "  No TrueType Thai face demonstrated both mark and mkmk\n",
            candidates);

        return false;
    }


    // ====================================================================
    // testThaiMarkStack
    //
    // Full checkpoint:
    //
    //   UTF-8
    //     -> NFC
    //     -> grapheme segmentation
    //     -> Script
    //     -> bidi
    //     -> shaping run
    //     -> font run
    //     -> cmap / GSUB / metrics / GPOS
    //     -> SVG
    // ====================================================================

    static bool testThaiMarkStack(const ByteSpan& databaseData,
        const ByteSpan& fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf(
                    "Thai mark stack: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        if (databaseData.empty())
            return fail("empty Unicode database");

        if (fontData.empty())
            return fail("empty font data");


        // ------------------------------------------------------------
        // Unicode database.
        // ------------------------------------------------------------

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("unable to load Unicode database");


        // ------------------------------------------------------------
        // SVG facade and TrueType face.
        // ------------------------------------------------------------

        constexpr double fontSize = 96.0;

        SVGTextDrawer drawer;

        if (!drawer.load(databaseData, fontData, fontSize))
            return fail("unable to initialize SVGTextDrawer");

        FontFace face = drawer.face();

        if (!face)
            return fail("SVGTextDrawer produced no FontFace");


        // ------------------------------------------------------------
        // Controlled UTF-8 input.
        // ------------------------------------------------------------

        const ByteSpan text(
            reinterpret_cast<const uint8_t*>(kThaiMarkStackText),
            sizeof(kThaiMarkStackText) - 1);


        // ------------------------------------------------------------
        // Unicode front end.
        // ------------------------------------------------------------

        Utf8ScalarStream utf8(text);
        UnicodeNfcStream<Utf8ScalarStream> nfc(utf8, database);
        GraphemePropertyStream<decltype(nfc)> properties(nfc, database);
        GraphemeStream<decltype(properties)> graphemes(properties);
        UnicodeScriptStream<decltype(graphemes)> scripts(graphemes, database);
        UnicodeBidiStream<decltype(scripts)> bidi(scripts, database);

        BidiParagraphView paragraph{};

        if (!bidi(paragraph))
            return fail("Unicode pipeline produced no paragraph");

        if (paragraph.scalarCount != 3)
            return fail("expected exactly three normalized scalars");

        if (paragraph.clusterCount != 1)
            return fail("expected exactly one grapheme cluster");


        // ------------------------------------------------------------
        // Shaping run.
        // ------------------------------------------------------------

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
            return fail("Thai checkpoint is not LTR");

        const InternedKey scriptName =
            database.scriptISO15924(shapingRun.script);

        if (!scriptName || std::strcmp(scriptName, "Thai") != 0)
            return fail("shaping run was not resolved as Thai");

        ShapingRunView extraShapingRun{};

        if (shapingRuns(extraShapingRun))
            return fail("expected exactly one shaping run");

        if (!shapingRuns.ended())
            return fail("shaping-run itemizer did not end cleanly");


        // ------------------------------------------------------------
        // Font run.
        //
        // SVGTextDrawer itself is the one-face candidate provider here.
        // ------------------------------------------------------------

        FontRunItemizer fontRuns(shapingRun, drawer, database);

        if (fontRuns.failed())
            return fail("font-run itemizer initialization failed");

        FontRunView fontRun{};

        if (!fontRuns(fontRun))
            return fail("unable to obtain Thai font run");

        if (!fontRun.face || fontRun.face != face)
            return fail("unexpected Thai FontFace");

        if (!fontRun.completeCoverage)
            return fail("font does not completely cover the Thai cluster");

        if (fontRun.scalarCount != 3 ||
            fontRun.clusterCount != 1)
        {
            return fail("unexpected Thai font-run extent");
        }

        FontRunView extraFontRun{};

        if (fontRuns(extraFontRun))
            return fail("expected exactly one font run");

        if (!fontRuns.ended())
            return fail("font-run itemizer did not end cleanly");


        // ------------------------------------------------------------
        // Direct shaping probe.
        // ------------------------------------------------------------

        ThaiMarkStackProbeResult probe;

        if (!probeThaiMarkStackRun(fontRun, probe))
            return fail("OpenType Thai shaping probe failed");

        std::printf(
            "Thai mark-stack shaping\n"
            "  Face:        %s\n"
            "  Script:      %s\n"
            "  Scalars:     %u\n"
            "  Graphemes:   %u\n"
            "  Bidi level:  %u\n\n",
            face.fullName() ? face.fullName() : "(unnamed)",
            scriptName,
            static_cast<unsigned>(paragraph.scalarCount),
            static_cast<unsigned>(paragraph.clusterCount),
            static_cast<unsigned>(shapingRun.bidiLevel));

        dumpThaiMarkStackBuffer("GSUB + nominal metrics", probe.noGpos);
        dumpThaiMarkStackBuffer("mark", probe.markOnly);
        dumpThaiMarkStackBuffer("mark + mkmk", probe.markMkmk);
        dumpThaiMarkStackBuffer("normal policy", probe.normal);

        std::printf(
            "\n"
            "  mark effect: %s\n"
            "  mkmk effect: %s\n",
            probe.markChanged ? "PASS" : "NO CHANGE",
            probe.mkmkChanged ? "PASS" : "NO CHANGE");

        if (!probe.markChanged)
            return fail("mark feature produced no observable placement change");

        if (!probe.mkmkChanged)
            return fail("mkmk feature produced no observable MAI EK placement change");


        // ------------------------------------------------------------
        // Full SVG path.
        //
        // Requires SVGTextDrawer::openTypeScriptTag() to admit:
        //
        //     Thai -> OTAG("thai")
        // ------------------------------------------------------------

        constexpr double originX = 48.0;
        constexpr double baselineY = 130.0;

        if (!drawer.drawText(kThaiMarkStackText, originX, baselineY))
            return fail("SVGTextDrawer::drawText failed");

        const SVGTextDrawStats& stats = drawer.lastDrawStats();

        if (stats.normalizedScalars != 3)
            return fail("SVG path normalized-scalar count mismatch");

        if (stats.graphemes != 1)
            return fail("SVG path grapheme count mismatch");

        if (stats.paragraphCount != 1)
            return fail("SVG path paragraph count mismatch");

        if (stats.shapingRunCount != 1)
            return fail("SVG path shaping-run count mismatch");

        if (stats.fontRunCount != 1)
            return fail("SVG path font-run count mismatch");

        if (stats.shapedGlyphCount != probe.normal.size())
            return fail("SVG path shaped-glyph count mismatch");

        if (!(stats.finalPenX > stats.originX))
            return fail("SVG path produced no positive visual width");


        // ------------------------------------------------------------
        // SVG document.
        // ------------------------------------------------------------

        const std::string svg =
            drawer.document(0.0f, 0.0f, 480.0f, 220.0f);

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


        // ------------------------------------------------------------
        // Upstream bidi stream must also end cleanly.
        // ------------------------------------------------------------

        BidiParagraphView extraParagraph{};

        if (bidi(extraParagraph))
            return fail("expected exactly one bidi paragraph");

        if (!bidi.ended())
            return fail("bidi stream did not end cleanly");


        std::printf(
            "\n"
            "Thai mark stack: PASS\n"
            "  Face:              %s\n"
            "  Normalized scalars:%zu\n"
            "  Graphemes:         %zu\n"
            "  Shaping runs:      %zu\n"
            "  Font runs:         %zu\n"
            "  Shaped glyphs:     %zu\n"
            "  mark:              PASS\n"
            "  mkmk:              PASS\n"
            "  Provenance:        PASS\n"
            "  SVG definitions:   %zu\n"
            "  SVG bytes:         %zu\n",
            drawer.fontName() ? drawer.fontName() : "(unnamed)",
            stats.normalizedScalars,
            stats.graphemes,
            stats.shapingRunCount,
            stats.fontRunCount,
            stats.shapedGlyphCount,
            drawer.glyphDefinitionCount(),
            svg.size());

        return true;
    }


    // ====================================================================
    // Filename convenience overload.
    // ====================================================================

    static bool testThaiMarkStack(const char* databaseFilename,
        const char* fontFilename,
        const char* svgFilename = "test_thai_mark_stack.svg")
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "Thai mark stack: FAIL\n"
                "  Unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "Thai mark stack: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        std::string svg;

        if (!testThaiMarkStack(
            ByteSpan(databaseBytes.data(), databaseBytes.size()),
            ByteSpan(fontBytes.data(), fontBytes.size()),
            &svg))
        {
            return false;
        }

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!output)
        {
            std::printf(
                "Thai mark stack: FAIL\n"
                "  Unable to create SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        output.write(svg.data(), static_cast<std::streamsize>(svg.size()));

        if (!output)
        {
            std::printf(
                "Thai mark stack: FAIL\n"
                "  Unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf(
            "  SVG file:          %s\n",
            svgFilename);

        return true;
    }

} // namespace waavs