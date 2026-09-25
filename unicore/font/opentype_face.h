#pragma once

#include "font_face_view.h"
#include "font_face.h"
#include "font_interfaces.h"

// Table views
#include "opentype_name_view.h"
#include "opentype_cmap_view.h"
#include "unicode_coverage_builder.h"
#include "unicode_coverage_storage.h"
#include "opentype_head_view.h"
#include "opentype_maxp_view.h"

#include <utility>


namespace waavs
{
    class OpenTypeFaceData final : public IProvideFontFaceData, public IProvideOpenTypeTables
    {
    private:
        FontFaceView fView;

        NameView fName;
        CmapView fCmap;
        HeadView fHead;
        MaxpView fMaxp;
        UnicodeCoverageStorage fUnicodeCoverageStorage{};

        FontFaceProperties fProperties{};
        bool fValid{ false };


    public:
        explicit OpenTypeFaceData(FontFaceView view) noexcept
            : fView(std::move(view))
        {
            parse();
        }

        OpenTypeFaceData(FontResource resource, size_t faceOffset) noexcept
            : fView(std::move(resource), faceOffset)
        {
            parse();
        }


        bool isValid() const noexcept
        {
            return fValid;
        }


        FontName sourceLocation() const noexcept override
        {
            return fView.sourceLocation();
        }


        const FontFaceView& view() const noexcept
        {
            return fView;
        }


        FontName familyName() const noexcept override
        {
            return fName.familyName();
        }


        FontName subfamilyName() const noexcept override
        {
            return fName.subfamilyName();
        }


        FontName fullName() const noexcept override
        {
            return fName.fullName();
        }


        FontName postScriptName() const noexcept override
        {
            return fName.postScriptName();
        }


        FontFaceProperties properties() const noexcept override
        {
            return fProperties;
        }


        uint32_t glyphCount() const noexcept override
        {
            return fMaxp.glyphCount();
        }


        uint16_t unitsPerEm() const noexcept override
        {
            return fHead.unitsPerEm();
        }


        uint32_t glyphIndex(uint32_t codepoint) const noexcept override
        {
            return fCmap.glyphIndex(codepoint);
        }


        const UnicodeCoverage& unicodeCoverage() const noexcept override
        {
            return fUnicodeCoverageStorage.coverage();
        }


        // OpenType-specific access

        const TableRecord* getTable(Tag tag) const noexcept override
        {
            return fView.getTable(tag);
        }


        bool hasTable(Tag tag) const noexcept override
        {
            return fView.hasTable(tag);
        }


        const NameView& nameView() const noexcept
        {
            return fName;
        }


        const CmapView& cmapView() const noexcept
        {
            return fCmap;
        }


        const HeadView& headView() const noexcept
        {
            return fHead;
        }


        const MaxpView& maxpView() const noexcept
        {
            return fMaxp;
        }


    private:
        inline bool parse() noexcept
        {
            if (!fView)
                return false;

            if (!parseCoreTables())
                return false;

            fValid = true;
            return true;
        }


        inline bool parseCoreTables() noexcept
        {
            if (fView.hasTable(TagConstants::MAXP) &&
                !fMaxp.reset(fView))
            {
                return false;
            }

            if (fView.hasTable(TagConstants::HEAD) &&
                !fHead.reset(fView))
            {
                return false;
            }

            if (fView.hasTable(TagConstants::NAME) &&
                !fName.reset(fView))
            {
                return false;
            }

            if (fView.hasTable(TagConstants::CMAP))
            {
                if (!fCmap.reset(fView, fMaxp.glyphCount()))
                    return false;

                if (!fCmap.buildCoverage(fUnicodeCoverageStorage))
                    return false;
            }

            return true;
        }
    };


    inline FontFace parseFontFace(FontFaceView view)
    {
        if (!view)
            return {};

        auto data =
            std::make_shared<OpenTypeFaceData>(
                std::move(view));

        if (!data->isValid())
            return {};

        return FontFace(std::move(data));
    }
}