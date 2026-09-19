// test_script_shaping_ir_thai_full.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "opentype_container.h"
#include "opentype_horizontal_shaper.h"
#include "opentype_nominal_glyphs.h"
#include "opentype_nominal_metrics.h"
#include "script_shaping_ir_executor.h"
#include "script_shaping_policy_compiler.h"

namespace waavs
{
    static inline void dumpThaiScriptShapingBuffer(const ScriptShapingBuffer& buffer)
    {
        std::printf(
            "  Scalar items: %zu\n",
            buffer.size());

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const ScriptShapingItem& item = buffer[i];

            std::printf(
                "  [%zu] U+%04X src=[%u,%u) flags=0x%08X\n",
                i,
                static_cast<unsigned>(item.value),
                static_cast<unsigned>(item.scalarOffset),
                static_cast<unsigned>(item.scalarOffset + item.scalarCount),
                static_cast<unsigned>(item.flags));
        }
    }


    static inline void dumpThaiOpenTypeShapingBuffer(const OpenTypeShapingBuffer& buffer)
    {
        std::printf(
            "  Glyphs: %zu\n",
            buffer.size());

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const OpenTypeShapingGlyph& glyph = buffer[i];

            std::printf(
                "  [%zu] gid=%u src=[%u,%u)\n",
                i,
                static_cast<unsigned>(glyph.glyphId),
                static_cast<unsigned>(glyph.scalarOffset),
                static_cast<unsigned>(glyph.scalarOffset + glyph.scalarCount));
        }
    }


    static inline void dumpThaiShapedGlyphBuffer(const ShapedGlyphBuffer& buffer)
    {
        std::printf(
            "  Glyphs: %zu\n",
            buffer.size());

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const ShapedGlyph& glyph = buffer[i];

            std::printf(
                "  [%zu] gid=%u src=[%u,%u) "
                "advance=(%d,%d) offset=(%d,%d)\n",
                i,
                static_cast<unsigned>(glyph.shaping.glyphId),
                static_cast<unsigned>(glyph.shaping.scalarOffset),
                static_cast<unsigned>(
                    glyph.shaping.scalarOffset +
                    glyph.shaping.scalarCount),
                glyph.placement.advanceX,
                glyph.placement.advanceY,
                glyph.placement.offsetX,
                glyph.placement.offsetY);
        }
    }


    static bool runScriptShapingIRThaiFull(const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping IR Thai full: FAIL\n"
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
        // Compile Thai Script IR.
        // ------------------------------------------------------------

        ScriptShapingIR ir;

        if (!compileScriptShapingIRForScript(OTAG("thai"), ir))
            return fail("unable to compile Thai Script IR");

        if (ir.instructions.size() != 5)
            return fail("unexpected Thai Script IR instruction count");

        if (ir.instructions[0].op != ScriptShapingIROp::ScalarReplace)
            return fail("Thai instruction 0 is not ScalarReplace");

        if (ir.instructions[1].op != ScriptShapingIROp::ScalarMoveLeftAcrossRange)
            return fail("Thai instruction 1 is not ScalarMoveLeftAcrossRange");


        // ------------------------------------------------------------
        // Input:
        //
        //   U+0E14 DO DEK
        //   U+0E4B MAI CHATTAWA
        //   U+0E33 SARA AM
        //
        // Expected scalar-domain result:
        //
        //   U+0E14
        //   U+0E4D NIKHAHIT
        //   U+0E4B MAI CHATTAWA
        //   U+0E32 SARA AA
        // ------------------------------------------------------------

        static constexpr uint32_t inputValues[] =
        {
            0x0E14,
            0x0E4B,
            0x0E33
        };

        size_t faceCount = 0;
        size_t testedFaces = 0;

        FontFace face;

        while (container(face))
        {
            ++faceCount;

            const GlyphId doDekGlyph = face.glyphIndex(0x0E14);
            const GlyphId nikhahitGlyph = face.glyphIndex(0x0E4D);
            const GlyphId maiChattawaGlyph = face.glyphIndex(0x0E4B);
            const GlyphId saraAaGlyph = face.glyphIndex(0x0E32);

            if (doDekGlyph == 0 ||
                nikhahitGlyph == 0 ||
                maiChattawaGlyph == 0 ||
                saraAaGlyph == 0)
            {
                continue;
            }

            UnicodeScalar scalars[3]{};

            for (size_t i = 0; i < 3; ++i)
                scalars[i].value = inputValues[i];

            FontRunView run{};

            run.scalars = scalars;
            run.scalarCount = 3;
            run.face = face;
            run.bidiLevel = 0;
            run.completeCoverage = true;


            // ------------------------------------------------------------
            // 1. Scalar-domain Script IR.
            // ------------------------------------------------------------

            ScriptShapingBuffer scriptInput;

            if (!scriptInput.reset(run))
                return fail("unable to initialize ScriptShapingBuffer");

            if (!applyScriptShapingIRScalars(ir, scriptInput))
                return fail("Thai scalar execution failed");

            if (scriptInput.size() != 4)
                return fail("Thai scalar execution produced wrong item count");

            if (scriptInput[0].value != 0x0E14 ||
                scriptInput[1].value != 0x0E4D ||
                scriptInput[2].value != 0x0E4B ||
                scriptInput[3].value != 0x0E32)
            {
                return fail("Thai scalar ordering is incorrect");
            }

            if (scriptInput[0].scalarOffset != 0 ||
                scriptInput[0].scalarCount != 1)
            {
                return fail("DO DEK provenance is incorrect");
            }

            if (scriptInput[1].scalarOffset != 2 ||
                scriptInput[1].scalarCount != 1)
            {
                return fail("NIKHAHIT provenance is incorrect");
            }

            if (scriptInput[2].scalarOffset != 1 ||
                scriptInput[2].scalarCount != 1)
            {
                return fail("MAI CHATTAWA provenance is incorrect");
            }

            if (scriptInput[3].scalarOffset != 2 ||
                scriptInput[3].scalarCount != 1)
            {
                return fail("SARA AA provenance is incorrect");
            }

            std::printf(
                "Script shaping IR Thai full: AFTER SCALAR IR\n");

            dumpThaiScriptShapingBuffer(scriptInput);


            // ------------------------------------------------------------
            // 2. cmap.
            // ------------------------------------------------------------

            OpenTypeShapingBuffer shaping;

            if (!mapOpenTypeNominalGlyphs(scriptInput, shaping))
                return fail("Thai cmap mapping failed");

            if (shaping.size() != 4)
                return fail("Thai cmap produced wrong glyph count");

            if (shaping[0].glyphId != doDekGlyph ||
                shaping[1].glyphId != nikhahitGlyph ||
                shaping[2].glyphId != maiChattawaGlyph ||
                shaping[3].glyphId != saraAaGlyph)
            {
                return fail("Thai nominal glyph mapping is incorrect");
            }

            std::printf(
                "Script shaping IR Thai full: AFTER CMAP\n");

            dumpThaiOpenTypeShapingBuffer(shaping);


            // ------------------------------------------------------------
            // Resolve layout tables.
            // ------------------------------------------------------------

            const FontRunView* shapingRun = shaping.input();

            if (!shapingRun || !shapingRun->face)
                return fail("shaping buffer lost FontRunView");

            OpenTypeHorizontalFaceTables tables;

            if (!resolveOpenTypeHorizontalFaceTables(*shapingRun, tables))
                return fail("unable to resolve OpenType layout tables");


            // ------------------------------------------------------------
            // 3. GSUB.
            // ------------------------------------------------------------

            if (tables.gsub &&
                !applyScriptShapingIRGsub(
                    tables.gsub->data,
                    OTAG("thai"),
                    0,
                    ir,
                    true,
                    true,
                    tables.gdef,
                    shaping))
            {
                return fail("Thai Script IR GSUB failed");
            }

            if (shaping.empty())
                return fail("Thai GSUB produced an empty buffer");

            for (const OpenTypeShapingGlyph& glyph : shaping)
            {
                if (glyph.scalarCount == 0)
                    return fail("post-GSUB glyph has empty provenance");

                if (glyph.scalarOffset >= run.scalarCount)
                    return fail("post-GSUB scalarOffset is out of range");

                if (glyph.scalarCount > run.scalarCount - glyph.scalarOffset)
                    return fail("post-GSUB scalar extent is out of range");
            }

            std::printf(
                "Script shaping IR Thai full: AFTER GSUB\n");

            dumpThaiOpenTypeShapingBuffer(shaping);


            // ------------------------------------------------------------
            // 4. Nominal horizontal metrics.
            // ------------------------------------------------------------

            ShapedGlyphBuffer positioned;

            if (!buildOpenTypeHorizontalShapedGlyphs(shaping, positioned))
                return fail("Thai nominal metrics failed");

            if (positioned.empty())
                return fail("Thai metrics produced an empty buffer");

            std::printf(
                "Script shaping IR Thai full: AFTER NOMINAL METRICS\n");

            dumpThaiShapedGlyphBuffer(positioned);


            // ------------------------------------------------------------
            // 5. GPOS.
            // ------------------------------------------------------------

            if (tables.gpos &&
                !applyScriptShapingIRGpos(
                    tables.gpos->data,
                    OTAG("thai"),
                    0,
                    ir,
                    true,
                    true,
                    tables.gdef,
                    positioned,
                    false))
            {
                return fail("Thai Script IR GPOS failed");
            }

            for (const ShapedGlyph& glyph : positioned)
            {
                if (glyph.shaping.scalarCount == 0)
                    return fail("final glyph has empty provenance");

                if (glyph.shaping.scalarOffset >= run.scalarCount)
                    return fail("final scalarOffset is out of range");

                if (glyph.shaping.scalarCount >
                    run.scalarCount - glyph.shaping.scalarOffset)
                {
                    return fail("final scalar extent is out of range");
                }
            }

            std::printf(
                "Script shaping IR Thai full: AFTER GPOS\n");

            dumpThaiShapedGlyphBuffer(positioned);

            ++testedFaces;
        }


        if (faceCount == 0)
            return fail("container produced no font faces");

        if (testedFaces == 0)
            return fail("no face contained required Thai glyphs");

        std::printf(
            "Script shaping IR Thai full: PASS\n"
            "  Faces:           %zu\n"
            "  Faces tested:    %zu\n"
            "  IR instructions: %zu\n",
            faceCount,
            testedFaces,
            ir.instructions.size());

        return true;
    }


    static bool runScriptShapingIRThaiFull(const char* fontFilename)
    {
        std::vector<uint8_t> fontBytes;

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "Script shaping IR Thai full: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return runScriptShapingIRThaiFull(
            ByteSpan(fontBytes.data(), fontBytes.size()));
    }


    static inline void testScriptShapingIRThaiFull(const char* fontFilename)
    {
        runScriptShapingIRThaiFull(fontFilename);
    }

} // namespace waavs