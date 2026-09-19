// test_script_shaping_ir_thai_conformance.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "script_shaping_buffer.h"
#include "script_shaping_ir_executor.h"
#include "script_shaping_policy_compiler.h"

namespace waavs
{
    struct ThaiScalarExpected
    {
        uint32_t value;
        uint32_t sourceOffset;
        bool generated;
    };

    struct ThaiScalarConformanceCase
    {
        const char* name;
        const uint32_t* input;
        size_t inputCount;
        const ThaiScalarExpected* expected;
        size_t expectedCount;
    };


    static inline bool runThaiScalarConformanceCase(
        const ScriptShapingIR& ir,
        const ThaiScalarConformanceCase& test)
    {
        UnicodeScalar scalars[16]{};

        if (test.inputCount > 16)
            return false;

        for (size_t i = 0; i < test.inputCount; ++i)
            scalars[i].value = test.input[i];

        FontRunView run{};

        run.scalars = scalars;
        run.scalarCount = test.inputCount;
        run.bidiLevel = 0;
        run.completeCoverage = true;

        ScriptShapingBuffer buffer;

        if (!buffer.reset(run))
            return false;

        if (!applyScriptShapingIRScalars(ir, buffer))
            return false;

        if (buffer.size() != test.expectedCount)
            return false;

        for (size_t i = 0; i < test.expectedCount; ++i)
        {
            const ScriptShapingItem& actual = buffer[i];
            const ThaiScalarExpected& expected = test.expected[i];

            if (actual.value != expected.value)
                return false;

            if (actual.scalarOffset != expected.sourceOffset)
                return false;

            if (actual.scalarCount != 1)
                return false;

            const bool generated =
                (actual.flags & ScriptShapingItemFlagGenerated) != 0;

            if (generated != expected.generated)
                return false;
        }

        return true;
    }


    static inline bool runScriptShapingIRThaiConformance()
    {
        ScriptShapingIR ir;

        if (!compileScriptShapingIRForScript(OTAG("thai"), ir))
            return false;


        // ------------------------------------------------------------
        // 1. Plain consonant.
        // ------------------------------------------------------------

        static constexpr uint32_t inputPlain[] =
        {
            0x0E14
        };

        static constexpr ThaiScalarExpected expectedPlain[] =
        {
            { 0x0E14, 0, false }
        };


        // ------------------------------------------------------------
        // 2. Plain tone marks remain unchanged.
        // ------------------------------------------------------------

        static constexpr uint32_t inputTone1[] =
        {
            0x0E14, 0x0E48
        };

        static constexpr ThaiScalarExpected expectedTone1[] =
        {
            { 0x0E14, 0, false },
            { 0x0E48, 1, false }
        };

        static constexpr uint32_t inputTone2[] =
        {
            0x0E14, 0x0E49
        };

        static constexpr ThaiScalarExpected expectedTone2[] =
        {
            { 0x0E14, 0, false },
            { 0x0E49, 1, false }
        };

        static constexpr uint32_t inputTone3[] =
        {
            0x0E14, 0x0E4A
        };

        static constexpr ThaiScalarExpected expectedTone3[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4A, 1, false }
        };

        static constexpr uint32_t inputTone4[] =
        {
            0x0E14, 0x0E4B
        };

        static constexpr ThaiScalarExpected expectedTone4[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4B, 1, false }
        };


        // ------------------------------------------------------------
        // 3. SARA AM without a tone mark.
        //
        // U+0E33 -> U+0E4D U+0E32
        // ------------------------------------------------------------

        static constexpr uint32_t inputSaraAm[] =
        {
            0x0E14, 0x0E33
        };

        static constexpr ThaiScalarExpected expectedSaraAm[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4D, 1, true },
            { 0x0E32, 1, true }
        };


        // ------------------------------------------------------------
        // 4-7. SARA AM following each Thai tone mark.
        //
        // The generated NIKHAHIT moves before the tone mark.
        // ------------------------------------------------------------

        static constexpr uint32_t inputSaraAmTone1[] =
        {
            0x0E14, 0x0E48, 0x0E33
        };

        static constexpr ThaiScalarExpected expectedSaraAmTone1[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4D, 2, true },
            { 0x0E48, 1, false },
            { 0x0E32, 2, true }
        };

        static constexpr uint32_t inputSaraAmTone2[] =
        {
            0x0E14, 0x0E49, 0x0E33
        };

        static constexpr ThaiScalarExpected expectedSaraAmTone2[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4D, 2, true },
            { 0x0E49, 1, false },
            { 0x0E32, 2, true }
        };

        static constexpr uint32_t inputSaraAmTone3[] =
        {
            0x0E14, 0x0E4A, 0x0E33
        };

        static constexpr ThaiScalarExpected expectedSaraAmTone3[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4D, 2, true },
            { 0x0E4A, 1, false },
            { 0x0E32, 2, true }
        };

        static constexpr uint32_t inputSaraAmTone4[] =
        {
            0x0E14, 0x0E4B, 0x0E33
        };

        static constexpr ThaiScalarExpected expectedSaraAmTone4[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4D, 2, true },
            { 0x0E4B, 1, false },
            { 0x0E32, 2, true }
        };


        // ------------------------------------------------------------
        // 8. Literal NIKHAHIT is not treated as generated.
        //
        // This deliberately uses tone + literal NIKHAHIT to verify that
        // ScalarMoveLeftAcrossRange is constrained by transient state.
        // ------------------------------------------------------------

        static constexpr uint32_t inputLiteralNikhahit[] =
        {
            0x0E14, 0x0E4B, 0x0E4D
        };

        static constexpr ThaiScalarExpected expectedLiteralNikhahit[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4B, 1, false },
            { 0x0E4D, 2, false }
        };


        // ------------------------------------------------------------
        // 9. A non-tone barrier stops movement.
        //
        // Generated NIKHAHIT may cross only the immediately preceding
        // contiguous U+0E48..U+0E4B range.
        // ------------------------------------------------------------

        static constexpr uint32_t inputBarrier[] =
        {
            0x0E14,
            0x0E4B,
            0x0E01,
            0x0E33
        };

        static constexpr ThaiScalarExpected expectedBarrier[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4B, 1, false },
            { 0x0E01, 2, false },
            { 0x0E4D, 3, true },
            { 0x0E32, 3, true }
        };


        // ------------------------------------------------------------
        // 10. Two independent SARA AM clusters.
        //
        // This verifies repeated replacement and repeated movement in one
        // scalar-domain pass.
        // ------------------------------------------------------------

        static constexpr uint32_t inputTwoClusters[] =
        {
            0x0E14,
            0x0E48,
            0x0E33,

            0x0E01,
            0x0E4B,
            0x0E33
        };

        static constexpr ThaiScalarExpected expectedTwoClusters[] =
        {
            { 0x0E14, 0, false },
            { 0x0E4D, 2, true },
            { 0x0E48, 1, false },
            { 0x0E32, 2, true },

            { 0x0E01, 3, false },
            { 0x0E4D, 5, true },
            { 0x0E4B, 4, false },
            { 0x0E32, 5, true }
        };


        // ------------------------------------------------------------
        // 11. SARA AM at the beginning of a run.
        //
        // We are testing transformation semantics here, not validity.
        // There is no preceding tone mark, so decomposition occurs without
        // movement.
        // ------------------------------------------------------------

        static constexpr uint32_t inputLeadingSaraAm[] =
        {
            0x0E33
        };

        static constexpr ThaiScalarExpected expectedLeadingSaraAm[] =
        {
            { 0x0E4D, 0, true },
            { 0x0E32, 0, true }
        };


        // ------------------------------------------------------------
        // 12. Adjacent ordinary scalars are preserved around SARA AM.
        // ------------------------------------------------------------

        static constexpr uint32_t inputSurrounded[] =
        {
            0x0E01,
            0x0E33,
            0x0E14
        };

        static constexpr ThaiScalarExpected expectedSurrounded[] =
        {
            { 0x0E01, 0, false },
            { 0x0E4D, 1, true },
            { 0x0E32, 1, true },
            { 0x0E14, 2, false }
        };


        static constexpr ThaiScalarConformanceCase tests[] =
        {
            {
                "plain consonant",
                inputPlain,
                sizeof(inputPlain) / sizeof(inputPlain[0]),
                expectedPlain,
                sizeof(expectedPlain) / sizeof(expectedPlain[0])
            },
            {
                "tone U+0E48",
                inputTone1,
                sizeof(inputTone1) / sizeof(inputTone1[0]),
                expectedTone1,
                sizeof(expectedTone1) / sizeof(expectedTone1[0])
            },
            {
                "tone U+0E49",
                inputTone2,
                sizeof(inputTone2) / sizeof(inputTone2[0]),
                expectedTone2,
                sizeof(expectedTone2) / sizeof(expectedTone2[0])
            },
            {
                "tone U+0E4A",
                inputTone3,
                sizeof(inputTone3) / sizeof(inputTone3[0]),
                expectedTone3,
                sizeof(expectedTone3) / sizeof(expectedTone3[0])
            },
            {
                "tone U+0E4B",
                inputTone4,
                sizeof(inputTone4) / sizeof(inputTone4[0]),
                expectedTone4,
                sizeof(expectedTone4) / sizeof(expectedTone4[0])
            },
            {
                "SARA AM",
                inputSaraAm,
                sizeof(inputSaraAm) / sizeof(inputSaraAm[0]),
                expectedSaraAm,
                sizeof(expectedSaraAm) / sizeof(expectedSaraAm[0])
            },
            {
                "SARA AM + tone U+0E48",
                inputSaraAmTone1,
                sizeof(inputSaraAmTone1) / sizeof(inputSaraAmTone1[0]),
                expectedSaraAmTone1,
                sizeof(expectedSaraAmTone1) / sizeof(expectedSaraAmTone1[0])
            },
            {
                "SARA AM + tone U+0E49",
                inputSaraAmTone2,
                sizeof(inputSaraAmTone2) / sizeof(inputSaraAmTone2[0]),
                expectedSaraAmTone2,
                sizeof(expectedSaraAmTone2) / sizeof(expectedSaraAmTone2[0])
            },
            {
                "SARA AM + tone U+0E4A",
                inputSaraAmTone3,
                sizeof(inputSaraAmTone3) / sizeof(inputSaraAmTone3[0]),
                expectedSaraAmTone3,
                sizeof(expectedSaraAmTone3) / sizeof(expectedSaraAmTone3[0])
            },
            {
                "SARA AM + tone U+0E4B",
                inputSaraAmTone4,
                sizeof(inputSaraAmTone4) / sizeof(inputSaraAmTone4[0]),
                expectedSaraAmTone4,
                sizeof(expectedSaraAmTone4) / sizeof(expectedSaraAmTone4[0])
            },
            {
                "literal NIKHAHIT",
                inputLiteralNikhahit,
                sizeof(inputLiteralNikhahit) / sizeof(inputLiteralNikhahit[0]),
                expectedLiteralNikhahit,
                sizeof(expectedLiteralNikhahit) / sizeof(expectedLiteralNikhahit[0])
            },
            {
                "movement barrier",
                inputBarrier,
                sizeof(inputBarrier) / sizeof(inputBarrier[0]),
                expectedBarrier,
                sizeof(expectedBarrier) / sizeof(expectedBarrier[0])
            },
            {
                "two clusters",
                inputTwoClusters,
                sizeof(inputTwoClusters) / sizeof(inputTwoClusters[0]),
                expectedTwoClusters,
                sizeof(expectedTwoClusters) / sizeof(expectedTwoClusters[0])
            },
            {
                "leading SARA AM",
                inputLeadingSaraAm,
                sizeof(inputLeadingSaraAm) / sizeof(inputLeadingSaraAm[0]),
                expectedLeadingSaraAm,
                sizeof(expectedLeadingSaraAm) / sizeof(expectedLeadingSaraAm[0])
            },
            {
                "surrounded SARA AM",
                inputSurrounded,
                sizeof(inputSurrounded) / sizeof(inputSurrounded[0]),
                expectedSurrounded,
                sizeof(expectedSurrounded) / sizeof(expectedSurrounded[0])
            }
        };


        size_t passedCount = 0;

        for (const ThaiScalarConformanceCase& test : tests)
        {
            if (!runThaiScalarConformanceCase(ir, test))
            {
                std::printf(
                    "Script shaping IR Thai conformance: FAIL\n"
                    "  Case: %s\n",
                    test.name);

                return false;
            }

            ++passedCount;
        }

        std::printf(
            "Script shaping IR Thai conformance: PASS\n"
            "  Cases:           %zu\n"
            "  Passed:          %zu\n"
            "  IR instructions: %zu\n",
            sizeof(tests) / sizeof(tests[0]),
            passedCount,
            ir.instructions.size());

        return true;
    }


    static inline void testScriptShapingIRThaiConformance()
    {
        runScriptShapingIRThaiConformance();
    }

} // namespace waavs