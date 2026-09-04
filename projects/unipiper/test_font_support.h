// test_font_support.h

#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <vector>

#include "font_support.h"


namespace waavs
{
    // ========================================================================
    // FontSupportTestFaceData
    //
    // Minimal deterministic IProvideFontFaceData implementation.
    //
    // Every code point listed in mGlyphs maps to a synthetic non-zero glyph
    // index. Everything else maps to glyph zero.
    //
    // The test deliberately does not depend on an actual font file.
    // ========================================================================

    class FontSupportTestFaceData final : public IProvideFontFaceData
    {
    public:
        explicit FontSupportTestFaceData(std::initializer_list<uint32_t> glyphs)
            : mGlyphs(glyphs)
        {}


        // ====================================================================
        // IProvideFontFaceData
        // ====================================================================

        FontName sourceLocation() const noexcept override
        {
            return nullptr;
        }


        FontName familyName() const noexcept override
        {
            return nullptr;
        }


        FontName subfamilyName() const noexcept override
        {
            return nullptr;
        }


        FontName fullName() const noexcept override
        {
            return nullptr;
        }


        FontName postScriptName() const noexcept override
        {
            return nullptr;
        }


        FontFaceProperties properties() const noexcept override
        {
            return {};
        }


        uint32_t glyphCount() const noexcept override
        {
            // Glyph zero is .notdef.
            return static_cast<uint32_t>(mGlyphs.size()) + 1u;
        }


        uint16_t unitsPerEm() const noexcept override
        {
            return 1000;
        }


        uint32_t glyphIndex(uint32_t codepoint) const noexcept override
        {
            for (size_t i = 0; i < mGlyphs.size(); ++i)
            {
                if (mGlyphs[i] == codepoint)
                    return static_cast<uint32_t>(i) + 1u;
            }

            return 0;
        }


        bool supportsCodepoint(uint32_t codepoint) const noexcept override
        {
            return glyphIndex(codepoint) != 0;
        }


        const UnicodeCoverage& unicodeCoverage() const noexcept override
        {
            return mCoverage;
        }


    private:
        std::vector<uint32_t> mGlyphs{};
        UnicodeCoverage mCoverage{};
    };


    // ========================================================================
    // makeFontSupportTestFace
    // ========================================================================

    static FontFace makeFontSupportTestFace(std::initializer_list<uint32_t> glyphs)
    {
        return FontFace(
            std::make_shared<FontSupportTestFaceData>(glyphs));
    }


    // ========================================================================
    // testFontSupport
    // ========================================================================

    static bool testFontSupport(const ByteSpan& databaseData)
    {
        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Font support: FAIL\n"
                "  Unable to initialize Unicode database\n");

            return false;
        }


        uint32_t scalarCases = 0;
        uint32_t scalarPassed = 0;

        uint32_t clusterCases = 0;
        uint32_t clusterPassed = 0;


        // ====================================================================
        // Scalar fallback policy
        // ====================================================================

        auto checkScalar = [&](const char* description,
            uint32_t cp, bool expected) -> bool
            {
                ++scalarCases;

                const bool actual =
                    fontFallbackScalarRequiresGlyph(
                        cp,
                        database);

                if (actual != expected)
                {
                    std::printf(
                        "Font support: FAIL: %s\n"
                        "  Code point:              U+%04X\n"
                        "  Expected requires glyph: %s\n"
                        "  Actual requires glyph:   %s\n",
                        description,
                        cp,
                        expected ? "YES" : "NO",
                        actual ? "YES" : "NO");

                    return false;
                }

                ++scalarPassed;
                return true;
            };


        if (!checkScalar(
            "ordinary Latin scalar",
            0x0041,
            true))
        {
            return false;
        }


        if (!checkScalar(
            "combining acute accent",
            0x0301,
            true))
        {
            return false;
        }


        if (!checkScalar(
            "ZWNJ",
            0x200C,
            false))
        {
            return false;
        }


        if (!checkScalar(
            "ZWJ",
            0x200D,
            false))
        {
            return false;
        }


        if (!checkScalar(
            "variation selector 16",
            0xFE0F,
            false))
        {
            return false;
        }


        if (!checkScalar(
            "supplementary variation selector",
            0xE0100,
            false))
        {
            return false;
        }


        if (!checkScalar(
            "right-to-left isolate",
            0x2067,
            false))
        {
            return false;
        }


        if (!checkScalar(
            "word joiner",
            0x2060,
            false))
        {
            return false;
        }


        // ====================================================================
        // Cluster support helper
        // ====================================================================

        auto checkCluster = [&](const char* description,
            const FontFace& face,
            std::initializer_list<uint32_t> values,
            uint32_t scalarOffset,
            uint32_t scalarCount,
            bool expected) -> bool
            {
                ++clusterCases;


                std::vector<UnicodeScalar> scalars;
                scalars.reserve(values.size());


                for (uint32_t value : values)
                {
                    UnicodeScalar scalar{};

                    scalar.value = value;

                    scalars.push_back(scalar);
                }


                ShapingRunView run{};

                run.scalars =
                    scalars.empty()
                    ? nullptr
                    : scalars.data();

                run.scalarCount =
                    static_cast<uint32_t>(
                        scalars.size());


                ShapingCluster cluster{};

                cluster.scalarOffset = scalarOffset;
                cluster.scalarCount = scalarCount;


                const bool actual =
                    fontSupportsCluster(
                        face,
                        run,
                        cluster,
                        database);


                if (actual != expected)
                {
                    std::printf(
                        "Font support: FAIL: %s\n"
                        "  Scalar offset: %u\n"
                        "  Scalar count:  %u\n"
                        "  Expected:      %s\n"
                        "  Actual:        %s\n",
                        description,
                        scalarOffset,
                        scalarCount,
                        expected
                        ? "SUPPORTED"
                        : "UNSUPPORTED",
                        actual
                        ? "SUPPORTED"
                        : "UNSUPPORTED");

                    return false;
                }


                ++clusterPassed;
                return true;
            };


        // ====================================================================
        // Synthetic faces
        // ====================================================================

        const FontFace latin =
            makeFontSupportTestFace({
                0x0041,
                0x0042
                });


        const FontFace latinWithMark =
            makeFontSupportTestFace({
                0x0041,
                0x0042,
                0x0301
                });


        const FontFace heart =
            makeFontSupportTestFace({
                0x2764
                });


        // ====================================================================
        // Ordinary scalar coverage
        // ====================================================================

        if (!checkCluster(
            "supported ordinary scalar",
            latin,
            { 0x0041 },
            0,
            1,
            true))
        {
            return false;
        }


        if (!checkCluster(
            "missing ordinary scalar",
            latin,
            { 0x0043 },
            0,
            1,
            false))
        {
            return false;
        }


        // ====================================================================
        // Combining marks require nominal glyph coverage
        // ====================================================================

        if (!checkCluster(
            "base and combining mark both covered",
            latinWithMark,
            { 0x0041, 0x0301 },
            0,
            2,
            true))
        {
            return false;
        }


        if (!checkCluster(
            "missing combining mark rejects cluster",
            latin,
            { 0x0041, 0x0301 },
            0,
            2,
            false))
        {
            return false;
        }


        // ====================================================================
        // Join controls do not independently require glyphs
        // ====================================================================

        if (!checkCluster(
            "ZWJ does not require nominal glyph",
            latin,
            { 0x0041, 0x200D, 0x0042 },
            0,
            3,
            true))
        {
            return false;
        }


        if (!checkCluster(
            "ZWNJ does not require nominal glyph",
            latin,
            { 0x0041, 0x200C, 0x0042 },
            0,
            3,
            true))
        {
            return false;
        }


        // ====================================================================
        // Variation selectors
        //
        // Current behavior:
        //
        //     base + VS16
        //
        // requires nominal coverage for the base but not for VS16 itself.
        //
        // cmap format 14 sequence handling can refine this later.
        // ====================================================================

        if (!checkCluster(
            "variation selector does not independently require glyph",
            heart,
            { 0x2764, 0xFE0F },
            0,
            2,
            true))
        {
            return false;
        }


        if (!checkCluster(
            "missing variation base rejects cluster",
            latin,
            { 0x2764, 0xFE0F },
            0,
            2,
            false))
        {
            return false;
        }


        // ====================================================================
        // Ignorable/control-only clusters
        // ====================================================================

        if (!checkCluster(
            "ZWJ-only cluster requires no nominal glyph",
            latin,
            { 0x200D },
            0,
            1,
            true))
        {
            return false;
        }


        if (!checkCluster(
            "variation-selector-only cluster requires no nominal glyph",
            latin,
            { 0xFE0F },
            0,
            1,
            true))
        {
            return false;
        }


        // ====================================================================
        // Cluster offsets are relative to run.scalars
        // ====================================================================

        if (!checkCluster(
            "non-zero scalar offset",
            latinWithMark,
            { 0x1234, 0x0041, 0x0301, 0x5678 },
            1,
            2,
            true))
        {
            return false;
        }


        if (!checkCluster(
            "missing scalar inside selected cluster",
            latin,
            { 0x1234, 0x0041, 0x0043, 0x5678 },
            1,
            2,
            false))
        {
            return false;
        }


        // ====================================================================
        // Geometry failure paths
        // ====================================================================

        if (!checkCluster(
            "cluster offset past run",
            latin,
            { 0x0041 },
            2,
            1,
            false))
        {
            return false;
        }


        if (!checkCluster(
            "cluster count extends past run",
            latin,
            { 0x0041, 0x0042 },
            1,
            2,
            false))
        {
            return false;
        }


        if (!checkCluster(
            "empty cluster rejected",
            latin,
            { 0x0041 },
            0,
            0,
            false))
        {
            return false;
        }


        // ====================================================================
        // Invalid face
        // ====================================================================

        {
            FontFace invalidFace{};

            if (!checkCluster(
                "invalid font face rejected",
                invalidFace,
                { 0x0041 },
                0,
                1,
                false))
            {
                return false;
            }
        }


        // ====================================================================
        // Null scalar storage
        // ====================================================================

        {
            ++clusterCases;


            ShapingRunView run{};

            run.scalars = nullptr;
            run.scalarCount = 1;


            ShapingCluster cluster{};

            cluster.scalarOffset = 0;
            cluster.scalarCount = 1;


            const bool actual =
                fontSupportsCluster(
                    latin,
                    run,
                    cluster,
                    database);


            if (actual)
            {
                std::printf(
                    "Font support: FAIL: null scalar storage accepted\n");

                return false;
            }


            ++clusterPassed;
        }


        // ====================================================================
        // Direct FontFace glyph mapping sanity checks
        //
        // These verify that this test is actually exercising
        // FontFace::glyphIndex()/hasGlyph(), not the coverage object.
        // ====================================================================

        if (latin.glyphIndex(0x0041) == 0)
        {
            std::printf(
                "Font support: FAIL: synthetic glyph mapping missing U+0041\n");

            return false;
        }


        if (!latin.hasGlyph(0x0041))
        {
            std::printf(
                "Font support: FAIL: FontFace::hasGlyph failed for U+0041\n");

            return false;
        }


        if (latin.hasGlyph(0x0043))
        {
            std::printf(
                "Font support: FAIL: FontFace::hasGlyph accepted missing U+0043\n");

            return false;
        }


        // ====================================================================
        // Diagnostics
        // ====================================================================

        std::printf(
            "Font support: PASS\n"
            "  Scalar policy cases:     %u\n"
            "  Scalar policy passed:    %u\n"
            "  Cluster support cases:   %u\n"
            "  Cluster support passed:  %u\n",
            scalarCases,
            scalarPassed,
            clusterCases,
            clusterPassed);


        return
            scalarPassed == scalarCases &&
            clusterPassed == clusterCases;
    }


    // ========================================================================
    // Convenience filename overload
    // ========================================================================

    static bool testFontSupport(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;


        if (!readFileData(
            databaseFilename,
            fileData))
        {
            std::printf(
                "Font support: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testFontSupport(
            databaseData);
    }

} // namespace waavs