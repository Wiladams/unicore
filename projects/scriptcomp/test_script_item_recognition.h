// test_script_item_recognition.h
#pragma once

#include "test_core.h"

#include "script_item_classifier_dsl.h"
#include "script_item_classifier_validator.h"
#include "script_item_recognition.h"
#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"
#include "unicode_database.h"

#include <cstdio>
#include <vector>

namespace waavs
{
    static bool testScriptItemRecognition(const ByteSpan& databaseData)
    {
        if (!databaseData)
            return false;

        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Script item recognition: FAIL: invalid Unicode database\n");

            return false;
        }


        // ------------------------------------------------------------
        // Recognition vocabulary.
        // ------------------------------------------------------------

        ScriptRecognitionDSL grammar;

        const ScriptItemKindRef ra = grammar.kind("Ra");
        const ScriptItemKindRef halant = grammar.kind("Halant");
        const ScriptItemKindRef consonant = grammar.kind("Consonant");
        const ScriptItemKindRef nukta = grammar.kind("Nukta");
        const ScriptItemKindRef matraPre = grammar.kind("MatraPre");
        const ScriptItemKindRef other = grammar.kind("Other");

        const ScriptRoleRef rephCandidate = grammar.role("RephCandidate");
        const ScriptRoleRef base = grammar.role("Base");
        const ScriptRoleRef preBaseMatra = grammar.role("PreBaseMatra");

        const ScriptUnitTypeRef consonantSyllable =
            grammar.unitType("ConsonantSyllable");

        if (!ra || !halant || !consonant || !nukta || !matraPre || !other ||
            !rephCandidate || !base || !preBaseMatra || !consonantSyllable)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Recognition grammar.
        //
        //     [Ra Halant]?
        //     (Consonant Halant)*
        //     Consonant
        //     MatraPre?
        //
        // The final consonant becomes Base.
        // ------------------------------------------------------------

        const ScriptExprRef reph =
            grammar.capture(
                rephCandidate,
                grammar.seq({
                    grammar.match(ra),
                    grammar.match(halant)
                    }));

        const ScriptExprRef conjunctPrefix =
            grammar.seq({
                grammar.match(consonant),
                grammar.match(halant)
                });

        const ScriptExprRef baseExpr =
            grammar.capture(
                base,
                grammar.match(consonant));

        const ScriptExprRef preBaseMatraExpr =
            grammar.capture(
                preBaseMatra,
                grammar.match(matraPre));

        if (!reph || !conjunctPrefix || !baseExpr || !preBaseMatraExpr)
            return false;

        const ScriptExprRef syllable =
            grammar.seq({
                grammar.opt(reph),
                grammar.zeroOrMore(conjunctPrefix),
                baseExpr,
                grammar.opt(preBaseMatraExpr)
                });

        if (!syllable)
            return false;

        if (!grammar.unit(consonantSyllable, syllable))
            return false;


        // ------------------------------------------------------------
        // Compile recognition grammar.
        // ------------------------------------------------------------

        ScriptRecognitionIR recognitionIR;

        if (!compileScriptRecognitionIR(
            grammar.description(),
            recognitionIR))
        {
            std::printf(
                "Script item recognition: FAIL: recognition compile\n");

            return false;
        }


        // ------------------------------------------------------------
        // Item classifier.
        // ------------------------------------------------------------

        ScriptItemClassifierDSL classifier;

        if (!classifier.rule(
            ra,
            classifier.cp(0x0930)))
        {
            return false;
        }

        if (!classifier.rule(
            halant,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Virama)))
        {
            return false;
        }

        if (!classifier.rule(
            nukta,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Nukta)))
        {
            return false;
        }

        if (!classifier.rule(
            matraPre,
            classifier.ALL({
                classifier.isc(
                    UnicodeIndicSyllabicCategory::VowelDependent),

                classifier.ipc(
                    UnicodeIndicPositionalCategory::Left)
                })))
        {
            return false;
        }

        if (!classifier.rule(
            consonant,
            classifier.isc(
                UnicodeIndicSyllabicCategory::Consonant)))
        {
            return false;
        }

        if (!classifier.defaultKind(other))
            return false;


        const ScriptItemClassifierValidationResult classifierValidation =
            validateScriptItemClassifierDescription(
                classifier.description(),
                grammar.description());

        if (!classifierValidation)
        {
            std::printf(
                "Script item recognition: FAIL: classifier validation\n");

            return false;
        }


        // ------------------------------------------------------------
        // Real Devanagari input:
        //
        //   RA VIRAMA KA VIRAMA GA VOWEL SIGN I
        //
        // Expected kinds:
        //
        //   Ra Halant Consonant Halant Consonant MatraPre
        // ------------------------------------------------------------

        static constexpr uint32_t codePoints[] =
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
            recognitionIR,
            database,
            codePoints,
            static_cast<uint32_t>(
                sizeof(codePoints) / sizeof(codePoints[0])),
            result))
        {
            std::printf(
                "Script item recognition: FAIL: recognition\n");

            return false;
        }


        // ------------------------------------------------------------
        // Classified kind stream.
        // ------------------------------------------------------------

        static ScriptItemKindId expectedKinds[] =
        {
            ra.id,
            halant.id,
            consonant.id,
            halant.id,
            consonant.id,
            matraPre.id
        };

        if (result.kindCount() !=
            sizeof(expectedKinds) / sizeof(expectedKinds[0]))
        {
            std::printf(
                "Script item recognition: FAIL: kind count\n");

            return false;
        }

        for (size_t i = 0;
            i < sizeof(expectedKinds) / sizeof(expectedKinds[0]);
            ++i)
        {
            if (result.kindAt(i) != expectedKinds[i])
            {
                std::printf(
                    "Script item recognition: FAIL: kind %zu\n",
                    i);

                return false;
            }
        }


        // ------------------------------------------------------------
        // Recognized unit.
        // ------------------------------------------------------------

        if (result.unitCount() != 1)
        {
            std::printf(
                "Script item recognition: FAIL: expected one unit\n");

            return false;
        }

        const ScriptRecognitionUnit* unit =
            result.unit(0);

        if (!unit ||
            unit->type != consonantSyllable.id ||
            unit->span.first != 0 ||
            unit->span.count != 6)
        {
            std::printf(
                "Script item recognition: FAIL: unit span/type\n");

            return false;
        }


        // ------------------------------------------------------------
        // RephCandidate = RA VIRAMA
        // ------------------------------------------------------------

        const ScriptRoleBinding* rephBinding =
            result.roleFor(
                *unit,
                rephCandidate.id);

        if (!rephBinding ||
            rephBinding->span.first != 0 ||
            rephBinding->span.count != 2)
        {
            std::printf(
                "Script item recognition: FAIL: RephCandidate\n");

            return false;
        }


        // ------------------------------------------------------------
        // Base = final consonant, GA.
        // ------------------------------------------------------------

        const ScriptRoleBinding* baseBinding =
            result.roleFor(
                *unit,
                base.id);

        if (!baseBinding ||
            baseBinding->span.first != 4 ||
            baseBinding->span.count != 1)
        {
            std::printf(
                "Script item recognition: FAIL: Base\n");

            return false;
        }


        // ------------------------------------------------------------
        // PreBaseMatra = VOWEL SIGN I.
        // ------------------------------------------------------------

        const ScriptRoleBinding* matraBinding =
            result.roleFor(
                *unit,
                preBaseMatra.id);

        if (!matraBinding ||
            matraBinding->span.first != 5 ||
            matraBinding->span.count != 1)
        {
            std::printf(
                "Script item recognition: FAIL: PreBaseMatra\n");

            return false;
        }


        std::printf(
            "Script item recognition: PASS\n"
            "  Input scalars:       6\n"
            "  Classified kinds:    6\n"
            "  Unit:                ConsonantSyllable [0,6)\n"
            "  RephCandidate:       [0,2)\n"
            "  Base:                [4,5)\n"
            "  PreBaseMatra:        [5,6)\n");

        return true;
    }


    static bool testScriptItemRecognition(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Script item recognition: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        return testScriptItemRecognition(
            ByteSpan(fileData.data(), fileData.size()));
    }

} // namespace waavs