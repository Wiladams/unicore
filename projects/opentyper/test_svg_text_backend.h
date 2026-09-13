// test_svg_text_backend.h
#pragma once

#include "test_core.h"

#include "opentype_container.h"
#include "opentype_glyf.h"
#include "opentype_types.h"
#include "svg_text_backend.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace waavs
{
    // ====================================================================
    // countOccurrences
    // ====================================================================

    static size_t countOccurrences(const std::string& text, const std::string& needle)
    {
        if (needle.empty())
            return 0;

        size_t count = 0;
        size_t position = 0;

        while ((position = text.find(needle, position)) != std::string::npos)
        {
            ++count;
            position += needle.size();
        }

        return count;
    }


    // ====================================================================
    // makeSVGTestGlyfDecoder
    //
    // This is only construction plumbing. OpenTypeGlyfDecoder remains the
    // one and only glyph decoder.
    // ====================================================================

    static bool makeSVGTestGlyfDecoder(const FontFace& face, OpenTypeGlyfDecoder& decoder)
    {
        decoder = {};

        if (!face)
            return false;

        const IProvideOpenTypeTables* tables =
            dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

        if (!tables)
            return false;

        const TableRecord* glyf = tables->getTable(TagConstants::GLYF);
        const TableRecord* loca = tables->getTable(TagConstants::LOCA);
        const TableRecord* head = tables->getTable(TagConstants::HEAD);

        if (!glyf || !loca || !head || head->data.size() < 54)
            return false;

        // head.indexToLocFormat is int16 at byte offset 50.
        OpenTypeByteStream headStream(head->data);

        if (!headStream.seek(50))
            return false;

        int16_t locaFormat = 0;

        if (!headStream.readInt16(locaFormat))
            return false;

        decoder = OpenTypeGlyfDecoder(glyf->data, loca->data, face.glyphCount(), locaFormat);
        return decoder.isValid();
    }


    // ====================================================================
    // testSVGTextBackend
    //
    // Test the real path:
    //
    //     font bytes
    //         -> OpenTypeContainer
    //         -> FontFace
    //         -> OpenTypeGlyfDecoder
    //         -> SVGTextBackend
    //         -> SVG
    //
    // The generated visual is:
    //
    //     A     B     A
    //
    // There should be two definitions and three uses.
    // ====================================================================

    static bool testSVGTextBackend(ByteSpan fontData, std::string* svgOutput = nullptr)
    {
        auto fail = [](const char* message)
            {
                std::printf("SVG text backend: FAIL: %s\n", message);
                return false;
            };

        if (fontData.empty())
            return fail("empty font data");


        // ------------------------------------------------------------
        // OpenTypeContainer retains SharedMemBuff, so copy the supplied
        // ByteSpan into the normal shared font backing store.
        // ------------------------------------------------------------

        SharedMemBuff buffer(fontData.size());

        if (!buffer)
            return fail("unable to allocate font buffer");

        std::memcpy(buffer.data(), fontData.begin(), fontData.size());

        OpenTypeContainer container(buffer);

        if (!container.isValid())
            return fail("invalid OpenType container");


        // ------------------------------------------------------------
        // Find the first face in the resource that has glyf outlines.
        //
        // This naturally handles both ordinary TTF files and TTC files.
        // ------------------------------------------------------------

        FontFace face;
        OpenTypeGlyfDecoder decoder;
        bool foundFace = false;

        while (container(face))
        {
            if (makeSVGTestGlyfDecoder(face, decoder))
            {
                foundFace = true;
                break;
            }
        }

        if (!foundFace)
            return fail("no glyf-based FontFace found");

        if (face.unitsPerEm() == 0)
            return fail("FontFace has zero unitsPerEm");


        // ------------------------------------------------------------
        // Use recognizable cmap-selected glyphs instead of arbitrary
        // low-numbered glyph IDs.
        // ------------------------------------------------------------

        const uint32_t glyphA = face.glyphIndex('A');
        const uint32_t glyphB = face.glyphIndex('B');

        if (glyphA == 0)
            return fail("font does not contain glyph A");

        if (glyphB == 0)
            return fail("font does not contain glyph B");

        if (glyphA == glyphB)
            return fail("A and B map to the same glyph");


        // ------------------------------------------------------------
        // Independently verify that the real decoder can emit both.
        // ------------------------------------------------------------

        {
            SVGPathDataSink sink;

            if (!decoder.emitGlyphPath(glyphA, sink) || sink.data.empty())
                return fail("unable to decode glyph A");
        }

        {
            SVGPathDataSink sink;

            if (!decoder.emitGlyphPath(glyphB, sink) || sink.data.empty())
                return fail("unable to decode glyph B");
        }


        // ------------------------------------------------------------
        // Backend integration.
        //
        // A and B create definitions.
        // The second A must reuse the existing A definition.
        // ------------------------------------------------------------

        SVGTextBackend backend;

        const float fontSize = 96.0f;
        const float scale = fontSize / static_cast<float>(face.unitsPerEm());

        if (!backend.emitGlyph(face, decoder, glyphA, 70.0f, 180.0f, scale))
            return fail("unable to emit first A");

        if (!backend.emitGlyph(face, decoder, glyphB, 190.0f, 180.0f, scale))
            return fail("unable to emit B");

        if (!backend.emitGlyph(face, decoder, glyphA, 310.0f, 180.0f, scale))
            return fail("unable to emit repeated A");


        // ------------------------------------------------------------
        // Cache state.
        // ------------------------------------------------------------

        if (backend.fontCount() != 1)
            return fail("unexpected font count");

        if (backend.glyphDefinitionCount() != 2)
            return fail("unexpected glyph definition count");


        // ------------------------------------------------------------
        // Invalid input must not add another definition.
        // ------------------------------------------------------------

        if (backend.emitGlyph(face, decoder, face.glyphCount(), 0.0f, 0.0f, scale))
            return fail("accepted out-of-range glyph ID");

        if (backend.glyphDefinitionCount() != 2)
            return fail("invalid glyph modified definition cache");

        if (backend.emitGlyph(face, decoder, glyphA, 0.0f, 0.0f, 0.0f))
            return fail("accepted zero scale");

        if (backend.glyphDefinitionCount() != 2)
            return fail("invalid scale modified definition cache");


        // ------------------------------------------------------------
        // Generate SVG.
        // ------------------------------------------------------------

        const std::string svg = backend.document(0.0f, 0.0f, 450.0f, 240.0f);

        if (svg.empty())
            return fail("generated SVG is empty");


        // ------------------------------------------------------------
        // Verify definitions and uses.
        // ------------------------------------------------------------

        const std::string idA = "f0g" + std::to_string(glyphA);
        const std::string idB = "f0g" + std::to_string(glyphB);

        const std::string pathA = "<path id=\"" + idA + "\"";
        const std::string pathB = "<path id=\"" + idB + "\"";
        const std::string useA = "<use href=\"#" + idA + "\"";
        const std::string useB = "<use href=\"#" + idB + "\"";

        if (countOccurrences(svg, "<path id=\"") != 2)
            return fail("expected exactly two path definitions");

        if (countOccurrences(svg, "<use href=\"") != 3)
            return fail("expected exactly three glyph uses");

        if (countOccurrences(svg, pathA) != 1)
            return fail("glyph A definition count is incorrect");

        if (countOccurrences(svg, pathB) != 1)
            return fail("glyph B definition count is incorrect");

        if (countOccurrences(svg, useA) != 2)
            return fail("glyph A use count is incorrect");

        if (countOccurrences(svg, useB) != 1)
            return fail("glyph B use count is incorrect");


        // ------------------------------------------------------------
        // Verify placement made it through to SVG.
        // ------------------------------------------------------------

        if (svg.find("translate(70 180)") == std::string::npos)
            return fail("first glyph position missing");

        if (svg.find("translate(190 180)") == std::string::npos)
            return fail("second glyph position missing");

        if (svg.find("translate(310 180)") == std::string::npos)
            return fail("third glyph position missing");


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

        std::printf(
            "SVG text backend: PASS\n"
            "  Face:               %s\n"
            "  Units per em:       %u\n"
            "  Glyph A:            %u\n"
            "  Glyph B:            %u\n"
            "  Definitions:        %zu\n"
            "  Glyph uses:         3\n"
            "  SVG bytes:          %zu\n",
            faceName,
            static_cast<unsigned>(face.unitsPerEm()),
            static_cast<unsigned>(glyphA),
            static_cast<unsigned>(glyphB),
            backend.glyphDefinitionCount(),
            svg.size());


        // ------------------------------------------------------------
        // Clear.
        // ------------------------------------------------------------

        backend.clear();

        if (backend.fontCount() != 0 || backend.glyphDefinitionCount() != 0)
            return fail("clear did not reset backend");

        return true;
    }


    // ====================================================================
    // Filename convenience overload
    //
    // Also writes a browser-viewable SVG when svgFilename is supplied.
    // ====================================================================

    static bool testSVGTextBackend(const char* fontFilename,
        const char* svgFilename = "test_svg_text_backend.svg")
    {
        if (!fontFilename || !*fontFilename)
        {
            std::printf("SVG text backend: FAIL: missing font filename\n");
            return false;
        }

        std::vector<uint8_t> fileData;

        if (!readFileData(fontFilename, fileData))
        {
            std::printf(
                "SVG text backend: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename);

            return false;
        }

        const ByteSpan fontData(fileData.data(), fileData.size());

        std::string svg;

        if (!testSVGTextBackend(fontData, &svg))
            return false;

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG text backend: FAIL: unable to create SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        output.write(svg.data(), static_cast<std::streamsize>(svg.size()));

        if (!output)
        {
            std::printf(
                "SVG text backend: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf("  SVG file:           %s\n", svgFilename);
        return true;
    }

} // namespace waavs