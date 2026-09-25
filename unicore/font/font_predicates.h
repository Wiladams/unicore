
#pragma once

#include "core_nametable.h"

#include "font_face.h"
#include "font_face_view.h"
#include "font_faceset.h"

#include "opentype_cmap_view.h"
#include "opentype_head_view.h"
#include "opentype_maxp_view.h"
#include "opentype_name_view.h"

#include "unicode_coverage.h"

#include <functional>
#include <utility>


namespace waavs
{
    // ================================================================
    // Font query predicate type
    //
    // Font discovery, filtering, sorting, grouping, and cataloging
    // operate on FontFaceView. Promotion to FontFace happens only at
    // an explicit materializing terminal.
    // ================================================================

    using FontFaceViewPredFn = std::function<bool(const FontFaceView&)>;


    // ================================================================
    // Intern external name text once, during predicate construction.
    // ================================================================

    WG_NODISCARD
        static FontName internName(const char* name) noexcept
    {
        if (!name || !*name)
            return FontName{};

        return WSNameSet::INTERN(name);
    }


    // ================================================================
    // Interned name matching
    //
    // NameView returns names interned through WSNameSet.
    //
    // The const char* overload interns the query string once when the
    // predicate is constructed. Matching is then an interned-name
    // identity comparison rather than a repeated string comparison.
    // ================================================================

    // ------------------------------------------------
    inline FontFaceViewPredFn familyInterned(FontName name)
    {
        return [name](const FontFaceView& face) noexcept
            {
                NameView names(face);
                return names && names.familyName() == name;
            };
    }

    inline FontFaceViewPredFn familyName(const char* name)
    {
        return familyInterned(internName(name));
    }


    // ------------------------------------------------
    inline FontFaceViewPredFn subfamilyInterned(FontName name)
    {
        return [name](const FontFaceView& face) noexcept
            {
                NameView names(face);
                return names && names.subfamilyName() == name;
            };
    }

    inline FontFaceViewPredFn subfamily(const char* name)
    {
        return subfamilyInterned(internName(name));
    }


    // ------------------------------------------------
    inline FontFaceViewPredFn fullNameInterned(FontName name)
    {
        return [name](const FontFaceView& face) noexcept
            {
                NameView names(face);
                return names && names.fullName() == name;
            };
    }

    inline FontFaceViewPredFn fullName(const char* name)
    {
        return fullNameInterned(internName(name));
    }


    // ------------------------------------------------
    inline FontFaceViewPredFn postScriptNameInterned(FontName name)
    {
        return [name](const FontFaceView& face) noexcept
            {
                NameView names(face);
                return names && names.postScriptName() == name;
            };
    }

    inline FontFaceViewPredFn postScriptName(const char* name)
    {
        return postScriptNameInterned(internName(name));
    }


    // ================================================================
    // Basic face properties
    // ================================================================

    inline FontFaceViewPredFn glyphCount(uint32_t count)
    {
        return [count](const FontFaceView& face) noexcept
            {
                MaxpView maxp(face);
                return maxp && maxp.glyphCount() == count;
            };
    }


    inline FontFaceViewPredFn minGlyphCount(uint32_t count)
    {
        return [count](const FontFaceView& face) noexcept
            {
                MaxpView maxp(face);
                return maxp && maxp.glyphCount() >= count;
            };
    }


    inline FontFaceViewPredFn maxGlyphCount(uint32_t count)
    {
        return [count](const FontFaceView& face) noexcept
            {
                MaxpView maxp(face);
                return maxp && maxp.glyphCount() <= count;
            };
    }


    // ------------------------------------------------
    inline FontFaceViewPredFn unitsPerEm(uint16_t value)
    {
        return [value](const FontFaceView& face) noexcept
            {
                HeadView head(face);
                return head && head.unitsPerEm() == value;
            };
    }


    // ================================================================
    // Table presence
    // ================================================================

    inline FontFaceViewPredFn hasTable(Tag tag)
    {
        return [tag](const FontFaceView& face) noexcept
            {
                return face.hasTable(tag);
            };
    }


    // ================================================================
    // Unicode coverage
    //
    // UnicodeCoverage is a non-owning immutable view. Capturing it by
    // value copies the view, not the underlying page storage. The
    // storage backing the required coverage must therefore remain alive
    // for the lifetime of the predicate/query.
    // ================================================================

    inline FontFaceViewPredFn covers(uint32_t codepoint)
    {
        return [codepoint](const FontFaceView& face) noexcept
            {
                CmapView cmap(face);

                return
                    cmap &&
                    cmap.unicodeCoverage().contains(codepoint);
            };
    }


    inline FontFaceViewPredFn covers(UnicodeCoverage required)
    {
        return [required](const FontFaceView& face) noexcept
            {
                CmapView cmap(face);

                return
                    cmap &&
                    cmap.unicodeCoverage().containsAll(required);
            };
    }


    inline FontFaceViewPredFn intersects(UnicodeCoverage coverage)
    {
        return [coverage](const FontFaceView& face) noexcept
            {
                CmapView cmap(face);

                return
                    cmap &&
                    cmap.unicodeCoverage().intersects(coverage);
            };
    }


    // ================================================================
    // Structural terminals
    // ================================================================

    // ------------------------------------------------
    // Return the first matching FontFaceView without promoting it.
    // ------------------------------------------------

    template<typename Source>
    FontFaceView findFirstViewIn(Source source)
    {
        FontFaceView view;

        if (source(view))
            return view;

        return {};
    }


    struct FirstViewOp {};

    inline constexpr FirstViewOp firstView;


    template<typename Source>
    FontFaceView operator|(Source source, FirstViewOp)
    {
        return findFirstViewIn(std::move(source));
    }


    // ================================================================
    // Materializing terminals
    //
    // These are the boundary where structural FontFaceView objects are
    // promoted into parsed FontFace objects.
    // ================================================================

    // ------------------------------------------------
    // Gather all matching views into a concrete FontFaceSet.
    //
    // Filtering remains in the structural/view layer. Only surviving
    // faces are promoted through parseFontFace().
    // ------------------------------------------------

    template<typename Source>
    FontFaceSet gatherFrom(Source source)
    {
        FontFaceSet result;
        FontFaceView view;

        while (source(view))
        {
            FontFace face = parseFontFace(std::move(view));

            if (face)
                result.addFontFace(std::move(face));
        }

        return result;
    }


    struct GatherOp {};

    inline constexpr GatherOp gather;


    template<typename Source>
    FontFaceSet operator|(Source source, GatherOp)
    {
        return gatherFrom(std::move(source));
    }


    // ------------------------------------------------
    // Return the first matching face promoted to FontFace.
    //
    // A structurally valid FontFaceView may still fail promotion, so
    // continue until a valid FontFace is produced or the source ends.
    // ------------------------------------------------

    template<typename Source>
    FontFace findFirstIn(Source source)
    {
        FontFaceView view;

        while (source(view))
        {
            FontFace face = parseFontFace(std::move(view));

            if (face)
                return face;
        }

        return {};
    }


    struct FirstOp {};

    inline constexpr FirstOp first;


    template<typename Source>
    FontFace operator|(Source source, FirstOp)
    {
        return findFirstIn(std::move(source));
    }
}
