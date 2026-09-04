// test_opentype_nominal_glyphs.h

#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <vector>

#include "opentype_nominal_glyphs.h"


namespace waavs
{
    // ========================================================================
    // NominalGlyphTestMapping
    // ========================================================================

    struct NominalGlyphTestMapping
    {
        uint32_t codePoint{ 0 };
        uint32_t glyphId{ 0 };
    };


    // ========================================================================
    // NominalGlyphTestFaceData
    //
    // Minimal deterministic FontFace provider with explicit cmap results.
    // ========================================================================

    class NominalGlyphTestFaceData final : public IProvideFontFaceData
    {
    public:
        explicit NominalGlyphTestFaceData(std::initializer_list<NominalGlyphTestMapping> mappings) : mMappings(mappings) {}

        FontName sourceLocation() const noexcept override { return nullptr; }
        FontName familyName() const noexcept override { return nullptr; }
        FontName subfamilyName() const noexcept override { return nullptr; }
        FontName fullName() const noexcept override { return nullptr; }
        FontName postScriptName() const noexcept override { return nullptr; }

        FontFaceProperties properties() const noexcept override { return {}; }
        uint32_t glyphCount() const noexcept override { return 256; }
        uint16_t unitsPerEm() const noexcept override { return 1000; }

        uint32_t glyphIndex(uint32_t codePoint) const noexcept override
        {
            for (const NominalGlyphTestMapping& mapping : mMappings)
            {
                if (mapping.codePoint == codePoint)
                    return mapping.glyphId;
            }

            return 0;
        }

        bool supportsCodepoint(uint32_t codePoint) const noexcept override { return glyphIndex(codePoint) != 0; }

        const UnicodeCoverage& unicodeCoverage() const noexcept override { return mCoverage; }

    private:
        std::vector<NominalGlyphTestMapping> mMappings{};
        UnicodeCoverage mCoverage{};
    };


    // ========================================================================
    // makeNominalGlyphTestFace
    // ========================================================================

    static FontFace makeNominalGlyphTestFace(std::initializer_list<NominalGlyphTestMapping> mappings)
    {
        return FontFace(std::make_shared<NominalGlyphTestFaceData>(mappings));
    }


    // ========================================================================
    // makeNominalGlyphTestRun
    // ========================================================================

    static FontRunView makeNominalGlyphTestRun(const std::vector<UnicodeScalar>& scalars, const FontFace& face, uint8_t bidiLevel = 0)
    {
        FontRunView run{};

        run.scalars = scalars.empty() ? nullptr : scalars.data();
        run.scalarCount = static_cast<uint32_t>(scalars.size());
        run.face = face;
        run.bidiLevel = bidiLevel;

        return run;
    }


    // ========================================================================
    // testOpenTypeNominalGlyphs
    // ========================================================================

    static bool testOpenTypeNominalGlyphs()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType nominal glyphs: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Ordinary cmap conversion.
        //
        // Each scalar produces exactly one nominal glyph and retains a
        // one-scalar provenance span.
        // ====================================================================

        {
            ++cases;

            const FontFace face = makeNominalGlyphTestFace({
                { 0x0041, 11 },
                { 0x0042, 22 },
                { 0x0043, 33 }
                });

            std::vector<UnicodeScalar> scalars(3);

            scalars[0].value = 0x0041;
            scalars[1].value = 0x0042;
            scalars[2].value = 0x0043;

            const FontRunView run = makeNominalGlyphTestRun(scalars, face);

            OpenTypeShapingBuffer buffer;

            if (!mapOpenTypeNominalGlyphs(run, buffer))
                return fail("case 1 mapping failed");

            if (buffer.input() != &run)
                return fail("case 1 input run not retained");

            if (buffer.size() != 3)
                return fail("case 1 wrong glyph count");

            if (buffer[0].glyphId != 11 || buffer[1].glyphId != 22 || buffer[2].glyphId != 33)
                return fail("case 1 wrong nominal glyph IDs");

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 1)
                return fail("case 1 first scalar provenance");

            if (buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1)
                return fail("case 1 second scalar provenance");

            if (buffer[2].scalarOffset != 2 || buffer[2].scalarCount != 1)
                return fail("case 1 third scalar provenance");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Missing cmap entry becomes glyph zero.
        //
        // Nominal mapping must not fail merely because cmap returns .notdef.
        // ====================================================================

        {
            ++cases;

            const FontFace face = makeNominalGlyphTestFace({
                { 0x0041, 11 },
                { 0x0042, 22 }
                });

            std::vector<UnicodeScalar> scalars(3);

            scalars[0].value = 0x0041;
            scalars[1].value = 0x2603;
            scalars[2].value = 0x0042;

            const FontRunView run = makeNominalGlyphTestRun(scalars, face);

            OpenTypeShapingBuffer buffer;

            if (!mapOpenTypeNominalGlyphs(run, buffer))
                return fail("case 2 mapping failed");

            if (buffer.size() != 3)
                return fail("case 2 wrong glyph count");

            if (buffer[0].glyphId != 11 || buffer[1].glyphId != 0 || buffer[2].glyphId != 22)
                return fail("case 2 .notdef was not preserved");

            if (buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1)
                return fail("case 2 .notdef provenance");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // A default-ignorable scalar without a nominal glyph remains in the
        // shaping stream as glyph zero.
        //
        // No default-ignorable filtering belongs in this stage.
        // ====================================================================

        {
            ++cases;

            const FontFace face = makeNominalGlyphTestFace({
                { 0x0041, 11 },
                { 0x0042, 22 }
                });

            std::vector<UnicodeScalar> scalars(3);

            scalars[0].value = 0x0041;
            scalars[1].value = 0x200D;
            scalars[2].value = 0x0042;

            const FontRunView run = makeNominalGlyphTestRun(scalars, face);

            OpenTypeShapingBuffer buffer;

            if (!mapOpenTypeNominalGlyphs(run, buffer))
                return fail("case 3 mapping failed");

            if (buffer.size() != 3)
                return fail("case 3 ZWJ disappeared from shaping stream");

            if (buffer[0].glyphId != 11 || buffer[1].glyphId != 0 || buffer[2].glyphId != 22)
                return fail("case 3 incorrect ZWJ nominal mapping");

            if (buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1)
                return fail("case 3 ZWJ provenance");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // RTL input remains in logical order.
        //
        // Nominal cmap mapping performs no visual reordering.
        // ====================================================================

        {
            ++cases;

            const FontFace face = makeNominalGlyphTestFace({
                { 0x0627, 31 },
                { 0x0644, 32 },
                { 0x0645, 33 }
                });

            std::vector<UnicodeScalar> scalars(3);

            scalars[0].value = 0x0627;
            scalars[1].value = 0x0644;
            scalars[2].value = 0x0645;

            const FontRunView run = makeNominalGlyphTestRun(scalars, face, 1);

            OpenTypeShapingBuffer buffer;

            if (!mapOpenTypeNominalGlyphs(run, buffer))
                return fail("case 4 RTL mapping failed");

            if (buffer.size() != 3)
                return fail("case 4 wrong RTL glyph count");

            if (buffer[0].glyphId != 31 || buffer[1].glyphId != 32 || buffer[2].glyphId != 33)
                return fail("case 4 RTL input was reordered");

            if (buffer[0].scalarOffset != 0 || buffer[1].scalarOffset != 1 || buffer[2].scalarOffset != 2)
                return fail("case 4 RTL scalar provenance");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Empty FontRunView with a valid face produces an empty shaping
        // buffer and succeeds.
        // ====================================================================

        {
            ++cases;

            const FontFace face = makeNominalGlyphTestFace({
                { 0x0041, 11 }
                });

            const std::vector<UnicodeScalar> scalars{};
            const FontRunView run = makeNominalGlyphTestRun(scalars, face);

            OpenTypeShapingBuffer buffer;

            if (!mapOpenTypeNominalGlyphs(run, buffer))
                return fail("case 5 empty run failed");

            if (!buffer.empty())
                return fail("case 5 empty run produced glyphs");

            if (buffer.input() != &run)
                return fail("case 5 empty run input not retained");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Invalid inputs are rejected and leave the shaping buffer empty.
        // ====================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            FontRunView invalidFaceRun{};

            UnicodeScalar scalar{};
            scalar.value = 0x0041;

            invalidFaceRun.scalars = &scalar;
            invalidFaceRun.scalarCount = 1;

            if (mapOpenTypeNominalGlyphs(invalidFaceRun, buffer))
                return fail("case 6 invalid face accepted");

            if (!buffer.empty() || buffer.input() != nullptr)
                return fail("case 6 invalid face left stale output");


            const FontFace face = makeNominalGlyphTestFace({
                { 0x0041, 11 }
                });

            FontRunView invalidScalarRun{};
            invalidScalarRun.face = face;
            invalidScalarRun.scalars = nullptr;
            invalidScalarRun.scalarCount = 1;

            if (mapOpenTypeNominalGlyphs(invalidScalarRun, buffer))
                return fail("case 6 null scalar storage accepted");

            if (!buffer.empty() || buffer.input() != nullptr)
                return fail("case 6 malformed input left stale output");

            ++passed;
        }


        std::printf(
            "OpenType nominal glyphs: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  Nominal cmap mapping:  PASS\n"
            "  Missing glyphs:        PASS\n"
            "  Default ignorables:    PASS\n"
            "  Logical order:         PASS\n"
            "  Scalar provenance:     PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs