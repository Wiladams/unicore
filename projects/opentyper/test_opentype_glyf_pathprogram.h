// test_opentype_glyf_pathprogram.h

#pragma once

#include "test_core.h"

#include "opentype_container.h"
#include "opentype_glyf.h"

#include "pathp/pathprogram.h"
#include "pathp/pathprogram_builder.h"
#include "svg_path_data_sink.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace waavs
{
    struct GlyfPathTestCase
    {
        uint32_t codepoint;
        const char* name;
    };





    static bool testPathProgramToSVG(const PathProgram& path, std::string& pathData)
    {
        SVGPathDataSink sink;

        if (!pathprogram_dispatch(path, sink))
            return false;

        pathData = std::move(sink.data);
        return true;
    }


    static bool testOpenTypeGlyfPathProgram(const ByteSpan& fontData,
        const char* svgFilename = "test_opentype_glyf_pathprogram.svg")
    {
        auto fail = [](const char* message)
            {
                std::printf("OpenType glyf PathProgram: FAIL: %s\n", message);
                return false;
            };


        // ================================================================
        // Copy the test input into the shared storage expected by the
        // OpenType container.
        // ================================================================

        if (fontData.empty())
            return fail("empty font data");

        SharedMemBuff source;

        if (!source.resetFromSize(fontData.size()))
            return fail("unable to allocate font buffer");

        std::memcpy(source.data(), fontData.data(), fontData.size());


        // ================================================================
        // Open the real OpenType resource.
        //
        // This also works for TTC resources. Find the first face containing
        // a TrueType glyf/loca outline pair.
        // ================================================================

        OpenTypeContainer container(source);

        if (!container.isValid())
            return fail("invalid OpenType container");

        FontFace face;

        const IProvideOpenTypeTables* tables = nullptr;
        const TableRecord* glyfTable = nullptr;
        const TableRecord* locaTable = nullptr;
        const TableRecord* headTable = nullptr;

        size_t faceIndex = 0;
        bool foundGlyfFace = false;

        while (container(face))
        {
            tables = dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

            if (!tables)
            {
                ++faceIndex;
                continue;
            }

            glyfTable = tables->getTable(OTAG("glyf"));
            locaTable = tables->getTable(OTAG("loca"));
            headTable = tables->getTable(OTAG("head"));

            if (glyfTable && locaTable && headTable)
            {
                foundGlyfFace = true;
                break;
            }

            ++faceIndex;
        }

        if (!foundGlyfFace)
            return fail("no face containing glyf/loca/head");


        // ================================================================
        // Establish the information required by OpenTypeGlyfDecoder.
        // ================================================================

        int16_t locaFormat = 0;

        if (!testReadIndexToLocFormat(*headTable, locaFormat))
            return fail("invalid head.indexToLocFormat");

        const uint32_t glyphCount = face.glyphCount();
        const uint16_t unitsPerEm = face.unitsPerEm();

        if (glyphCount == 0)
            return fail("font reports zero glyphs");

        if (unitsPerEm == 0)
            return fail("font reports zero unitsPerEm");

        OpenTypeGlyfDecoder decoder(
            glyfTable->data,
            locaTable->data,
            glyphCount,
            locaFormat);

        if (!decoder.isValid())
            return fail("OpenTypeGlyfDecoder is invalid");


        // ================================================================
        // Pick recognizable glyphs.
        //
        // U+00E9 and U+00C5 are particularly interesting because many
        // TrueType fonts implement them as composite glyphs.
        // ================================================================

        static constexpr GlyfPathTestCase cases[] =
        {
            { 0x0041, "A" },
            { 0x0067, "g" },
            { 0x0026, "ampersand" },
            { 0x0040, "at" },
            { 0x00E9, "eacute" },
            { 0x00C5, "Aring" }
        };


        struct GeneratedGlyph
        {
            uint32_t codepoint{ 0 };
            uint32_t glyphId{ 0 };
            const char* name{ nullptr };
            PathProgram path;
            std::string svgPath;
        };

        std::vector<GeneratedGlyph> glyphs;


        // ================================================================
        // glyf -> PathProgramBuilder -> PathProgram
        // ================================================================

        for (const GlyfPathTestCase& item : cases)
        {
            const uint32_t glyphId = face.glyphIndex(item.codepoint);

            if (glyphId == 0 || glyphId >= glyphCount)
            {
                std::printf(
                    "  U+%04X %-10s : not present\n",
                    unsigned(item.codepoint),
                    item.name);

                continue;
            }


            PathProgramBuilder builder;

            if (!decoder.emitGlyphPath(glyphId, builder))
            {
                std::printf(
                    "  U+%04X %-10s : glyph decode failed\n",
                    unsigned(item.codepoint),
                    item.name);

                return false;
            }


            PathProgram path = std::move(builder.prog);

            if (path.ops.empty() || path.ops.back() != OP_END)
                return fail("decoded PathProgram is not terminated");


            // ------------------------------------------------------------
            // PathProgram -> SVG sink
            // ------------------------------------------------------------

            std::string svgPath;

            if (!testPathProgramToSVG(path, svgPath))
                return fail("PathProgram SVG dispatch failed");

            if (svgPath.empty())
                return fail("non-empty glyph produced empty SVG path");


            std::printf(
                "  U+%04X %-10s glyph=%-5u ops=%-5zu args=%-5zu svg=%zu bytes\n",
                unsigned(item.codepoint),
                item.name,
                unsigned(glyphId),
                path.ops.size(),
                path.args.size(),
                svgPath.size());


            GeneratedGlyph generated;
            generated.codepoint = item.codepoint;
            generated.glyphId = glyphId;
            generated.name = item.name;
            generated.path = std::move(path);
            generated.svgPath = std::move(svgPath);

            glyphs.push_back(std::move(generated));
        }


        if (glyphs.size() < 3)
            return fail("font did not contain enough test glyphs");


        // ================================================================
        // Generate an SVG contact sheet.
        //
        // TrueType coordinates use +Y upward. SVG normally uses +Y
        // downward, so each glyph is placed with scale(s, -s).
        // ================================================================

        const double glyphHeight = 180.0;
        const double scale = glyphHeight / double(unitsPerEm);

        const double cellWidth = 240.0;
        const double cellHeight = 260.0;
        const double baseline = 210.0;

        const double svgWidth = cellWidth * double(glyphs.size());
        const double svgHeight = cellHeight;


        std::ofstream svg(svgFilename, std::ios::binary);

        if (!svg)
            return fail("unable to create SVG output file");


        svg <<
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" "
            "viewBox=\"0 0 " << svgWidth << " " << svgHeight << "\" "
            "width=\"" << svgWidth << "\" height=\"" << svgHeight << "\">\n"
            "  <rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";


        for (size_t i = 0; i < glyphs.size(); ++i)
        {
            const GeneratedGlyph& glyph = glyphs[i];

            const double x = cellWidth * double(i) + 30.0;

            svg <<
                "  <g>\n"
                "    <path d=\"" << glyph.svgPath << "\" "
                "transform=\"translate(" << x << " " << baseline << ") "
                "scale(" << scale << " " << -scale << ")\" "
                "fill=\"#202020\"/>\n"
                "    <text x=\"" << x << "\" y=\"240\" "
                "font-family=\"sans-serif\" font-size=\"14\">"
                << glyph.name
                << "  U+";

            char cpText[16];
            std::snprintf(cpText, sizeof(cpText), "%04X", unsigned(glyph.codepoint));

            svg << cpText <<
                "</text>\n"
                "  </g>\n";
        }


        svg << "</svg>\n";
        svg.close();

        if (!svg)
            return fail("error writing SVG output");


        // ================================================================
        // Diagnostics
        // ================================================================

        std::printf(
            "OpenType glyf PathProgram: PASS\n"
            "  Face index:         %zu\n"
            "  Font:               %s\n"
            "  Glyph count:        %u\n"
            "  Units per em:       %u\n"
            "  loca format:        %d\n"
            "  Glyphs rendered:    %zu\n"
            "  SVG output:         %s\n",
            faceIndex,
            face.fullName() ? face.fullName() : "(unnamed)",
            unsigned(glyphCount),
            unsigned(unitsPerEm),
            int(locaFormat),
            glyphs.size(),
            svgFilename);

        return true;
    }


    // ====================================================================
    // Convenience filename overload
    // ====================================================================

    static bool testOpenTypeGlyfPathProgram(const char* fontFilename,
        const char* svgFilename = "test_opentype_glyf_pathprogram.svg")
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(fontFilename, fileData))
        {
            std::printf(
                "OpenType glyf PathProgram: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename);

            return false;
        }

        const ByteSpan source(fileData.data(), fileData.size());

        return testOpenTypeGlyfPathProgram(source, svgFilename);
    }
}