// test_opentype_layout_selection.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "opentype_layout_selection.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendLayoutSelectionU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendLayoutSelectionU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchLayoutSelectionU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Synthetic layout table.
    //
    // Scripts:
    //
    //   latn
    //       DefaultLangSys
    //           required: rlig
    //           optional: liga kern calt
    //
    //       TRK
    //           required: rlig
    //           optional: locl liga
    //
    //   DFLT
    //       DefaultLangSys
    //           required: none
    //           optional: calt
    //
    // Features:
    //
    //   0 rlig -> lookup 3
    //   1 liga -> lookups 2,4
    //   2 kern -> lookups 1,4
    //   3 calt -> lookup 0
    //   4 locl -> lookup 5
    //
    // LookupList contains six synthetic lookup slots. Their contents are not
    // interpreted by layout selection; only the LookupList count is needed.
    //
    // Patch offsets are retained for structural failure tests.
    // ====================================================================

    struct LayoutSelectionTestTable
    {
        std::vector<uint8_t> data{};

        size_t latnDefaultRequiredPatch{ 0 };
        size_t latnDefaultFirstOptionalPatch{ 0 };
        size_t ligaFirstLookupPatch{ 0 };
    };


    static LayoutSelectionTestTable makeLayoutSelectionTestTable()
    {
        LayoutSelectionTestTable result;
        std::vector<uint8_t>& data = result.data;


        // ------------------------------------------------------------
        // Layout header 1.0.
        // ------------------------------------------------------------

        appendLayoutSelectionU16(data, 1);
        appendLayoutSelectionU16(data, 0);

        const size_t scriptListPatch = data.size();
        appendLayoutSelectionU16(data, 0);

        const size_t featureListPatch = data.size();
        appendLayoutSelectionU16(data, 0);

        const size_t lookupListPatch = data.size();
        appendLayoutSelectionU16(data, 0);


        // ------------------------------------------------------------
        // ScriptList.
        // ------------------------------------------------------------

        const size_t scriptListBegin = data.size();

        patchLayoutSelectionU16(
            data, scriptListPatch,
            static_cast<uint16_t>(scriptListBegin));

        appendLayoutSelectionU16(data, 2);

        appendLayoutSelectionU32(data, OTAG("latn"));

        const size_t latnScriptPatch = data.size();
        appendLayoutSelectionU16(data, 0);

        appendLayoutSelectionU32(data, OTAG("DFLT"));

        const size_t defaultScriptPatch = data.size();
        appendLayoutSelectionU16(data, 0);


        // ------------------------------------------------------------
        // latn Script.
        // ------------------------------------------------------------

        const size_t latnScriptBegin = data.size();

        patchLayoutSelectionU16(
            data, latnScriptPatch,
            static_cast<uint16_t>(
                latnScriptBegin - scriptListBegin));

        const size_t latnDefaultLangSysPatch = data.size();
        appendLayoutSelectionU16(data, 0);

        appendLayoutSelectionU16(data, 1);

        appendLayoutSelectionU32(data, OTAG("TRK "));

        const size_t trkLangSysPatch = data.size();
        appendLayoutSelectionU16(data, 0);


        // latn DefaultLangSys.

        const size_t latnDefaultLangSysBegin = data.size();

        patchLayoutSelectionU16(
            data, latnDefaultLangSysPatch,
            static_cast<uint16_t>(
                latnDefaultLangSysBegin - latnScriptBegin));

        appendLayoutSelectionU16(data, 0);

        result.latnDefaultRequiredPatch = data.size();
        appendLayoutSelectionU16(data, 0);

        appendLayoutSelectionU16(data, 3);

        result.latnDefaultFirstOptionalPatch = data.size();
        appendLayoutSelectionU16(data, 1);
        appendLayoutSelectionU16(data, 2);
        appendLayoutSelectionU16(data, 3);


        // latn TRK LangSys.

        const size_t trkLangSysBegin = data.size();

        patchLayoutSelectionU16(
            data, trkLangSysPatch,
            static_cast<uint16_t>(
                trkLangSysBegin - latnScriptBegin));

        appendLayoutSelectionU16(data, 0);
        appendLayoutSelectionU16(data, 0);
        appendLayoutSelectionU16(data, 2);
        appendLayoutSelectionU16(data, 4);
        appendLayoutSelectionU16(data, 1);


        // ------------------------------------------------------------
        // DFLT Script.
        // ------------------------------------------------------------

        const size_t defaultScriptBegin = data.size();

        patchLayoutSelectionU16(
            data, defaultScriptPatch,
            static_cast<uint16_t>(
                defaultScriptBegin - scriptListBegin));

        const size_t defaultLangSysPatch = data.size();
        appendLayoutSelectionU16(data, 0);

        appendLayoutSelectionU16(data, 0);


        // DFLT DefaultLangSys.

        const size_t defaultLangSysBegin = data.size();

        patchLayoutSelectionU16(
            data, defaultLangSysPatch,
            static_cast<uint16_t>(
                defaultLangSysBegin - defaultScriptBegin));

        appendLayoutSelectionU16(data, 0);
        appendLayoutSelectionU16(data, 0xFFFFu);
        appendLayoutSelectionU16(data, 1);
        appendLayoutSelectionU16(data, 3);


        // ------------------------------------------------------------
        // FeatureList.
        // ------------------------------------------------------------

        const size_t featureListBegin = data.size();

        patchLayoutSelectionU16(
            data, featureListPatch,
            static_cast<uint16_t>(featureListBegin));

        appendLayoutSelectionU16(data, 5);

        const uint32_t featureTags[] =
        {
            OTAG("rlig"),
            OTAG("liga"),
            OTAG("kern"),
            OTAG("calt"),
            OTAG("locl")
        };

        size_t featureOffsetPatches[5]{};

        for (size_t i = 0; i < 5; ++i)
        {
            appendLayoutSelectionU32(data, featureTags[i]);

            featureOffsetPatches[i] = data.size();
            appendLayoutSelectionU16(data, 0);
        }


        // Feature 0: rlig -> lookup 3.

        {
            const size_t begin = data.size();

            patchLayoutSelectionU16(
                data, featureOffsetPatches[0],
                static_cast<uint16_t>(
                    begin - featureListBegin));

            appendLayoutSelectionU16(data, 0);
            appendLayoutSelectionU16(data, 1);
            appendLayoutSelectionU16(data, 3);
        }


        // Feature 1: liga -> lookups 2,4.

        {
            const size_t begin = data.size();

            patchLayoutSelectionU16(
                data, featureOffsetPatches[1],
                static_cast<uint16_t>(
                    begin - featureListBegin));

            appendLayoutSelectionU16(data, 0);
            appendLayoutSelectionU16(data, 2);

            result.ligaFirstLookupPatch = data.size();

            appendLayoutSelectionU16(data, 2);
            appendLayoutSelectionU16(data, 4);
        }


        // Feature 2: kern -> lookups 1,4.

        {
            const size_t begin = data.size();

            patchLayoutSelectionU16(
                data, featureOffsetPatches[2],
                static_cast<uint16_t>(
                    begin - featureListBegin));

            appendLayoutSelectionU16(data, 0);
            appendLayoutSelectionU16(data, 2);
            appendLayoutSelectionU16(data, 1);
            appendLayoutSelectionU16(data, 4);
        }


        // Feature 3: calt -> lookup 0.

        {
            const size_t begin = data.size();

            patchLayoutSelectionU16(
                data, featureOffsetPatches[3],
                static_cast<uint16_t>(
                    begin - featureListBegin));

            appendLayoutSelectionU16(data, 0);
            appendLayoutSelectionU16(data, 1);
            appendLayoutSelectionU16(data, 0);
        }


        // Feature 4: locl -> lookup 5.

        {
            const size_t begin = data.size();

            patchLayoutSelectionU16(
                data, featureOffsetPatches[4],
                static_cast<uint16_t>(
                    begin - featureListBegin));

            appendLayoutSelectionU16(data, 0);
            appendLayoutSelectionU16(data, 1);
            appendLayoutSelectionU16(data, 5);
        }


        // ------------------------------------------------------------
        // LookupList.
        //
        // Selection needs only a structurally valid count/offset array.
        // ------------------------------------------------------------

        const size_t lookupListBegin = data.size();

        patchLayoutSelectionU16(
            data, lookupListPatch,
            static_cast<uint16_t>(lookupListBegin));

        appendLayoutSelectionU16(data, 6);

        for (size_t i = 0; i < 6; ++i)
            appendLayoutSelectionU16(data, 0);

        return result;
    }


    // ====================================================================
    // Comparison helpers.
    // ====================================================================

    static bool layoutSelectionLookupsEqual(
        const OpenTypeLayoutLookupPlan& plan,
        std::initializer_list<uint16_t> expected) noexcept
    {
        if (plan.lookupIndices.size() != expected.size())
            return false;

        size_t index = 0;

        for (uint16_t value : expected)
        {
            if (plan.lookupIndices[index] != value)
                return false;

            ++index;
        }

        return true;
    }


    static bool layoutSelectionFeaturesEqual(
        const OpenTypeLayoutLookupPlan& plan,
        std::initializer_list<uint32_t> tags,
        std::initializer_list<bool> required) noexcept
    {
        if (plan.features.size() != tags.size() ||
            plan.features.size() != required.size())
        {
            return false;
        }

        auto tagIt = tags.begin();
        auto requiredIt = required.begin();

        for (size_t i = 0; i < plan.features.size(); ++i, ++tagIt, ++requiredIt)
        {
            if (plan.features[i].tag != *tagIt ||
                plan.features[i].required != *requiredIt)
            {
                return false;
            }
        }

        return true;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeLayoutSelection()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType layout selection: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const LayoutSelectionTestTable source =
            makeLayoutSelectionTestTable();

        const ByteSpan table(
            source.data.data(),
            source.data.size());


        // ====================================================================
        // Case 1 - Required feature.
        //
        // No optional features requested.
        //
        // latn DefaultLangSys requires feature 0:
        //
        //   rlig -> lookup 3
        //
        // The default request behavior must still include it.
        // ====================================================================

        {
            ++cases;

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 1 selection");

            if (!layoutSelectionFeaturesEqual(
                plan,
                { OTAG("rlig") },
                { true }))
            {
                return fail("case 1 required feature");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 3 }))
            {
                return fail("case 1 required lookup");
            }

            if (plan.scriptTag != OTAG("latn") ||
                plan.languageTag != 0 ||
                plan.usedDefaultScript ||
                !plan.usedDefaultLanguage ||
                !plan.lookupListData)
            {
                return fail("case 1 resolved state");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2 - Lookup union and LookupList ordering.
        //
        // Requested:
        //
        //   liga -> 2,4
        //   kern -> 1,4
        //
        // Required:
        //
        //   rlig -> 3
        //
        // Union:
        //
        //   1,2,3,4
        //
        // Feature request order must not determine lookup execution order.
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("liga"),
                OTAG("kern")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.featureTags = features;
            request.featureTagCount = 2;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 2 selection");

            if (!layoutSelectionFeaturesEqual(
                plan,
                {
                    OTAG("rlig"),
                    OTAG("liga"),
                    OTAG("kern")
                },
                {
                    true,
                    false,
                    false
                }))
            {
                return fail("case 2 selected features");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 1, 2, 3, 4 }))
            {
                return fail("case 2 lookup union/order");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3 - Exact named LangSys.
        //
        // TRK exposes:
        //
        //   required rlig
        //   optional locl, liga
        //
        // Request locl.
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("locl")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.languageTag = OTAG("TRK ");
            request.featureTags = features;
            request.featureTagCount = 1;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 3 selection");

            if (plan.scriptTag != OTAG("latn") ||
                plan.languageTag != OTAG("TRK ") ||
                plan.usedDefaultScript ||
                plan.usedDefaultLanguage)
            {
                return fail("case 3 named LangSys state");
            }

            if (!layoutSelectionFeaturesEqual(
                plan,
                {
                    OTAG("rlig"),
                    OTAG("locl")
                },
                {
                    true,
                    false
                }))
            {
                return fail("case 3 features");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 3, 5 }))
            {
                return fail("case 3 lookups");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Named language fallback to DefaultLangSys.
        //
        // DEU is absent from latn.
        //
        // With fallback enabled, resolve latn DefaultLangSys.
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("liga")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.languageTag = OTAG("DEU ");
            request.featureTags = features;
            request.featureTagCount = 1;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 4 selection");

            if (plan.scriptTag != OTAG("latn") ||
                plan.languageTag != 0 ||
                plan.usedDefaultScript ||
                !plan.usedDefaultLanguage)
            {
                return fail("case 4 fallback state");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 2, 3, 4 }))
            {
                return fail("case 4 lookups");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - Disable language fallback.
        //
        // Use the convenience overload deliberately.
        //
        // This also proves that adding includeRequiredFeature to the request
        // did not shift the convenience overload's fallback arguments.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint32_t> features =
            {
                OTAG("liga")
            };

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table,
                    OTAG("latn"),
                    OTAG("DEU "),
                    features,
                    plan,
                    true,
                    false);

            if (result !=
                OpenTypeLayoutSelectionResult::NoLanguageSystem)
            {
                return fail("case 5 language fallback control");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - DFLT script fallback.
        //
        // cyrl is absent.
        //
        // DFLT exposes:
        //
        //   optional calt -> lookup 0
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("calt")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("cyrl");
            request.featureTags = features;
            request.featureTagCount = 1;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 6 selection");

            if (plan.scriptTag != OTAG("DFLT") ||
                !plan.usedDefaultScript ||
                !plan.usedDefaultLanguage)
            {
                return fail("case 6 DFLT state");
            }

            if (!layoutSelectionFeaturesEqual(
                plan,
                { OTAG("calt") },
                { false }))
            {
                return fail("case 6 features");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 0 }))
            {
                return fail("case 6 lookups");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Disable DFLT script fallback.
        //
        // Again use the convenience overload deliberately. If its aggregate
        // initializer was not fixed after inserting includeRequiredFeature,
        // this case incorrectly falls through to DFLT.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint32_t> features =
            {
                OTAG("calt")
            };

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table,
                    OTAG("cyrl"),
                    0,
                    features,
                    plan,
                    false,
                    true);

            if (result !=
                OpenTypeLayoutSelectionResult::NoScript)
            {
                return fail("case 7 script fallback control");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Unsupported optional feature.
        //
        // Missing optional features are ignored.
        //
        // The required rlig feature remains selected.
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("xxxx")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.featureTags = features;
            request.featureTagCount = 1;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 8 selection");

            if (!layoutSelectionFeaturesEqual(
                plan,
                { OTAG("rlig") },
                { true }))
            {
                return fail("case 8 required feature");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 3 }))
            {
                return fail("case 8 lookup plan");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9 - Feature and lookup de-duplication.
        //
        // Repeated requests for the same FeatureIndex must not duplicate the
        // selected feature or any lookup.
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("liga"),
                OTAG("liga"),
                OTAG("kern"),
                OTAG("liga")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.featureTags = features;
            request.featureTagCount = 4;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 9 selection");

            if (!layoutSelectionFeaturesEqual(
                plan,
                {
                    OTAG("rlig"),
                    OTAG("liga"),
                    OTAG("kern")
                },
                {
                    true,
                    false,
                    false
                }))
            {
                return fail("case 9 feature deduplication");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 1, 2, 3, 4 }))
            {
                return fail("case 9 lookup deduplication");
            }

            ++passed;
        }


        // ====================================================================
        // Case 10 - Structural failures.
        // ====================================================================

        {
            ++cases;


            // Empty table.

            {
                OpenTypeLayoutFeatureRequest request;
                request.scriptTag = OTAG("latn");

                OpenTypeLayoutLookupPlan plan;

                if (selectOpenTypeLayoutLookups(
                    ByteSpan{}, request, plan) !=
                    OpenTypeLayoutSelectionResult::Invalid)
                {
                    return fail("case 10 empty table");
                }
            }


            // scriptTag == 0.

            {
                OpenTypeLayoutFeatureRequest request;

                OpenTypeLayoutLookupPlan plan;

                if (selectOpenTypeLayoutLookups(
                    table, request, plan) !=
                    OpenTypeLayoutSelectionResult::Invalid)
                {
                    return fail("case 10 zero script");
                }
            }


            // Non-zero feature count with NULL featureTags.

            {
                OpenTypeLayoutFeatureRequest request;
                request.scriptTag = OTAG("latn");
                request.featureTagCount = 1;

                OpenTypeLayoutLookupPlan plan;

                if (selectOpenTypeLayoutLookups(
                    table, request, plan) !=
                    OpenTypeLayoutSelectionResult::Invalid)
                {
                    return fail("case 10 null feature tags");
                }
            }


            // Truncated layout header.

            {
                OpenTypeLayoutFeatureRequest request;
                request.scriptTag = OTAG("latn");

                OpenTypeLayoutLookupPlan plan;

                if (selectOpenTypeLayoutLookups(
                    ByteSpan(source.data.data(), 5),
                    request, plan) !=
                    OpenTypeLayoutSelectionResult::Invalid)
                {
                    return fail("case 10 truncated header");
                }
            }


            // Invalid optional FeatureIndex in LangSys.

            {
                LayoutSelectionTestTable broken =
                    makeLayoutSelectionTestTable();

                patchLayoutSelectionU16(
                    broken.data,
                    broken.latnDefaultFirstOptionalPatch,
                    99);

                OpenTypeLayoutFeatureRequest request;
                request.scriptTag = OTAG("latn");

                OpenTypeLayoutLookupPlan plan;

                if (selectOpenTypeLayoutLookups(
                    ByteSpan(
                        broken.data.data(),
                        broken.data.size()),
                    request, plan) !=
                    OpenTypeLayoutSelectionResult::Invalid)
                {
                    return fail("case 10 invalid optional feature index");
                }
            }


            // Selected feature references an invalid LookupList index.

            {
                LayoutSelectionTestTable broken =
                    makeLayoutSelectionTestTable();

                patchLayoutSelectionU16(
                    broken.data,
                    broken.ligaFirstLookupPatch,
                    99);

                const uint32_t features[] =
                {
                    OTAG("liga")
                };

                OpenTypeLayoutFeatureRequest request;
                request.scriptTag = OTAG("latn");
                request.featureTags = features;
                request.featureTagCount = 1;

                OpenTypeLayoutLookupPlan plan;

                if (selectOpenTypeLayoutLookups(
                    ByteSpan(
                        broken.data.data(),
                        broken.data.size()),
                    request, plan) !=
                    OpenTypeLayoutSelectionResult::Invalid)
                {
                    return fail("case 10 invalid lookup index");
                }
            }

            ++passed;
        }


        // ====================================================================
        // Case 11 - Required feature suppression.
        //
        // Later shaping stages must be able to select optional features
        // without executing the LangSys required feature again.
        //
        // Request:
        //
        //   includeRequiredFeature = false
        //   liga
        //
        // Expected:
        //
        //   selected feature: liga only
        //   lookups:          2,4
        //
        // rlig / lookup 3 must not appear.
        // ====================================================================

        {
            ++cases;

            const uint32_t features[] =
            {
                OTAG("liga")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.featureTags = features;
            request.featureTagCount = 1;
            request.includeRequiredFeature = false;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    table, request, plan);

            if (result != OpenTypeLayoutSelectionResult::Success)
                return fail("case 11 selection");

            if (!layoutSelectionFeaturesEqual(
                plan,
                { OTAG("liga") },
                { false }))
            {
                return fail("case 11 required suppression");
            }

            if (!layoutSelectionLookupsEqual(
                plan, { 2, 4 }))
            {
                return fail("case 11 lookup plan");
            }

            ++passed;
        }


        // ====================================================================
        // Case 12 - Suppressed required feature is still validated.
        //
        // Suppression affects selection only.
        //
        // A malformed ReqFeatureIndex remains malformed layout data even when
        // includeRequiredFeature == false.
        // ====================================================================

        {
            ++cases;

            LayoutSelectionTestTable broken =
                makeLayoutSelectionTestTable();

            patchLayoutSelectionU16(
                broken.data,
                broken.latnDefaultRequiredPatch,
                99);

            const uint32_t features[] =
            {
                OTAG("liga")
            };

            OpenTypeLayoutFeatureRequest request;
            request.scriptTag = OTAG("latn");
            request.featureTags = features;
            request.featureTagCount = 1;
            request.includeRequiredFeature = false;

            OpenTypeLayoutLookupPlan plan;

            const OpenTypeLayoutSelectionResult result =
                selectOpenTypeLayoutLookups(
                    ByteSpan(
                        broken.data.data(),
                        broken.data.size()),
                    request, plan);

            if (result !=
                OpenTypeLayoutSelectionResult::Invalid)
            {
                return fail(
                    "case 12 suppressed invalid required feature accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType layout selection: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Required feature:           PASS\n"
            "  Lookup union/order:         PASS\n"
            "  Named LangSys:              PASS\n"
            "  Default LangSys fallback:   PASS\n"
            "  Language fallback control:  PASS\n"
            "  DFLT script fallback:       PASS\n"
            "  Script fallback control:    PASS\n"
            "  Unsupported feature:        PASS\n"
            "  Feature deduplication:      PASS\n"
            "  Structural failures:        PASS\n"
            "  Required suppression:       PASS\n"
            "  Required validation:        PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs