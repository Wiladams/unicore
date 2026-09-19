// test_script_recognition_dump.h
#pragma once

#include "test_core.h"

#include "script_recognition_dsl.h"
#include "script_recognition_dump.h"
#include "script_recognition_validator.h"

namespace waavs
{
    static inline bool runScriptRecognitionDump()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef ra = dsl.kind("Ra");
        const ScriptItemKindRef halant = dsl.kind("Halant");
        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef matraPre = dsl.kind("MatraPre");

        const ScriptRoleRef rephCandidate = dsl.role("RephCandidate");
        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef preBaseMatra = dsl.role("PreBaseMatra");

        if (!ra || !halant || !consonant || !matraPre)
            return false;

        if (!rephCandidate || !base || !preBaseMatra)
            return false;

        const ScriptExprRef Ra = dsl.match(ra);
        const ScriptExprRef H = dsl.match(halant);
        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef MPre = dsl.match(matraPre);

        const ScriptExprRef reph =
            dsl.capture(
                rephCandidate,
                dsl.seq({
                    Ra,
                    H
                    }));

        const ScriptExprRef syllable =
            dsl.seq({
                dsl.opt(reph),
                dsl.capture(base, C),
                dsl.opt(
                    dsl.capture(
                        preBaseMatra,
                        MPre))
                });

        if (!syllable)
            return false;

        if (!dsl.unit("ConsonantSyllable", syllable))
            return false;

        const ScriptRecognitionValidationResult validation =
            validateScriptRecognitionDescription(dsl.description());

        if (!validation)
            return false;

        std::string output;

        if (!dumpScriptRecognitionDescription(dsl.description(), output))
            return false;

        static constexpr const char* expected =
            "kind Ra\n"
            "kind Halant\n"
            "kind Consonant\n"
            "kind MatraPre\n"
            "\n"
            "role RephCandidate\n"
            "role Base\n"
            "role PreBaseMatra\n"
            "\n"
            "unit ConsonantSyllable =\n"
            "    seq(\n"
            "        opt(capture(RephCandidate, seq(\n"
            "            Ra,\n"
            "            Halant\n"
            "        ))),\n"
            "        capture(Base, Consonant),\n"
            "        opt(capture(PreBaseMatra, MatraPre))\n"
            "    )\n";

        if (output != expected)
        {
            printf(
                "Script recognition dump: FAIL\n"
                "Expected:\n"
                "%s"
                "\nActual:\n"
                "%s",
                expected,
                output.c_str());

            return false;
        }

        printf(
            "Script recognition dump: PASS\n"
            "%s",
            output.c_str());

        return true;
    }


    static inline void testScriptRecognitionDump()
    {
        runScriptRecognitionDump();
    }

} // namespace waavs