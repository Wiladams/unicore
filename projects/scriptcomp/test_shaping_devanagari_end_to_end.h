// test_shaping_devanagari_end_to_end.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "item_classifier_devanagari.h"
#include "opentype_container.h"
#include "opentype_face.h"
#include "opentype_horizontal_shaper.h"
#include "opentype_nominal_glyphs.h"
#include "opentype_nominal_metrics.h"
#include "recognition_devanagari.h"
#include "script_item_classifier_validator.h"
#include "script_item_recognition.h"
#include "script_recognition_compiler.h"
#include "script_shaping_ir_executor.h"
#include "shaping_devanagari.h"
#include "unicode_database.h"

namespace waavs
{
    struct DevanagariEndToEndCase
    {
        const char* name{ nullptr };
        const uint32_t* values{ nullptr };
        size_t count{ 0 };
    };


    static inline void dumpDevanagariEndToEndShaping(
        const char* phase,
        const OpenTypeShapingBuffer& buffer)
    {
        std::printf(
            "    %s: %zu glyphs\n",
            phase,
            buffer.size());

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const OpenTypeShapingGlyph& glyph = buffer[i];

            std::printf(
                "      [%zu] gid=%u src=[%u,%u)\n",
                i,
                static_cast<unsigned>(glyph.glyphId),
                static_cast<unsigned>(glyph.scalarOffset),
                static_cast<unsigned>(glyph.scalarOffset + glyph.scalarCount));
        }
    }


    static inline void dumpDevanagariEndToEndPositioned(
        const ShapedGlyphBuffer& buffer)
    {
        std::printf(
            "    GPOS: %zu glyphs\n",
            buffer.size());

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const ShapedGlyph& glyph = buffer[i];

            std::printf(
                "      [%zu] gid=%u src=[%u,%u) "
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


    static inline bool validateDevanagariEndToEndProvenance(
        const OpenTypeShapingBuffer& buffer,
        size_t scalarCount)
    {
        if (buffer.empty())
            return false;

        for (const OpenTypeShapingGlyph& glyph : buffer)
        {
            if (glyph.scalarCount == 0)
                return false;

            if (glyph.scalarOffset >= scalarCount)
                return false;

            if (glyph.scalarCount > scalarCount - glyph.scalarOffset)
                return false;
        }

        return true;
    }


    static inline bool validateDevanagariEndToEndProvenance(
        const ShapedGlyphBuffer& buffer,
        size_t scalarCount)
    {
        if (buffer.empty())
            return false;

        for (const ShapedGlyph& glyph : buffer)
        {
            if (glyph.shaping.scalarCount == 0)
                return false;

            if (glyph.shaping.scalarOffset >= scalarCount)
                return false;

            if (glyph.shaping.scalarCount >
                scalarCount - glyph.shaping.scalarOffset)
            {
                return false;
            }
        }

        return true;
    }


    static inline bool runDevanagariEndToEndCase(
        const DevanagariEndToEndCase& testCase,
        const UnicodeDatabase& database,
        const ScriptItemClassifierDescription& classifier,
        const ScriptRecognitionIR& recognitionIR,
        const ScriptShapingIR& shapingIR,
        const FontFace& face,
        const OpenTypeHorizontalFaceTables& tables)
    {
        auto fail =
            [&testCase](const char* message)
            {
                std::printf(
                    "  %-34s FAIL: %s\n",
                    testCase.name ? testCase.name : "(unnamed)",
                    message);

                return false;
            };

        if (!testCase.name || !testCase.values || testCase.count == 0)
            return fail("invalid test case");


        // ------------------------------------------------------------
        // 1. Classify and recognize one Devanagari shaping unit.
        // ------------------------------------------------------------

        ScriptRecognitionResult recognized;

        if (!recognizeScriptItems(
            classifier,
            recognitionIR,
            database,
            testCase.values,
            testCase.count,
            recognized))
        {
            return fail("classification/recognition failed");
        }

        if (recognized.unitCount() != 1)
            return fail("expected exactly one recognized unit");

        const ScriptRecognitionUnit* unit = recognized.unit(0);

        if (!unit)
            return fail("unable to resolve recognized unit");

        if (unit->span.first != 0 ||
            unit->span.count != testCase.count)
        {
            return fail("recognized unit does not cover complete input");
        }


        // ------------------------------------------------------------
        // 2. Build FontRunView.
        // ------------------------------------------------------------

        std::vector<UnicodeScalar> scalars(testCase.count);

        for (size_t i = 0; i < testCase.count; ++i)
            scalars[i].value = testCase.values[i];

        FontRunView run{};

        run.scalars = scalars.data();
        run.scalarCount = scalars.size();
        run.face = face;
        run.bidiLevel = 0;
        run.completeCoverage = true;


        // ------------------------------------------------------------
        // 3. Scalar-domain Script IR.
        //
        // Devanagari currently has no scalar rewriting, but execute this
        // phase anyway so the test follows the real pipeline.
        // ------------------------------------------------------------

        ScriptShapingBuffer scriptInput;

        if (!scriptInput.reset(run))
            return fail("unable to initialize ScriptShapingBuffer");

        if (!applyScriptShapingIRScalars(shapingIR, scriptInput))
            return fail("scalar Script IR failed");

        if (scriptInput.empty())
            return fail("scalar Script IR produced empty input");


        // ------------------------------------------------------------
        // 4. cmap.
        // ------------------------------------------------------------

        OpenTypeShapingBuffer shaping;

        if (!mapOpenTypeNominalGlyphs(scriptInput, shaping))
            return fail("nominal glyph mapping failed");

        if (!validateDevanagariEndToEndProvenance(
            shaping,
            testCase.count))
        {
            return fail("invalid cmap provenance");
        }

        dumpDevanagariEndToEndShaping("CMAP", shaping);


        // ------------------------------------------------------------
        // 5. GSUB + semantic Indic reordering.
        //
        // Unlike the Thai test, this must use the recognition-aware
        // overload because Devanagari Script IR contains Unit, Role and
        // Derived selections plus semantic Indic resolver operations.
        // ------------------------------------------------------------

        ScriptShapingSelectionState selectionState;

        if (!selectionState.reset(shapingIR.derivedSelectionCount))
            return fail("unable to initialize shaping selection state");

        if (tables.gsub &&
            !applyScriptShapingIRGsub(
                tables.gsub->data,
                OTAG("dev2"),
                0,
                shapingIR,
                true,
                true,
                tables.gdef,
                recognized,
                *unit,
                selectionState,
                shaping))
        {
            return fail("Devanagari Script IR GSUB failed");
        }

        if (!validateDevanagariEndToEndProvenance(
            shaping,
            testCase.count))
        {
            return fail("invalid post-GSUB provenance");
        }

        dumpDevanagariEndToEndShaping("GSUB", shaping);


        // ------------------------------------------------------------
        // 6. Nominal horizontal metrics.
        // ------------------------------------------------------------

        ShapedGlyphBuffer positioned;

        if (!buildOpenTypeHorizontalShapedGlyphs(
            shaping,
            positioned))
        {
            return fail("nominal horizontal metrics failed");
        }

        if (!validateDevanagariEndToEndProvenance(
            positioned,
            testCase.count))
        {
            return fail("invalid nominal-metrics provenance");
        }


        // ------------------------------------------------------------
        // 7. GPOS.
        //
        // Devanagari is LTR, so runRightToLeft is false.
        // ------------------------------------------------------------

        if (tables.gpos &&
            !applyScriptShapingIRGpos(
                tables.gpos->data,
                OTAG("dev2"),
                0,
                shapingIR,
                true,
                true,
                tables.gdef,
                positioned,
                false))
        {
            return fail("Devanagari Script IR GPOS failed");
        }

        if (!validateDevanagariEndToEndProvenance(
            positioned,
            testCase.count))
        {
            return fail("invalid final provenance");
        }

        dumpDevanagariEndToEndPositioned(positioned);

        std::printf(
            "  %-34s PASS\n",
            testCase.name);

        return true;
    }


    static bool testDevanagariEndToEnd(
        const ByteSpan& databaseData,
        const ByteSpan& fontData)
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Devanagari end-to-end: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        if (!databaseData)
            return fail("empty Unicode database");

        if (!fontData)
            return fail("empty font data");

        UnicodeDatabase database(databaseData);

        if (!database)
            return fail("invalid Unicode database");


        // ------------------------------------------------------------
        // Compile classification and recognition.
        // ------------------------------------------------------------

        ScriptRecognitionDSL grammar;
        DevanagariItemKinds kinds{};
        DevanagariRecognition recognition{};

        if (!defineDevanagariItemKinds(
            grammar,
            kinds))
        {
            return fail("unable to define Devanagari item kinds");
        }

        ScriptItemClassifierDSL classifier;

        if (!defineDevanagariItemClassifier(
            classifier,
            kinds))
        {
            return fail("unable to define Devanagari classifier");
        }

        if (!defineDevanagariRecognition(
            grammar,
            kinds,
            recognition))
        {
            return fail("unable to define Devanagari recognition");
        }

        const ScriptItemClassifierValidationResult classifierValidation =
            validateScriptItemClassifierDescription(
                classifier.description(),
                grammar.description());

        if (!classifierValidation)
            return fail("Devanagari classifier validation failed");

        ScriptRecognitionIR recognitionIR;

        if (!compileScriptRecognitionIR(
            grammar.description(),
            recognitionIR))
        {
            return fail("unable to compile Devanagari recognition IR");
        }


        // ------------------------------------------------------------
        // Compile Devanagari Script Shaping IR.
        // ------------------------------------------------------------

        ScriptShapingIRBuilder shapingBuilder;
        DevanagariShapingSelections shapingSelections{};

        if (!appendDevanagariShaping(
            kinds,
            recognition,
            shapingBuilder,
            shapingSelections))
        {
            return fail("unable to build Devanagari shaping program");
        }

        ScriptShapingIR shapingIR;

        if (!shapingBuilder.finalize(shapingIR))
            return fail("unable to finalize Devanagari shaping IR");

        if (!shapingSelections.valid())
            return fail("invalid Devanagari shaping selections");


        // ------------------------------------------------------------
        // Open font container.
        // ------------------------------------------------------------

        SharedMemBuff fontBuffer(fontData.size());

        if (!fontBuffer)
            return fail("unable to allocate font buffer");

        std::memcpy(
            fontBuffer.data(),
            fontData.begin(),
            fontData.size());

        OpenTypeContainer container(fontBuffer);

        if (!container.isValid())
            return fail("invalid OpenType container");


        // ------------------------------------------------------------
        // Behavioral smoke corpus.
        //
        // These deliberately exercise semantic paths rather than freezing
        // Noto glyph IDs yet. Once this output agrees with HarfBuzz, the
        // interesting cases can be promoted to exact conformance checks.
        // ------------------------------------------------------------

        static constexpr uint32_t kSimple[] =
        {
            0x0915 // KA
        };

        static constexpr uint32_t kPreBaseMatra[] =
        {
            0x0915, // KA
            0x093F  // VOWEL SIGN I
        };

        static constexpr uint32_t kConjunct[] =
        {
            0x0915, // KA
            0x094D, // VIRAMA
            0x0917  // GA
        };

        static constexpr uint32_t kReph[] =
        {
            0x0930, // RA
            0x094D, // VIRAMA
            0x0915  // KA
        };

        static constexpr uint32_t kRephConjunctMatra[] =
        {
            0x0930, // RA
            0x094D, // VIRAMA
            0x0915, // KA
            0x094D, // VIRAMA
            0x0917, // GA
            0x093F  // VOWEL SIGN I
        };

        static constexpr uint32_t kZWJ[] =
        {
            0x0915, // KA
            0x094D, // VIRAMA
            0x200D, // ZWJ
            0x0917  // GA
        };

        static constexpr uint32_t kZWNJ[] =
        {
            0x0915, // KA
            0x094D, // VIRAMA
            0x200C, // ZWNJ
            0x0917  // GA
        };

        static constexpr uint32_t kExplicitHalant[] =
        {
            0x0915, // KA
            0x094D  // VIRAMA
        };

        static constexpr uint32_t kPostBaseModifier[] =
        {
            0x0915, // KA
            0x093E, // VOWEL SIGN AA
            0x0902  // ANUSVARA
        };

        static constexpr uint32_t kVedic[] =
        {
            0x0915, // KA
            0x0951  // DEVANAGARI STRESS SIGN UDATTA
        };

        static constexpr DevanagariEndToEndCase kCases[] =
        {
            { "Simple consonant", kSimple, sizeof(kSimple) / sizeof(kSimple[0]) },
            { "Pre-base matra", kPreBaseMatra, sizeof(kPreBaseMatra) / sizeof(kPreBaseMatra[0]) },
            { "Conjunct", kConjunct, sizeof(kConjunct) / sizeof(kConjunct[0]) },
            { "Reph", kReph, sizeof(kReph) / sizeof(kReph[0]) },
            { "Reph + conjunct + matra", kRephConjunctMatra, sizeof(kRephConjunctMatra) / sizeof(kRephConjunctMatra[0]) },
            { "ZWJ conjunct", kZWJ, sizeof(kZWJ) / sizeof(kZWJ[0]) },
            { "ZWNJ conjunct", kZWNJ, sizeof(kZWNJ) / sizeof(kZWNJ[0]) },
            { "Explicit halant", kExplicitHalant, sizeof(kExplicitHalant) / sizeof(kExplicitHalant[0]) },
            { "Post-base matra + modifier", kPostBaseModifier, sizeof(kPostBaseModifier) / sizeof(kPostBaseModifier[0]) },
            { "Vedic sign", kVedic, sizeof(kVedic) / sizeof(kVedic[0]) }
        };


        // ------------------------------------------------------------
        // Test every usable face.
        // ------------------------------------------------------------

        size_t faceCount = 0;
        size_t testedFaces = 0;
        size_t passedCases = 0;

        FontFaceView view;

        while (container(view))
        {
            ++faceCount;

            FontFace face = parseFontFace(std::move(view));

            if (!face)
                continue;

            // KA is enough to identify whether this face is relevant.
            if (face.glyphIndex(0x0915) == 0)
                continue;

            FontRunView probeRun{};
            UnicodeScalar probeScalar{};
            probeScalar.value = 0x0915;

            probeRun.scalars = &probeScalar;
            probeRun.scalarCount = 1;
            probeRun.face = face;
            probeRun.bidiLevel = 0;
            probeRun.completeCoverage = true;

            OpenTypeHorizontalFaceTables tables;

            if (!resolveOpenTypeHorizontalFaceTables(
                probeRun,
                tables))
            {
                return fail("unable to resolve OpenType face tables");
            }

            ++testedFaces;

            std::printf(
                "Devanagari end-to-end: FACE %zu\n",
                faceCount - 1);

            for (const DevanagariEndToEndCase& testCase : kCases)
            {
                if (!runDevanagariEndToEndCase(
                    testCase,
                    database,
                    classifier.description(),
                    recognitionIR,
                    shapingIR,
                    face,
                    tables))
                {
                    return false;
                }

                ++passedCases;
            }
        }

        if (faceCount == 0)
            return fail("font container produced no faces");

        if (testedFaces == 0)
            return fail("no face contained Devanagari KA");

        std::printf(
            "Devanagari end-to-end: PASS\n"
            "  Faces:             %zu\n"
            "  Faces tested:      %zu\n"
            "  Cases passed:      %zu\n"
            "  IR instructions:   %zu\n",
            faceCount,
            testedFaces,
            passedCases,
            shapingIR.instructions.size());

        return true;
    }


    static bool testDevanagariEndToEnd(
        const char* databaseFilename,
        const char* fontFilename)
    {
        std::vector<uint8_t> databaseBytes;
        std::vector<uint8_t> fontBytes;

        if (!readFileData(
            databaseFilename,
            databaseBytes))
        {
            std::printf(
                "Devanagari end-to-end: FAIL\n"
                "  Unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        if (!readFileData(
            fontFilename,
            fontBytes))
        {
            std::printf(
                "Devanagari end-to-end: FAIL\n"
                "  Unable to read font\n"
                "  File: %s\n",
                fontFilename ? fontFilename : "(null)");

            return false;
        }

        return testDevanagariEndToEnd(
            ByteSpan(
                databaseBytes.data(),
                databaseBytes.size()),
            ByteSpan(
                fontBytes.data(),
                fontBytes.size()));
    }

} // namespace waavs
