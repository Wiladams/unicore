#pragma once

#include "test_core.h"

#include <cstdio>

#include "font_directory_view.h"
#include "font_filter.h"
#include "font_predicates.h"
#include "opentype_horizontal_shaper.h"

namespace waavs
{
    struct LatinLigatureProbeResult
    {
        GlyphId fi{ 0 };
        GlyphId fl{ 0 };
        GlyphId ffi{ 0 };
        GlyphId ffl{ 0 };
    };


    // ====================================================================
    // shapeLatinLigatureProbe
    //
    // Shape one ASCII sequence twice:
    //
    //     baseline:
    //         required LangSys feature only
    //
    //     liga:
    //         required LangSys feature + liga
    //
    // A successful probe requires:
    //
    //     baseline glyph count == scalar count
    //     liga glyph count     == 1
    //
    // The resulting ligature must retain the complete logical scalar span.
    // ====================================================================

    static bool shapeLatinLigatureProbe(const FontFace& face,
        const uint32_t* codepoints, uint32_t count, GlyphId& ligatureGlyph)
    {
        ligatureGlyph = 0;

        if (!face || !codepoints || count < 2 || count > 3)
            return false;

        UnicodeScalar scalars[3]{};

        for (uint32_t i = 0; i < count; ++i)
        {
            if (!face.hasGlyph(codepoints[i]))
                return false;

            scalars[i].value = codepoints[i];
        }

        FontRunView run{};

        run.scalars = scalars;
        run.scalarCount = count;
        run.face = face;
        run.bidiLevel = 0;
        run.completeCoverage = true;


        // ------------------------------------------------------------
        // Baseline: no optional GSUB features.
        //
        // Required LangSys features remain enabled by the explicit
        // shaping API, so the liga-on probe differs only by liga.
        // ------------------------------------------------------------

        OpenTypeHorizontalShapeRequest baselineRequest{};

        baselineRequest.scriptTag = OTAG("latn");
        baselineRequest.languageTag = 0;
        baselineRequest.fallbackToDefaultScript = false;
        baselineRequest.fallbackToDefaultLanguage = true;

        ShapedGlyphBuffer baseline;

        if (!shapeOpenTypeHorizontalRun(run, baselineRequest, baseline))
            return false;

        if (baseline.size() != count)
            return false;


        // ------------------------------------------------------------
        // Explicit standard ligatures.
        // ------------------------------------------------------------

        const uint32_t ligaFeature = OTAG("liga");

        OpenTypeHorizontalShapeRequest ligaRequest = baselineRequest;

        ligaRequest.gsubFeatureTags = &ligaFeature;
        ligaRequest.gsubFeatureCount = 1;

        ShapedGlyphBuffer ligature;

        if (!shapeOpenTypeHorizontalRun(run, ligaRequest, ligature))
            return false;

        if (ligature.size() != 1)
            return false;

        const ShapedGlyph& glyph = ligature[0];

        if (glyph.shaping.glyphId == 0)
            return false;

        if (glyph.shaping.scalarOffset != 0 ||
            glyph.shaping.scalarCount != count)
        {
            return false;
        }

        ligatureGlyph = glyph.shaping.glyphId;
        return true;
    }


    // ====================================================================
    // probeStandardLatinLigatures
    //
    // Require the traditional standard Latin ligatures:
    //
    //     fi
    //     fl
    //     ffi
    //     ffl
    //
    // Each must collapse to exactly one glyph under liga.
    // ====================================================================

    static bool probeStandardLatinLigatures(const FontFace& face,
        LatinLigatureProbeResult& result)
    {
        result = {};

        static constexpr uint32_t fi[] =
        {
            0x0066, 0x0069
        };

        static constexpr uint32_t fl[] =
        {
            0x0066, 0x006C
        };

        static constexpr uint32_t ffi[] =
        {
            0x0066, 0x0066, 0x0069
        };

        static constexpr uint32_t ffl[] =
        {
            0x0066, 0x0066, 0x006C
        };

        if (!shapeLatinLigatureProbe(face, fi, 2, result.fi))
            return false;

        if (!shapeLatinLigatureProbe(face, fl, 2, result.fl))
            return false;

        if (!shapeLatinLigatureProbe(face, ffi, 3, result.ffi))
            return false;

        if (!shapeLatinLigatureProbe(face, ffl, 3, result.ffl))
            return false;

        return true;
    }


    // ====================================================================
    // supportsStandardLatinLigatures
    //
    // Behavioral font predicate suitable for the font query pipeline.
    // ====================================================================

    static FontFacePredFn supportsStandardLatinLigatures()
    {
        return [](const FontFace& face)
            {
                LatinLigatureProbeResult result;
                return probeStandardLatinLigatures(face, result);
            };
    }


    // ====================================================================
    // testFindLigatureFonts
    // ====================================================================

    static bool testFindLigatureFonts(const char* fontDirectory)
    {
        if (!fontDirectory || !*fontDirectory)
        {
            std::printf(
                "Latin ligature font search: FAIL\n"
                "  Invalid font directory\n");

            return false;
        }


        // ------------------------------------------------------------
        // Cheap coverage tests first.
        //
        // Only fonts containing f, i and l reach the relatively expensive
        // behavioral shaping predicate.
        // ------------------------------------------------------------

        auto fonts =
            FontDirectoryView(fontDirectory)
            | covers(0x0066)
            | covers(0x0069)
            | covers(0x006C)
            | supportsStandardLatinLigatures();


        size_t matchCount = 0;
        FontFace face{};

        std::printf(
            "Latin ligature font search\n"
            "  Directory: %s\n\n",
            fontDirectory);


        while (fonts(face))
        {
            LatinLigatureProbeResult result;

            if (!probeStandardLatinLigatures(face, result))
            {
                std::printf(
                    "Latin ligature font search: FAIL\n"
                    "  Predicate/probe disagreement\n");

                return false;
            }

            ++matchCount;

            std::printf(
                "  [%zu] %s\n"
                "       Family:   %s\n"
                "       fi:       glyph %u\n"
                "       fl:       glyph %u\n"
                "       ffi:      glyph %u\n"
                "       ffl:      glyph %u\n"
                "       Location: %s\n\n",
                matchCount,
                face.fullName() ? face.fullName() : "(unnamed)",
                face.familyName() ? face.familyName() : "(unnamed)",
                static_cast<unsigned>(result.fi),
                static_cast<unsigned>(result.fl),
                static_cast<unsigned>(result.ffi),
                static_cast<unsigned>(result.ffl),
                face.sourceLocation() ? face.sourceLocation() : "(unknown)");
        }


        if (matchCount == 0)
        {
            std::printf(
                "Latin ligature font search: FAIL\n"
                "  No font produced all four standard ligatures\n");

            return false;
        }


        std::printf(
            "Latin ligature font search: PASS\n"
            "  Matching faces: %zu\n"
            "  fi:             PASS\n"
            "  fl:             PASS\n"
            "  ffi:            PASS\n"
            "  ffl:            PASS\n"
            "  Provenance:     PASS\n",
            matchCount);

        return true;
    }

} // namespace waavs