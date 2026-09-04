// test_unicode_shaping_run_context.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../ucdbdemo/test_core.h"

#include "unicode_database.h"
#include "unicode_bidi_analysis.h"
#include "unicode_shaping_run_itemizer.h"


namespace waavs
{
    static bool testUnicodeShapingRunContext(const ByteSpan& databaseData)
    {
        UnicodeDatabase database(databaseData);

        if (!database)
        {
            std::printf(
                "Unicode shaping-run context: FAIL\n"
                "  Unable to initialize Unicode database\n");

            return false;
        }


        auto findScript = [&](const char* iso15924) -> UnicodeScriptIndex
            {
                for (uint32_t i = 0; i < database.scriptCount(); ++i)
                {
                    InternedKey tag = database.scriptISO15924(i);

                    if (tag && std::strcmp(tag, iso15924) == 0)
                        return static_cast<UnicodeScriptIndex>(i);
                }

                return kUnicodeScriptIndexInvalid;
            };


        const UnicodeScriptIndex latin = findScript("Latn");
        const UnicodeScriptIndex arabic = findScript("Arab");
        const UnicodeScriptIndex devanagari = findScript("Deva");
        const UnicodeScriptIndex telugu = findScript("Telu");
        const UnicodeScriptIndex common = findScript("Zyyy");
        const UnicodeScriptIndex inherited = findScript("Zinh");


        if (latin == kUnicodeScriptIndexInvalid ||
            arabic == kUnicodeScriptIndexInvalid ||
            devanagari == kUnicodeScriptIndexInvalid ||
            telugu == kUnicodeScriptIndexInvalid ||
            common == kUnicodeScriptIndexInvalid ||
            inherited == kUnicodeScriptIndexInvalid)
        {
            std::printf(
                "Unicode shaping-run context: FAIL\n"
                "  Unable to locate required Script indices\n");

            return false;
        }


        struct ClusterSpec
        {
            uint32_t codePoint;
            UnicodeScriptIndex script;
            UnicodeBidiLevel level;
        };


        struct RunSpec
        {
            UnicodeScriptIndex script;
            UnicodeBidiLevel level;
            uint32_t scalarCount;
        };


        uint32_t caseCount = 0;
        uint32_t passedCount = 0;


        auto checkCase = [&](const char* description,
            const ClusterSpec* input,
            uint32_t inputCount,
            const RunSpec* expected,
            uint32_t expectedCount) -> bool
            {
                ++caseCount;


                std::vector<UnicodeScalar> scalars(inputCount);
                std::vector<UnicodeBidiClass> originalTypes(inputCount);
                std::vector<UnicodeBidiLevel> levels(inputCount);
                std::vector<ShapingCluster> clusters(inputCount);
                std::vector<ScriptClusterInfo> scripts(inputCount);


                for (uint32_t i = 0; i < inputCount; ++i)
                {
                    scalars[i].value = input[i].codePoint;

                    originalTypes[i] =
                        database.bidiClass(input[i].codePoint);

                    levels[i] =
                        input[i].level;

                    clusters[i].scalarOffset = i;
                    clusters[i].scalarCount = 1;
                    clusters[i].normalizedBegin = i;
                    clusters[i].source = SourceRange{ i, i + 1 };

                    scripts[i].script = input[i].script;
                }


                BidiParagraphView paragraph{};

                paragraph.scalars = scalars.data();
                paragraph.originalTypes = originalTypes.data();
                paragraph.levels = levels.data();
                paragraph.scalarCount = inputCount;

                paragraph.clusters = clusters.data();
                paragraph.scripts = scripts.data();
                paragraph.clusterCount = inputCount;

                paragraph.paragraphLevel = 0;
                paragraph.normalizedBegin = 0;
                paragraph.source = SourceRange{ 0, inputCount };


                ShapingRunItemizer itemizer(
                    paragraph,
                    database);


                if (itemizer.failed())
                {
                    std::printf(
                        "Unicode shaping-run context: FAIL\n"
                        "  Case: %s\n"
                        "  Itemizer rejected paragraph\n",
                        description);

                    return false;
                }


                std::printf(
                    "\n%s\n",
                    description);


                uint32_t runIndex = 0;
                ShapingRunView run{};


                while (itemizer(run))
                {
                    InternedKey scriptTag =
                        database.scriptISO15924(run.script);


                    std::printf(
                        "  Run %u: %-4s level=%u scalars=%u",
                        runIndex,
                        scriptTag ? scriptTag : "????",
                        static_cast<unsigned>(run.bidiLevel),
                        run.scalarCount);


                    std::printf(
                        "  code points:");


                    for (const UnicodeScalar& scalar : run)
                    {
                        std::printf(
                            " U+%04X",
                            scalar.value);
                    }


                    std::printf("\n");


                    if (runIndex >= expectedCount)
                    {
                        std::printf(
                            "Unicode shaping-run context: FAIL\n"
                            "  Case: %s\n"
                            "  Too many runs\n",
                            description);

                        return false;
                    }


                    const RunSpec& wanted =
                        expected[runIndex];


                    if (run.script != wanted.script ||
                        run.bidiLevel != wanted.level ||
                        run.scalarCount != wanted.scalarCount ||
                        run.clusterCount != wanted.scalarCount)
                    {
                        InternedKey expectedTag =
                            database.scriptISO15924(wanted.script);


                        std::printf(
                            "Unicode shaping-run context: FAIL\n"
                            "  Case: %s\n"
                            "  Run:  %u\n"
                            "  Expected: %s level=%u scalars=%u\n"
                            "  Actual:   %s level=%u scalars=%u\n",
                            description,
                            runIndex,
                            expectedTag ? expectedTag : "????",
                            static_cast<unsigned>(wanted.level),
                            wanted.scalarCount,
                            scriptTag ? scriptTag : "????",
                            static_cast<unsigned>(run.bidiLevel),
                            run.scalarCount);

                        return false;
                    }


                    ++runIndex;
                }


                if (!itemizer.ended())
                {
                    std::printf(
                        "Unicode shaping-run context: FAIL\n"
                        "  Case: %s\n"
                        "  Itemizer did not reach End\n",
                        description);

                    return false;
                }


                if (runIndex != expectedCount)
                {
                    std::printf(
                        "Unicode shaping-run context: FAIL\n"
                        "  Case: %s\n"
                        "  Expected runs: %u\n"
                        "  Actual runs:   %u\n",
                        description,
                        expectedCount,
                        runIndex);

                    return false;
                }


                ++passedCount;
                return true;
            };


        // ====================================================================
        // Case 1
        //
        // Matching Latin context absorbs Common.
        //
        // Latn Common Latn
        //      ->
        // Latn
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0061, latin,  0 },
                { 0x0020, common, 0 },
                { 0x0062, latin,  0 }
            };


            const RunSpec expected[] =
            {
                { latin, 0, 3 }
            };


            if (!checkCase(
                "Latn Common Latn",
                input,
                3,
                expected,
                1))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 2
        //
        // Different surrounding scripts leave Common unresolved.
        //
        // Latn Common Arab
        //      ->
        // Latn | Zyyy | Arab
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0061, latin,  0 },
                { 0x0020, common, 0 },
                { 0x0627, arabic, 0 }
            };


            const RunSpec expected[] =
            {
                { latin,  0, 1 },
                { common, 0, 1 },
                { arabic, 0, 1 }
            };


            if (!checkCase(
                "Latn Common Arab",
                input,
                3,
                expected,
                3))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 3
        //
        // Matching Arabic context absorbs Common.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0627, arabic, 1 },
                { 0x0020, common, 1 },
                { 0x0644, arabic, 1 }
            };


            const RunSpec expected[] =
            {
                { arabic, 1, 3 }
            };


            if (!checkCase(
                "Arab Common Arab",
                input,
                3,
                expected,
                1))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 4
        //
        // Different RTL-level Script context remains separated.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0627, arabic, 1 },
                { 0x0020, common, 1 },
                { 0x0061, latin,  1 }
            };


            const RunSpec expected[] =
            {
                { arabic, 1, 1 },
                { common, 1, 1 },
                { latin,  1, 1 }
            };


            if (!checkCase(
                "Arab Common Latn",
                input,
                3,
                expected,
                3))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 5
        //
        // Indic script boundary with neutral separator.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0915, devanagari, 0 },
                { 0x007C, common,     0 },
                { 0x0C15, telugu,     0 }
            };


            const RunSpec expected[] =
            {
                { devanagari, 0, 1 },
                { common,     0, 1 },
                { telugu,     0, 1 }
            };


            if (!checkCase(
                "Deva Common Telu",
                input,
                3,
                expected,
                3))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 6
        //
        // Leading Common adopts following strong Script.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0020, common, 0 },
                { 0x007C, common, 0 },
                { 0x0061, latin,  0 }
            };


            const RunSpec expected[] =
            {
                { latin, 0, 3 }
            };


            if (!checkCase(
                "Leading Common Latn",
                input,
                3,
                expected,
                1))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 7
        //
        // Trailing Common adopts preceding strong Script.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0061, latin,  0 },
                { 0x0020, common, 0 },
                { 0x007C, common, 0 }
            };


            const RunSpec expected[] =
            {
                { latin, 0, 3 }
            };


            if (!checkCase(
                "Latn trailing Common",
                input,
                3,
                expected,
                1))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 8
        //
        // Inherited behaves contextually just like Common.
        //
        // This is synthetic at the cluster level. In real text a combining
        // acute would normally belong to the preceding grapheme cluster.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0061, latin,     0 },
                { 0x0301, inherited, 0 },
                { 0x0062, latin,     0 }
            };


            const RunSpec expected[] =
            {
                { latin, 0, 3 }
            };


            if (!checkCase(
                "Latn Inherited Latn",
                input,
                3,
                expected,
                1))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 9
        //
        // Exact bidi level is a hard run boundary even with identical Script.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0061, latin, 0 },
                { 0x0062, latin, 2 }
            };


            const RunSpec expected[] =
            {
                { latin, 0, 1 },
                { latin, 2, 1 }
            };


            if (!checkCase(
                "Latn level 0 then Latn level 2",
                input,
                2,
                expected,
                2))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 10
        //
        // Script context must not cross a bidi-level boundary.
        //
        // The Common cluster is isolated at level 1, so the Latin clusters at
        // level 0 cannot resolve it.
        // ====================================================================

        {
            const ClusterSpec input[] =
            {
                { 0x0061, latin,  0 },
                { 0x0020, common, 1 },
                { 0x0062, latin,  0 }
            };


            const RunSpec expected[] =
            {
                { latin,  0, 1 },
                { common, 1, 1 },
                { latin,  0, 1 }
            };


            if (!checkCase(
                "Latn level boundary Common Latn",
                input,
                3,
                expected,
                3))
            {
                return false;
            }
        }


        // ====================================================================
        // Summary
        // ====================================================================

        std::printf(
            "\n"
            "Unicode shaping-run context: PASS\n"
            "  Cases:  %u\n"
            "  Passed: %u\n",
            caseCount,
            passedCount);


        return true;
    }


    static bool testUnicodeShapingRunContext(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;


        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Unicode shaping-run context: FAIL\n"
                "  Unable to read database: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testUnicodeShapingRunContext(
            databaseData);
    }

} // namespace waavs