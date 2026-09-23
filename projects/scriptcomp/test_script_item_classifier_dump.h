// test_script_item_classifier_dump.h
#pragma once

#include "test_core.h"

#include "script_item_classifier_dsl.h"
#include "script_item_classifier_dump.h"

namespace waavs
{
    static inline bool runScriptItemClassifierDump()
    {
        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;

        const ScriptItemKindRef ra =
            grammar.kind("Ra");

        const ScriptItemKindRef halant =
            grammar.kind("Halant");

        const ScriptItemKindRef consonant =
            grammar.kind("Consonant");

        const ScriptItemKindRef nukta =
            grammar.kind("Nukta");

        const ScriptItemKindRef matraPre =
            grammar.kind("MatraPre");

        const ScriptItemKindRef other =
            grammar.kind("Other");


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


        std::string output;

        if (!dumpScriptItemClassifierDescription(
            classifier.description(),
            grammar.description(),
            output))
        {
            return false;
        }


        static constexpr const char* expected =
            "rule Ra =\n"
            "    cp(U+0930)\n"
            "\n"
            "rule Halant =\n"
            "    isc(Virama)\n"
            "\n"
            "rule Nukta =\n"
            "    isc(Nukta)\n"
            "\n"
            "rule MatraPre =\n"
            "    ALL(\n"
            "        isc(VowelDependent),\n"
            "        ipc(Left)\n"
            "    )\n"
            "\n"
            "rule Consonant =\n"
            "    isc(Consonant)\n"
            "\n"
            "default Other\n";


        if (output != expected)
        {
            printf(
                "Script item classifier dump: FAIL\n"
                "Expected:\n"
                "%s"
                "\nActual:\n"
                "%s",
                expected,
                output.c_str());

            return false;
        }


        printf(
            "Script item classifier dump: PASS\n"
            "%s",
            output.c_str());

        return true;
    }


    static inline void testScriptItemClassifierDump()
    {
        runScriptItemClassifierDump();
    }

} // namespace waavs