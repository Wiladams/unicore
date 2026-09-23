// test_script_item_classifier_ucdb.h
#pragma once

#include "test_core.h"

#include "script_item_classifier_dsl.h"
#include "script_item_classifier_ucdb.h"
#include "unicode_database.h"

#include <cstdio>
#include <vector>

namespace waavs
{
    static bool testScriptItemClassifierUCDB(const ByteSpan& databaseData)
    {
        if (!databaseData)
            return false;

        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Script item classifier UCDB: FAIL: invalid Unicode database\n");

            return false;
        }


        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;

        const ScriptItemKindRef ra = grammar.kind("Ra");
        const ScriptItemKindRef halant = grammar.kind("Halant");
        const ScriptItemKindRef consonant = grammar.kind("Consonant");
        const ScriptItemKindRef nukta = grammar.kind("Nukta");
        const ScriptItemKindRef matraPre = grammar.kind("MatraPre");
        const ScriptItemKindRef other = grammar.kind("Other");

        if (!ra || !halant || !consonant || !nukta || !matraPre || !other)
            return false;


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


        const ScriptItemClassifierDescription& description =
            classifier.description();


        if (classifyScriptItem(
            description,
            database,
            0x0930) != ra.id)
        {
            return false;
        }


        if (classifyScriptItem(
            description,
            database,
            0x094D) != halant.id)
        {
            return false;
        }


        if (classifyScriptItem(
            description,
            database,
            0x093C) != nukta.id)
        {
            return false;
        }


        if (classifyScriptItem(
            description,
            database,
            0x093F) != matraPre.id)
        {
            return false;
        }


        if (classifyScriptItem(
            description,
            database,
            0x0915) != consonant.id)
        {
            return false;
        }


        if (classifyScriptItem(
            description,
            database,
            0x0020) != other.id)
        {
            return false;
        }


        std::printf(
            "Script item classifier UCDB: PASS\n"
            "  U+0930 -> Ra\n"
            "  U+094D -> Halant\n"
            "  U+093C -> Nukta\n"
            "  U+093F -> MatraPre\n"
            "  U+0915 -> Consonant\n"
            "  U+0020 -> Other\n");

        return true;
    }


    static bool testScriptItemClassifierUCDB(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Script item classifier UCDB: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename ? databaseFilename : "(null)");

            return false;
        }

        return testScriptItemClassifierUCDB(
            ByteSpan(fileData.data(), fileData.size()));
    }

} // namespace waavs