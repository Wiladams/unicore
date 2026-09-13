// test_opentype_glyf_pathprogram_gallery.h

#pragma once

#include "test_core.h"

#include "font_face.h"
#include "opentype_container.h"
#include "opentype_glyf.h"
#include "opentype_types.h"

#include "pathp/pathprogram.h"
#include "pathp/pathprogram_builder.h"
#include "svg_path_data_sink.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace waavs
{
    enum class TestGlyfKind : uint8_t
    {
        Invalid,
        Empty,
        Simple,
        Composite
    };


    struct TestGlyfTextCase
    {
        const char* name;
        const uint32_t* codepoints;
        size_t count;
    };


    struct TestPathBoundsSink
    {
        bool hasPoint{ false };

        float minX{ 0.0f };
        float minY{ 0.0f };
        float maxX{ 0.0f };
        float maxY{ 0.0f };

        void add(float x, float y) noexcept
        {
            if (!hasPoint)
            {
                minX = maxX = x;
                minY = maxY = y;
                hasPoint = true;
                return;
            }

            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }

        bool onMoveTo(float x, float y) noexcept
        {
            add(x, y);
            return true;
        }

        bool onLineTo(float x, float y) noexcept
        {
            add(x, y);
            return true;
        }

        bool onQuadTo(float x1, float y1, float x, float y) noexcept
        {
            add(x1, y1);
            add(x, y);
            return true;
        }

        bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y) noexcept
        {
            add(x1, y1);
            add(x2, y2);
            add(x, y);
            return true;
        }

        bool onArcTo(float rx, float ry, float rotation, float largeArc, float sweep, float x, float y) noexcept
        {
            (void)rx;
            (void)ry;
            (void)rotation;
            (void)largeArc;
            (void)sweep;

            add(x, y);
            return true;
        }

        bool onClose() noexcept
        {
            return true;
        }

        bool onEnd() noexcept
        {
            return true;
        }

        float width() const noexcept
        {
            return hasPoint ? maxX - minX : 0.0f;
        }
    };


    // ========================================================================
    // Test text
    //
    // Universal character escapes keep the C++ source itself ASCII-only.
    // ========================================================================

    static constexpr uint32_t kGlyfTextLatin[] =
    {
        0x00C5, 'n', 'g', 's', 't', 'r', 0x00F6, 'm',
        0x20,
        'c', 'a', 'f', 0x00E9,
        0x20,
        'N', 'o', 0x00EB, 'l',
        0x20,
        'f', 'a', 0x00E7, 'a', 'd', 'e'
    };


    static constexpr uint32_t kGlyfTextRomance[] =
    {
        'C', 'r', 0x00E8, 'm', 'e',
        0x20,
        'b', 'r', 0x00FB, 'l', 0x00E9, 'e',
        0x20,
        'd', 0x00E9, 'j', 0x00E0,
        0x20,
        'v', 'u',
        0x20,
        'p', 'i', 0x00F1, 'a', 't', 'a',
        0x20,
        'j', 'a', 'l', 'a', 'p', 'e', 0x00F1, 'o'
    };


    // "Tieng Viet: Truong dai hoc" using precomposed Vietnamese characters.
    //
    // The interesting glyphs include:
    //
    //   U+1EBF  LATIN SMALL LETTER E WITH CIRCUMFLEX AND ACUTE
    //   U+1EC7  LATIN SMALL LETTER E WITH CIRCUMFLEX AND DOT BELOW
    //   U+1EDD  LATIN SMALL LETTER O WITH HORN AND GRAVE
    //   U+1EA1  LATIN SMALL LETTER A WITH DOT BELOW
    //   U+1ECD  LATIN SMALL LETTER O WITH DOT BELOW
    //
    // These are particularly interesting composite candidates.

    static constexpr uint32_t kGlyfTextVietnamese[] =
    {
        'T', 'i', 0x1EBF, 'n', 'g',
        0x20,
        'V', 'i', 0x1EC7, 't', ':',
        0x20,
        'T', 'r', 0x01B0, 0x1EDD, 'n', 'g',
        0x20,
        0x0111, 0x1EA1, 'i',
        0x20,
        'h', 0x1ECD, 'c'
    };


    // Latin Extended-A composite candidates:
    //
    // A/ring/acute, a/ring/acute
    // A/dieresis/macron, a/dieresis/macron
    // AE/acute, ae/acute
    // O/stroke/acute, o/stroke/acute

    static constexpr uint32_t kGlyfTextExtendedLatin[] =
    {
        0x01FA, 0x20, 0x01FB,
        0x20,
        0x01DE, 0x20, 0x01DF,
        0x20,
        0x01FC, 0x20, 0x01FD,
        0x20,
        0x01FE, 0x20, 0x01FF
    };


    // Particularly interesting stacked-diacritic characters.

    static constexpr uint32_t kGlyfTextDeepAccents[] =
    {
        0x1EAF, 0x20, // a with breve and acute
        0x1EB7, 0x20, // a with breve and dot below
        0x1EC7, 0x20, // e with circumflex and dot below
        0x1ED9, 0x20, // o with circumflex and dot below
        0x1EE5, 0x20, // u with dot below
        0x1EEF        // u with horn and tilde
    };


    // Polytonic Greek. These may be stored as simple or composite glyphs
    // depending on the font.

    static constexpr uint32_t kGlyfTextGreek[] =
    {
        0x1F08, 0x03B8, 0x03AE, 0x03BD, 0x03B1,
        0x20,
        0x1F48, 0x03B4, 0x03C5, 0x03C3, 0x03C3, 0x03B5, 0x03CD, 0x03C2
    };


    static constexpr TestGlyfTextCase kGlyfTextCases[] =
    {
        { "Latin accents",       kGlyfTextLatin,         std::size(kGlyfTextLatin) },
        { "Romance accents",     kGlyfTextRomance,       std::size(kGlyfTextRomance) },
        { "Vietnamese",          kGlyfTextVietnamese,    std::size(kGlyfTextVietnamese) },
        { "Latin Extended-A",    kGlyfTextExtendedLatin, std::size(kGlyfTextExtendedLatin) },
        { "Deep accents",        kGlyfTextDeepAccents,   std::size(kGlyfTextDeepAccents) },
        { "Polytonic Greek",     kGlyfTextGreek,         std::size(kGlyfTextGreek) }
    };


    // ========================================================================
    // loca helpers
    // ========================================================================

    static bool testGlyfReadLocaOffset(const TableRecord& locaTable, int16_t locaFormat,
        uint32_t glyphCount, uint32_t index, uint32_t& result)
    {
        result = 0;

        if (index > glyphCount)
            return false;

        OpenTypeByteStream stream(locaTable.data);

        if (locaFormat == 0)
        {
            if (!stream.seek(size_t(index) * 2u))
                return false;

            uint16_t value = 0;

            if (!stream.readUInt16(value))
                return false;

            result = uint32_t(value) * 2u;
            return true;
        }

        if (locaFormat == 1)
        {
            if (!stream.seek(size_t(index) * 4u))
                return false;

            return stream.readUInt32(result);
        }

        return false;
    }


    static TestGlyfKind testGlyfKind(const TableRecord& glyfTable, const TableRecord& locaTable,
        int16_t locaFormat, uint32_t glyphCount, uint32_t glyphId)
    {
        if (glyphId >= glyphCount)
            return TestGlyfKind::Invalid;

        uint32_t begin = 0;
        uint32_t end = 0;

        if (!testGlyfReadLocaOffset(locaTable, locaFormat, glyphCount, glyphId, begin) ||
            !testGlyfReadLocaOffset(locaTable, locaFormat, glyphCount, glyphId + 1u, end))
        {
            return TestGlyfKind::Invalid;
        }

        if (begin > end || size_t(end) > glyfTable.data.size())
            return TestGlyfKind::Invalid;

        if (begin == end)
            return TestGlyfKind::Empty;

        const ByteSpan glyphData = glyfTable.data.subSpan(begin, size_t(end) - begin);

        if (glyphData.size() < 2)
            return TestGlyfKind::Invalid;

        OpenTypeByteStream stream(glyphData);

        int16_t contourCount = 0;

        if (!stream.readInt16(contourCount))
            return TestGlyfKind::Invalid;

        return contourCount < 0
            ? TestGlyfKind::Composite
            : TestGlyfKind::Simple;
    }





    // ========================================================================
    // Path conversion
    //
    // Test two independent routes:
    //
    //   glyf -> SVG sink
    //
    // and:
    //
    //   glyf -> PathProgramBuilder -> PathProgram -> SVG sink
    //
    // The generated SVG path data should be identical.
    // ========================================================================

    static bool testBuildGlyfPath(OpenTypeGlyfDecoder& decoder, uint32_t glyphId,
        PathProgram& path, std::string& svgPath, TestPathBoundsSink& bounds)
    {
        // ------------------------------------------------------------
        // Direct glyf -> SVG sink
        // ------------------------------------------------------------

        SVGPathDataSink directSVG;

        if (!decoder.emitGlyphPath(glyphId, directSVG))
            return false;


        // ------------------------------------------------------------
        // glyf -> PathProgram
        // ------------------------------------------------------------

        PathProgramBuilder builder;

        if (!decoder.emitGlyphPath(glyphId, builder))
            return false;

        path = std::move(builder.prog);

        if (path.ops.empty() || path.ops.back() != OP_END)
            return false;


        // ------------------------------------------------------------
        // PathProgram -> SVG sink
        // ------------------------------------------------------------

        SVGPathDataSink replaySVG;

        if (!pathprogram_dispatch(path, replaySVG))
            return false;


        // The two sink routes should describe exactly the same path.
        if (directSVG.data != replaySVG.data)
            return false;


        // ------------------------------------------------------------
        // Collect approximate bounds for gallery layout.
        //
        // Control points are included, making these bounds conservative.
        // ------------------------------------------------------------

        bounds = {};

        if (!pathprogram_dispatch(path, bounds))
            return false;

        svgPath = std::move(replaySVG.data);
        return true;
    }


    // ========================================================================
    // Main ByteSpan test
    // ========================================================================

    static bool testOpenTypeGlyfPathProgramGallery(const ByteSpan& fontData,
        const char* svgFilename = "test_opentype_glyf_pathprogram_gallery.svg")
    {
        auto fail = [](const char* message)
            {
                std::printf("OpenType glyf PathProgram gallery: FAIL: %s\n", message);
                return false;
            };


        if (fontData.empty())
            return fail("empty font data");


        // ====================================================================
        // Build the font container.
        // ====================================================================

        SharedMemBuff source;

        if (!source.resetFromSize(fontData.size()))
            return fail("unable to allocate font source");

        std::memcpy(source.data(), fontData.data(), fontData.size());

        OpenTypeContainer container(source);

        if (!container.isValid())
            return fail("invalid OpenType container");


        // ====================================================================
        // Find the first face with TrueType outlines.
        // ====================================================================

        FontFace face;

        const IProvideOpenTypeTables* tables = nullptr;
        const TableRecord* glyfTable = nullptr;
        const TableRecord* locaTable = nullptr;
        const TableRecord* headTable = nullptr;

        size_t faceIndex = 0;

        while (container(face))
        {
            tables = dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

            if (tables)
            {
                glyfTable = tables->getTable(OTAG("glyf"));
                locaTable = tables->getTable(OTAG("loca"));
                headTable = tables->getTable(OTAG("head"));

                if (glyfTable && locaTable && headTable)
                    break;
            }

            ++faceIndex;
        }

        if (!face || !glyfTable || !locaTable || !headTable)
            return fail("no TrueType glyf face found");


        // ====================================================================
        // Decoder setup.
        // ====================================================================

        int16_t locaFormat = 0;

        if (!testReadIndexToLocFormat(*headTable, locaFormat))
            return fail("invalid indexToLocFormat");

        const uint32_t glyphCount = face.glyphCount();
        const uint16_t unitsPerEm = face.unitsPerEm();

        if (glyphCount == 0 || unitsPerEm == 0)
            return fail("invalid font geometry");

        OpenTypeGlyfDecoder decoder(glyfTable->data, locaTable->data, glyphCount, locaFormat);

        if (!decoder)
            return fail("invalid glyf decoder");


        // ====================================================================
        // SVG gallery setup.
        // ====================================================================

        const double glyphPixelHeight = 110.0;
        const double scale = glyphPixelHeight / double(unitsPerEm);

        const double leftMargin = 190.0;
        const double topMargin = 50.0;
        const double rowHeight = 165.0;
        const double glyphGap = 12.0;

        const double svgWidth = 1800.0;
        const double svgHeight = topMargin + rowHeight * double(std::size(kGlyfTextCases)) + 50.0;

        std::ofstream svg(svgFilename, std::ios::binary);

        if (!svg)
            return fail("unable to create SVG file");

        svg <<
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" "
            "viewBox=\"0 0 " << svgWidth << " " << svgHeight << "\">\n"
            "  <rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
            "  <style>\n"
            "    .label { font: 16px sans-serif; fill: #222; }\n"
            "    .simple { fill: #202020; }\n"
            "    .composite { fill: #b03030; }\n"
            "  </style>\n";


        size_t totalCharacters = 0;
        size_t generatedGlyphs = 0;
        size_t missingGlyphs = 0;
        size_t simpleGlyphs = 0;
        size_t compositeGlyphs = 0;
        size_t emptyGlyphs = 0;
        size_t totalOps = 0;


        // ====================================================================
        // Render each test phrase.
        // ====================================================================

        for (size_t row = 0; row < std::size(kGlyfTextCases); ++row)
        {
            const TestGlyfTextCase& testCase = kGlyfTextCases[row];

            const double baseline = topMargin + rowHeight * double(row) + 110.0;
            double cursorX = leftMargin;

            svg <<
                "  <text class=\"label\" x=\"20\" y=\"" << baseline - 35.0 << "\">"
                << testCase.name <<
                "</text>\n";


            std::printf("%s\n", testCase.name);


            for (size_t i = 0; i < testCase.count; ++i)
            {
                const uint32_t cp = testCase.codepoints[i];

                ++totalCharacters;


                // --------------------------------------------------------
                // Space
                // --------------------------------------------------------

                if (cp == 0x20)
                {
                    cursorX += double(unitsPerEm) * scale * 0.40;
                    continue;
                }


                // --------------------------------------------------------
                // cmap
                // --------------------------------------------------------

                const uint32_t glyphId = face.glyphIndex(cp);

                if (glyphId == 0 || glyphId >= glyphCount)
                {
                    ++missingGlyphs;

                    std::printf(
                        "  U+%04X  missing\n",
                        unsigned(cp));

                    cursorX += double(unitsPerEm) * scale * 0.60;
                    continue;
                }


                const TestGlyfKind kind =
                    testGlyfKind(*glyfTable, *locaTable, locaFormat, glyphCount, glyphId);

                switch (kind)
                {
                case TestGlyfKind::Simple:
                    ++simpleGlyphs;
                    break;

                case TestGlyfKind::Composite:
                    ++compositeGlyphs;
                    break;

                case TestGlyfKind::Empty:
                    ++emptyGlyphs;
                    break;

                default:
                    return fail("invalid glyf record");
                }


                // --------------------------------------------------------
                // glyf -> PathProgram -> SVG
                // --------------------------------------------------------

                PathProgram path;
                std::string pathData;
                TestPathBoundsSink bounds;

                if (!testBuildGlyfPath(decoder, glyphId, path, pathData, bounds))
                {
                    std::printf(
                        "OpenType glyf PathProgram gallery: FAIL\n"
                        "  Code point: U+%04X\n"
                        "  Glyph ID:   %u\n",
                        unsigned(cp),
                        unsigned(glyphId));

                    return false;
                }

                ++generatedGlyphs;
                totalOps += path.ops.size();


                const char* kindName =
                    kind == TestGlyfKind::Composite ? "composite" :
                    kind == TestGlyfKind::Simple ? "simple" :
                    kind == TestGlyfKind::Empty ? "empty" :
                    "invalid";


                std::printf(
                    "  U+%04X  glyph=%-5u %-9s ops=%zu args=%zu\n",
                    unsigned(cp),
                    unsigned(glyphId),
                    kindName,
                    path.ops.size(),
                    path.args.size());


                // --------------------------------------------------------
                // Render glyph.
                //
                // Font coordinates are Y-up. SVG is Y-down.
                // --------------------------------------------------------

                if (!pathData.empty() && bounds.hasPoint)
                {
                    const double tx = cursorX - double(bounds.minX) * scale;

                    svg <<
                        "  <path class=\""
                        << (kind == TestGlyfKind::Composite ? "composite" : "simple")
                        << "\" d=\"" << pathData << "\" "
                        "transform=\"translate(" << tx << " " << baseline << ") "
                        "scale(" << scale << " " << -scale << ")\"/>\n";


                    const double glyphWidth =
                        std::max(
                            double(bounds.width()) * scale,
                            double(unitsPerEm) * scale * 0.20);

                    cursorX += glyphWidth + glyphGap;
                }
                else
                {
                    cursorX += double(unitsPerEm) * scale * 0.40;
                }
            }


            std::printf("\n");
        }


        // ====================================================================
        // Legend
        // ====================================================================

        const double legendY = svgHeight - 20.0;

        svg <<
            "  <rect x=\"20\" y=\"" << legendY - 11.0
            << "\" width=\"12\" height=\"12\" fill=\"#202020\"/>\n"
            "  <text class=\"label\" x=\"40\" y=\"" << legendY
            << "\">simple glyf</text>\n"

            "  <rect x=\"160\" y=\"" << legendY - 11.0
            << "\" width=\"12\" height=\"12\" fill=\"#b03030\"/>\n"
            "  <text class=\"label\" x=\"180\" y=\"" << legendY
            << "\">composite glyf</text>\n"

            "</svg>\n";

        svg.close();

        if (!svg)
            return fail("SVG write failed");


        // ====================================================================
        // Diagnostics
        // ====================================================================

        std::printf(
            "OpenType glyf PathProgram gallery: PASS\n"
            "  Face index:             %zu\n"
            "  Font:                   %s\n"
            "  Glyph count:            %u\n"
            "  Units per em:           %u\n"
            "  loca format:            %d\n"
            "  Text cases:             %zu\n"
            "  Characters tested:      %zu\n"
            "  Glyphs generated:       %zu\n"
            "  Simple glyfs:           %zu\n"
            "  Composite glyfs:        %zu\n"
            "  Empty glyfs:            %zu\n"
            "  Missing glyphs:         %zu\n"
            "  PathProgram ops:        %zu\n"
            "  Direct/roundtrip SVG:   PASS\n"
            "  SVG output:             %s\n",
            faceIndex,
            face.fullName() ? face.fullName() : "(unnamed)",
            unsigned(glyphCount),
            unsigned(unitsPerEm),
            int(locaFormat),
            std::size(kGlyfTextCases),
            totalCharacters,
            generatedGlyphs,
            simpleGlyphs,
            compositeGlyphs,
            emptyGlyphs,
            missingGlyphs,
            totalOps,
            svgFilename);


        if (compositeGlyphs == 0)
        {
            std::printf(
                "  Note: no selected character was stored as a composite glyf.\n"
                "        Try another TrueType font for composite coverage.\n");
        }

        return generatedGlyphs != 0;
    }


    // ========================================================================
    // Filename convenience overload
    // ========================================================================

    static bool testOpenTypeGlyfPathProgramGallery(const char* fontFilename,
        const char* svgFilename = "test_opentype_glyf_pathprogram_gallery.svg")
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(fontFilename, fileData))
        {
            std::printf(
                "OpenType glyf PathProgram gallery: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename);

            return false;
        }

        return testOpenTypeGlyfPathProgramGallery(
            ByteSpan(fileData.data(), fileData.size()),
            svgFilename);
    }
}