// test_recognition_devanagari_integration.h
#pragma once

#include "test_core.h"

#include "item_classifier_devanagari.h"
#include "recognition_devanagari.h"
#include "script_item_classifier_validator.h"
#include "script_item_recognition.h"
#include "script_recognition_compiler.h"
#include "unicode_database.h"

#include <cstdio>
#include <initializer_list>
#include <vector>

namespace waavs
{
    static bool expectDevanagariKinds(
        const ScriptRecognitionResult& result,
        std::initializer_list<ScriptItemKindId> expected)
    {
        if (result.kindCount() != expected.size())
            return false;

        size_t index = 0;

        for (ScriptItemKindId kind : expected)
        {
            if (result.kindAt(index) != kind)
                return false;

            ++index;
        }

        return true;
    }


    static bool expectDevanagariUnit(
        const ScriptRecognitionResult& result,
        ScriptUnitTypeId type,
        uint32_t first,
        uint32_t count,
        const ScriptRecognitionUnit*& unit)
    {
        if (result.unitCount() != 1)
            return false;

        unit = result.unit(0);

        return unit &&
            unit->type == type &&
            unit->span.first == first &&
            unit->span.count == count;
    }


    static bool expectDevanagariRole(
        const ScriptRecognitionResult& result,
        const ScriptRecognitionUnit& unit,
        ScriptRoleId role,
        uint32_t first,
        uint32_t count)
    {
        const ScriptRoleBinding* binding = result.roleFor(unit, role);

        return binding &&
            binding->span.first == first &&
            binding->span.count == count;
    }


    static bool testDevanagariRecognitionIntegration(const ByteSpan& databaseData)
    {
        if (!databaseData)
            return false;

        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Devanagari recognition integration: FAIL: invalid Unicode database\n");

            return false;
        }


        // ------------------------------------------------------------
        // Shared script vocabulary.
        // ------------------------------------------------------------

        ScriptRecognitionDSL grammar;
        DevanagariItemKinds kinds{};
        DevanagariRecognition recognition{};

        if (!defineDevanagariItemKinds(grammar, kinds))
        {
            std::printf(
                "Devanagari recognition integration: FAIL: item kinds\n");

            return false;
        }


        // ------------------------------------------------------------
        // Classifier.
        // ------------------------------------------------------------

        ScriptItemClassifierDSL classifier;

        if (!defineDevanagariItemClassifier(classifier, kinds))
        {
            std::printf(
                "Devanagari recognition integration: FAIL: classifier\n");

            return false;
        }


        // ------------------------------------------------------------
        // Recognition grammar.
        // ------------------------------------------------------------

        if (!defineDevanagariRecognition(
            grammar,
            kinds,
            recognition))
        {
            std::printf(
                "Devanagari recognition integration: FAIL: recognition definition\n");

            return false;
        }


        const ScriptItemClassifierValidationResult classifierValidation =
            validateScriptItemClassifierDescription(
                classifier.description(),
                grammar.description());

        if (!classifierValidation)
        {
            std::printf(
                "Devanagari recognition integration: FAIL: classifier validation\n"
                "  Error: %u\n"
                "  Index: %u\n",
                static_cast<unsigned>(classifierValidation.error),
                static_cast<unsigned>(classifierValidation.index));

            return false;
        }


        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(
            grammar.description(),
            ir))
        {
            std::printf(
                "Devanagari recognition integration: FAIL: recognition compile\n");

            return false;
        }


        // ============================================================
        // 1. Simple consonant: KA
        //
        //     KA
        //       -> Consonant
        //       -> ConsonantSyllable
        //       -> BaseCandidate [0,1)
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0915 // KA
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                1,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.consonant.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: KA kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.consonantSyllable.id,
                0,
                1,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: KA unit\n");

                return false;
            }

            if (!expectDevanagariRole(
                result,
                *unit,
                recognition.baseCandidate.id,
                0,
                1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: KA base\n");

                return false;
            }
        }


        // ============================================================
        // 2. Pre-base matra: KA + VOWEL SIGN I
        //
        //     KA I
        //       -> Consonant MatraPre
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0915, // KA
                0x093F  // VOWEL SIGN I
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                2,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.consonant.id,
                    kinds.matraPre.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: pre-base matra kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.consonantSyllable.id,
                0,
                2,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: pre-base matra unit\n");

                return false;
            }

            if (!expectDevanagariRole(result, *unit, recognition.baseCandidate.id, 0, 1) ||
                !expectDevanagariRole(result, *unit, recognition.preBaseMatras.id, 1, 1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: pre-base matra roles\n");

                return false;
            }
        }


        // ============================================================
        // 3. Reph candidate: RA + VIRAMA + KA
        //
        //     Ra Halant Consonant
        //
        //     RephCandidate [0,2)
        //     BaseCandidate [2,3)
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0930, // RA
                0x094D, // VIRAMA
                0x0915  // KA
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                3,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.ra.id,
                    kinds.halant.id,
                    kinds.consonant.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: reph kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.consonantSyllable.id,
                0,
                3,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: reph unit\n");

                return false;
            }

            if (!expectDevanagariRole(result, *unit, recognition.rephCandidate.id, 0, 2) ||
                !expectDevanagariRole(result, *unit, recognition.baseCandidate.id, 2, 1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: reph roles\n");

                return false;
            }
        }


        // ============================================================
        // 4. Consonant sequence: KA + VIRAMA + GA
        //
        //     Consonant Halant Consonant
        //
        //     ConsonantSequence [0,2)
        //     BaseCandidate     [2,3)
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0915, // KA
                0x094D, // VIRAMA
                0x0917  // GA
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                3,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.consonant.id,
                    kinds.halant.id,
                    kinds.consonant.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: consonant sequence kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.consonantSyllable.id,
                0,
                3,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: consonant sequence unit\n");

                return false;
            }

            if (!expectDevanagariRole(result, *unit, recognition.consonantSequence.id, 0, 2) ||
                !expectDevanagariRole(result, *unit, recognition.baseCandidate.id, 2, 1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: consonant sequence roles\n");

                return false;
            }
        }


        // ============================================================
        // 5. Reph + consonant sequence + pre-base matra.
        //
        //     RA VIRAMA KA VIRAMA GA I
        //
        //     Ra Halant Consonant Halant Consonant MatraPre
        //
        //     RephCandidate     [0,2)
        //     ConsonantSequence [2,4)
        //     BaseCandidate     [4,5)
        //     PreBaseMatras     [5,6)
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0930, // RA
                0x094D, // VIRAMA
                0x0915, // KA
                0x094D, // VIRAMA
                0x0917, // GA
                0x093F  // VOWEL SIGN I
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                6,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.ra.id,
                    kinds.halant.id,
                    kinds.consonant.id,
                    kinds.halant.id,
                    kinds.consonant.id,
                    kinds.matraPre.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: full cluster kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.consonantSyllable.id,
                0,
                6,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: full cluster unit\n");

                return false;
            }

            if (!expectDevanagariRole(result, *unit, recognition.rephCandidate.id, 0, 2) ||
                !expectDevanagariRole(result, *unit, recognition.consonantSequence.id, 2, 2) ||
                !expectDevanagariRole(result, *unit, recognition.baseCandidate.id, 4, 1) ||
                !expectDevanagariRole(result, *unit, recognition.preBaseMatras.id, 5, 1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: full cluster roles\n");

                return false;
            }
        }


        // ============================================================
        // 6. Post-base matra + modifier.
        //
        //     KA + AA + ANUSVARA
        //
        //     Consonant MatraPost Bindu
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0915, // KA
                0x093E, // VOWEL SIGN AA
                0x0902  // ANUSVARA
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                3,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.consonant.id,
                    kinds.matraPost.id,
                    kinds.bindu.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: post-base kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.consonantSyllable.id,
                0,
                3,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: post-base unit\n");

                return false;
            }

            if (!expectDevanagariRole(result, *unit, recognition.baseCandidate.id, 0, 1) ||
                !expectDevanagariRole(result, *unit, recognition.postBaseMatras.id, 1, 1) ||
                !expectDevanagariRole(result, *unit, recognition.modifiers.id, 2, 1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: post-base roles\n");

                return false;
            }
        }


        // ============================================================
        // 7. Independent vowel + modifier.
        //
        //     A + ANUSVARA
        //
        //     VowelIndependent Bindu
        //       -> VowelSyllable
        // ============================================================

        {
            const uint32_t input[] =
            {
                0x0905, // LETTER A
                0x0902  // ANUSVARA
            };

            ScriptRecognitionResult result;

            if (!recognizeScriptItems(
                classifier.description(),
                ir,
                database,
                input,
                2,
                result))
            {
                return false;
            }

            if (!expectDevanagariKinds(
                result,
                {
                    kinds.vowelIndependent.id,
                    kinds.bindu.id
                }))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: vowel kinds\n");

                return false;
            }

            const ScriptRecognitionUnit* unit = nullptr;

            if (!expectDevanagariUnit(
                result,
                recognition.vowelSyllable.id,
                0,
                2,
                unit))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: vowel unit\n");

                return false;
            }

            if (!expectDevanagariRole(
                result,
                *unit,
                recognition.modifiers.id,
                1,
                1))
            {
                std::printf(
                    "Devanagari recognition integration: FAIL: vowel modifiers\n");

                return false;
            }
        }


        std::printf(
            "Devanagari recognition integration: PASS\n"
            "  Simple consonant:                    PASS\n"
            "  Pre-base matra:                      PASS\n"
            "  Reph candidate:                      PASS\n"
            "  Consonant sequence:                  PASS\n"
            "  Reph + sequence + pre-base matra:    PASS\n"
            "  Post-base matra + modifier:          PASS\n"
            "  Independent vowel + modifier:        PASS\n");

        return true;
    }


    static bool testDevanagariRecognitionIntegration(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Devanagari recognition integration: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        return testDevanagariRecognitionIntegration(
            ByteSpan(fileData.data(), fileData.size()));
    }

} // namespace waavs