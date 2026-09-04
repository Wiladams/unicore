// test_opentype_layout_feature_selection.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_layout_feature_selection.h"

namespace waavs
{
    struct OpenTypeLayoutFeatureSelectionTestData
    {
        std::vector<uint8_t> bytes{};
        size_t latnRequiredFeatureOffset{ 0 };
        size_t latnFirstFeatureIndexOffset{ 0 };
        size_t ccmpFeatureOffsetField{ 0 };
        size_t lookupListOffset{ 0 };
    };


    static void appendLayoutSelectionTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendLayoutSelectionTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchLayoutSelectionTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static OpenTypeLayoutFeatureSelectionTestData makeOpenTypeLayoutFeatureSelectionTestData()
    {
        OpenTypeLayoutFeatureSelectionTestData result{};
        std::vector<uint8_t>& data = result.bytes;


        // ================================================================
        // GSUB/GPOS common header
        // ================================================================

        appendLayoutSelectionTestU16(data, 1);
        appendLayoutSelectionTestU16(data, 0);

        const size_t scriptListPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);

        const size_t featureListPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);

        const size_t lookupListPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);


        // ================================================================
        // ScriptList
        //
        // DFLT -> ccmp
        //
        // latn:
        //   required = rlig
        //
        //   LangSys order deliberately scrambled:
        //
        //     smcp
        //     liga
        //     rlig
        //     calt
        //     ccmp
        //     dlig
        //     clig
        //     locl
        //
        // Policy should reorder the selected optional features.
        // ================================================================

        const size_t scriptListOffset = data.size();
        appendLayoutSelectionTestU16(data, 2);

        appendLayoutSelectionTestU32(data, OTAG("DFLT"));
        const size_t dfltScriptPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);

        appendLayoutSelectionTestU32(data, OTAG("latn"));
        const size_t latnScriptPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);


        // ----------------------------------------------------------------
        // DFLT Script
        // ----------------------------------------------------------------

        const size_t dfltScriptOffset = data.size();
        patchLayoutSelectionTestU16(data, dfltScriptPatch, static_cast<uint16_t>(dfltScriptOffset - scriptListOffset));

        const size_t dfltDefaultPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);
        appendLayoutSelectionTestU16(data, 0);

        const size_t dfltDefaultOffset = data.size();
        patchLayoutSelectionTestU16(data, dfltDefaultPatch, static_cast<uint16_t>(dfltDefaultOffset - dfltScriptOffset));

        appendLayoutSelectionTestU16(data, 0);
        appendLayoutSelectionTestU16(data, kOpenTypeFeatureIndexInvalid);
        appendLayoutSelectionTestU16(data, 1);
        appendLayoutSelectionTestU16(data, 4); // ccmp


        // ----------------------------------------------------------------
        // latn Script
        // ----------------------------------------------------------------

        const size_t latnScriptOffset = data.size();
        patchLayoutSelectionTestU16(data, latnScriptPatch, static_cast<uint16_t>(latnScriptOffset - scriptListOffset));

        const size_t latnDefaultPatch = data.size();
        appendLayoutSelectionTestU16(data, 0);
        appendLayoutSelectionTestU16(data, 0);

        const size_t latnDefaultOffset = data.size();
        patchLayoutSelectionTestU16(data, latnDefaultPatch, static_cast<uint16_t>(latnDefaultOffset - latnScriptOffset));

        appendLayoutSelectionTestU16(data, 0);

        result.latnRequiredFeatureOffset = data.size();
        appendLayoutSelectionTestU16(data, 5); // rlig

        appendLayoutSelectionTestU16(data, 8);

        result.latnFirstFeatureIndexOffset = data.size();

        appendLayoutSelectionTestU16(data, 3); // smcp
        appendLayoutSelectionTestU16(data, 1); // liga
        appendLayoutSelectionTestU16(data, 5); // rlig
        appendLayoutSelectionTestU16(data, 2); // calt
        appendLayoutSelectionTestU16(data, 4); // ccmp
        appendLayoutSelectionTestU16(data, 0); // dlig
        appendLayoutSelectionTestU16(data, 6); // clig
        appendLayoutSelectionTestU16(data, 7); // locl


        // ================================================================
        // FeatureList
        //
        // 0 dlig
        // 1 liga
        // 2 calt
        // 3 smcp
        // 4 ccmp
        // 5 rlig
        // 6 clig
        // 7 locl
        //
        // Every FeatureRecord points at the same harmless empty Feature
        // table. Selection should never dereference that table anyway.
        // ================================================================

        const size_t featureListOffset = data.size();
        appendLayoutSelectionTestU16(data, 8);

        const Tag featureTags[] = {
            OTAG("dlig"),
            OTAG("liga"),
            OTAG("calt"),
            OTAG("smcp"),
            OTAG("ccmp"),
            OTAG("rlig"),
            OTAG("clig"),
            OTAG("locl")
        };

        size_t featureOffsetFields[8]{};

        for (size_t i = 0; i < 8; ++i)
        {
            appendLayoutSelectionTestU32(data, featureTags[i]);
            featureOffsetFields[i] = data.size();
            appendLayoutSelectionTestU16(data, 0);
        }

        result.ccmpFeatureOffsetField = featureOffsetFields[4];


        // Shared empty Feature table.

        const size_t emptyFeatureOffset = data.size();
        const uint16_t relativeFeatureOffset = static_cast<uint16_t>(emptyFeatureOffset - featureListOffset);

        for (size_t i = 0; i < 8; ++i)
            patchLayoutSelectionTestU16(data, featureOffsetFields[i], relativeFeatureOffset);

        appendLayoutSelectionTestU16(data, 0); // featureParamsOffset
        appendLayoutSelectionTestU16(data, 0); // lookupIndexCount


        // ================================================================
        // LookupList
        //
        // Nothing is needed yet.
        // ================================================================

        result.lookupListOffset = data.size();
        appendLayoutSelectionTestU16(data, 0);


        // ================================================================
        // Patch common header
        // ================================================================

        patchLayoutSelectionTestU16(data, scriptListPatch, static_cast<uint16_t>(scriptListOffset));
        patchLayoutSelectionTestU16(data, featureListPatch, static_cast<uint16_t>(featureListOffset));
        patchLayoutSelectionTestU16(data, lookupListPatch, static_cast<uint16_t>(result.lookupListOffset));

        return result;
    }


    static bool testOpenTypeLayoutFeatureSelection()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType layout feature selection: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Complete latn path.
        //
        // Required:
        //   rlig
        //
        // Optional policy order:
        //   ccmp
        //   locl
        //   liga
        //   clig
        //   calt
        //
        // smcp and dlig must not be selected.
        // ====================================================================

        {
            ++cases;

            const OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();
            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(layout, OTAG("latn"), plan))
                return fail("case 1 selection failed");

            if (!plan.hasRequiredFeature())
                return fail("case 1 required feature missing");

            if (plan.requiredFeature().tag != OTAG("rlig") || plan.requiredFeature().featureIndex != 5)
                return fail("case 1 wrong required feature");

            if (plan.size() != 5)
                return fail("case 1 wrong optional feature count");

            const Tag expectedTags[] = {
                OTAG("ccmp"),
                OTAG("locl"),
                OTAG("liga"),
                OTAG("clig"),
                OTAG("calt")
            };

            const OpenTypeFeatureIndex expectedIndices[] = {
                4, 7, 1, 6, 2
            };

            for (size_t i = 0; i < 5; ++i)
            {
                if (plan[i].tag != expectedTags[i] || plan[i].featureIndex != expectedIndices[i])
                    return fail("case 1 wrong policy order");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Missing requested script falls back to DFLT.
        // ====================================================================

        {
            ++cases;

            const OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();
            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(layout, OTAG("arab"), plan))
                return fail("case 2 DFLT fallback failed");

            if (plan.hasRequiredFeature())
                return fail("case 2 unexpected required feature");

            if (plan.size() != 1 || plan[0].tag != OTAG("ccmp") || plan[0].featureIndex != 4)
                return fail("case 2 wrong DFLT selection");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Direct adapter with custom policy.
        //
        // Required rlig remains present regardless of requested tags.
        // ====================================================================

        {
            ++cases;

            const OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();
            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));

            const OpenTypeLayoutLangSysView langSys = layout.script(OTAG("latn")).defaultLangSys();
            const OpenTypeLayoutFeatureListView features = layout.features();

            const Tag desired[] = {
                OTAG("liga"),
                OTAG("dlig")
            };

            OpenTypeFeaturePlan plan;

            if (!selectOpenTypeLayoutFeatures(langSys, features, desired, 2, plan))
                return fail("case 3 direct adapter failed");

            if (!plan.hasRequiredFeature() ||
                plan.requiredFeature().tag != OTAG("rlig") ||
                plan.requiredFeature().featureIndex != 5)
            {
                return fail("case 3 required feature");
            }

            if (plan.size() != 2)
                return fail("case 3 optional count");

            if (plan[0].tag != OTAG("liga") || plan[0].featureIndex != 1)
                return fail("case 3 liga");

            if (plan[1].tag != OTAG("dlig") || plan[1].featureIndex != 0)
                return fail("case 3 dlig");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Feature tables remain lazy.
        //
        // Corrupt ccmp's Feature-table offset. Selection uses only the
        // FeatureRecord tag, so ccmp must still be selected.
        // ====================================================================

        {
            ++cases;

            OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();

            patchLayoutSelectionTestU16(data.bytes, data.ccmpFeatureOffsetField, 0xFFFF);

            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));
            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(layout, OTAG("latn"), plan))
                return fail("case 4 lazy Feature selection failed");

            if (plan.size() != 5 || plan[0].tag != OTAG("ccmp") || plan[0].featureIndex != 4)
                return fail("case 4 Feature table was unnecessarily dereferenced");

            if (layout.features().feature(4))
                return fail("case 4 malformed Feature unexpectedly valid");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // LookupList remains completely untouched during feature selection.
        // ====================================================================

        {
            ++cases;

            OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();

            patchLayoutSelectionTestU16(data.bytes, data.lookupListOffset, 0xFFFF);

            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));
            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(layout, OTAG("latn"), plan))
                return fail("case 5 corrupt LookupList affected selection");

            if (!plan.hasRequiredFeature() || plan.size() != 5)
                return fail("case 5 wrong plan");

            if (layout.lookups())
                return fail("case 5 malformed LookupList unexpectedly valid");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Invalid ordinary FeatureList reference is rejected before a plan
        // is produced.
        // ====================================================================

        {
            ++cases;

            OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();

            patchLayoutSelectionTestU16(data.bytes, data.latnFirstFeatureIndexOffset, 99);

            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));

            OpenTypeFeaturePlan plan;
            plan.addFeature(OTAG("test"), 0);

            if (selectGenericOpenTypeGsubFeatures(layout, OTAG("latn"), plan))
                return fail("case 6 invalid feature index accepted");

            if (!plan.empty())
                return fail("case 6 failure left partial plan");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Invalid required FeatureList reference is also rejected before a
        // plan is produced.
        // ====================================================================

        {
            ++cases;

            OpenTypeLayoutFeatureSelectionTestData data = makeOpenTypeLayoutFeatureSelectionTestData();

            patchLayoutSelectionTestU16(data.bytes, data.latnRequiredFeatureOffset, 99);

            const OpenTypeLayoutView layout(ByteSpan(data.bytes.data(), data.bytes.size()));

            OpenTypeFeaturePlan plan;
            plan.addFeature(OTAG("test"), 0);

            if (selectGenericOpenTypeGsubFeatures(layout, OTAG("latn"), plan))
                return fail("case 7 invalid required feature accepted");

            if (!plan.empty())
                return fail("case 7 failure left partial plan");

            ++passed;
        }


        std::printf(
            "OpenType layout feature selection: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  Layout integration:    PASS\n"
            "  Policy ordering:       PASS\n"
            "  DFLT fallback:         PASS\n"
            "  Required feature:      PASS\n"
            "  Optional filtering:    PASS\n"
            "  Lazy Feature access:   PASS\n"
            "  Lazy Lookup access:    PASS\n"
            "  Cross-table failures:  PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs