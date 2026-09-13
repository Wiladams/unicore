// test_svg_glyph_definition_cache.h
#pragma once

#include "test_core.h"
#include "svg_glyph_definition_cache.h"
#include "font_directory_view.h"

namespace waavs
{
    static bool testSVGGlyphDefinitionCache(const FontFace& fontA, const FontFace& fontB)
    {
        if (!fontA || !fontB || fontA == fontB)
            return false;

        SVGGlyphDefinitionCache cache;
        SVGGlyphDefinition def;

        // Nothing exists initially.
        if (cache.lookup(fontA, 42, def))
            return false;

        // Preparing assigns f0 but does not commit glyph 42.
        if (!cache.prepare(fontA, 42, def))
            return false;

        if (def.fontIndex != 0 || def.glyphId != 42 || def.id() != "f0g42")
            return false;

        if (cache.fontCount() != 1 || cache.glyphCount() != 0)
            return false;

        if (cache.contains(fontA, 42))
            return false;

        // Successful SVG definition emission would happen here.

        if (!cache.commit(def))
            return false;

        if (!cache.contains(fontA, 42))
            return false;

        if (cache.fontCount() != 1 || cache.glyphCount() != 1)
            return false;

        if (!cache.lookup(fontA, 42, def))
            return false;

        if (def.id() != "f0g42")
            return false;

        // Same font, another glyph.
        if (!cache.prepare(fontA, 57, def))
            return false;

        if (def.id() != "f0g57")
            return false;

        if (!cache.commit(def))
            return false;

        // Different font, same numeric glyph ID.
        if (!cache.prepare(fontB, 42, def))
            return false;

        if (def.fontIndex != 1 || def.id() != "f1g42")
            return false;

        if (!cache.commit(def))
            return false;

        if (cache.fontCount() != 2 || cache.glyphCount() != 3)
            return false;

        // Duplicate commit is an error.
        if (cache.commit(def))
            return false;

        const FontFace* cachedA = cache.font(0);
        const FontFace* cachedB = cache.font(1);

        if (!cachedA || !cachedB)
            return false;

        if (*cachedA != fontA || *cachedB != fontB)
            return false;

        cache.clear();

        if (cache.fontCount() != 0 || cache.glyphCount() != 0)
            return false;

        return true;
    }

    static bool testSVGGlyphDefinitionCache(const char* fontDirectory)
    {
        FontDirectoryView fonts(fontDirectory);

        FontFace fontA;
        FontFace fontB;

        if (!fonts(fontA)) {
            std::printf("SVG glyph definition cache: FAIL: unable to find first font face\n");
            return false;
        }

        while (fonts(fontB)) {
            if (fontB != fontA)
                return testSVGGlyphDefinitionCache(fontA, fontB);
        }

        std::printf("SVG glyph definition cache: FAIL: unable to find second distinct font face\n");
        return false;
    }
} // namespace waavs