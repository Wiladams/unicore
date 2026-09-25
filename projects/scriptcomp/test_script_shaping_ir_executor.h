// test_script_shaping_ir_executor.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "opentype_container.h"
#include "opentype_face.h"
#include "opentype_horizontal_shaper.h"
#include "opentype_nominal_glyphs.h"
#include "opentype_nominal_metrics.h"
#include "script_shaping_ir_executor.h"
#include "script_shaping_policy_compiler.h"

namespace waavs
{
    // ========================================================================
    // Buffer comparison
    // ========================================================================

    static bool scriptShapingIRExecutorGlyphMatches(const ShapedGlyph& a, const ShapedGlyph& b) noexcept
    {
        return
            a.shaping.glyphId == b.shaping.glyphId &&
            a.shaping.scalarOffset == b.shaping.scalarOffset &&
            a.shaping.scalarCount == b.shaping.scalarCount &&
            a.shaping.ligature.id == b.shaping.ligature.id &&
            a.shaping.ligature.component == b.shaping.ligature.component &&
            a.shaping.ligature.componentCount == b.shaping.ligature.componentCount &&
            a.placement.advanceX == b.placement.advanceX &&
            a.placement.advanceY == b.placement.advanceY &&
            a.placement.offsetX == b.placement.offsetX &&
            a.placement.offsetY == b.placement.offsetY;
    }


    static bool scriptShapingIRExecutorBufferMatches(const ShapedGlyphBuffer& a, const ShapedGlyphBuffer& b) noexcept
    {
        if (a.size() != b.size())
            return false;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!scriptShapingIRExecutorGlyphMatches(a[i], b[i]))
                return false;
        }

        return true;
    }


    // ========================================================================
    // Script IR shaping path
    //
    // Mirror the current horizontal shaper using ScriptShapingIR for feature
    // stage execution.
    // ========================================================================

    static bool shapeScriptShapingIRExecutorTestRun(
        const FontRunView& input, uint32_t scriptTag, uint32_t languageTag,
        const ScriptShapingIR& ir, ShapedGlyphBuffer& output)
    {
        if (!input.face || scriptTag == 0)
            return false;


        // ------------------------------------------------------------
        // 1. cmap.
        // ------------------------------------------------------------
        ScriptShapingBuffer scriptInput;

        if (!scriptInput.reset(input))
            return false;

        if (!applyScriptShapingIRScalars(ir, scriptInput))
            return false;

        OpenTypeShapingBuffer shaping;

        if (!mapOpenTypeNominalGlyphs(scriptInput, shaping))
            return false;



        const FontRunView* run = shaping.input();

        if (!run || !run->face)
            return false;


        // ------------------------------------------------------------
        // Face layout tables.
        // ------------------------------------------------------------

        OpenTypeHorizontalFaceTables tables;

        if (!resolveOpenTypeHorizontalFaceTables(*run, tables))
            return false;


        // ------------------------------------------------------------
        // 3. Script IR GSUB.
        // ------------------------------------------------------------

        if (tables.gsub &&
            !applyScriptShapingIRGsub(
                tables.gsub->data, scriptTag, languageTag,
                ir, true, true, tables.gdef, shaping))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 4. Nominal horizontal metrics.
        // ------------------------------------------------------------

        ShapedGlyphBuffer positioned;

        if (!buildOpenTypeHorizontalShapedGlyphs(shaping, positioned))
            return false;


        // ------------------------------------------------------------
        // 5. Script IR GPOS.
        // ------------------------------------------------------------

        if (tables.gpos)
        {
            const bool runRightToLeft = (run->bidiLevel & 1u) != 0;

            if (!applyScriptShapingIRGpos(
                tables.gpos->data, scriptTag, languageTag,
                ir, true, true, tables.gdef, positioned, runRightToLeft))
            {
                return false;
            }
        }


        output = std::move(positioned);
        return true;
    }


    // ========================================================================
    // Build a small Latin FontRunView.
    //
    // The text deliberately contains:
    //
    //   AVATAR       kerning-sensitive pairs
    //   office       common ffi/fi ligature opportunity
    //   affine       another ligature opportunity
    //   Waavs        mixed kerning/contextual cases
    //
    // The run itself is deliberately minimal. The current cmap/OpenType
    // horizontal shaping path requires only scalars, face and bidi level.
    // ========================================================================

    static bool makeScriptShapingIRExecutorLatinRun(FontFace face, std::vector<UnicodeScalar>& scalars, FontRunView& run)
    {
        static constexpr uint32_t values[] =
        {
            'A', 'V', 'A', 'T', 'A', 'R', ' ',
            'o', 'f', 'f', 'i', 'c', 'e', ' ',
            'a', 'f', 'f', 'i', 'n', 'e', ' ',
            'W', 'a', 'a', 'v', 's'
        };

        scalars.clear();
        scalars.reserve(sizeof(values) / sizeof(values[0]));

        for (uint32_t value : values)
        {
            UnicodeScalar scalar{};
            scalar.value = value;
            scalars.push_back(scalar);
        }

        run = {};
        run.scalars = scalars.data();
        run.scalarCount = static_cast<uint32_t>(scalars.size());
        run.face = face;
        run.bidiLevel = 0;
        run.completeCoverage = true;

        return static_cast<bool>(run.face);
    }


    // ========================================================================
    // testScriptShapingIRExecutor
    //
    // Differential test:
    //
    //   existing OpenTypeShapingPolicy execution
    //
    // versus
    //
    //   OpenTypeShapingPolicy
    //       -> ScriptShapingIR
    //       -> ScriptShapingIRExecutor
    //
    // Final glyph identity, provenance and placement must match exactly.
    // ========================================================================

    static bool testScriptShapingIRExecutor(const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping IR executor: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        if (fontData.empty())
            return fail("empty font data");


        SharedMemBuff fontBuffer(fontData.size());

        if (!fontBuffer)
            return fail("unable to allocate font buffer");

        std::memcpy(fontBuffer.data(), fontData.begin(), fontData.size());

        OpenTypeContainer container(fontBuffer);

        if (!container.isValid())
            return fail("invalid OpenType container");


        // ------------------------------------------------------------
        // Compile Latin Script IR once.
        // ------------------------------------------------------------

        ScriptShapingIR ir;

        if (!compileScriptShapingIRForScript(OTAG("latn"), ir))
            return fail("unable to compile Latin Script IR");

        if (ir.empty())
            return fail("compiled Latin Script IR is empty");


        // ------------------------------------------------------------
        // Test every face that provides the tables required by the horizontal
        // shaping path.
        // ------------------------------------------------------------

        size_t faceCount = 0;
        size_t testedFaces = 0;
        size_t comparedGlyphs = 0;

        FontFaceView view;

        while (container(view))
        {
            ++faceCount;

            if (!view.hasTable(OTAG("hhea")) ||
                !view.hasTable(OTAG("hmtx")))
            {
                continue;
            }

            FontFace face = parseFontFace(std::move(view));

            if (!face)
                continue;


            // ------------------------------------------------------------
            // Input run.
            // ------------------------------------------------------------

            std::vector<UnicodeScalar> scalars;
            FontRunView run;

            if (!makeScriptShapingIRExecutorLatinRun(face, scalars, run))
                return fail("unable to construct Latin FontRunView");


            // ------------------------------------------------------------
            // Existing policy path.
            // ------------------------------------------------------------

            ShapedGlyphBuffer policyResult;

            if (!shapeOpenTypeHorizontalRun(
                run, OTAG("latn"), 0,
                kOpenTypeLatinShapingPolicy,
                policyResult))
            {
                return fail("existing policy shaping path failed");
            }


            // ------------------------------------------------------------
            // Script IR path.
            // ------------------------------------------------------------

            ShapedGlyphBuffer irResult;

            if (!shapeScriptShapingIRExecutorTestRun(
                run, OTAG("latn"), 0, ir, irResult))
            {
                return fail("Script IR shaping path failed");
            }


            // ------------------------------------------------------------
            // Differential.
            // ------------------------------------------------------------

            if (!scriptShapingIRExecutorBufferMatches(policyResult, irResult))
            {
                std::printf(
                    "Script shaping IR executor: FAIL\n"
                    "  Final buffer mismatch\n"
                    "  Policy glyphs: %zu\n"
                    "  IR glyphs:     %zu\n",
                    policyResult.size(),
                    irResult.size());

                const size_t count =
                    policyResult.size() < irResult.size()
                    ? policyResult.size()
                    : irResult.size();

                for (size_t i = 0; i < count; ++i)
                {
                    if (scriptShapingIRExecutorGlyphMatches(policyResult[i], irResult[i]))
                        continue;

                    const ShapedGlyph& oldGlyph = policyResult[i];
                    const ShapedGlyph& newGlyph = irResult[i];

                    std::printf(
                        "  First mismatch: %zu\n"
                        "    Policy gid: %u\n"
                        "    IR gid:     %u\n"
                        "    Policy src: [%u,%u)\n"
                        "    IR src:     [%u,%u)\n"
                        "    Policy advance: (%d,%d)\n"
                        "    IR advance:     (%d,%d)\n"
                        "    Policy offset:  (%d,%d)\n"
                        "    IR offset:      (%d,%d)\n",
                        i,
                        static_cast<unsigned>(oldGlyph.shaping.glyphId),
                        static_cast<unsigned>(newGlyph.shaping.glyphId),
                        static_cast<unsigned>(oldGlyph.shaping.scalarOffset),
                        static_cast<unsigned>(oldGlyph.shaping.scalarOffset + oldGlyph.shaping.scalarCount),
                        static_cast<unsigned>(newGlyph.shaping.scalarOffset),
                        static_cast<unsigned>(newGlyph.shaping.scalarOffset + newGlyph.shaping.scalarCount),
                        oldGlyph.placement.advanceX,
                        oldGlyph.placement.advanceY,
                        newGlyph.placement.advanceX,
                        newGlyph.placement.advanceY,
                        oldGlyph.placement.offsetX,
                        oldGlyph.placement.offsetY,
                        newGlyph.placement.offsetX,
                        newGlyph.placement.offsetY);

                    break;
                }

                return false;
            }

            ++testedFaces;
            comparedGlyphs += policyResult.size();
        }


        if (faceCount == 0)
            return fail("container produced no font faces");

        if (testedFaces == 0)
            return fail("no face could exercise horizontal shaping");


        std::printf(
            "Script shaping IR executor: PASS\n"
            "  Faces:           %zu\n"
            "  Faces tested:    %zu\n"
            "  Compared glyphs: %zu\n"
            "  IR instructions: %zu\n",
            faceCount,
            testedFaces,
            comparedGlyphs,
            ir.instructions.size());

        return true;
    }


    // ========================================================================
    // Filename convenience overload.
    // ========================================================================

    static bool testScriptShapingIRExecutor(const char* fontFilename)
    {
        std::vector<uint8_t> fontBytes;

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "Script shaping IR executor: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return testScriptShapingIRExecutor(ByteSpan(fontBytes.data(), fontBytes.size()));
    }

} // namespace waavs