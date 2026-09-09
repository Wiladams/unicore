// test_opentype_gsub_orchestrator.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_orchestrator.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGsubOrchestratorU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubOrchestratorU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Coverage Format 1.
    // ====================================================================

    static void appendGsubOrchestratorCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGsubOrchestratorU16(data, 1);
        appendGsubOrchestratorU16(data, 1);
        appendGsubOrchestratorU16(data, glyphId);
    }


    // ====================================================================
    // MultipleSubst Format 1.
    //
    // sourceGlyph -> replacements[]
    // ====================================================================

    static std::vector<uint8_t> makeGsubOrchestratorMultiple(
        uint16_t sourceGlyph, const uint16_t* replacements, uint16_t replacementCount)
    {
        std::vector<uint8_t> data;

        appendGsubOrchestratorU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubOrchestratorU16(data, 0);

        appendGsubOrchestratorU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubOrchestratorU16(data, 0);


        patchGsubOrchestratorU16(data, sequencePatch, static_cast<uint16_t>(data.size()));

        appendGsubOrchestratorU16(data, replacementCount);

        for (uint16_t i = 0; i < replacementCount; ++i)
            appendGsubOrchestratorU16(data, replacements[i]);


        patchGsubOrchestratorU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGsubOrchestratorCoverage(data, sourceGlyph);

        return data;
    }


    // ====================================================================
    // SingleSubst Format 2.
    // ====================================================================

    static std::vector<uint8_t> makeGsubOrchestratorSingle(uint16_t sourceGlyph, uint16_t replacementGlyph)
    {
        std::vector<uint8_t> data;

        appendGsubOrchestratorU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubOrchestratorU16(data, 0);

        appendGsubOrchestratorU16(data, 1);
        appendGsubOrchestratorU16(data, replacementGlyph);

        patchGsubOrchestratorU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGsubOrchestratorCoverage(data, sourceGlyph);

        return data;
    }


    // ====================================================================
    // Malformed SingleSubst.
    //
    // Format 2 claims one substitute glyph but the table ends before that
    // glyph or its Coverage can be read.
    // ====================================================================

    static std::vector<uint8_t> makeGsubOrchestratorMalformedSingle()
    {
        std::vector<uint8_t> data;

        appendGsubOrchestratorU16(data, 2);
        appendGsubOrchestratorU16(data, 6);
        appendGsubOrchestratorU16(data, 1);

        return data;
    }


    // ====================================================================
    // LookupList builder.
    // ====================================================================

    struct GsubOrchestratorLookupSpec
    {
        uint16_t lookupType{ 0 };
        uint16_t lookupFlag{ 0 };
        std::vector<uint8_t> subtable{};
    };


    static std::vector<uint8_t> makeGsubOrchestratorLookupList(
        const std::vector<GsubOrchestratorLookupSpec>& specs)
    {
        std::vector<uint8_t> data;

        appendGsubOrchestratorU16(data, static_cast<uint16_t>(specs.size()));

        std::vector<size_t> lookupPatches;
        lookupPatches.reserve(specs.size());

        for (size_t i = 0; i < specs.size(); ++i)
        {
            lookupPatches.push_back(data.size());
            appendGsubOrchestratorU16(data, 0);
        }

        for (size_t i = 0; i < specs.size(); ++i)
        {
            patchGsubOrchestratorU16(data, lookupPatches[i], static_cast<uint16_t>(data.size()));

            const size_t lookupBegin = data.size();

            appendGsubOrchestratorU16(data, specs[i].lookupType);
            appendGsubOrchestratorU16(data, specs[i].lookupFlag);
            appendGsubOrchestratorU16(data, 1);

            const size_t subtablePatch = data.size();
            appendGsubOrchestratorU16(data, 0);

            patchGsubOrchestratorU16(
                data, subtablePatch,
                static_cast<uint16_t>(data.size() - lookupBegin));

            data.insert(data.end(), specs[i].subtable.begin(), specs[i].subtable.end());
        }

        return data;
    }


    // ====================================================================
    // Standard synthetic LookupList.
    //
    // Lookup 0:
    //
    //   MultipleSubst
    //   20 -> 200 201 202
    //
    // Lookup 1:
    //
    //   SingleSubst
    //   202 -> 250
    //
    // Lookup 2:
    //
    //   malformed SingleSubst
    // ====================================================================

    static std::vector<uint8_t> makeGsubOrchestratorStandardLookupList()
    {
        const uint16_t replacements[] =
        {
            200, 201, 202
        };

        return makeGsubOrchestratorLookupList(
            {
                {
                    2, 0,
                    makeGsubOrchestratorMultiple(
                        20, replacements, 3)
                },
                {
                    1, 0,
                    makeGsubOrchestratorSingle(
                        202, 250)
                },
                {
                    1, 0,
                    makeGsubOrchestratorMalformedSingle()
                }
            });
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static void appendGsubOrchestratorGlyph(
        OpenTypeShapingBuffer& buffer, uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};

        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        buffer.pushBack(glyph);
    }


    static OpenTypeShapingBuffer makeGsubOrchestratorBuffer()
    {
        OpenTypeShapingBuffer buffer;

        appendGsubOrchestratorGlyph(buffer, 10, 0);
        appendGsubOrchestratorGlyph(buffer, 20, 1);
        appendGsubOrchestratorGlyph(buffer, 30, 2);

        return buffer;
    }


    static bool gsubOrchestratorGlyphsEqual(
        const OpenTypeShapingBuffer& buffer,
        const uint32_t* expected, size_t count) noexcept
    {
        if (buffer.size() != count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (buffer[i].glyphId != expected[i])
                return false;
        }

        return true;
    }


    static OpenTypeLayoutLookupPlan makeGsubOrchestratorPlan(
        const std::vector<uint8_t>& lookupList,
        std::initializer_list<uint16_t> indices)
    {
        OpenTypeLayoutLookupPlan plan;

        plan.lookupListData =
            ByteSpan(lookupList.data(), lookupList.size());

        plan.lookupIndices.assign(
            indices.begin(), indices.end());

        return plan;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGsubOrchestrator()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB orchestrator: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> lookupListData =
            makeGsubOrchestratorStandardLookupList();

        const OpenTypeLayoutLookupListView lookups(
            ByteSpan(
                lookupListData.data(),
                lookupListData.size()));

        if (!lookups || lookups.size() != 3)
            return fail("synthetic LookupList invalid");


        // ================================================================
        // Case 1 - Ordered top-level lookup interaction.
        //
        // Initial:
        //
        //   10 20 30
        //
        // Lookup 0:
        //
        //   20 -> 200 201 202
        //
        // Intermediate:
        //
        //   10 200 201 202 30
        //
        // Lookup 1:
        //
        //   202 -> 250
        //
        // Final:
        //
        //   10 200 201 250 30
        //
        // Lookup 1 can only match if it sees Lookup 0's output.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGsubOrchestratorPlan(
                    lookupListData,
                    { 0, 1 });

            if (!applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 1 apply");
            }

            const uint32_t expected[] =
            {
                10, 200, 201, 250, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 5))
            {
                return fail("case 1 ordered interaction");
            }


            // MultipleSubst output inherits the source glyph's provenance.
            // The later SingleSubst must preserve it.

            if (buffer[1].scalarOffset != 1 ||
                buffer[2].scalarOffset != 1 ||
                buffer[3].scalarOffset != 1 ||
                buffer[1].scalarCount != 1 ||
                buffer[2].scalarCount != 1 ||
                buffer[3].scalarCount != 1)
            {
                return fail("case 1 provenance");
            }

            if (buffer[0].scalarOffset != 0 ||
                buffer[4].scalarOffset != 2)
            {
                return fail("case 1 unaffected provenance");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Whole-plan rollback.
        //
        // Plan:
        //
        //   Lookup 0 succeeds:
        //
        //      20 -> 200 201 202
        //
        //   Lookup 2 is malformed.
        //
        // The caller must still see the ORIGINAL:
        //
        //      10 20 30
        //
        // not:
        //
        //      10 200 201 202 30
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGsubOrchestratorPlan(
                    lookupListData,
                    { 0, 2 });

            if (applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 2 malformed lookup accepted");
            }

            const uint32_t expected[] =
            {
                10, 20, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 3))
            {
                return fail("case 2 whole-plan rollback");
            }

            if (buffer[0].scalarOffset != 0 ||
                buffer[1].scalarOffset != 1 ||
                buffer[2].scalarOffset != 2)
            {
                return fail("case 2 provenance rollback");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Empty plan is a successful no-op.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGsubOrchestratorPlan(
                    lookupListData,
                    {});

            if (!applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 3 empty plan rejected");
            }

            const uint32_t expected[] =
            {
                10, 20, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 3))
            {
                return fail("case 3 empty plan mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Lookup plan must be strictly increasing.
        //
        // Reverse order would also produce different shaping semantics, so
        // the orchestrator rejects plans that did not come from the layout
        // selector's LookupList-order normalization.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGsubOrchestratorPlan(
                    lookupListData,
                    { 1, 0 });

            if (applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 4 reversed plan accepted");
            }

            const uint32_t expected[] =
            {
                10, 20, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 3))
            {
                return fail("case 4 reversed-plan mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Duplicate lookup index rejected.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGsubOrchestratorPlan(
                    lookupListData,
                    { 0, 0 });

            if (applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 5 duplicate lookup accepted");
            }

            const uint32_t expected[] =
            {
                10, 20, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 3))
            {
                return fail("case 5 duplicate-plan mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Out-of-range LookupList index rejected.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGsubOrchestratorPlan(
                    lookupListData,
                    { 0, 3 });

            if (applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 6 out-of-range lookup accepted");
            }

            const uint32_t expected[] =
            {
                10, 20, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 3))
            {
                return fail("case 6 out-of-range mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Missing LookupList data rejected.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer =
                makeGsubOrchestratorBuffer();

            OpenTypeLayoutLookupPlan plan;
            plan.lookupIndices = { 0 };

            if (applyOpenTypeGsubLookupPlan(
                plan, buffer))
            {
                return fail("case 7 missing LookupList accepted");
            }

            const uint32_t expected[] =
            {
                10, 20, 30
            };

            if (!gsubOrchestratorGlyphsEqual(
                buffer, expected, 3))
            {
                return fail("case 7 missing-data mutation");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB orchestrator: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Ordered lookup execution:   PASS\n"
            "  Topology propagation:       PASS\n"
            "  Provenance propagation:     PASS\n"
            "  Whole-plan rollback:        PASS\n"
            "  Empty plan:                 PASS\n"
            "  Lookup order validation:    PASS\n"
            "  Duplicate rejection:        PASS\n"
            "  Index validation:           PASS\n"
            "  Missing LookupList:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs