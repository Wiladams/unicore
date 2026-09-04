// test_opentype_feature_selection.h

#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "opentype_feature_selection.h"


namespace waavs
{
    static bool testOpenTypeFeatureSelection()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType feature selection: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Generic GSUB features are selected from the features actually
        // referenced by the LangSys.
        //
        // Selection order follows shaping policy rather than FeatureList or
        // LangSys ordering.
        //
        // Available through LangSys:
        //
        //      calt
        //      liga
        //      smcp
        //      rlig
        //
        // ccmp exists in FeatureList but is not referenced by LangSys.
        //
        // Expected:
        //
        //      rlig
        //      liga
        //      calt
        // ====================================================================

        {
            ++cases;

            const OpenTypeFeatureDirectoryEntry directory[] = {
                { OTAG("calt") },
                { OTAG("liga") },
                { OTAG("smcp") },
                { OTAG("ccmp") },
                { OTAG("rlig") }
            };

            const OpenTypeFeatureIndex featureIndices[] = { 0, 1, 2, 4 };

            OpenTypeLangSysView langSys{};
            langSys.featureIndices = featureIndices;
            langSys.featureCount = 4;

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(langSys, directory, 5, plan))
                return fail("case 1 selection failed");

            if (plan.hasRequiredFeature())
                return fail("case 1 unexpected required feature");

            if (plan.size() != 3)
                return fail("case 1 wrong selected feature count");

            if (plan[0].tag != OTAG("rlig") || plan[0].featureIndex != 4)
                return fail("case 1 rlig selection");

            if (plan[1].tag != OTAG("liga") || plan[1].featureIndex != 1)
                return fail("case 1 liga selection");

            if (plan[2].tag != OTAG("calt") || plan[2].featureIndex != 0)
                return fail("case 1 calt selection");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // All generic policy features are selected in policy order even when
        // FeatureList and LangSys order are different.
        // ====================================================================

        {
            ++cases;

            const OpenTypeFeatureDirectoryEntry directory[] = {
                { OTAG("liga") },
                { OTAG("calt") },
                { OTAG("locl") },
                { OTAG("clig") },
                { OTAG("rlig") },
                { OTAG("ccmp") }
            };

            const OpenTypeFeatureIndex featureIndices[] = { 1, 0, 5, 3, 2, 4 };

            OpenTypeLangSysView langSys{};
            langSys.featureIndices = featureIndices;
            langSys.featureCount = 6;

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(langSys, directory, 6, plan))
                return fail("case 2 selection failed");

            if (plan.size() != 6)
                return fail("case 2 wrong selected feature count");

            const Tag expectedTags[] = {
                OTAG("ccmp"),
                OTAG("locl"),
                OTAG("rlig"),
                OTAG("liga"),
                OTAG("clig"),
                OTAG("calt")
            };

            const OpenTypeFeatureIndex expectedIndices[] = { 5, 2, 4, 0, 3, 1 };

            for (size_t i = 0; i < 6; ++i)
            {
                if (plan[i].tag != expectedTags[i] || plan[i].featureIndex != expectedIndices[i])
                    return fail("case 2 policy ordering");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Features that exist in FeatureList but are not referenced by the
        // selected LangSys are unavailable and must not be selected.
        // ====================================================================

        {
            ++cases;

            const OpenTypeFeatureDirectoryEntry directory[] = {
                { OTAG("ccmp") },
                { OTAG("liga") },
                { OTAG("calt") }
            };

            const OpenTypeFeatureIndex featureIndices[] = { 1 };

            OpenTypeLangSysView langSys{};
            langSys.featureIndices = featureIndices;
            langSys.featureCount = 1;

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(langSys, directory, 3, plan))
                return fail("case 3 selection failed");

            if (plan.size() != 1)
                return fail("case 3 unavailable features selected");

            if (plan[0].tag != OTAG("liga") || plan[0].featureIndex != 1)
                return fail("case 3 wrong available feature");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // A feature referenced by LangSys but not requested by shaping policy
        // remains disabled.
        // ====================================================================

        {
            ++cases;

            const OpenTypeFeatureDirectoryEntry directory[] = {
                { OTAG("smcp") },
                { OTAG("dlig") },
                { OTAG("salt") }
            };

            const OpenTypeFeatureIndex featureIndices[] = { 0, 1, 2 };

            OpenTypeLangSysView langSys{};
            langSys.featureIndices = featureIndices;
            langSys.featureCount = 3;

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(langSys, directory, 3, plan))
                return fail("case 4 selection failed");

            if (!plan.empty())
                return fail("case 4 optional features were enabled");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Required feature is retained regardless of the generic policy.
        //
        // It is stored separately from ordinary policy-selected features.
        // ====================================================================

        {
            ++cases;

            const OpenTypeFeatureDirectoryEntry directory[] = {
                { OTAG("liga") },
                { OTAG("test") },
                { OTAG("calt") }
            };

            const OpenTypeFeatureIndex featureIndices[] = { 0, 2 };

            OpenTypeLangSysView langSys{};
            langSys.featureIndices = featureIndices;
            langSys.featureCount = 2;
            langSys.requiredFeatureIndex = 1;

            OpenTypeFeaturePlan plan;

            if (!selectGenericOpenTypeGsubFeatures(langSys, directory, 3, plan))
                return fail("case 5 selection failed");

            if (!plan.hasRequiredFeature())
                return fail("case 5 required feature missing");

            if (plan.requiredFeature().tag != OTAG("test") || plan.requiredFeature().featureIndex != 1)
                return fail("case 5 wrong required feature");

            if (plan.size() != 2)
                return fail("case 5 wrong ordinary feature count");

            if (plan[0].tag != OTAG("liga") || plan[0].featureIndex != 0)
                return fail("case 5 liga selection");

            if (plan[1].tag != OTAG("calt") || plan[1].featureIndex != 2)
                return fail("case 5 calt selection");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Malformed feature indices are rejected.
        // ====================================================================

        {
            ++cases;

            const OpenTypeFeatureDirectoryEntry directory[] = {
                { OTAG("liga") },
                { OTAG("calt") }
            };

            const OpenTypeFeatureIndex badLangSysIndices[] = { 0, 3 };

            OpenTypeLangSysView badLangSys{};
            badLangSys.featureIndices = badLangSysIndices;
            badLangSys.featureCount = 2;

            OpenTypeFeaturePlan plan;

            if (selectGenericOpenTypeGsubFeatures(badLangSys, directory, 2, plan))
                return fail("case 6 out-of-range LangSys feature accepted");

            if (!plan.empty())
                return fail("case 6 malformed LangSys left partial output");


            OpenTypeLangSysView badRequired{};
            badRequired.requiredFeatureIndex = 7;

            if (selectGenericOpenTypeGsubFeatures(badRequired, directory, 2, plan))
                return fail("case 6 out-of-range required feature accepted");

            if (!plan.empty())
                return fail("case 6 malformed required feature left partial output");

            ++passed;
        }


        std::printf(
            "OpenType feature selection: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  Generic features:      PASS\n"
            "  Policy ordering:       PASS\n"
            "  LangSys filtering:     PASS\n"
            "  Optional features:     PASS\n"
            "  Required feature:      PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs