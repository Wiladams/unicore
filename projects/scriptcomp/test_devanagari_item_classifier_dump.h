// test_devanagari_item_classifier_dump.h
#pragma once

#include "test_core.h"

#include "item_classifier_devanagari.h"
#include "script_item_classifier_dump.h"
#include "script_item_classifier_validator.h"

#include <cstdio>
#include <string>

namespace waavs
{
    static bool testDevanagariItemClassifierDump()
    {
        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;
        DevanagariItemKinds kinds{};

        if (!defineDevanagariItemKinds(grammar, kinds))
        {
            std::printf(
                "Devanagari item classifier dump: FAIL: item kinds\n");

            return false;
        }

        if (!defineDevanagariItemClassifier(classifier, kinds))
        {
            std::printf(
                "Devanagari item classifier dump: FAIL: classifier\n");

            return false;
        }


        const ScriptItemClassifierValidationResult validation =
            validateScriptItemClassifierDescription(
                classifier.description(),
                grammar.description());

        if (!validation)
        {
            std::printf(
                "Devanagari item classifier dump: FAIL: validation\n"
                "  Error: %u\n"
                "  Index: %u\n",
                static_cast<unsigned>(validation.error),
                static_cast<unsigned>(validation.index));

            return false;
        }


        std::string output;

        if (!dumpScriptItemClassifierDescription(
            classifier.description(),
            grammar.description(),
            output))
        {
            std::printf(
                "Devanagari item classifier dump: FAIL: dump\n");

            return false;
        }


        std::printf(
            "Devanagari item classifier dump: PASS\n"
            "%s",
            output.c_str());

        return true;
    }

} // namespace waavs