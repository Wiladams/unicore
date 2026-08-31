// test_unicode_script_analysis.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_database.h"
#include "unicode_script_analysis.h"


namespace waavs
{
    namespace unicode_script_analysis_test_detail
    {
        // ====================================================================
        // ScriptAnalysisTestCase
        //
        // Fixed-size arrays keep individual test cases self-contained and easy
        // to add to the table below.
        //
        // expectedResolved:
        //
        //      nullptr     -> expected to remain unresolved
        //      ISO tag     -> expected resolved Script
        //
        // ====================================================================

        struct ScriptAnalysisTestCase
        {
            const char* name;

            uint32_t codePoints[8];
            uint32_t codePointCount;

            const char* expectedCandidates[8];
            uint32_t expectedCandidateCount;

            const char* expectedResolved;
        };

        struct ScriptContextTestCase
        {
            const char* name;

            uint32_t leftCodePoint;
            uint32_t middleCodePoint;
            uint32_t rightCodePoint;

            bool expectedChanged;
            const char* expectedMiddleScript;
        };


        // ====================================================================
// TestGrapheme
// ====================================================================

        struct TestGrapheme
        {
            uint32_t codePoints[8]{};
            uint32_t codePointCount{ 0 };
        };


        // ====================================================================
        // TestGraphemeSource
        //
        // Synthetic GraphemeClusterView source.
        //
        // Like GraphemeStream, the returned view remains valid only until the
        // next call to operator(). Reusing mScalars deliberately exercises the
        // ownership promotion performed by UnicodeScriptStream.
        // ====================================================================

        class TestGraphemeSource
        {
        public:
            TestGraphemeSource(const TestGrapheme* graphemes,
                uint32_t graphemeCount) noexcept
                : mGraphemes(graphemes),
                mGraphemeCount(graphemeCount)
            {}


            bool operator()(GraphemeClusterView& out)
            {
                out = {};

                if (mStatus != TextStreamStatus::Ready)
                    return false;

                if (mIndex >= mGraphemeCount)
                {
                    mStatus = TextStreamStatus::End;
                    return false;
                }


                // ------------------------------------------------------------
                // Reuse this storage on every call, matching GraphemeStream's
                // borrowed-view lifetime contract.
                // ------------------------------------------------------------

                mScalars.clear();

                const TestGrapheme& input =
                    mGraphemes[mIndex];

                mScalars.reserve(
                    input.codePointCount);

                for (uint32_t i = 0;
                    i < input.codePointCount;
                    ++i)
                {
                    UnicodeScalar scalar{};

                    scalar.value =
                        input.codePoints[i];

                    scalar.source.begin =
                        mScalarIndex;

                    scalar.source.end =
                        mScalarIndex + 1u;

                    mScalars.push_back(
                        scalar);

                    ++mScalarIndex;
                }


                out.scalars =
                    mScalars.empty()
                    ? nullptr
                    : mScalars.data();

                out.scalarCount =
                    static_cast<uint32_t>(
                        mScalars.size());

                out.normalizedBegin =
                    mNormalizedIndex;

                if (!mScalars.empty())
                {
                    out.source.begin =
                        mScalars.front().source.begin;

                    out.source.end =
                        mScalars.back().source.end;
                }


                mNormalizedIndex +=
                    out.scalarCount;

                ++mIndex;

                return true;
            }


            [[nodiscard]]
            TextStreamStatus status() const noexcept
            {
                return mStatus;
            }


        private:
            const TestGrapheme* mGraphemes{ nullptr };
            uint32_t mGraphemeCount{ 0 };

            uint32_t mIndex{ 0 };

            ScalarIndex mNormalizedIndex{ 0 };
            TextOffset mScalarIndex{ 0 };

            std::vector<UnicodeScalar> mScalars{};

            TextStreamStatus mStatus{ TextStreamStatus::Ready };
        };

        struct ScriptStreamTestCase
        {
            const char* name;

            TestGrapheme graphemes[8]{};
            uint32_t graphemeCount{ 0 };

            const char* expectedScripts[8]{};
        };


        // ====================================================================
        // findScript
        // ====================================================================

        [[nodiscard]]
        static UnicodeScriptIndex findScript(const UnicodeDatabase& database,
            const char* iso15924)
        {
            if (!iso15924)
                return kUnicodeScriptIndexInvalid;

            for (uint32_t i = 0; i < database.scriptCount(); ++i)
            {
                InternedKey tag = database.scriptISO15924(i);

                if (tag && std::strcmp(tag, iso15924) == 0)
                    return static_cast<UnicodeScriptIndex>(i);
            }

            return kUnicodeScriptIndexInvalid;
        }


        // ====================================================================
        // printScriptSet
        // ====================================================================

        static void printScriptSet(const UnicodeDatabase& database,
            const UnicodeScriptSet& set)
        {
            std::printf("{");

            bool first = true;

            for (uint32_t i = 0; i < database.scriptCount(); ++i)
            {
                const UnicodeScriptIndex script =
                    static_cast<UnicodeScriptIndex>(i);

                if (!set.contains(script))
                    continue;

                InternedKey tag = database.scriptISO15924(i);

                if (!first)
                    std::printf(", ");

                std::printf("%s", tag ? tag : "?");

                first = false;
            }

            std::printf("}");
        }


        // ====================================================================
        // buildExpectedSet
        // ====================================================================

        [[nodiscard]]
        static bool buildExpectedSet(const UnicodeDatabase& database,
            const ScriptAnalysisTestCase& test, UnicodeScriptSet& out)
        {
            out.clear();

            for (uint32_t i = 0; i < test.expectedCandidateCount; ++i)
            {
                const UnicodeScriptIndex script =
                    findScript(database, test.expectedCandidates[i]);

                if (script == kUnicodeScriptIndexInvalid)
                {
                    std::printf(
                        "Unknown expected Script tag: %s\n",
                        test.expectedCandidates[i]);

                    return false;
                }

                out.add(script);
            }

            return true;
        }


        [[nodiscard]]
        static bool classifySingleGrapheme(const UnicodeDatabase& database,
            uint32_t codePoint, UnicodeScalar& scalar, ScriptGrapheme& out)
        {
            scalar = {};
            scalar.value = codePoint;
            scalar.source.begin = 0;
            scalar.source.end = 1;

            GraphemeClusterView grapheme{};

            grapheme.scalars = &scalar;
            grapheme.scalarCount = 1;
            grapheme.normalizedBegin = 0;
            grapheme.source.begin = 0;
            grapheme.source.end = 1;

            return classifyGraphemeScript(grapheme, database, out);
        }

        // ====================================================================
        // runCase
        // ====================================================================

        [[nodiscard]]
        static bool runCase(const UnicodeDatabase& database,
            const ScriptAnalysisTestCase& test)
        {
            if (test.codePointCount == 0 ||
                test.codePointCount > 8 ||
                test.expectedCandidateCount > 8)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case: %s\n"
                    "  Invalid test-case definition\n",
                    test.name);

                return false;
            }


            // ---------------------------------------------------------------
            // Construct one synthetic grapheme.
            //
            // The script classifier only needs scalar values, but provide
            // reasonable source metadata so the GraphemeClusterView is valid
            // in the same form used by the real stream.
            // ---------------------------------------------------------------

            UnicodeScalar scalars[8]{};

            for (uint32_t i = 0; i < test.codePointCount; ++i)
            {
                scalars[i].value = test.codePoints[i];
                scalars[i].source.begin = i;
                scalars[i].source.end = i + 1u;
            }


            GraphemeClusterView grapheme{};

            grapheme.scalars = scalars;
            grapheme.scalarCount = test.codePointCount;
            grapheme.normalizedBegin = 0;
            grapheme.source.begin = 0;
            grapheme.source.end = test.codePointCount;


            // ---------------------------------------------------------------
            // Classify.
            // ---------------------------------------------------------------

            ScriptGrapheme actual{};

            if (!classifyGraphemeScript(grapheme, database, actual))
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case: %s\n"
                    "  classifyGraphemeScript() returned false\n",
                    test.name);

                return false;
            }


            // ---------------------------------------------------------------
            // Expected candidate set.
            // ---------------------------------------------------------------

            UnicodeScriptSet expectedCandidates{};

            if (!buildExpectedSet(database, test, expectedCandidates))
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case: %s\n"
                    "  Unable to construct expected candidate set\n",
                    test.name);

                return false;
            }


            if (actual.candidates != expectedCandidates)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case:       %s\n"
                    "  Candidates: expected ",
                    test.name);

                printScriptSet(database, expectedCandidates);

                std::printf(", got ");

                printScriptSet(database, actual.candidates);

                std::printf("\n");

                return false;
            }


            // ---------------------------------------------------------------
            // Expected resolved Script.
            // ---------------------------------------------------------------

            UnicodeScriptIndex expectedResolved =
                kUnicodeScriptIndexInvalid;

            if (test.expectedResolved)
            {
                expectedResolved =
                    findScript(database, test.expectedResolved);

                if (expectedResolved == kUnicodeScriptIndexInvalid)
                {
                    std::printf(
                        "Unicode script analysis: FAIL\n"
                        "  Case: %s\n"
                        "  Unknown expected resolved Script: %s\n",
                        test.name,
                        test.expectedResolved);

                    return false;
                }
            }


            if (actual.script != expectedResolved)
            {
                InternedKey actualTag =
                    actual.script != kUnicodeScriptIndexInvalid
                    ? database.scriptISO15924(actual.script)
                    : nullptr;

                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case:     %s\n"
                    "  Resolved: expected %s, got %s\n",
                    test.name,
                    test.expectedResolved
                    ? test.expectedResolved
                    : "unresolved",
                    actualTag
                    ? actualTag
                    : "unresolved");

                return false;
            }


            return true;
        }


        [[nodiscard]]
        static bool runContextCase(const UnicodeDatabase& database,
            const ScriptContextTestCase& test)
        {
            const UnicodeScriptIndex commonScript =
                findScript(database, "Zyyy");

            const UnicodeScriptIndex inheritedScript =
                findScript(database, "Zinh");

            if (commonScript == kUnicodeScriptIndexInvalid ||
                inheritedScript == kUnicodeScriptIndexInvalid)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Unable to resolve Common or Inherited Script indices\n");

                return false;
            }


            UnicodeScalar leftScalar{};
            UnicodeScalar middleScalar{};
            UnicodeScalar rightScalar{};

            ScriptGrapheme left{};
            ScriptGrapheme middle{};
            ScriptGrapheme right{};


            if (!classifySingleGrapheme(
                database,
                test.leftCodePoint,
                leftScalar,
                left))
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case: %s\n"
                    "  Unable to classify left grapheme\n",
                    test.name);

                return false;
            }


            if (!classifySingleGrapheme(
                database,
                test.middleCodePoint,
                middleScalar,
                middle))
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case: %s\n"
                    "  Unable to classify middle grapheme\n",
                    test.name);

                return false;
            }


            if (!classifySingleGrapheme(
                database,
                test.rightCodePoint,
                rightScalar,
                right))
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case: %s\n"
                    "  Unable to classify right grapheme\n",
                    test.name);

                return false;
            }


            const bool changed =
                resolveMiddleScriptContext(
                    left,
                    middle,
                    right,
                    commonScript,
                    inheritedScript);


            if (changed != test.expectedChanged)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case:    %s\n"
                    "  Changed: expected %s, got %s\n",
                    test.name,
                    test.expectedChanged ? "true" : "false",
                    changed ? "true" : "false");

                return false;
            }


            UnicodeScriptIndex expectedScript =
                kUnicodeScriptIndexInvalid;

            if (test.expectedMiddleScript)
            {
                expectedScript =
                    findScript(
                        database,
                        test.expectedMiddleScript);

                if (expectedScript ==
                    kUnicodeScriptIndexInvalid)
                {
                    std::printf(
                        "Unicode script analysis: FAIL\n"
                        "  Case: %s\n"
                        "  Unknown expected Script: %s\n",
                        test.name,
                        test.expectedMiddleScript);

                    return false;
                }
            }


            if (middle.script != expectedScript)
            {
                InternedKey actualTag =
                    middle.script !=
                    kUnicodeScriptIndexInvalid
                    ? database.scriptISO15924(
                        middle.script)
                    : nullptr;

                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Case:     %s\n"
                    "  Resolved: expected %s, got %s\n",
                    test.name,
                    test.expectedMiddleScript
                    ? test.expectedMiddleScript
                    : "unresolved",
                    actualTag
                    ? actualTag
                    : "unresolved");

                return false;
            }


            return true;
        }

        // ====================================================================
// runStreamCase
// ====================================================================

        [[nodiscard]]
        static bool runStreamCase(const UnicodeDatabase& database,
            const ScriptStreamTestCase& test)
        {
            TestGraphemeSource source(
                test.graphemes,
                test.graphemeCount);

            UnicodeScriptStream<TestGraphemeSource> stream(
                source,
                database);


            if (stream.status() !=
                TextStreamStatus::Ready)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Stream case: %s\n"
                    "  Stream did not initialize\n",
                    test.name);

                return false;
            }


            uint32_t outputIndex = 0;

            ScriptGrapheme item{};

            while (stream(item))
            {
                if (outputIndex >=
                    test.graphemeCount)
                {
                    std::printf(
                        "Unicode script analysis: FAIL\n"
                        "  Stream case: %s\n"
                        "  Produced too many graphemes\n",
                        test.name);

                    return false;
                }


                UnicodeScriptIndex expected =
                    kUnicodeScriptIndexInvalid;

                const char* expectedTag =
                    test.expectedScripts[outputIndex];


                if (expectedTag)
                {
                    expected =
                        findScript(
                            database,
                            expectedTag);

                    if (expected ==
                        kUnicodeScriptIndexInvalid)
                    {
                        std::printf(
                            "Unicode script analysis: FAIL\n"
                            "  Stream case: %s\n"
                            "  Unknown expected Script: %s\n",
                            test.name,
                            expectedTag);

                        return false;
                    }
                }


                if (item.script != expected)
                {
                    InternedKey actualTag =
                        item.script !=
                        kUnicodeScriptIndexInvalid
                        ? database.scriptISO15924(
                            item.script)
                        : nullptr;

                    std::printf(
                        "Unicode script analysis: FAIL\n"
                        "  Stream case: %s\n"
                        "  Grapheme:    %u\n"
                        "  Expected:    %s\n"
                        "  Actual:      %s\n",
                        test.name,
                        outputIndex,
                        expectedTag
                        ? expectedTag
                        : "unresolved",
                        actualTag
                        ? actualTag
                        : "unresolved");

                    return false;
                }


                // ------------------------------------------------------------
                // Verify the emitted grapheme itself survived the upstream
                // borrowed-view lifetime transition.
                // ------------------------------------------------------------

                const TestGrapheme& expectedGrapheme =
                    test.graphemes[outputIndex];

                if (item.grapheme.scalarCount !=
                    expectedGrapheme.codePointCount)
                {
                    std::printf(
                        "Unicode script analysis: FAIL\n"
                        "  Stream case: %s\n"
                        "  Grapheme:    %u\n"
                        "  Scalar count mismatch\n",
                        test.name,
                        outputIndex);

                    return false;
                }


                for (uint32_t i = 0;
                    i < expectedGrapheme.codePointCount;
                    ++i)
                {
                    if (item.grapheme.scalars[i].value !=
                        expectedGrapheme.codePoints[i])
                    {
                        std::printf(
                            "Unicode script analysis: FAIL\n"
                            "  Stream case: %s\n"
                            "  Grapheme:    %u\n"
                            "  Scalar:      %u\n"
                            "  Expected:    U+%04X\n"
                            "  Actual:      U+%04X\n",
                            test.name,
                            outputIndex,
                            i,
                            expectedGrapheme.codePoints[i],
                            item.grapheme.scalars[i].value);

                        return false;
                    }
                }


                ++outputIndex;
            }


            if (stream.status() !=
                TextStreamStatus::End)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Stream case: %s\n"
                    "  Stream did not end cleanly\n",
                    test.name);

                return false;
            }


            if (outputIndex != test.graphemeCount)
            {
                std::printf(
                    "Unicode script analysis: FAIL\n"
                    "  Stream case: %s\n"
                    "  Expected graphemes: %u\n"
                    "  Actual graphemes:   %u\n",
                    test.name,
                    test.graphemeCount,
                    outputIndex);

                return false;
            }


            return true;
        }

    } // namespace unicode_script_analysis_test_detail


    // ========================================================================
    // testUnicodeScriptAnalysis
    // ========================================================================

    static bool testUnicodeScriptAnalysis(const ByteSpan& source)
    {
        using namespace unicode_script_analysis_test_detail;

        UnicodeDatabase database;

        if (!database.reset(source))
        {
            std::printf(
                "Unicode script analysis: FAIL\n"
                "  Unable to load Unicode database\n");

            return false;
        }

        if (!database.hasScript() || !database.hasScriptExtensions())
        {
            std::printf(
                "Unicode script analysis: FAIL\n"
                "  Database lacks Script or Script_Extensions\n");

            return false;
        }


        // ====================================================================
        // Test cases
        //
        // Adding a basic case should normally require only one new row.
        //
        // For example:
        //
        //      {
        //          "Some character",
        //          { 0x1234 }, 1,
        //          { "Abcd" }, 1,
        //          "Abcd"
        //      },
        //
        // Multiple scalars and multiple candidate Scripts are also supported.
        // ====================================================================

        static const ScriptAnalysisTestCase cases[] =
        {
            {
                "Latin capital A",
                { 0x0041 }, 1,
                { "Latn" }, 1,
                "Latn"
            },

            {
                "Greek small alpha",
                { 0x03B1 }, 1,
                { "Grek" }, 1,
                "Grek"
            },

            {
                "Telugu KA",
                { 0x0C15 }, 1,
                { "Telu" }, 1,
                "Telu"
            },

            {
                "Arabic alef",
                { 0x0627 }, 1,
                { "Arab" }, 1,
                "Arab"
            },

            {
                "Latin a plus combining acute",
                { 0x0061, 0x0301 }, 2,
                { "Latn" }, 1,
                "Latn"
            },

            {
                "Latin a plus combining Latin small a",
                { 0x0061, 0x0363 }, 2,
                { "Latn" }, 1,
                "Latn"
            },

            {
                "Katakana KA plus prolonged sound mark",
                { 0x30AB, 0x30FC }, 2,
                { "Kana" }, 1,
                "Kana"
            },

            {
                "Hiragana KA plus prolonged sound mark",
                { 0x304B, 0x30FC }, 2,
                { "Hira" }, 1,
                "Hira"
            },

            {
                "Prolonged sound mark alone",
                { 0x30FC }, 1,
                { "Hira", "Kana" }, 2,
                nullptr
            }
        };

        static const ScriptContextTestCase contextCases[] =
        {
            {
                "Latin Common Latin",
                0x0061,
                0x002E,
                0x0062,
                true,
                "Latn"
            },

            {
                "Latin Inherited Latin",
                0x0061,
                0x0301,
                0x0062,
                true,
                "Latn"
            },

            {
                "Latin Common Arabic",
                0x0061,
                0x002E,
                0x0627,
                false,
                "Zyyy"
            },

            {
                "Katakana ambiguous Katakana",
                0x30AB,
                0x30FC,
                0x30AD,
                true,
                "Kana"
            },

            {
                "Hiragana ambiguous Hiragana",
                0x304B,
                0x30FC,
                0x304D,
                true,
                "Hira"
            },

            {
                "Latin ambiguous Latin",
                0x0061,
                0x30FC,
                0x0062,
                false,
                nullptr
            }
        };


        static const ScriptStreamTestCase streamCases[] =
        {
            {
                "Latin period Latin",
                {
                    { { 0x0061 }, 1 },
                    { { 0x002E }, 1 },
                    { { 0x0062 }, 1 }
                },
                3,
                { "Latn", "Latn", "Latn" }
            },

            {
                "Latin space Latin",
                {
                    { { 0x0061 }, 1 },
                    { { 0x0020 }, 1 },
                    { { 0x0062 }, 1 }
                },
                3,
                { "Latn", "Latn", "Latn" }
            },

            {
                "Latin period Arabic",
                {
                    { { 0x0061 }, 1 },
                    { { 0x002E }, 1 },
                    { { 0x0627 }, 1 }
                },
                3,
                { "Latn", "Zyyy", "Arab" }
            },

            {
                "Katakana prolonged mark Katakana",
                {
                    { { 0x30AB }, 1 },
                    { { 0x30FC }, 1 },
                    { { 0x30AD }, 1 }
                },
                3,
                { "Kana", "Kana", "Kana" }
            },

            {
                "Hiragana prolonged mark Hiragana",
                {
                    { { 0x304B }, 1 },
                    { { 0x30FC }, 1 },
                    { { 0x304D }, 1 }
                },
                3,
                { "Hira", "Hira", "Hira" }
            },

            {
                "Latin prolonged mark Latin",
                {
                    { { 0x0061 }, 1 },
                    { { 0x30FC }, 1 },
                    { { 0x0062 }, 1 }
                },
                3,
                { "Latn", nullptr, "Latn" }
            },

            {
                "Single Latin grapheme",
                {
                    { { 0x0061 }, 1 }
                },
                1,
                { "Latn" }
            },

            {
                "Two Latin graphemes",
                {
                    { { 0x0061 }, 1 },
                    { { 0x0062 }, 1 }
                },
                2,
                { "Latn", "Latn" }
            },

            {
                "Single Common grapheme",
                {
                    { { 0x002E }, 1 }
                },
                1,
                { "Zyyy" }
            }
        };


        // ====================================================================
        // Execute
        // ====================================================================

        uint32_t passed = 0;

        for (const ScriptAnalysisTestCase& test : cases)
        {
            if (!runCase(database, test))
                return false;

            ++passed;
        }

        uint32_t contextPassed = 0;

        for (const ScriptContextTestCase& test : contextCases)
        {
            if (!runContextCase(database, test))
                return false;

            ++contextPassed;
        }

        uint32_t streamPassed = 0;

        for (const ScriptStreamTestCase& test : streamCases)
        {
            if (!runStreamCase(database, test))
                return false;

            ++streamPassed;
        }

        std::printf(
            "Unicode script analysis: PASS\n"
            "  Local cases:       %zu\n"
            "  Local passed:      %u\n"
            "  Context cases:     %zu\n"
            "  Context passed:    %u\n"
            "  Stream cases:      %zu\n"
            "  Stream passed:     %u\n",
            sizeof(cases) / sizeof(cases[0]),
            passed,
            sizeof(contextCases) / sizeof(contextCases[0]),
            contextPassed,
            sizeof(streamCases) / sizeof(streamCases[0]),
            streamPassed);


        return true;
    }


    // ========================================================================
    // Convenience filename overload
    // ========================================================================

    static bool testUnicodeScriptAnalysis(const char* filename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(filename, fileData))
        {
            std::printf(
                "Unicode script analysis: FAIL\n"
                "  Unable to read file: %s\n",
                filename);

            return false;
        }

        return testUnicodeScriptAnalysis(
            ByteSpan(fileData.data(), fileData.size()));
    }

} // namespace waavs