// test_opentype_nominal_metrics_integration.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "opentype_nominal_metrics.h"

namespace waavs
{
    static void appendNominalIntegrationU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendNominalIntegrationS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendNominalIntegrationU16(data, static_cast<uint16_t>(value));
    }


    static std::vector<uint8_t> makeNominalIntegrationHhea(uint16_t numberOfHMetrics)
    {
        std::vector<uint8_t> data;

        appendNominalIntegrationU16(data, 1);
        appendNominalIntegrationU16(data, 0);

        appendNominalIntegrationS16(data, 800);
        appendNominalIntegrationS16(data, -200);
        appendNominalIntegrationS16(data, 100);

        appendNominalIntegrationU16(data, 900);

        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationS16(data, 0);

        appendNominalIntegrationS16(data, 1);
        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationS16(data, 0);

        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationS16(data, 0);

        appendNominalIntegrationS16(data, 0);
        appendNominalIntegrationU16(data, numberOfHMetrics);

        return data;
    }


    static std::vector<uint8_t> makeNominalIntegrationHmtx()
    {
        std::vector<uint8_t> data;

        appendNominalIntegrationU16(data, 500);
        appendNominalIntegrationS16(data, 10);

        appendNominalIntegrationU16(data, 600);
        appendNominalIntegrationS16(data, -20);

        appendNominalIntegrationU16(data, 700);
        appendNominalIntegrationS16(data, 30);

        appendNominalIntegrationS16(data, 40);
        appendNominalIntegrationS16(data, -50);
        appendNominalIntegrationS16(data, 60);

        return data;
    }


    // ====================================================================
    // Synthetic OpenType face provider.
    // ====================================================================

    class NominalIntegrationOpenTypeFace final :
        public IProvideFontFaceData,
        public IProvideOpenTypeTables
    {
    public:
        explicit NominalIntegrationOpenTypeFace(bool includeHmtx = true)
            : fHhea(makeNominalIntegrationHhea(3))
            , fHmtx(makeNominalIntegrationHmtx())
            , fIncludeHmtx(includeHmtx)
        {
            fHheaTable.tag = OTAG("hhea");
            fHheaTable.length = static_cast<uint32_t>(fHhea.size());
            fHheaTable.data = ByteSpan(fHhea.data(), fHhea.size());

            fHmtxTable.tag = OTAG("hmtx");
            fHmtxTable.length = static_cast<uint32_t>(fHmtx.size());
            fHmtxTable.data = ByteSpan(fHmtx.data(), fHmtx.size());
        }

        FontName sourceLocation() const noexcept override { return "synthetic-opentype"; }
        FontName familyName() const noexcept override { return "Synthetic"; }
        FontName subfamilyName() const noexcept override { return "Regular"; }
        FontName fullName() const noexcept override { return "Synthetic Regular"; }
        FontName postScriptName() const noexcept override { return "Synthetic-Regular"; }

        FontFaceProperties properties() const noexcept override
        {
            return {};
        }

        uint32_t glyphCount() const noexcept override { return 6; }
        uint16_t unitsPerEm() const noexcept override { return 1000; }

        uint32_t glyphIndex(uint32_t) const noexcept override
        {
            return 0;
        }

        const UnicodeCoverage& unicodeCoverage() const noexcept override
        {
            return fCoverage;
        }

        const TableRecord* getTable(Tag tag) const noexcept override
        {
            if (tag == OTAG("hhea"))
                return &fHheaTable;

            if (tag == OTAG("hmtx") && fIncludeHmtx)
                return &fHmtxTable;

            return nullptr;
        }

        bool hasTable(Tag tag) const noexcept override
        {
            return getTable(tag) != nullptr;
        }

    private:
        UnicodeCoverage fCoverage{};

        std::vector<uint8_t> fHhea{};
        std::vector<uint8_t> fHmtx{};

        TableRecord fHheaTable{};
        TableRecord fHmtxTable{};

        bool fIncludeHmtx{ true };
    };


    // ====================================================================
    // Synthetic non-OpenType face provider.
    // ====================================================================

    class NominalIntegrationGenericFace final : public IProvideFontFaceData
    {
    public:
        FontName sourceLocation() const noexcept override { return "synthetic-generic"; }
        FontName familyName() const noexcept override { return "Generic"; }
        FontName subfamilyName() const noexcept override { return "Regular"; }
        FontName fullName() const noexcept override { return "Generic Regular"; }
        FontName postScriptName() const noexcept override { return "Generic-Regular"; }

        FontFaceProperties properties() const noexcept override
        {
            return {};
        }

        uint32_t glyphCount() const noexcept override { return 6; }
        uint16_t unitsPerEm() const noexcept override { return 1000; }

        uint32_t glyphIndex(uint32_t) const noexcept override
        {
            return 0;
        }

        const UnicodeCoverage& unicodeCoverage() const noexcept override
        {
            return fCoverage;
        }

    private:
        UnicodeCoverage fCoverage{};
    };


    static void appendNominalIntegrationGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId,
        uint32_t scalarOffset, uint32_t scalarCount)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;

        buffer.pushBack(glyph);
    }


    static bool testOpenTypeNominalMetricsIntegration()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType nominal metrics integration: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - FontFace exposes OpenType table capability.
        // ====================================================================

        {
            ++cases;

            const FontFace face(
                std::make_shared<NominalIntegrationOpenTypeFace>());

            const IProvideOpenTypeTables* tables =
                openTypeTableProvider(face);

            if (!tables ||
                !tables->hasTable(OTAG("hhea")) ||
                !tables->hasTable(OTAG("hmtx")) ||
                !openTypeTable(face, OTAG("hhea")) ||
                !openTypeTable(face, OTAG("hmtx")))
            {
                return fail("case 1 OpenType table capability");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Non-OpenType face does not expose OpenType tables.
        // ====================================================================

        {
            ++cases;

            const FontFace face(
                std::make_shared<NominalIntegrationGenericFace>());

            if (openTypeTableProvider(face) != nullptr ||
                openTypeTable(face, OTAG("hhea")) != nullptr)
            {
                return fail("case 2 generic face capability");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Integrated post-GSUB conversion.
        // ====================================================================

        {
            ++cases;

            FontRunView run{};
            run.face = FontFace(
                std::make_shared<NominalIntegrationOpenTypeFace>());

            OpenTypeShapingBuffer input;
            input.reset(run);

            appendNominalIntegrationGlyph(input, 1, 0, 1);
            appendNominalIntegrationGlyph(input, 4, 1, 3);
            appendNominalIntegrationGlyph(input, 5, 4, 1);

            ShapedGlyphBuffer output;

            if (!buildOpenTypeHorizontalShapedGlyphs(input, output))
                return fail("case 3 integrated conversion");

            if (output.size() != 3)
                return fail("case 3 output size");

            if (output[0].shaping.glyphId != 1 ||
                output[0].shaping.scalarOffset != 0 ||
                output[0].shaping.scalarCount != 1 ||
                output[0].placement.advanceX != 600)
            {
                return fail("case 3 glyph 0");
            }

            if (output[1].shaping.glyphId != 4 ||
                output[1].shaping.scalarOffset != 1 ||
                output[1].shaping.scalarCount != 3 ||
                output[1].placement.advanceX != 700)
            {
                return fail("case 3 glyph 1");
            }

            if (output[2].shaping.glyphId != 5 ||
                output[2].shaping.scalarOffset != 4 ||
                output[2].shaping.scalarCount != 1 ||
                output[2].placement.advanceX != 700)
            {
                return fail("case 3 glyph 2");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Missing hmtx fails transactionally.
        // ====================================================================

        {
            ++cases;

            FontRunView run{};
            run.face = FontFace(
                std::make_shared<NominalIntegrationOpenTypeFace>(false));

            OpenTypeShapingBuffer input;
            input.reset(run);

            appendNominalIntegrationGlyph(input, 1, 0, 1);

            ShapedGlyphBuffer output;

            ShapedGlyph sentinel{};
            sentinel.shaping.glyphId = 999;
            sentinel.placement.advanceX = 1234;

            output.pushBack(sentinel);

            if (buildOpenTypeHorizontalShapedGlyphs(input, output))
                return fail("case 4 missing hmtx accepted");

            if (output.size() != 1 ||
                output[0].shaping.glyphId != 999 ||
                output[0].placement.advanceX != 1234)
            {
                return fail("case 4 transactional failure");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Non-OpenType face rejected.
        // ====================================================================

        {
            ++cases;

            FontRunView run{};
            run.face = FontFace(
                std::make_shared<NominalIntegrationGenericFace>());

            OpenTypeShapingBuffer input;
            input.reset(run);

            appendNominalIntegrationGlyph(input, 1, 0, 1);

            ShapedGlyphBuffer output;

            if (buildOpenTypeHorizontalShapedGlyphs(input, output))
                return fail("case 5 generic face accepted");

            if (!output.empty())
                return fail("case 5 output mutation");

            ++passed;
        }


        std::printf(
            "OpenType nominal metrics integration: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  OpenType capability:       PASS\n"
            "  Generic face isolation:    PASS\n"
            "  Integrated conversion:     PASS\n"
            "  Missing table failure:     PASS\n"
            "  Non-OpenType rejection:    PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs