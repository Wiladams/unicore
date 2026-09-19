// test_script_shaping_ir_thai_cmap.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "opentype_container.h"
#include "opentype_nominal_glyphs.h"
#include "script_shaping_ir_executor.h"
#include "script_shaping_policy_compiler.h"

namespace waavs
{
    static bool runScriptShapingIRThaiCmap(const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping IR Thai cmap: FAIL\n"
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
        // Compile the Thai Script IR.
        // ------------------------------------------------------------

        ScriptShapingIR ir;

        if (!compileScriptShapingIRForScript(OTAG("thai"), ir))
            return fail("unable to compile Thai Script IR");

        if (ir.instructions.empty())
            return fail("compiled Thai Script IR is empty");

        if (ir.instructions[0].op != ScriptShapingIROp::ScalarReplace)
            return fail("Thai Script IR does not begin with ScalarReplace");


        // ------------------------------------------------------------
        // Test every face containing both replacement scalars:
        //
        //   U+0E4D NIKHAHIT
        //   U+0E32 SARA AA
        //
        // U+0E33 itself does not need to be present in cmap because the
        // scalar rewrite happens before cmap.
        // ------------------------------------------------------------

        size_t faceCount = 0;
        size_t testedFaces = 0;

        FontFace face;

        while (container(face))
        {
            ++faceCount;

            const GlyphId nikhahitGlyph = face.glyphIndex(0x0E4D);
            const GlyphId saraAaGlyph = face.glyphIndex(0x0E32);

            if (nikhahitGlyph == 0 || saraAaGlyph == 0)
                continue;


            // ------------------------------------------------------------
            // Input scalar sequence:
            //
            //   [0] U+0E01 KO KAI
            //   [1] U+0E33 SARA AM
            //
            // Script-domain result:
            //
            //   [0] U+0E01
            //   [1] U+0E4D
            //   [2] U+0E32
            //
            // Both replacement items retain source extent [1,2).
            // ------------------------------------------------------------

            UnicodeScalar scalars[2]{};

            scalars[0].value = 0x0E01;
            scalars[1].value = 0x0E33;

            FontRunView run{};

            run.scalars = scalars;
            run.scalarCount = 2;
            run.face = face;
            run.bidiLevel = 0;
            run.completeCoverage = true;


            // ------------------------------------------------------------
            // Scalar-domain shaping.
            // ------------------------------------------------------------

            ScriptShapingBuffer scriptInput;

            if (!scriptInput.reset(run))
                return fail("unable to initialize ScriptShapingBuffer");

            if (!applyScriptShapingIRScalars(ir, scriptInput))
                return fail("Thai scalar Script IR execution failed");

            if (scriptInput.size() != 3)
                return fail("Thai scalar rewrite produced wrong item count");

            if (scriptInput[0].value != 0x0E01)
                return fail("KO KAI changed unexpectedly");

            if (scriptInput[1].value != 0x0E4D)
                return fail("SARA AM did not produce NIKHAHIT");

            if (scriptInput[2].value != 0x0E32)
                return fail("SARA AM did not produce SARA AA");


            // ------------------------------------------------------------
            // Provenance before cmap.
            // ------------------------------------------------------------

            if (scriptInput[0].scalarOffset != 0 ||
                scriptInput[0].scalarCount != 1)
            {
                return fail("KO KAI provenance is incorrect");
            }

            if (scriptInput[1].scalarOffset != 1 ||
                scriptInput[1].scalarCount != 1)
            {
                return fail("NIKHAHIT provenance is incorrect");
            }

            if (scriptInput[2].scalarOffset != 1 ||
                scriptInput[2].scalarCount != 1)
            {
                return fail("SARA AA provenance is incorrect");
            }


            // ------------------------------------------------------------
            // cmap.
            // ------------------------------------------------------------

            OpenTypeShapingBuffer shaping;

            if (!mapOpenTypeNominalGlyphs(scriptInput, shaping))
                return fail("cmap mapping failed");

            if (shaping.size() != 3)
                return fail("cmap produced wrong glyph count");


            // ------------------------------------------------------------
            // Glyph identities.
            // ------------------------------------------------------------

            const GlyphId koKaiGlyph = face.glyphIndex(0x0E01);

            if (shaping[0].glyphId != koKaiGlyph)
                return fail("KO KAI glyph mismatch");

            if (shaping[1].glyphId != nikhahitGlyph)
                return fail("NIKHAHIT glyph mismatch");

            if (shaping[2].glyphId != saraAaGlyph)
                return fail("SARA AA glyph mismatch");


            // ------------------------------------------------------------
            // Provenance must survive cmap unchanged.
            // ------------------------------------------------------------

            if (shaping[0].scalarOffset != 0 ||
                shaping[0].scalarCount != 1)
            {
                return fail("KO KAI cmap provenance is incorrect");
            }

            if (shaping[1].scalarOffset != 1 ||
                shaping[1].scalarCount != 1)
            {
                return fail("NIKHAHIT cmap provenance is incorrect");
            }

            if (shaping[2].scalarOffset != 1 ||
                shaping[2].scalarCount != 1)
            {
                return fail("SARA AA cmap provenance is incorrect");
            }

            ++testedFaces;
        }

        if (faceCount == 0)
            return fail("container produced no font faces");

        if (testedFaces == 0)
            return fail("no face contained the required Thai glyphs");

        std::printf(
            "Script shaping IR Thai cmap: PASS\n"
            "  Faces:        %zu\n"
            "  Faces tested: %zu\n"
            "  IR items:     3\n"
            "  Source items: 2\n",
            faceCount,
            testedFaces);

        return true;
    }


    static bool runScriptShapingIRThaiCmap(const char* fontFilename)
    {
        std::vector<uint8_t> fontBytes;

        if (!readFileData(fontFilename, fontBytes))
        {
            std::printf(
                "Script shaping IR Thai cmap: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return runScriptShapingIRThaiCmap(
            ByteSpan(fontBytes.data(), fontBytes.size()));
    }


    static inline void testScriptShapingIRThaiCmap(const char* fontFilename)
    {
        runScriptShapingIRThaiCmap(fontFilename);
    }

} // namespace waavs