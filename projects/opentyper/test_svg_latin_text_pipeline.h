// test_svg_latin_text_pipeline.h
#pragma once

#include "test_core.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "font_interfaces.h"
#include "font_run_itemizer.h"

#include "opentype_container.h"
#include "opentype_glyf.h"
#include "opentype_horizontal_shaper.h"

#include "svg_text_run_adapter.h"

#include "unicode_bidi_analysis.h"
#include "unicode_grapheme_stream.h"
#include "unicode_script_analysis.h"
#include "unicode_shaping_run_itemizer.h"

namespace waavs
{
    // ====================================================================
    // SVGFullTextAsciiGraphemeSource
    //
    // The fixed test string is ASCII, so UTF-8 decoding, NFC normalization,
    // and grapheme segmentation are identity operations here.
    //
    // This supplies their exact output shape while everything from Script
    // analysis onward uses the real production pipeline.
    // ====================================================================

    class SVGFullTextAsciiGraphemeSource
    {
    public:
        explicit SVGFullTextAsciiGraphemeSource(const char* text) noexcept
            : mText(text), mLength(text ? std::strlen(text) : 0)
        {
            if (!text || mLength > std::numeric_limits<uint32_t>::max())
                mStatus = TextStreamStatus::InvalidInput;
        }

        [[nodiscard]] TextStreamStatus status() const noexcept { return mStatus; }

        bool operator()(GraphemeClusterView& out)
        {
            out = {};

            if (mStatus != TextStreamStatus::Ready)
                return false;

            if (mIndex >= mLength)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }

            const uint8_t value = static_cast<uint8_t>(mText[mIndex]);

            if (value >= 0x80u)
            {
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }

            const SourceRange source{
                static_cast<TextOffset>(mIndex),
                static_cast<TextOffset>(mIndex + 1u)
            };

            mScalar = {};
            mScalar.value = value;
            mScalar.source = source;

            out.scalars = &mScalar;
            out.scalarCount = 1;
            out.normalizedBegin = static_cast<ScalarIndex>(mIndex);
            out.source = source;

            ++mIndex;
            return true;
        }

    private:
        const char* mText{ nullptr };
        size_t mLength{ 0 };
        size_t mIndex{ 0 };
        UnicodeScalar mScalar{};
        TextStreamStatus mStatus{ TextStreamStatus::Ready };
    };


    // ====================================================================
    // SVGFullTextFontProvider
    //
    // Single-face candidate provider for FontRunItemizer.
    //
    // This deliberately avoids the old FontFaceSet machinery.
    // ====================================================================

    class SVGFullTextFontProvider final : public IProvideFontFaces
    {
    public:
        explicit SVGFullTextFontProvider(const FontFace& face) noexcept
            : mFace(face)
        {}

        [[nodiscard]] size_t fontFaceCount() const noexcept override
        {
            return mFace ? 1u : 0u;
        }

        [[nodiscard]] FontFace fontFace(size_t index) const noexcept override
        {
            return index == 0 && mFace ? mFace : FontFace{};
        }

    private:
        FontFace mFace{};
    };


    // ====================================================================
    // makeSVGFullTextGlyfDecoder
    // ====================================================================

    static bool makeSVGFullTextGlyfDecoder(const FontFace& face, OpenTypeGlyfDecoder& decoder)
    {
        decoder = {};

        if (!face)
            return false;

        const auto* tables = dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

        if (!tables)
            return false;

        const TableRecord* glyf = tables->getTable(TagConstants::GLYF);
        const TableRecord* loca = tables->getTable(TagConstants::LOCA);
        const TableRecord* head = tables->getTable(TagConstants::HEAD);

        if (!glyf || !loca || !head || head->data.size() < 52)
            return false;

        OpenTypeByteStream stream(head->data);

        if (!stream.seek(50))
            return false;

        int16_t locaFormat = 0;

        if (!stream.readInt16(locaFormat))
            return false;

        decoder = OpenTypeGlyfDecoder(glyf->data, loca->data, face.glyphCount(), locaFormat);
        return decoder.isValid();
    }


    // ====================================================================
    // readSVGFullTextNominalAdvance
    //
    // Read the post-GSUB glyph's nominal hmtx advance without GPOS.
    // ====================================================================

    static bool readSVGFullTextNominalAdvance(const IProvideOpenTypeTables& tables,
        uint32_t glyphId, uint32_t glyphCount, uint16_t& advance)
    {
        advance = 0;

        if (glyphCount == 0 || glyphId >= glyphCount)
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
    // svgFullTextFaceSupportsText
    // ====================================================================

    static bool svgFullTextFaceSupportsText(const FontFace& face, const char* text)
    {
        if (!face || !text)
            return false;

        const auto* p = reinterpret_cast<const unsigned char*>(text);

        while (*p)
        {
            if (*p >= 0x80u || face.glyphIndex(*p) == 0)
                return false;

            ++p;
        }

        return true;
    }


    // ====================================================================
    // findSVGFullTextFace
    //
    // For the current test, LiberationSans-Regular.ttf should satisfy this
    // immediately.
    // ====================================================================

    static bool findSVGFullTextFace(ByteSpan fontData, const char* text,
        FontFace& face, OpenTypeGlyfDecoder& decoder, SharedMemBuff& backing)
    {
        face = {};
        decoder = {};
        backing.reset();

        if (fontData.empty() || !text)
            return false;

        if (!backing.resetFromSize(fontData.size()))
            return false;

        std::memcpy(backing.data(), fontData.data(), fontData.size());

        OpenTypeContainer container(backing);

        if (!container.isValid())
            return false;

        FontFace candidate;

        while (container(candidate))
        {
            if (!candidate || candidate.unitsPerEm() == 0 ||
                !svgFullTextFaceSupportsText(candidate, text))
            {
                continue;
            }

            const auto* tables =
                dynamic_cast<const IProvideOpenTypeTables*>(candidate.operator->());

            if (!tables || !tables->getTable(TagConstants::GPOS))
                continue;

            OpenTypeGlyfDecoder candidateDecoder;

            if (!makeSVGFullTextGlyfDecoder(candidate, candidateDecoder))
                continue;

            face = candidate;
            decoder = candidateDecoder;
            return true;
        }

        return false;
    }


    // ====================================================================
    // testSVGLatinTextPipeline
    //
    //     AVATAR To Waavs is fine
    //
    // Pipeline:
    //
    //     ASCII post-grapheme stream
    //       -> Script analysis
    //       -> bidi analysis
    //       -> ShapingRunView
    //       -> font selection / FontRunView
    //       -> cmap
    //       -> GSUB
    //       -> hmtx
    //       -> GPOS
    //       -> ShapedGlyphView
    //       -> size scaling / pen positioning
    //       -> SVG
    //
    // Top row:
    //
    //     normal shaping + GPOS
    //
    // Bottom row:
    //
    //     same final post-GSUB glyph IDs, but nominal hmtx placement
    //
    // Thus GSUB results such as an fi ligature remain identical between
    // rows. Only GPOS positioning is removed from the second row.
    // ====================================================================

    static bool testSVGLatinTextPipeline(const ByteSpan& databaseData,
        ByteSpan fontData, std::string* svgOutput = nullptr)
    {
        static constexpr char kText[] = "AVATAR To Waavs is fine";

        auto fail = [](const char* message)
            {
                std::printf("SVG Latin text pipeline: FAIL: %s\n", message);
                return false;
            };


        // ================================================================
        // Unicode database
        // ================================================================

        UnicodeDatabase database(databaseData);

        if (!database.valid())
            return fail("unable to attach Unicode database");


        // ================================================================
        // Font
        // ================================================================

        SharedMemBuff fontBacking;
        FontFace face;
        OpenTypeGlyfDecoder decoder;

        if (!findSVGFullTextFace(fontData, kText, face, decoder, fontBacking))
            return fail("font does not provide required Latin glyf/GPOS support");

        const auto* tables =
            dynamic_cast<const IProvideOpenTypeTables*>(face.operator->());

        if (!tables)
            return fail("selected face does not provide OpenType tables");


        // ================================================================
        // Single font candidate provider
        // ================================================================

        SVGFullTextFontProvider candidates(face);


        // ================================================================
        // Grapheme -> Script
        // ================================================================

        SVGFullTextAsciiGraphemeSource graphemes(kText);
        UnicodeScriptStream<SVGFullTextAsciiGraphemeSource> scriptStream(graphemes, database);

        if (scriptStream.status() == TextStreamStatus::InvalidInput)
            return fail("unable to initialize Script stream");


        // ================================================================
        // Script -> bidi paragraph
        // ================================================================

        UnicodeBidiStream<decltype(scriptStream)> bidiStream(scriptStream, database);

        if (bidiStream.failed())
            return fail("unable to initialize bidi stream");

        BidiParagraphView paragraph{};

        if (!bidiStream(paragraph))
            return fail("unable to produce bidi paragraph");

        const size_t textLength = std::strlen(kText);

        if (paragraph.scalarCount != textLength)
            return fail("unexpected paragraph scalar count");

        if (!paragraph.leftToRight())
            return fail("Latin paragraph did not resolve LTR");


        // ================================================================
        // SVG state
        // ================================================================

        constexpr double fontSize = 96.0;
        constexpr double originX = 40.0;
        constexpr double shapedBaselineY = 125.0;
        constexpr double nominalBaselineY = 285.0;

        SVGTextBackend backend;

        double shapedPenX = originX;
        double nominalPenX = originX;

        size_t shapingRunCount = 0;
        size_t fontRunCount = 0;
        size_t shapedGlyphCount = 0;
        size_t adjustedGlyphCount = 0;
        size_t ligatureGlyphCount = 0;

        int64_t shapedAdvanceDesign = 0;
        int64_t nominalAdvanceDesign = 0;


        // ================================================================
        // Bidi paragraph -> shaping runs
        // ================================================================

        ShapingRunItemizer shapingRuns(paragraph, database);

        if (shapingRuns.failed())
            return fail("unable to initialize shaping-run itemizer");

        ShapingRunView shapingRun;

        while (shapingRuns(shapingRun))
        {
            ++shapingRunCount;

            if (shapingRun.rightToLeft())
                return fail("unexpected RTL shaping run");


            // ============================================================
            // Shaping run -> font runs
            //
            // FontRunItemizer requires IProvideFontFaces. The test-local
            // SVGFullTextFontProvider supplies exactly that interface.
            // ============================================================

            FontRunItemizer fontRuns(shapingRun, candidates, database);

            if (fontRuns.failed())
                return fail("unable to initialize font-run itemizer");

            FontRunView fontRun;

            while (fontRuns(fontRun))
            {
                ++fontRunCount;

                if (!fontRun.completeCoverage)
                    return fail("test font does not completely cover text");

                if (fontRun.face != face)
                    return fail("unexpected fallback face");

                if ((fontRun.bidiLevel & 1u) != 0)
                    return fail("unexpected RTL font run");


                // ========================================================
                // FontRunView -> real OpenType shaping
                // ========================================================

                ShapedGlyphBuffer shaped;

                if (!shapeOpenTypeHorizontalRun(
                    fontRun, OTAG("latn"), 0, shaped))
                {
                    return fail("OpenType shaping failed");
                }

                if (shaped.empty())
                    return fail("shaping produced no glyphs");

                shapedGlyphCount += shaped.size();


                // ========================================================
                // Build nominal comparison from final post-GSUB glyph IDs.
                // ========================================================

                ShapedGlyphBuffer nominal = shaped;

                for (size_t i = 0; i < shaped.size(); ++i)
                {
                    uint16_t nominalAdvance = 0;

                    if (!readSVGFullTextNominalAdvance(
                        *tables,
                        shaped[i].shaping.glyphId,
                        face.glyphCount(),
                        nominalAdvance))
                    {
                        return fail("unable to read nominal glyph advance");
                    }

                    shapedAdvanceDesign += shaped[i].placement.advanceX;
                    nominalAdvanceDesign += nominalAdvance;

                    if (shaped[i].placement.advanceX !=
                        static_cast<int32_t>(nominalAdvance) ||
                        shaped[i].placement.advanceY != 0 ||
                        shaped[i].placement.offsetX != 0 ||
                        shaped[i].placement.offsetY != 0)
                    {
                        ++adjustedGlyphCount;
                    }

                    if (shaped[i].shaping.scalarCount > 1)
                        ++ligatureGlyphCount;

                    nominal[i].placement.advanceX =
                        static_cast<int32_t>(nominalAdvance);

                    nominal[i].placement.advanceY = 0;
                    nominal[i].placement.offsetX = 0;
                    nominal[i].placement.offsetY = 0;
                }


                // ========================================================
                // Shaped + GPOS row
                // ========================================================

                ShapedGlyphView shapedView(shaped);
                HorizontalGlyphPositioningResult shapedResult{};

                if (!emitHorizontalLTRSVGRun(
                    fontRun,
                    shapedView,
                    decoder,
                    fontSize,
                    shapedPenX,
                    shapedBaselineY,
                    backend,
                    &shapedResult))
                {
                    return fail("unable to emit shaped SVG run");
                }

                shapedPenX = shapedResult.penX;


                // ========================================================
                // Nominal comparison row
                // ========================================================

                ShapedGlyphView nominalView(nominal);
                HorizontalGlyphPositioningResult nominalResult{};

                if (!emitHorizontalLTRSVGRun(
                    fontRun,
                    nominalView,
                    decoder,
                    fontSize,
                    nominalPenX,
                    nominalBaselineY,
                    backend,
                    &nominalResult))
                {
                    return fail("unable to emit nominal SVG run");
                }

                nominalPenX = nominalResult.penX;
            }

            if (!fontRuns.ended())
                return fail("font-run itemization failed");
        }

        if (!shapingRuns.ended())
            return fail("shaping-run itemization failed");


        // ================================================================
        // With one Latin face this should remain one shaping run and one
        // font run despite the spaces.
        // ================================================================

        if (shapingRunCount != 1)
            return fail("expected exactly one shaping run");

        if (fontRunCount != 1)
            return fail("expected exactly one font run");


        // ================================================================
        // GPOS evidence
        // ================================================================

        if (adjustedGlyphCount == 0)
            return fail("GPOS produced no placement changes");

        if (shapedAdvanceDesign == nominalAdvanceDesign)
            return fail("GPOS produced no total horizontal advance change");

        if (!(shapedPenX < nominalPenX))
            return fail("shaped run was not narrower than nominal run");


        // ================================================================
        // Verify design units -> layout units.
        // ================================================================

        const double scale =
            fontSize / static_cast<double>(face.unitsPerEm());

        const int64_t advanceDeltaDesign =
            shapedAdvanceDesign - nominalAdvanceDesign;

        const double expectedLayoutDelta =
            static_cast<double>(advanceDeltaDesign) * scale;

        const double measuredLayoutDelta =
            shapedPenX - nominalPenX;

        if (std::fabs(expectedLayoutDelta - measuredLayoutDelta) > 1.0e-7)
            return fail("GPOS design-unit delta did not survive composition");


        // ================================================================
        // Verify clean bidi EOF.
        //
        // Pulling again invalidates paragraph, so do it only after all
        // downstream consumers are finished.
        // ================================================================

        BidiParagraphView extraParagraph{};

        if (bidiStream(extraParagraph))
            return fail("unexpected second bidi paragraph");

        if (!bidiStream.ended())
            return fail("bidi stream did not end cleanly");


        // ================================================================
        // SVG
        // ================================================================

        const double maxPenX =
            shapedPenX > nominalPenX ? shapedPenX : nominalPenX;

        const float documentWidth =
            static_cast<float>(maxPenX + 50.0);

        constexpr float documentHeight = 350.0f;

        const std::string svg =
            backend.document(0.0f, 0.0f, documentWidth, documentHeight);

        if (svg.empty())
            return fail("generated SVG is empty");

        if (svg.find("<svg") == std::string::npos ||
            svg.find("<path") == std::string::npos ||
            svg.find("<use") == std::string::npos)
        {
            return fail("generated document is missing SVG content");
        }

        if (backend.glyphDefinitionCount() == 0)
            return fail("no glyph definitions were generated");

        if (svgOutput)
            *svgOutput = svg;


        // ================================================================
        // Diagnostics
        // ================================================================

        const char* faceName = face.fullName();

        if (!faceName)
            faceName = face.familyName();

        if (!faceName)
            faceName = "(unnamed)";

        std::printf(
            "SVG Latin text pipeline: PASS\n"
            "  Text:                  %s\n"
            "  Face:                  %s\n"
            "  Scalars:               %zu\n"
            "  Shaping runs:          %zu\n"
            "  Font runs:             %zu\n"
            "  Shaped glyphs:         %zu\n"
            "  Ligature glyphs:       %zu\n"
            "  GPOS-adjusted glyphs:  %zu\n"
            "  Units per em:          %u\n"
            "  Font size:             %.2f\n"
            "  Scale:                 %.6f\n"
            "  Nominal advance:       %lld design units\n"
            "  Shaped advance:        %lld design units\n"
            "  GPOS advance delta:    %lld design units\n"
            "  GPOS layout delta:     %.4f\n"
            "  Shaped final pen:      %.4f\n"
            "  Nominal final pen:     %.4f\n"
            "  Glyph definitions:     %zu\n"
            "  SVG bytes:             %zu\n"
            "  Top row:               shaped + GPOS\n"
            "  Bottom row:            same GSUB glyphs + nominal hmtx\n",
            kText,
            faceName,
            textLength,
            shapingRunCount,
            fontRunCount,
            shapedGlyphCount,
            ligatureGlyphCount,
            adjustedGlyphCount,
            static_cast<unsigned>(face.unitsPerEm()),
            fontSize,
            scale,
            static_cast<long long>(nominalAdvanceDesign),
            static_cast<long long>(shapedAdvanceDesign),
            static_cast<long long>(advanceDeltaDesign),
            measuredLayoutDelta,
            shapedPenX,
            nominalPenX,
            backend.glyphDefinitionCount(),
            svg.size());

        return true;
    }


    // ====================================================================
    // Filename convenience overload
    // ====================================================================

    static bool testSVGLatinTextPipeline(const char* databaseFilename,
        const char* fontFilename,
        const char* svgFilename = "test_svg_latin_text_pipeline.svg")
    {
        if (!databaseFilename || !*databaseFilename ||
            !fontFilename || !*fontFilename)
        {
            std::printf(
                "SVG Latin text pipeline: FAIL: missing input filename\n");

            return false;
        }

        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(databaseFilename, databaseBytes))
        {
            std::printf(
                "SVG Latin text pipeline: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "SVG Latin text pipeline: FAIL: unable to read font\n"
                "  File: %s\n",
                fontFilename);

            return false;
        }

        const ByteSpan databaseData(databaseBytes.data(), databaseBytes.size());
        const ByteSpan fontData(fontBytes.data(), fontBytes.size());

        std::string svg;

        if (!testSVGLatinTextPipeline(databaseData, fontData, &svg))
            return false;

        if (!svgFilename || !*svgFilename)
            return true;

        std::ofstream output(svgFilename, std::ios::binary);

        if (!output)
        {
            std::printf(
                "SVG Latin text pipeline: FAIL: unable to create SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        output.write(svg.data(), static_cast<std::streamsize>(svg.size()));

        if (!output)
        {
            std::printf(
                "SVG Latin text pipeline: FAIL: unable to write SVG\n"
                "  File: %s\n",
                svgFilename);

            return false;
        }

        std::printf("  SVG file:              %s\n", svgFilename);
        return true;
    }

} // namespace waavs