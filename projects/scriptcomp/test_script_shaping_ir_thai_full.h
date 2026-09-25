// test_script_shaping_ir_thai_full.h
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

        FontFaceView view;

        while (container(view))
        {
            ++faceCount;

            FontFace face = parseFontFace(std::move(view));

            if (!face)
                continue;

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

            // ... rest unchanged ...
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