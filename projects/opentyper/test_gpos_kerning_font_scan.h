// test_gpos_kerning_font_scan.h
#pragma once

#include "test_core.h"

#include <cstdio>
#include <cstdint>
#include <limits>
#include <vector>

#include "font_directory_view.h"
#include "opentype_horizontal_shaper.h"

namespace waavs
{
    struct GposKerningScanPair
    {
        uint32_t first{ 0 };
        uint32_t second{ 0 };
        int32_t delta{ 0 };
    };


    // ====================================================================
    // readGposScanNominalAdvance
    // ====================================================================

    static bool readGposScanNominalAdvance(const IProvideOpenTypeTables& tables,
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
    // buildGposScanRun
    // ====================================================================

    static bool buildGposScanRun(const FontFace& face, uint32_t first,
        uint32_t second, UnicodeScalar(&scalars)[2],
        ShapingCluster(&clusters)[2], FontRunView& run)
    {
        run = {};

        if (!face || face.glyphIndex(first) == 0 || face.glyphIndex(second) == 0)
            return false;

        scalars[0].value = first;
        scalars[1].value = second;

        clusters[0].scalarOffset = 0;
        clusters[0].scalarCount = 1;
        clusters[0].normalizedBegin = 0;

        clusters[1].scalarOffset = 1;
        clusters[1].scalarCount = 1;
        clusters[1].normalizedBegin = 1;

        run.scalars = scalars;
        run.scalarCount = 2;
        run.clusters = clusters;
        run.clusterCount = 2;
        run.face = face;
        run.bidiLevel = 0;
        run.normalizedBegin = 0;
        run.completeCoverage = true;

        return true;
    }


    // ====================================================================
    // scanGposKerningPair
    // ====================================================================

    static bool scanGposKerningPair(const FontFace& face,
        const IProvideOpenTypeTables& tables,
        uint32_t first, uint32_t second,
        GposKerningScanPair& result)
    {
        result = {};

        UnicodeScalar scalars[2]{};
        ShapingCluster clusters[2]{};
        FontRunView run{};

        if (!buildGposScanRun(
            face, first, second,
            scalars, clusters, run))
        {
            return false;
        }

        ShapedGlyphBuffer shaped;

        if (!shapeOpenTypeHorizontalRun(
            run,
            OTAG("latn"),
            0,
            shaped))
        {
            return false;
        }

        if (shaped.size() != 2)
            return false;

        int64_t nominalTotal = 0;
        int64_t shapedTotal = 0;

        for (size_t i = 0; i < shaped.size(); ++i)
        {
            uint16_t nominal = 0;

            if (!readGposScanNominalAdvance(
                tables,
                shaped[i].shaping.glyphId,
                face.glyphCount(),
                nominal))
            {
                return false;
            }

            nominalTotal += nominal;
            shapedTotal += shaped[i].placement.advanceX;
        }

        const int64_t delta = shapedTotal - nominalTotal;

        if (delta == 0 ||
            delta < std::numeric_limits<int32_t>::min() ||
            delta > std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        result.first = first;
        result.second = second;
        result.delta = static_cast<int32_t>(delta);

        return true;
    }


    // ====================================================================
    // findGposKerningPair
    // ====================================================================

    static bool findGposKerningPair(const FontFace& face,
        const IProvideOpenTypeTables& tables,
        GposKerningScanPair& result)
    {
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
            { 'Y', 'a' },
            { 'W', 'a' },
            { 'W', 'o' },
            { 'F', 'o' },
            { 'F', 'a' },
            { 'P', 'a' },
            { 'L', 'T' },
            { 'L', 'V' },
            { 'R', 'T' }
        };

        for (const auto& pair : pairs)
        {
            if (scanGposKerningPair(
                face,
                tables,
                pair[0],
                pair[1],
                result))
            {
                return true;
            }
        }

        return false;
    }


    // ====================================================================
    // testGposKerningFontScan
    //
    // Scan every FontFace in a directory and report real fonts for which
    // Latin shaping changes the nominal horizontal advance.
    //
    // This is intentionally diagnostic. It reports the population at each
    // filtering stage so a zero-match result is itself useful information.
    // ====================================================================

    static bool testGposKerningFontScan(const char* fontDirectory)
    {
        if (!fontDirectory || !*fontDirectory)
        {
            std::printf("GPOS kerning font scan: FAIL: missing directory\n");
            return false;
        }

        FontDirectoryView fonts(fontDirectory);

        size_t faceCount = 0;
        size_t openTypeFaceCount = 0;
        size_t gposFaceCount = 0;
        size_t latinPairFaceCount = 0;
        size_t shapeSuccessCount = 0;
        size_t kernedFaceCount = 0;

        FontFace face;

        while (fonts(face))
        {
            ++faceCount;

            const auto* tables =
                dynamic_cast<const IProvideOpenTypeTables*>(
                    face.operator->());

            if (!tables)
                continue;

            ++openTypeFaceCount;

            if (!tables->getTable(TagConstants::GPOS))
                continue;

            ++gposFaceCount;


            // --------------------------------------------------------
            // Cheap Latin coverage check before trying shaping.
            // --------------------------------------------------------

            if (face.glyphIndex('A') == 0 ||
                face.glyphIndex('V') == 0)
            {
                continue;
            }

            ++latinPairFaceCount;


            // --------------------------------------------------------
            // First determine whether the normal horizontal shaper can
            // successfully shape a simple AV run at all.
            // --------------------------------------------------------

            {
                UnicodeScalar scalars[2]{};
                ShapingCluster clusters[2]{};
                FontRunView run{};

                if (!buildGposScanRun(
                    face, 'A', 'V',
                    scalars, clusters, run))
                {
                    continue;
                }

                ShapedGlyphBuffer shaped;

                if (!shapeOpenTypeHorizontalRun(
                    run,
                    OTAG("latn"),
                    0,
                    shaped))
                {
                    continue;
                }

                ++shapeSuccessCount;
            }


            // --------------------------------------------------------
            // Now search several familiar Latin kerning pairs.
            // --------------------------------------------------------

            GposKerningScanPair pair{};

            if (!findGposKerningPair(face, *tables, pair))
                continue;

            ++kernedFaceCount;

            const char* name = face.fullName();
            const char* source = face.sourceLocation();

            if (!name)
                name = face.familyName();

            if (!name)
                name = "(unnamed)";

            if (!source)
                source = "(unknown source)";

            std::printf(
                "  MATCH\n"
                "    Face:    %s\n"
                "    File:    %s\n"
                "    Pair:    %c%c\n"
                "    Delta:   %d design units\n"
                "    UPEM:    %u\n",
                name,
                source,
                static_cast<char>(pair.first),
                static_cast<char>(pair.second),
                pair.delta,
                static_cast<unsigned>(face.unitsPerEm()));
        }


        std::printf(
            "\n"
            "GPOS kerning font scan: %s\n"
            "  Faces scanned:          %zu\n"
            "  OpenType faces:         %zu\n"
            "  Faces with GPOS:        %zu\n"
            "  Faces with A/V:         %zu\n"
            "  AV shaping succeeded:   %zu\n"
            "  Kerning matches:        %zu\n",
            kernedFaceCount != 0 ? "PASS" : "NO MATCH",
            faceCount,
            openTypeFaceCount,
            gposFaceCount,
            latinPairFaceCount,
            shapeSuccessCount,
            kernedFaceCount);

        return kernedFaceCount != 0;
    }

} // namespace waavs