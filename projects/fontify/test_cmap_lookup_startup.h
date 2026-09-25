
// test_cmap_lookup_startup.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "opentype_cmap_view.h"

#include <chrono>
#include <cstdio>


namespace waavs
{
    static bool testCmapLookupStartup(const char* fontDirectory)
    {
        if (!fontDirectory || !*fontDirectory)
            return false;

        using Clock = std::chrono::steady_clock;

        FontDirectoryView fonts(fontDirectory, true);
        FontFaceView view;

        size_t facesScanned = 0;
        size_t cmapFaces = 0;
        size_t glyphLookups = 0;
        size_t glyphHits = 0;

        std::chrono::nanoseconds constructionTime{ 0 };
        std::chrono::nanoseconds lookupTime{ 0 };

        // A small spread across BMP and supplementary Unicode.
        static constexpr uint32_t kCodepoints[] =
        {
            0x0041,     // Latin A
            0x00E9,     // Latin e acute
            0x03B1,     // Greek alpha
            0x0416,     // Cyrillic Zhe
            0x05D0,     // Hebrew alef
            0x0627,     // Arabic alef
            0x0915,     // Devanagari ka
            0x0E01,     // Thai ko kai
            0x4E00,     // CJK ideograph
            0x1F600     // Grinning face
        };

        const auto totalStart = Clock::now();

        while (fonts(view))
        {
            ++facesScanned;

            const auto constructionStart = Clock::now();
            CmapView cmap(view);
            const auto constructionEnd = Clock::now();

            constructionTime += constructionEnd - constructionStart;

            if (!cmap)
                continue;

            ++cmapFaces;

            const auto lookupStart = Clock::now();

            for (uint32_t cp : kCodepoints)
            {
                const uint32_t glyph = cmap.glyphIndex(cp);

                ++glyphLookups;

                if (glyph != 0)
                    ++glyphHits;
            }

            const auto lookupEnd = Clock::now();

            lookupTime += lookupEnd - lookupStart;
        }

        const auto totalEnd = Clock::now();

        if (cmapFaces == 0)
        {
            std::printf(
                "cmap startup/lookup: FAIL: no readable cmap faces found\n"
                "  Directory: %s\n",
                fontDirectory);

            return false;
        }

        const double constructionMs =
            std::chrono::duration<double, std::milli>(constructionTime).count();

        const double lookupMs =
            std::chrono::duration<double, std::milli>(lookupTime).count();

        const double totalMs =
            std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

        const double constructionUsPerFace =
            1000.0 * constructionMs / static_cast<double>(cmapFaces);

        const double lookupNsPerCall =
            glyphLookups != 0
            ? 1000000.0 * lookupMs / static_cast<double>(glyphLookups)
            : 0.0;

        std::printf(
            "cmap startup/lookup: PASS\n"
            "  Directory:            %s\n"
            "  Faces scanned:        %zu\n"
            "  Cmap faces:           %zu\n"
            "  Glyph lookups:        %zu\n"
            "  Glyph hits:           %zu\n"
            "  Construction:         %.3f ms\n"
            "  Construction / face:  %.3f us\n"
            "  Lookup:               %.3f ms\n"
            "  Lookup / call:        %.3f ns\n"
            "  Total directory pass: %.3f ms\n",
            fontDirectory,
            facesScanned,
            cmapFaces,
            glyphLookups,
            glyphHits,
            constructionMs,
            constructionUsPerFace,
            lookupMs,
            lookupNsPerCall,
            totalMs);

        return true;
    }
}
