// test_svg_text_run_adapter.h
#pragma once

#include "test_core.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "opentype_container.h"
#include "opentype_glyf.h"
#include "opentype_horizontal_shaper.h"
#include "svg_text_run_adapter.h"

namespace waavs
{
    // ====================================================================
    // SVGKerningTestPair
    // ====================================================================

    struct SVGKerningTestPair
    {
        uint32_t first{ 0 };
        uint32_t second{ 0 };
        int32_t advanceDelta{ 0 };
    };


    // ====================================================================
    // makeSVGShapingTestGlyfDecoder
    // ====================================================================

    static bool makeSVGShapingTestGlyfDecoder(const FontFace& face, OpenTypeGlyfDecoder& decoder)
    {
        decoder = {};

        if (!face)
            return false;

        const auto* tables =
            dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

        if (!tables)
            return false;

        const TableRecord* glyf = tables->getTable(TagConstants::GLYF);
        const TableRecord* loca = tables->getTable(TagConstants::LOCA);
        const TableRecord* head = tables->getTable(TagConstants::HEAD);

        if (!glyf || !loca || !head || head->data.size() < 52)
            return false;

        OpenTypeByteStream stream(head->data);

        if (!stream.seek(50))
            return false;

        int16_t locaFormat = 0;

        if (!stream.readInt16(locaFormat))
            return false;

        decoder = OpenTypeGlyfDecoder(glyf->data, loca->data, face.glyphCount(), locaFormat);
        return decoder.isValid();
    }


    // ====================================================================
    // readSVGTestHorizontalAdvance
    //
    // Read the nominal hmtx advance for one final glyph ID.
    //
    // This deliberately bypasses GPOS. Comparing this value with the final
    // ShapedGlyph::placement.advanceX lets the test prove that positioning
    // changed the nominal metric.
    // ====================================================================

    static bool readSVGTestHorizontalAdvance(const IProvideOpenTypeTables& tables,
        uint32_t glyphId, uint32_t glyphCount, uint16_t& advance)
    {
        advance = 0;

        if (glyphId >= glyphCount || glyphCount == 0)
            return false;

        const TableRecord* hhea = tables.getTable(TagConstants::HHEA);
        const TableRecord* hmtx = tables.getTable(TagConstants::HMTX);

        if (!hhea || !hmtx || hhea->data.size() < 36)
            return false;

        const uint8_t* hp = hhea->data.begin() + 34;

        const uint16_t numberOfHMetrics =
            (static_cast<uint16_t>(hp[0]) << 8) |
            static_cast<uint16_t>(hp[1]);

        if (numberOfHMetrics == 0 || numberOfHMetrics > glyphCount)
            return false;

        const uint32_t metricIndex =
            glyphId < numberOfHMetrics
            ? glyphId
            : static_cast<uint32_t>(numberOfHMetrics - 1u);

        const size_t offset = static_cast<size_t>(metricIndex) * 4u;

        if (offset + 2u > hmtx->data.size())
            return false;

        const uint8_t* mp = hmtx->data.begin() + offset;

        advance =
            (static_cast<uint16_t>(mp[0]) << 8) |
            static_cast<uint16_t>(mp[1]);

        return true;
    }


    // ====================================================================
    // buildSVGTestFontRun
    // ====================================================================

    static bool buildSVGTestFontRun(const FontFace& face,
        const uint32_t* codepoints, size_t codepointCount,
        std::vector<UnicodeScalar>& scalars,
        std::vector<ShapingCluster>& clusters,
        FontRunView& run)
    {
        run = {};
        scalars.clear();
        clusters.clear();

        if (!face || (!codepoints && codepointCount != 0) ||
            codepointCount > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        scalars.reserve(codepointCount);
        clusters.reserve(codepointCount);

        for (size_t i = 0; i < codepointCount; ++i)
        {
            if (face.glyphIndex(codepoints[i]) == 0)
                return false;

            UnicodeScalar scalar{};
            scalar.value = codepoints[i];

            ShapingCluster cluster{};
            cluster.scalarOffset = static_cast<uint32_t>(i);
            cluster.scalarCount = 1;
            cluster.normalizedBegin = static_cast<ScalarIndex>(i);

            scalars.push_back(scalar);
            clusters.push_back(cluster);
        }

        run.scalars = scalars.empty() ? nullptr : scalars.data();
        run.scalarCount = static_cast<uint32_t>(scalars.size());
        run.clusters = clusters.empty() ? nullptr : clusters.data();
        run.clusterCount = static_cast<uint32_t>(clusters.size());
        run.face = face;
        run.bidiLevel = 0;
        run.normalizedBegin = 0;
        run.completeCoverage = true;

        return true;
    }


    // ====================================================================
    // shapeSVGTestLatinRun
    // ====================================================================

    static bool shapeSVGTestLatinRun(const FontRunView& run, ShapedGlyphBuffer& shaped)
    {
        shaped.clear();

        return shapeOpenTypeHorizontalRun(
            run,
            OTAG("latn"),
            0,
            shaped);
    }


    // ====================================================================
    // findSVGTestKerningPair
    //
    // Find a familiar Latin pair for which the final shaped horizontal
    // advance differs from the nominal hmtx advance.
    //
    // Requiring an advance difference specifically targets ordinary GPOS
    // pair kerning rather than merely detecting an unrelated mark offset.
    // ====================================================================

    static bool findSVGTestKerningPair(const FontFace& face,
        const IProvideOpenTypeTables& tables, SVGKerningTestPair& result)
    {
        result = {};

        static constexpr uint32_t pairs[][2] =
        {
            { 'A', 'V' },
            { 'V', 'A' },
            { 'A', 'W' },
            { 'A', 'T' },
            { 'T', 'o' },
            { 'T', 'a' },
            { 'T', 'e' },
            { 'Y', 'o' },
            { 'W', 'a' },
            { 'W', 'o' },
            { 'F', 'o' },
            { 'F', 'a' },
            { 'P', 'a' },
            { 'L', 'T' }
        };

        for (const auto& pair : pairs)
        {
            std::vector<UnicodeScalar> scalars;
            std::vector<ShapingCluster> clusters;
            FontRunView run{};

            if (!buildSVGTestFontRun(face, pair, 2, scalars, clusters, run))
                continue;

            ShapedGlyphBuffer shaped;

            if (!shapeSVGTestLatinRun(run, shaped) || shaped.size() != 2)
                continue;

            int64_t nominalAdvance = 0;
            int64_t shapedAdvance = 0;
            bool valid = true;

            for (size_t i = 0; i < shaped.size(); ++i)
            {
                uint16_t nominal = 0;

                if (!readSVGTestHorizontalAdvance(
                    tables,
                    shaped[i].shaping.glyphId,
                    face.glyphCount(),
                    nominal))
                {
                    valid = false;
                    break;
                }

                nominalAdvance += nominal;
                shapedAdvance += shaped[i].placement.advanceX;
            }

            if (!valid)
                continue;

            const int64_t delta = shapedAdvance - nominalAdvance;

            if (delta == 0 ||
                delta < std::numeric_limits<int32_t>::min() ||
                delta > std::numeric_limits<int32_t>::max())
            {
                continue;
            }

            result.first = pair[0];
            result.second = pair[1];
            result.advanceDelta = static_cast<int32_t>(delta);
            return true;
        }

        return false;
    }


    // ====================================================================
    // testSVGTextRunAdapter
    //
    // Real integration path:
    //
    //     font bytes
    //         -> FontFace
    //         -> FontRunView
    //         -> cmap
    //         -> GSUB
    //         -> nominal hmtx
    //         -> GPOS kerning
    //         -> ShapedGlyphView
    //         -> design-unit scaling / pen accumulation
    //         -> SVGTextBackend
    //         -> SVG
    //
    // SVG output contains two rows:
    //
    //     top:     normal shaped/GPOS pair
    //     bottom:  identical final glyph IDs with nominal hmtx placement
    //
    // The spacing difference should be visible.
    // ====================================================================

    static bool testSVGTextRunAdapter(ByteSpan fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf("SVG shaped text run: FAIL: %s\n", message);
                return false;
            };

        if (fontData.empty())
            return fail("empty font data");


        // ------------------------------------------------------------
        // Load font resource.
        // ------------------------------------------------------------

        SharedMemBuff buffer(fontData.size());

        if (!buffer)
            return fail("unable to allocate font buffer");

        std::memcpy(buffer.data(), fontData.begin(), fontData.size());

        OpenTypeContainer container(buffer);

        if (!container.isValid())
            return fail("invalid OpenType container");


        // ------------------------------------------------------------
        // Find a glyf-based face containing a real Latin GPOS kerning pair.
        // ------------------------------------------------------------

        FontFace face;
        OpenTypeGlyfDecoder decoder;
        SVGKerningTestPair pair{};
        bool foundFace = false;

        FontFace candidate;

        while (container(candidate))
        {
            const auto* tables =
                dynamic_cast<const IProvideOpenTypeTables*>(candidate.operator->());

            if (!tables)
                continue;

            if (!tables->getTable(TagConstants::GPOS))
                continue;

            OpenTypeGlyfDecoder candidateDecoder;

            if (!makeSVGShapingTestGlyfDecoder(candidate, candidateDecoder))
                continue;

            SVGKerningTestPair candidatePair{};

            if (!findSVGTestKerningPair(candidate, *tables, candidatePair))
                continue;

            face = candidate;
            decoder = candidateDecoder;
            pair = candidatePair;
            foundFace = true;
            break;
        }

        if (!foundFace)
            return fail("no glyf face with a detectable Latin GPOS kerning pair");

        if (face.unitsPerEm() == 0)
            return fail("face has zero unitsPerEm");

        const auto* tables =
            dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

        if (!tables)
            return fail("selected face does not provide OpenType tables");


        // ------------------------------------------------------------
        // Build the final two-character run.
        // ------------------------------------------------------------

        const uint32_t codepoints[2] =
        {
            pair.first,
            pair.second
        };

        std::vector<UnicodeScalar> scalars;
        std::vector<ShapingCluster> clusters;
        FontRunView run{};

        if (!buildSVGTestFontRun(
            face,
            codepoints,
            2,
            scalars,
            clusters,
            run))
        {
            return fail("unable to construct FontRunView");
        }


        // ------------------------------------------------------------
        // Real shaping.
        // ------------------------------------------------------------

        ShapedGlyphBuffer shaped;

        if (!shapeSVGTestLatinRun(run, shaped))
            return fail("horizontal shaping failed");

        if (shaped.size() != 2)
            return fail("kerning pair did not remain two glyphs");

        ShapedGlyphView shapedView(shaped);


        // ------------------------------------------------------------
        // Build a comparison buffer containing the SAME final glyph IDs
        // and provenance, but restore nominal hmtx placement.
        //
        // This isolates what GPOS changed.
        // ------------------------------------------------------------

        ShapedGlyphBuffer nominal = shaped;

        int64_t nominalAdvanceDesign = 0;
        int64_t shapedAdvanceDesign = 0;
        size_t adjustedGlyphCount = 0;

        for (size_t i = 0; i < shaped.size(); ++i)
        {
            uint16_t nominalAdvance = 0;

            if (!readSVGTestHorizontalAdvance(
                *tables,
                shaped[i].shaping.glyphId,
                face.glyphCount(),
                nominalAdvance))
            {
                return fail("unable to read nominal hmtx advance");
            }

            nominalAdvanceDesign += nominalAdvance;
            shapedAdvanceDesign += shaped[i].placement.advanceX;

            if (shaped[i].placement.advanceX != static_cast<int32_t>(nominalAdvance) ||
                shaped[i].placement.advanceY != 0 ||
                shaped[i].placement.offsetX != 0 ||
                shaped[i].placement.offsetY != 0)
            {
                ++adjustedGlyphCount;
            }

            nominal[i].placement.advanceX = nominalAdvance;
            nominal[i].placement.advanceY = 0;
            nominal[i].placement.offsetX = 0;
            nominal[i].placement.offsetY = 0;
        }

        if (adjustedGlyphCount == 0)
            return fail("GPOS produced no detectable placement change");

        if (shapedAdvanceDesign == nominalAdvanceDesign)
            return fail("selected pair produced no horizontal advance change");

        const int64_t measuredDelta =
            shapedAdvanceDesign - nominalAdvanceDesign;

        if (measuredDelta != pair.advanceDelta)
            return fail("kerning adjustment changed between probe and final shape");

        ShapedGlyphView nominalView(nominal);


        // ------------------------------------------------------------
        // Compose and emit both rows.
        //
        // Top row:
        //     shaped / GPOS
        //
        // Bottom row:
        //     nominal hmtx
        // ------------------------------------------------------------

        constexpr double fontSize = 180.0;
        constexpr double originX = 50.0;
        constexpr double shapedBaselineY = 190.0;
        constexpr double nominalBaselineY = 430.0;

        SVGTextBackend backend;

        HorizontalGlyphPositioningResult shapedResult{};
        HorizontalGlyphPositioningResult nominalResult{};

        if (!emitHorizontalLTRSVGRun(
            run,
            shapedView,
            decoder,
            fontSize,
            originX,
            shapedBaselineY,
            backend,
            &shapedResult))
        {
            return fail("unable to emit GPOS-positioned SVG run");
        }

        if (!emitHorizontalLTRSVGRun(
            run,
            nominalView,
            decoder,
            fontSize,
            originX,
            nominalBaselineY,
            backend,
            &nominalResult))
        {
            return fail("unable to emit nominal SVG comparison run");
        }


        // ------------------------------------------------------------
        // Prove that the composition bridge preserved the shaped advances.
        // ------------------------------------------------------------

        const double scale =
            fontSize / static_cast<double>(face.unitsPerEm());

        const double expectedShapedPen =
            originX + static_cast<double>(shapedAdvanceDesign) * scale;

        const double expectedNominalPen =
            originX + static_cast<double>(nominalAdvanceDesign) * scale;

        const double epsilon = 1.0e-8;

        if (std::fabs(shapedResult.scale - scale) > epsilon ||
            std::fabs(nominalResult.scale - scale) > epsilon)
        {
            return fail("incorrect design-unit scale");
        }

        if (std::fabs(shapedResult.penX - expectedShapedPen) > epsilon)
            return fail("GPOS advance did not reach final positioned pen");

        if (std::fabs(nominalResult.penX - expectedNominalPen) > epsilon)
            return fail("nominal comparison pen is incorrect");

        if (std::fabs(shapedResult.penX - nominalResult.penX) <= epsilon)
            return fail("GPOS and nominal runs have identical final positions");


        // ------------------------------------------------------------
        // Produce SVG.
        // ------------------------------------------------------------

        const double maxPen =
            std::max(shapedResult.penX, nominalResult.penX);

        const float documentWidth =
            static_cast<float>(std::max(400.0, maxPen + 60.0));

        const float documentHeight = 500.0f;

        const std::string svg =
            backend.document(
                0.0f,
                0.0f,
                documentWidth,
                documentHeight);

        if (svg.empty())
            return fail("generated SVG is empty");

        if (svg.find("<svg") == std::string::npos ||
            svg.find("<path") == std::string::npos ||
            svg.find("<use") == std::string::npos)
        {
            return fail("generated document does not contain expected SVG content");
        }

        if (svgOutput)
            *svgOutput = svg;


        // ------------------------------------------------------------
        // Diagnostics.
        // ------------------------------------------------------------

        const char* faceName = face.fullName();

        if (!faceName)
            faceName = face.familyName();

        if (!faceName)
            faceName = "(unnamed)";

        const double deltaUser =
            static_cast<double>(measuredDelta) * scale;

        std::printf(
            "SVG shaped text run: PASS\n"
            "  Face:                 %s\n"
            "  Pair:                 %c%c\n"
            "  Glyphs:               %zu\n"
            "  Units per em:         %u\n"
            "  Font size:            %.2f\n"
            "  Scale:                %.6f\n"
            "  Nominal advance:      %lld design units\n"
            "  Shaped advance:       %lld design units\n"
            "  GPOS advance delta:   %lld design units\n"
            "  GPOS layout delta:    %.4f\n"
            "  Adjusted glyphs:      %zu\n"
            "  GPOS final pen:       %.4f\n"
            "  Nominal final pen:    %.4f\n"
            "  Glyph definitions:    %zu\n"
            "  SVG bytes:            %zu\n"
            "  Top row:              GPOS positioned\n"
            "  Bottom row:           nominal hmtx\n",
            faceName,
            static_cast<char>(pair.first),
            static_cast<char>(pair.second),
            shaped.size(),
            static_cast<unsigned>(face.unitsPerEm()),
            fontSize,
            scale,
            static_cast<long long>(nominalAdvanceDesign),
            static_cast<long long>(shapedAdvanceDesign),
            static_cast<long long>(measuredDelta),
            deltaUser,
            adjustedGlyphCount,
            shapedResult.penX,
            nominalResult.penX,
            backend.glyphDefinitionCount(),
            svg.size());

        return true;
    }


    // ====================================================================
    // Filename convenience overload
    // ====================================================================

    static bool testSVGTextRunAdapter(const char* fontFilename,
        const char* svgFilename = "test_svg_text_run_adapter.svg")
    {
        if (!fontFilename || !*fontFilename)
        {
            std::printf("SVG shaped text run: FAIL: missing font filename\n");
            return false;
        }

        std::vector<uint8_t> fileData;

        if (!readFileData(fontFilename, fileData))
        {
            std::printf(
                "SVG shaped text run: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename);

            return false;
        }

        const ByteSpan fontData(fileData.data(), fileData.size());

        std::string svg;

        if (!testSVGTextRunAdapter(fontData, &svg))
            return false;

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG shaped text run: FAIL: unable to create SVG\n"
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
                "SVG shaped text run: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf("  SVG file:             %s\n", svgFilename);
        return true;
    }

} // namespace waavs