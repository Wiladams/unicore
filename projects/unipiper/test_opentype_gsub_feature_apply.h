// test_opentype_gsub_feature_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <utility>
#include <vector>

#include "opentype_gsub_feature_apply.h"

namespace waavs
{
    struct GsubApplyTestFeature
    {
        Tag tag{ 0 };
        std::vector<uint16_t> lookupIndices{};
    };


    struct GsubApplyTestLookup
    {
        uint16_t lookupType{ 1 };
        std::vector<std::pair<uint16_t, uint16_t>> substitutions{};
    };


    static void appendGsubApplyTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendGsubApplyTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchGsubApplyTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeGsubApplyTestFeatureList(
        const std::vector<GsubApplyTestFeature>& definitions)
    {
        std::vector<uint8_t> data;

        appendGsubApplyTestU16(data, static_cast<uint16_t>(definitions.size()));

        std::vector<size_t> offsetPatches;
        offsetPatches.reserve(definitions.size());

        for (const auto& definition : definitions)
        {
            appendGsubApplyTestU32(data, definition.tag);

            offsetPatches.push_back(data.size());
            appendGsubApplyTestU16(data, 0);
        }


        for (size_t i = 0; i < definitions.size(); ++i)
        {
            const uint16_t featureOffset = static_cast<uint16_t>(data.size());
            patchGsubApplyTestU16(data, offsetPatches[i], featureOffset);

            appendGsubApplyTestU16(data, 0);
            appendGsubApplyTestU16(
                data, static_cast<uint16_t>(definitions[i].lookupIndices.size()));

            for (uint16_t lookupIndex : definitions[i].lookupIndices)
                appendGsubApplyTestU16(data, lookupIndex);
        }

        return data;
    }


    static void appendGsubApplyTestSingleSubtable(std::vector<uint8_t>& data,
        uint16_t inputGlyph, uint16_t outputGlyph)
    {
        // SingleSubst Format 1.

        appendGsubApplyTestU16(data, 1);
        appendGsubApplyTestU16(data, 6);

        const uint16_t delta =
            static_cast<uint16_t>(uint32_t(outputGlyph) - uint32_t(inputGlyph));

        appendGsubApplyTestU16(data, delta);


        // Coverage Format 1.

        appendGsubApplyTestU16(data, 1);
        appendGsubApplyTestU16(data, 1);
        appendGsubApplyTestU16(data, inputGlyph);
    }


    static std::vector<uint8_t> makeGsubApplyTestLookupList(
        const std::vector<GsubApplyTestLookup>& definitions)
    {
        std::vector<uint8_t> data;

        appendGsubApplyTestU16(data, static_cast<uint16_t>(definitions.size()));

        std::vector<size_t> lookupPatches;
        lookupPatches.reserve(definitions.size());

        for (size_t i = 0; i < definitions.size(); ++i)
        {
            lookupPatches.push_back(data.size());
            appendGsubApplyTestU16(data, 0);
        }


        for (size_t lookupIndex = 0; lookupIndex < definitions.size(); ++lookupIndex)
        {
            const GsubApplyTestLookup& definition = definitions[lookupIndex];

            const size_t lookupOffset = data.size();
            patchGsubApplyTestU16(
                data, lookupPatches[lookupIndex], static_cast<uint16_t>(lookupOffset));

            appendGsubApplyTestU16(data, definition.lookupType);
            appendGsubApplyTestU16(data, 0);

            if (definition.lookupType != 1)
            {
                appendGsubApplyTestU16(data, 0);
                continue;
            }

            appendGsubApplyTestU16(
                data, static_cast<uint16_t>(definition.substitutions.size()));

            std::vector<size_t> subtablePatches;
            subtablePatches.reserve(definition.substitutions.size());

            for (size_t i = 0; i < definition.substitutions.size(); ++i)
            {
                subtablePatches.push_back(data.size());
                appendGsubApplyTestU16(data, 0);
            }


            for (size_t i = 0; i < definition.substitutions.size(); ++i)
            {
                const size_t subtableOffset = data.size() - lookupOffset;

                patchGsubApplyTestU16(
                    data, subtablePatches[i], static_cast<uint16_t>(subtableOffset));

                appendGsubApplyTestSingleSubtable(
                    data,
                    definition.substitutions[i].first,
                    definition.substitutions[i].second);
            }
        }

        return data;
    }


    static OpenTypeShapingBuffer makeGsubApplyTestBuffer(uint32_t glyphId)
    {
        OpenTypeShapingBuffer buffer;

        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = 7;
        glyph.scalarCount = 2;

        buffer.pushBack(glyph);

        return buffer;
    }


    static bool testOpenTypeGsubFeatureApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB feature apply: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Feature order:
        //
        //   liga -> lookups 2, 0
        //   ccmp -> lookups 1, 0
        //
        // LookupList:
        //
        //   0: 10 -> 20
        //   1: 20 -> 30
        //   2: 30 -> 40
        //
        // Correct LookupList execution order produces 40.
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubApplyTestFeature> featureDefs = {
                { OTAG("liga"), { 2, 0 } },
                { OTAG("ccmp"), { 1, 0 } }
            };

            const std::vector<GsubApplyTestLookup> lookupDefs = {
                { 1, { { 10, 20 } } },
                { 1, { { 20, 30 } } },
                { 1, { { 30, 40 } } }
            };

            const std::vector<uint8_t> featureData =
                makeGsubApplyTestFeatureList(featureDefs);

            const std::vector<uint8_t> lookupData =
                makeGsubApplyTestLookupList(lookupDefs);

            const OpenTypeLayoutFeatureListView features(
                ByteSpan(featureData.data(), featureData.size()));

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(lookupData.data(), lookupData.size()));


            OpenTypeFeaturePlan plan;
            plan.addFeature(OTAG("liga"), 0);
            plan.addFeature(OTAG("ccmp"), 1);

            OpenTypeShapingBuffer buffer = makeGsubApplyTestBuffer(10);

            if (!applyOpenTypeGsubFeaturePlan(plan, features, lookups, buffer))
                return fail("case 1 execution failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 40)
                return fail("case 1 wrong lookup execution order");

            if (buffer[0].scalarOffset != 7 || buffer[0].scalarCount != 2)
                return fail("case 1 provenance changed");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // The same lookup is referenced by two selected Features.
        //
        // Lookup 0 contains:
        //
        //   10 -> 20
        //   20 -> 25
        //
        // One execution produces 20.
        // Executing the same lookup twice would produce 25.
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubApplyTestFeature> featureDefs = {
                { OTAG("ccmp"), { 0 } },
                { OTAG("liga"), { 0 } }
            };

            const std::vector<GsubApplyTestLookup> lookupDefs = {
                { 1, { { 10, 20 }, { 20, 25 } } }
            };

            const std::vector<uint8_t> featureData =
                makeGsubApplyTestFeatureList(featureDefs);

            const std::vector<uint8_t> lookupData =
                makeGsubApplyTestLookupList(lookupDefs);

            const OpenTypeLayoutFeatureListView features(
                ByteSpan(featureData.data(), featureData.size()));

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(lookupData.data(), lookupData.size()));


            OpenTypeFeaturePlan plan;
            plan.addFeature(OTAG("ccmp"), 0);
            plan.addFeature(OTAG("liga"), 1);

            OpenTypeShapingBuffer buffer = makeGsubApplyTestBuffer(10);

            if (!applyOpenTypeGsubFeaturePlan(plan, features, lookups, buffer))
                return fail("case 2 execution failed");

            if (buffer[0].glyphId != 20)
                return fail("case 2 duplicate lookup executed more than once");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Transactional failure.
        //
        // Lookup 0 is supported and would change 10 -> 20.
        // Lookup 1 uses LookupType 9, which is not defined by GSUB.
        //
        // The complete operation must fail with the caller's buffer still
        // containing glyph 10.
        // ====================================================================

        {
            ++cases;

            const std::vector<GsubApplyTestFeature> featureDefs = {
                { OTAG("ccmp"), { 0, 1 } }
            };

            const std::vector<GsubApplyTestLookup> lookupDefs = {
                { 1, { { 10, 20 } } },
                { 9, {} }
            };

            const std::vector<uint8_t> featureData =
                makeGsubApplyTestFeatureList(featureDefs);

            const std::vector<uint8_t> lookupData =
                makeGsubApplyTestLookupList(lookupDefs);

            const OpenTypeLayoutFeatureListView features(
                ByteSpan(featureData.data(), featureData.size()));

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(lookupData.data(), lookupData.size()));


            OpenTypeFeaturePlan plan;
            plan.addFeature(OTAG("ccmp"), 0);

            OpenTypeShapingBuffer buffer = makeGsubApplyTestBuffer(10);

            if (applyOpenTypeGsubFeaturePlan(plan, features, lookups, buffer))
                return fail("case 3 unsupported lookup accepted");

            if (buffer.size() != 1 || buffer[0].glyphId != 10)
                return fail("case 3 failure partially mutated buffer");

            if (buffer[0].scalarOffset != 7 || buffer[0].scalarCount != 2)
                return fail("case 3 provenance changed");

            ++passed;
        }


        std::printf(
            "OpenType GSUB feature apply: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  LookupList order:      PASS\n"
            "  Lookup feed-through:   PASS\n"
            "  Lookup deduplication:  PASS\n"
            "  Transactional failure: PASS\n"
            "  Provenance:            PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs