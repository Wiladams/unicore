// script_item_recognition.h
#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "script_item_classifier_ucdb.h"
#include "script_recognition_interpreter.h"

namespace waavs
{
    [[nodiscard]]
    static inline bool classifyScriptItems(
        const ScriptItemClassifierDescription& classifier,
        const UnicodeDatabase& database,
        const uint32_t* codePoints,
        uint32_t count,
        std::vector<ScriptItemKindId>& result)
    {
        if (count != 0 && !codePoints)
            return false;

        std::vector<ScriptItemKindId> working;
        working.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            const ScriptItemKindId kind = classifyScriptItem(classifier, database, codePoints[i]);

            if (kind == kScriptItemKindInvalid)
                return false;

            working.push_back(kind);
        }

        result = std::move(working);
        return true;
    }


    [[nodiscard]]
    static inline bool classifyScriptItems(
        const ScriptItemClassifierDescription& classifier,
        const UnicodeDatabase& database,
        const std::vector<uint32_t>& codePoints,
        std::vector<ScriptItemKindId>& result)
    {
        if (codePoints.size() > std::numeric_limits<uint32_t>::max())
            return false;

        return classifyScriptItems(
            classifier,
            database,
            codePoints.data(),
            static_cast<uint32_t>(codePoints.size()),
            result);
    }


    [[nodiscard]]
    static inline bool recognizeScriptItems(
        const ScriptItemClassifierDescription& classifier,
        const ScriptRecognitionIR& recognition,
        const UnicodeDatabase& database,
        const uint32_t* codePoints,
        uint32_t count,
        ScriptRecognitionResult& result)
    {
        std::vector<ScriptItemKindId> kinds;

        if (!classifyScriptItems(classifier, database, codePoints, count, kinds))
            return false;

        return recognizeScriptKinds(recognition, kinds, result);
    }


    [[nodiscard]]
    static inline bool recognizeScriptItems(
        const ScriptItemClassifierDescription& classifier,
        const ScriptRecognitionIR& recognition,
        const UnicodeDatabase& database,
        const std::vector<uint32_t>& codePoints,
        ScriptRecognitionResult& result)
    {
        if (codePoints.size() > std::numeric_limits<uint32_t>::max())
            return false;

        return recognizeScriptItems(
            classifier,
            recognition,
            database,
            codePoints.data(),
            static_cast<uint32_t>(codePoints.size()),
            result);
    }

} // namespace waavs