// test_opentype_horizontal_shaper.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

#include "opentype_horizontal_shaper.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendHorizontalShaperU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendHorizontalShaperS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendHorizontalShaperU16(data, static_cast<uint16_t>(value));
    }

    static void appendHorizontalShaperU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchHorizontalShaperU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void patchHorizontalShaperU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Coverage Format 1.
    // ====================================================================

    static void appendHorizontalShaperCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU16(data, glyphId);
    }


    // ====================================================================
    // GSUB Type 1 SingleSubst Format 2.
    //
    // sourceGlyph -> replacementGlyph
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperGsubSingle(uint16_t sourceGlyph, uint16_t replacementGlyph)
    {
        std::vector<uint8_t> data;

        appendHorizontalShaperU16(data, 2);

        const size_t coveragePatch = data.size();
        appendHorizontalShaperU16(data, 0);

        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU16(data, replacementGlyph);

        patchHorizontalShaperU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendHorizontalShaperCoverage(data, sourceGlyph);

        return data;
    }


    // ====================================================================
    // GPOS Type 1 SinglePos Format 1.
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperGposSingle(
        uint16_t glyphId, uint16_t valueFormat, int16_t value)
    {
        std::vector<uint8_t> data;

        appendHorizontalShaperU16(data, 1);

        const size_t coveragePatch = data.size();
        appendHorizontalShaperU16(data, 0);

        appendHorizontalShaperU16(data, valueFormat);
        appendHorizontalShaperS16(data, value);

        patchHorizontalShaperU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendHorizontalShaperCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Generic GSUB/GPOS LookupList entry.
    // ====================================================================

    struct HorizontalShaperLookupSpec
    {
        uint16_t lookupType{ 0 };
        std::vector<uint8_t> subtable{};
    };


    // ====================================================================
    // Complete Layout table.
    //
    // One script:
    //
    //   latn
    //       DefaultLangSys
    //           FeatureIndex 0
    //
    // One feature:
    //
    //   featureTag
    //       LookupList indices 0..N-1
    //
    // This is sufficient to exercise the real Script/LangSys/Feature
    // selector inside shapeOpenTypeHorizontalRun().
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperLayoutTable(
        uint32_t featureTag, const std::vector<HorizontalShaperLookupSpec>& lookups)
    {
        std::vector<uint8_t> data;


        // ------------------------------------------------------------
        // Layout header 1.0.
        // ------------------------------------------------------------

        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU16(data, 0);

        const size_t scriptListPatch = data.size();
        appendHorizontalShaperU16(data, 0);

        const size_t featureListPatch = data.size();
        appendHorizontalShaperU16(data, 0);

        const size_t lookupListPatch = data.size();
        appendHorizontalShaperU16(data, 0);


        // ------------------------------------------------------------
        // ScriptList.
        // ------------------------------------------------------------

        patchHorizontalShaperU16(data, scriptListPatch, static_cast<uint16_t>(data.size()));

        const size_t scriptListBegin = data.size();

        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU32(data, OTAG("latn"));
        appendHorizontalShaperU16(data, 8);


        // Script table.

        const size_t scriptBegin = data.size();

        appendHorizontalShaperU16(data, 4);
        appendHorizontalShaperU16(data, 0);


        // DefaultLangSys.

        appendHorizontalShaperU16(data, 0);
        appendHorizontalShaperU16(data, 0xFFFFu);
        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU16(data, 0);

        (void)scriptListBegin;
        (void)scriptBegin;


        // ------------------------------------------------------------
        // FeatureList.
        // ------------------------------------------------------------

        patchHorizontalShaperU16(data, featureListPatch, static_cast<uint16_t>(data.size()));

        appendHorizontalShaperU16(data, 1);
        appendHorizontalShaperU32(data, featureTag);
        appendHorizontalShaperU16(data, 8);


        // Feature table.

        appendHorizontalShaperU16(data, 0);
        appendHorizontalShaperU16(data, static_cast<uint16_t>(lookups.size()));

        for (uint16_t i = 0; i < lookups.size(); ++i)
            appendHorizontalShaperU16(data, i);


        // ------------------------------------------------------------
        // LookupList.
        // ------------------------------------------------------------

        patchHorizontalShaperU16(data, lookupListPatch, static_cast<uint16_t>(data.size()));

        const size_t lookupListBegin = data.size();

        appendHorizontalShaperU16(data, static_cast<uint16_t>(lookups.size()));

        std::vector<size_t> lookupOffsetPatches;
        lookupOffsetPatches.reserve(lookups.size());

        for (size_t i = 0; i < lookups.size(); ++i)
        {
            lookupOffsetPatches.push_back(data.size());
            appendHorizontalShaperU16(data, 0);
        }

        for (size_t i = 0; i < lookups.size(); ++i)
        {
            const size_t lookupBegin = data.size();

            patchHorizontalShaperU16(
                data, lookupOffsetPatches[i],
                static_cast<uint16_t>(lookupBegin - lookupListBegin));

            appendHorizontalShaperU16(data, lookups[i].lookupType);
            appendHorizontalShaperU16(data, 0);
            appendHorizontalShaperU16(data, 1);
            appendHorizontalShaperU16(data, 8);

            data.insert(data.end(), lookups[i].subtable.begin(), lookups[i].subtable.end());
        }

        return data;
    }


    // ====================================================================
    // Synthetic GSUB.
    //
    // Feature:
    //
    //   liga
    //
    // Lookup 0:
    //
    //   glyph 10 -> glyph 20
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperGsub()
    {
        return makeHorizontalShaperLayoutTable(
            OTAG("liga"),
            {
                {
                    1,
                    makeHorizontalShaperGsubSingle(10, 20)
                }
            });
    }


    // ====================================================================
    // Synthetic GPOS.
    //
    // Feature:
    //
    //   kern
    //
    // Lookup 0:
    //
    //   glyph 20 xAdvance += 50
    //
    // Lookup 1:
    //
    //   glyph 30 xPlacement += 17
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperGpos()
    {
        return makeHorizontalShaperLayoutTable(
            OTAG("kern"),
            {
                {
                    1,
                    makeHorizontalShaperGposSingle(
                        20, 0x0004u, 50)
                },
                {
                    1,
                    makeHorizontalShaperGposSingle(
                        30, 0x0001u, 17)
                }
            });
    }


    // ====================================================================
    // Malformed GPOS.
    //
    // Layout structure is valid enough for feature selection, but the
    // selected Type 1 subtable is truncated.
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperMalformedGpos()
    {
        return makeHorizontalShaperLayoutTable(
            OTAG("kern"),
            {
                {
                    1,
                    std::vector<uint8_t>
                    {
                        0x00, 0x01
                    }
                }
            });
    }


    // ====================================================================
    // hhea.
    //
    // numberOfHMetrics = 64.
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperHhea()
    {
        std::vector<uint8_t> data(36, 0);

        patchHorizontalShaperU32(data, 0, 0x00010000u);
        patchHorizontalShaperU16(data, 34, 64);

        return data;
    }


    // ====================================================================
    // hmtx.
    //
    // All 64 glyphs have an explicit LongHorMetric.
    //
    // Important values:
    //
    //   glyph 10 -> 500
    //   glyph 20 -> 700
    //   glyph 30 -> 600
    //
    // If metrics are loaded BEFORE GSUB, the first output glyph would
    // incorrectly retain 500 rather than receiving glyph 20's 700.
    // ====================================================================

    static std::vector<uint8_t> makeHorizontalShaperHmtx()
    {
        std::vector<uint8_t> data;

        for (uint16_t glyphId = 0; glyphId < 64; ++glyphId)
        {
            uint16_t advance = 400;

            if (glyphId == 10)
                advance = 500;
            else if (glyphId == 20)
                advance = 700;
            else if (glyphId == 30)
                advance = 600;

            appendHorizontalShaperU16(data, advance);
            appendHorizontalShaperS16(data, 0);
        }

        return data;
    }


    // ====================================================================
    // Synthetic face.
    //
    // This avoids manufacturing an entire SFNT container while still using
    // the real FontFace abstraction and the real OpenType table-provider
    // seam used by the production shaping driver.
    //
    // Character mapping:
    //
    //   U+0041 -> glyph 10
    //   U+0042 -> glyph 30
    // ====================================================================

    class HorizontalShaperSyntheticFace final
        : public IProvideFontFaceData,
        public IProvideOpenTypeTables
    {
    public:
        HorizontalShaperSyntheticFace(
            std::vector<uint8_t> hhea,
            std::vector<uint8_t> hmtx,
            std::vector<uint8_t> gsub,
            std::vector<uint8_t> gpos)
            : fHhea(std::move(hhea)),
            fHmtx(std::move(hmtx)),
            fGsub(std::move(gsub)),
            fGpos(std::move(gpos))
        {
            initializeRecord(fHheaRecord, OTAG("hhea"), fHhea);
            initializeRecord(fHmtxRecord, OTAG("hmtx"), fHmtx);

            if (!fGsub.empty())
                initializeRecord(fGsubRecord, OTAG("GSUB"), fGsub);

            if (!fGpos.empty())
                initializeRecord(fGposRecord, OTAG("GPOS"), fGpos);
        }

        FontName sourceLocation() const noexcept { return nullptr; }
        FontName familyName() const noexcept override { return "Synthetic"; }
        FontName subfamilyName() const noexcept override { return "Regular"; }
        FontName fullName() const noexcept override { return "Synthetic Regular"; }
        FontName postScriptName() const noexcept override { return "Synthetic-Regular"; }

        FontFaceProperties properties() const noexcept override
        {
            return {};
        }

        uint32_t glyphCount() const noexcept override { return 64; }
        uint16_t unitsPerEm() const noexcept override { return 1000; }

        uint32_t glyphIndex(uint32_t codepoint) const noexcept override
        {
            switch (codepoint)
            {
            case 0x0041:
                return 10;

            case 0x0042:
                return 30;

            default:
                return 0;
            }
        }

        const UnicodeCoverage& unicodeCoverage() const noexcept override
        {
            static const UnicodeCoverage empty{};
            return empty;
        }

        const TableRecord* getTable(Tag tag) const noexcept override
        {
            if (tag == OTAG("hhea"))
                return &fHheaRecord;

            if (tag == OTAG("hmtx"))
                return &fHmtxRecord;

            if (tag == OTAG("GSUB"))
                return fGsub.empty() ? nullptr : &fGsubRecord;

            if (tag == OTAG("GPOS"))
                return fGpos.empty() ? nullptr : &fGposRecord;

            return nullptr;
        }

        bool hasTable(Tag tag) const noexcept override
        {
            return getTable(tag) != nullptr;
        }

    private:
        static void initializeRecord(
            TableRecord& record, Tag tag,
            const std::vector<uint8_t>& data) noexcept
        {
            record = {};
            record.tag = tag;
            record.length = static_cast<uint32_t>(data.size());
            record.data = ByteSpan(data.data(), data.size());
        }

    private:
        std::vector<uint8_t> fHhea{};
        std::vector<uint8_t> fHmtx{};
        std::vector<uint8_t> fGsub{};
        std::vector<uint8_t> fGpos{};

        TableRecord fHheaRecord{};
        TableRecord fHmtxRecord{};
        TableRecord fGsubRecord{};
        TableRecord fGposRecord{};
    };


    // ====================================================================
    // Synthetic FontRunView.
    // ====================================================================

    static FontRunView makeHorizontalShaperRun(
        const FontFace& face,
        const UnicodeScalar* scalars,
        uint32_t scalarCount,
        uint8_t bidiLevel = 0)
    {
        FontRunView run;

        run.scalars = scalars;
        run.scalarCount = scalarCount;
        run.face = face;
        run.bidiLevel = bidiLevel;
        run.completeCoverage = true;

        return run;
    }


    // ====================================================================
    // Request.
    // ====================================================================

    static OpenTypeHorizontalShapeRequest makeHorizontalShaperRequest()
    {
        static const uint32_t gsubFeatures[] =
        {
            OTAG("liga")
        };

        static const uint32_t gposFeatures[] =
        {
            OTAG("kern")
        };

        OpenTypeHorizontalShapeRequest request;

        request.scriptTag = OTAG("latn");

        request.gsubFeatureTags = gsubFeatures;
        request.gsubFeatureCount = 1;

        request.gposFeatureTags = gposFeatures;
        request.gposFeatureCount = 1;

        return request;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeHorizontalShaper()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType horizontal shaper: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        UnicodeScalar scalars[2]{};
        scalars[0].value = 0x0041;
        scalars[1].value = 0x0042;


        // ================================================================
        // Case 1 - Complete pipeline.
        //
        // cmap:
        //
        //   A B
        //   |
        //   v
        //   10 30
        //
        // GSUB liga:
        //
        //   10 -> 20
        //
        //   result:
        //
        //   20 30
        //
        // hmtx:
        //
        //   glyph 10 = 500
        //   glyph 20 = 700
        //   glyph 30 = 600
        //
        // Because metrics happen after GSUB:
        //
        //   20 -> advance 700
        //   30 -> advance 600
        //
        // GPOS kern:
        //
        //   glyph 20 xAdvance   += 50
        //   glyph 30 xPlacement += 17
        //
        // Final:
        //
        //   glyph 20: advanceX 750, offsetX 0
        //   glyph 30: advanceX 600, offsetX 17
        // ================================================================

        {
            ++cases;

            auto faceData =
                std::make_shared<HorizontalShaperSyntheticFace>(
                    makeHorizontalShaperHhea(),
                    makeHorizontalShaperHmtx(),
                    makeHorizontalShaperGsub(),
                    makeHorizontalShaperGpos());

            const FontFace face(faceData);
            const FontRunView run =
                makeHorizontalShaperRun(face, scalars, 2);

            const OpenTypeHorizontalShapeRequest request =
                makeHorizontalShaperRequest();

            ShapedGlyphBuffer output;

            if (!shapeOpenTypeHorizontalRun(
                run, request, output))
            {
                return fail("case 1 shape");
            }

            if (output.size() != 2)
                return fail("case 1 output count");

            if (output[0].shaping.glyphId != 20 ||
                output[1].shaping.glyphId != 30)
            {
                return fail("case 1 cmap/GSUB result");
            }

            if (output[0].placement.advanceX != 750 ||
                output[1].placement.advanceX != 600)
            {
                return fail("case 1 post-GSUB metrics/GPOS advance");
            }

            if (output[0].placement.offsetX != 0 ||
                output[1].placement.offsetX != 17 ||
                output[0].placement.offsetY != 0 ||
                output[1].placement.offsetY != 0)
            {
                return fail("case 1 GPOS placement");
            }

            if (output[0].shaping.scalarOffset != 0 ||
                output[0].shaping.scalarCount != 1 ||
                output[1].shaping.scalarOffset != 1 ||
                output[1].shaping.scalarCount != 1)
            {
                return fail("case 1 provenance");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Layout tables absent.
        //
        // A font with no GSUB and no GPOS is still shapeable:
        //
        //   cmap -> hmtx
        //
        // Expected:
        //
        //   glyph 10 -> 500
        //   glyph 30 -> 600
        // ================================================================

        {
            ++cases;

            auto faceData =
                std::make_shared<HorizontalShaperSyntheticFace>(
                    makeHorizontalShaperHhea(),
                    makeHorizontalShaperHmtx(),
                    std::vector<uint8_t>{},
                    std::vector<uint8_t>{});

            const FontFace face(faceData);
            const FontRunView run =
                makeHorizontalShaperRun(face, scalars, 2);

            const OpenTypeHorizontalShapeRequest request =
                makeHorizontalShaperRequest();

            ShapedGlyphBuffer output;

            if (!shapeOpenTypeHorizontalRun(
                run, request, output))
            {
                return fail("case 2 shape");
            }

            if (output.size() != 2 ||
                output[0].shaping.glyphId != 10 ||
                output[1].shaping.glyphId != 30)
            {
                return fail("case 2 nominal glyphs");
            }

            if (output[0].placement.advanceX != 500 ||
                output[1].placement.advanceX != 600 ||
                output[0].placement.offsetX != 0 ||
                output[1].placement.offsetX != 0)
            {
                return fail("case 2 nominal metrics");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Layout tables present but requested script absent.
        //
        // Tables contain only latn. Request arab with no DFLT table.
        //
        // Layout selection returns NoScript, which the whole driver treats
        // as a valid no-op.
        //
        // Result is therefore cmap + nominal metrics.
        // ================================================================

        {
            ++cases;

            auto faceData =
                std::make_shared<HorizontalShaperSyntheticFace>(
                    makeHorizontalShaperHhea(),
                    makeHorizontalShaperHmtx(),
                    makeHorizontalShaperGsub(),
                    makeHorizontalShaperGpos());

            const FontFace face(faceData);
            const FontRunView run =
                makeHorizontalShaperRun(face, scalars, 2);

            OpenTypeHorizontalShapeRequest request =
                makeHorizontalShaperRequest();

            request.scriptTag = OTAG("arab");

            ShapedGlyphBuffer output;

            if (!shapeOpenTypeHorizontalRun(
                run, request, output))
            {
                return fail("case 3 shape");
            }

            if (output.size() != 2 ||
                output[0].shaping.glyphId != 10 ||
                output[1].shaping.glyphId != 30)
            {
                return fail("case 3 layout no-op glyphs");
            }

            if (output[0].placement.advanceX != 500 ||
                output[1].placement.advanceX != 600 ||
                output[0].placement.offsetX != 0 ||
                output[1].placement.offsetX != 0)
            {
                return fail("case 3 layout no-op placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Whole-driver transactional failure.
        //
        // cmap succeeds.
        // GSUB succeeds.
        // metrics succeed.
        // GPOS feature selection succeeds.
        // GPOS execution then encounters a malformed selected subtable.
        //
        // The caller's existing output must remain untouched.
        // ================================================================

        {
            ++cases;

            auto faceData =
                std::make_shared<HorizontalShaperSyntheticFace>(
                    makeHorizontalShaperHhea(),
                    makeHorizontalShaperHmtx(),
                    makeHorizontalShaperGsub(),
                    makeHorizontalShaperMalformedGpos());

            const FontFace face(faceData);
            const FontRunView run =
                makeHorizontalShaperRun(face, scalars, 2);

            const OpenTypeHorizontalShapeRequest request =
                makeHorizontalShaperRequest();

            ShapedGlyphBuffer output;

            ShapedGlyph sentinel{};
            sentinel.shaping.glyphId = 999;
            sentinel.shaping.scalarOffset = 77;
            sentinel.shaping.scalarCount = 3;
            sentinel.placement.advanceX = 1234;
            sentinel.placement.offsetX = 567;

            output.pushBack(sentinel);

            if (shapeOpenTypeHorizontalRun(
                run, request, output))
            {
                return fail("case 4 malformed GPOS accepted");
            }

            if (output.size() != 1 ||
                output[0].shaping.glyphId != 999 ||
                output[0].shaping.scalarOffset != 77 ||
                output[0].shaping.scalarCount != 3 ||
                output[0].placement.advanceX != 1234 ||
                output[0].placement.offsetX != 567)
            {
                return fail("case 4 whole-driver rollback");
            }

            ++passed;
        }


        std::printf(
            "OpenType horizontal shaper: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Nominal cmap stage:         PASS\n"
            "  GSUB integration:           PASS\n"
            "  Post-GSUB metrics:          PASS\n"
            "  GPOS integration:           PASS\n"
            "  Provenance preservation:    PASS\n"
            "  Optional layout tables:     PASS\n"
            "  Missing-script no-op:       PASS\n"
            "  Whole-driver rollback:      PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs