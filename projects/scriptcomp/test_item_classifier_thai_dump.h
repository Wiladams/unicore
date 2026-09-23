// test_item_classifier_thai_dump.h
#pragma once

#include "test_core.h"

#include "item_classifier_thai.h"
#include "script_item_classifier_dump.h"
#include "script_item_classifier_validator.h"

#include <cstdio>
#include <string>

namespace waavs
{
    static bool testThaiItemClassifierDump()
    {
        ScriptRecognitionDSL grammar;
        ScriptItemClassifierDSL classifier;
        ThaiItemKinds kinds{};

        if (!defineThaiItemKinds(grammar, kinds))
        {
            std::printf(
                "Thai item classifier dump: FAIL: item kinds\n");

            return false;
        }

        if (!defineThaiItemClassifier(classifier, kinds))
        {
            std::printf(
                "Thai item classifier dump: FAIL: classifier\n");

            return false;
        }


        const ScriptItemClassifierValidationResult validation =
            validateScriptItemClassifierDescription(
                classifier.description(),
                grammar.description());

        if (!validation)
        {
            std::printf(
                "Thai item classifier dump: FAIL: validation\n"
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
                "Thai item classifier dump: FAIL: dump\n");

            return false;
        }


        std::printf(
            "Thai item classifier dump: PASS\n"
            "%s",
            output.c_str());

        return true;
    }

} // namespace waavs