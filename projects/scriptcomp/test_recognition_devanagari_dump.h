// test_recognition_devanagari_dump.h
#pragma once

#include "test_core.h"

#include "item_classifier_devanagari.h"
#include "recognition_devanagari.h"
#include "script_recognition_compiler.h"
#include "script_recognition_dump.h"
#include "script_recognition_validator.h"

#include <cstdio>
#include <string>

namespace waavs
{
    static bool testDevanagariRecognitionDump()
    {
        ScriptRecognitionDSL grammar;
        DevanagariItemKinds kinds{};
        DevanagariRecognition recognition{};

        if (!defineDevanagariItemKinds(grammar, kinds))
        {
            std::printf(
                "Devanagari recognition dump: FAIL: item kinds\n");

            return false;
        }

        if (!defineDevanagariRecognition(
            grammar,
            kinds,
            recognition))
        {
            std::printf(
                "Devanagari recognition dump: FAIL: recognition definition\n");

            return false;
        }


        const ScriptRecognitionValidationResult validation =
            validateScriptRecognitionDescription(
                grammar.description());

        if (!validation)
        {
            std::printf(
                "Devanagari recognition dump: FAIL: validation\n"
                "  Error: %u\n"
                "  Index: %u\n",
                static_cast<unsigned>(validation.error),
                static_cast<unsigned>(validation.index));

            return false;
        }


        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(
            grammar.description(),
            ir))
        {
            std::printf(
                "Devanagari recognition dump: FAIL: compile\n");

            return false;
        }


        std::string output;

        if (!dumpScriptRecognitionDescription(
            grammar.description(),
            output))
        {
            std::printf(
                "Devanagari recognition dump: FAIL: dump\n");

            return false;
        }


        std::printf(
            "Devanagari recognition dump: PASS\n"
            "  Kinds:        %zu\n"
            "  Roles:        %zu\n"
            "  Unit types:   %zu\n"
            "  Units:        %zu\n"
            "  Expressions:  %zu\n"
            "  IR ops:       %zu\n"
            "\n"
            "%s",
            grammar.description().kinds.size(),
            grammar.description().roles.size(),
            grammar.description().unitTypes.size(),
            grammar.description().units.size(),
            grammar.description().expressions.size(),
            ir.size(),
            output.c_str());

        return true;
    }

} // namespace waavs