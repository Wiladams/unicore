// opentype_horizontal_shaper.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <cstdio>


#include "opentype_nominal_glyphs.h"
#include "opentype_nominal_metrics.h"
#include "opentype_layout_selection.h"
#include "opentype_shaping_policy.h"
#include "opentype_gsub_orchestrator.h"
#include "opentype_gpos_orchestrator.h"
#include "opentype_face_tables.h"
#include "opentype_gdef_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeHorizontalShapeRequest
    //
    // Low-level explicit-feature request.
    //
    // This remains policy-free and is useful when the caller wants direct
    // control over one GSUB feature set and one GPOS feature set.
    //
    // Required LangSys features are included automatically.
    //
    // languageTag == 0 requests DefaultLangSys.
    // ====================================================================

    struct OpenTypeHorizontalShapeRequest
    {
        uint32_t scriptTag{ 0 };
        uint32_t languageTag{ 0 };

        const uint32_t* gsubFeatureTags{ nullptr };
        size_t gsubFeatureCount{ 0 };

        const uint32_t* gposFeatureTags{ nullptr };
        size_t gposFeatureCount{ 0 };

        bool fallbackToDefaultScript{ true };
        bool fallbackToDefaultLanguage{ true };
    };


    // ====================================================================
    // OpenTypeHorizontalFaceTables
    // ====================================================================

    struct OpenTypeHorizontalFaceTables
    {
        const TableRecord* gsub{ nullptr };
        const TableRecord* gpos{ nullptr };
        OpenTypeGdefView gdef{};
    };


    // ====================================================================
    // resolveOpenTypeHorizontalFaceTables
    //
    // GSUB, GPOS and GDEF are optional.
    //
    // If GDEF is present, however, malformed GDEF is a hard failure.
    // ====================================================================

    static inline bool resolveOpenTypeHorizontalFaceTables(
        const FontRunView& run, OpenTypeHorizontalFaceTables& result)
    {
        result = {};

        if (!run.face)
            return false;

        const IProvideOpenTypeTables* tables =
            openTypeTableProvider(run.face);

        if (!tables)
            return false;

        result.gsub = tables->getTable(OTAG("GSUB"));
        result.gpos = tables->getTable(OTAG("GPOS"));

        const TableRecord* gdefTable =
            tables->getTable(OTAG("GDEF"));

        if (gdefTable)
        {
            result.gdef = OpenTypeGdefView(gdefTable->data);

            if (!result.gdef)
                return false;
        }

        return true;
    }


    // ====================================================================
    // selectOpenTypeHorizontalLayoutPlan
    //
    // Common structural selector used by both the explicit-feature and
    // policy-driven shaping paths.
    //
    // selected:
    //
    //   true  -> Script/LangSys resolved and a plan may be executed
    //   false -> legal no-op because Script/LangSys was unavailable
    //
    // Invalid layout data remains a hard failure.
    // ====================================================================

    static inline bool selectOpenTypeHorizontalLayoutPlan(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const uint32_t* featureTags, size_t featureCount,
        bool includeRequiredFeature,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        OpenTypeLayoutLookupPlan& plan, bool& selected)
    {
        selected = false;
        plan.clear();

        if (!tableData || scriptTag == 0)
            return false;

        if (featureCount != 0 && !featureTags)
            return false;

        OpenTypeLayoutFeatureRequest layoutRequest;
        layoutRequest.scriptTag = scriptTag;
        layoutRequest.languageTag = languageTag;
        layoutRequest.featureTags = featureTags;
        layoutRequest.featureTagCount = featureCount;
        layoutRequest.includeRequiredFeature = includeRequiredFeature;
        layoutRequest.fallbackToDefaultScript = fallbackToDefaultScript;
        layoutRequest.fallbackToDefaultLanguage = fallbackToDefaultLanguage;

        const OpenTypeLayoutSelectionResult result =
            selectOpenTypeLayoutLookups(
                tableData, layoutRequest, plan);

        switch (result)
        {
        case OpenTypeLayoutSelectionResult::Success:
            //selected = true;
            selected = !plan.empty();
            return true;

        case OpenTypeLayoutSelectionResult::NoScript:
        case OpenTypeLayoutSelectionResult::NoLanguageSystem:
            plan.clear();
            return true;

        default:
            plan.clear();
            return false;
        }
    }


    // ====================================================================
    // Existing explicit-request selector.
    //
    // Preserve the original API. A flat explicit request represents one
    // complete shaping stage, so its required feature is included.
    // ====================================================================

    static inline bool selectOpenTypeHorizontalLayoutPlan(
        ByteSpan tableData, const OpenTypeHorizontalShapeRequest& request,
        const uint32_t* featureTags, size_t featureCount,
        OpenTypeLayoutLookupPlan& plan, bool& selected)
    {
        return selectOpenTypeHorizontalLayoutPlan(
            tableData,
            request.scriptTag,
            request.languageTag,
            featureTags,
            featureCount,
            true,
            request.fallbackToDefaultScript,
            request.fallbackToDefaultLanguage,
            plan,
            selected);
    }


    // ====================================================================
    // applyOpenTypeHorizontalGsubPolicy
    //
    // Execute GSUB policy stages in policy order.
    //
    // Each stage independently selects its feature set. Within that stage,
    // lookup execution remains normalized to LookupList order by the shared
    // layout selector.
    //
    // The policy controls which stage includes the LangSys required feature.
    // Normally only the first GSUB stage includes it.
    // ====================================================================

    static inline bool applyOpenTypeHorizontalGsubPolicy(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const OpenTypeShapingPolicy& policy,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer)
    {
        if (!tableData || scriptTag == 0)
        {
            std::printf("OpenType GSUB policy: invalid input\n");
            return false;
        }

        if (policy.gsubStageCount != 0 && !policy.gsubStages)
        {
            std::printf("OpenType GSUB policy: missing stages\n");
            return false;
        }

        for (size_t stageIndex = 0; stageIndex < policy.gsubStageCount; ++stageIndex)
        {
            const OpenTypeShapingFeatureStage& stage = policy.gsubStages[stageIndex];

            OpenTypeLayoutLookupPlan plan;
            bool selected = false;

            if (!selectOpenTypeHorizontalLayoutPlan(
                tableData, scriptTag, languageTag,
                stage.featureTags, stage.featureTagCount,
                stage.includeRequiredFeature,
                fallbackToDefaultScript, fallbackToDefaultLanguage,
                plan, selected))
            {
                std::printf(
                    "OpenType GSUB policy: stage %zu selection FAIL\n"
                    "  features: %zu\n"
                    "  glyphs:   %zu\n",
                    stageIndex,
                    stage.featureTagCount,
                    buffer.size());

                return false;
            }

            std::printf(
                "OpenType GSUB policy: stage %zu selected\n"
                "  selected: %s\n"
                "  features: %zu\n"
                "  lookups:  %zu\n"
                "  glyphs:   %zu\n",
                stageIndex,
                selected ? "yes" : "no",
                plan.features.size(),
                plan.lookupIndices.size(),
                buffer.size());

            if (selected && !applyOpenTypeGsubLookupPlan(plan, gdef, buffer))
            {
                std::printf(
                    "OpenType GSUB policy: stage %zu execution FAIL\n"
                    "  lookups: %zu\n"
                    "  glyphs:  %zu\n",
                    stageIndex,
                    plan.lookupIndices.size(),
                    buffer.size());

                const OpenTypeLayoutLookupListView lookups(plan.lookupListData);

                if (!lookups)
                {
                    std::printf("  LookupList: INVALID\n");
                    return false;
                }

                OpenTypeShapingBuffer diagnostic = buffer;

                for (size_t i = 0; i < plan.lookupIndices.size(); ++i)
                {
                    const uint16_t lookupIndex = plan.lookupIndices[i];
                    const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

                    if (!lookup)
                    {
                        std::printf(
                            "  lookup[%zu]: index=%u INVALID\n",
                            i,
                            static_cast<unsigned>(lookupIndex));

                        return false;
                    }

                    uint16_t effectiveType = 0;
                    const bool hasEffectiveType =
                        openTypeGsubEffectiveLookupType(lookup, effectiveType);

                    std::printf(
                        "  lookup[%zu]: index=%u type=%u effective=%u flag=0x%04x subtables=%u\n",
                        i,
                        static_cast<unsigned>(lookupIndex),
                        static_cast<unsigned>(lookup.lookupType()),
                        hasEffectiveType ? static_cast<unsigned>(effectiveType) : 0u,
                        static_cast<unsigned>(lookup.lookupFlag()),
                        static_cast<unsigned>(lookup.subtableCount()));

                    if (!hasEffectiveType)
                    {
                        std::printf("    effective type: FAIL\n");
                        return false;
                    }

                    if (!applyOpenTypeGsubLookup(
                        lookups,
                        lookupIndex,
                        gdef,
                        diagnostic))
                    {
                        std::printf(
                            "    execution: FAIL\n"
                            "    glyphs before/after: %zu\n",
                            diagnostic.size());

                        return false;
                    }

                    std::printf(
                        "    execution: PASS\n"
                        "    glyphs after: %zu\n",
                        diagnostic.size());
                }

                return false;
            }

            std::printf(
                "OpenType GSUB policy: stage %zu execution PASS\n"
                "  glyphs: %zu\n",
                stageIndex,
                buffer.size());
        }

        return true;
    }


    /*
    * // BUGBUG - temporary diagnostic
    static inline bool applyOpenTypeHorizontalGsubPolicy(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const OpenTypeShapingPolicy& policy,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer)
    {
        if (!tableData || scriptTag == 0)
            return false;

        if (policy.gsubStageCount != 0 && !policy.gsubStages)
            return false;

        for (size_t stageIndex = 0; stageIndex < policy.gsubStageCount; ++stageIndex)
        {
            const OpenTypeShapingFeatureStage& stage =
                policy.gsubStages[stageIndex];

            OpenTypeLayoutLookupPlan plan;
            bool selected = false;

            if (!selectOpenTypeHorizontalLayoutPlan(
                tableData,
                scriptTag,
                languageTag,
                stage.featureTags,
                stage.featureTagCount,
                stage.includeRequiredFeature,
                fallbackToDefaultScript,
                fallbackToDefaultLanguage,
                plan,
                selected))
            {
                return false;
            }

            if (selected &&
                !applyOpenTypeGsubLookupPlan(
                    plan, gdef, buffer))
            {
                return false;
            }
        }

        return true;
    }
    */


    // ====================================================================
    // applyOpenTypeHorizontalGposPolicy
    //
    // For now a horizontal shaping policy may contain at most one GPOS
    // stage.
    //
    // This preserves the current GPOS orchestrator invariant:
    //
    //   all participating positioning lookups
    //       -> one shared attachment graph
    //       -> one final attachment resolution
    //
    // Multiple GPOS policy stages would require a higher-level orchestrator
    // capable of sharing attachment state across plans.
    // ====================================================================

    static inline bool applyOpenTypeHorizontalGposPolicy(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const OpenTypeShapingPolicy& policy,
        bool fallbackToDefaultScript, bool fallbackToDefaultLanguage,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        if (!tableData || scriptTag == 0)
            return false;

        if (policy.gposStageCount == 0)
            return true;

        if (!policy.gposStages || policy.gposStageCount != 1)
            return false;

        const OpenTypeShapingFeatureStage& stage =
            policy.gposStages[0];

        OpenTypeLayoutLookupPlan plan;
        bool selected = false;

        if (!selectOpenTypeHorizontalLayoutPlan(
            tableData,
            scriptTag,
            languageTag,
            stage.featureTags,
            stage.featureTagCount,
            stage.includeRequiredFeature,
            fallbackToDefaultScript,
            fallbackToDefaultLanguage,
            plan,
            selected))
        {
            return false;
        }

        if (selected &&
            !applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, runRightToLeft))
        {
            return false;
        }

        return true;
    }


    // ====================================================================
    // shapeOpenTypeHorizontalRun
    //
    // Low-level explicit-feature path.
    //
    // This preserves the original API and remains useful for direct feature
    // control, synthetic testing and differential validation.
    //
    //     cmap
    //       -> GSUB
    //       -> nominal hmtx metrics
    //       -> GPOS
    //
    // The complete operation is transactional. output is replaced only after
    // every stage succeeds.
    //
    // RTL runs remain in logical glyph order.
    // ====================================================================

    [[nodiscard]]
    static inline bool shapeOpenTypeHorizontalRun(
        const FontRunView& input, const OpenTypeHorizontalShapeRequest& request,
        ShapedGlyphBuffer& output)
    {
        if (!input.face || request.scriptTag == 0)
            return false;

        if ((request.gsubFeatureCount != 0 && !request.gsubFeatureTags) ||
            (request.gposFeatureCount != 0 && !request.gposFeatureTags))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 1. cmap.
        // ------------------------------------------------------------

        OpenTypeShapingBuffer shaping;

        if (!mapOpenTypeNominalGlyphs(input, shaping))
            return false;

        const FontRunView* run = shaping.input();

        if (!run || !run->face)
            return false;


        // ------------------------------------------------------------
        // Face layout tables.
        // ------------------------------------------------------------

        OpenTypeHorizontalFaceTables tables;

        if (!resolveOpenTypeHorizontalFaceTables(
            *run, tables))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 2. GSUB.
        // ------------------------------------------------------------

        if (tables.gsub)
        {
            OpenTypeLayoutLookupPlan plan;
            bool selected = false;

            if (!selectOpenTypeHorizontalLayoutPlan(
                tables.gsub->data,
                request,
                request.gsubFeatureTags,
                request.gsubFeatureCount,
                plan,
                selected))
            {
                return false;
            }

            if (selected &&
                !applyOpenTypeGsubLookupPlan(
                    plan, tables.gdef, shaping))
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // 3. Nominal horizontal metrics.
        // ------------------------------------------------------------

        ShapedGlyphBuffer positioned;

        if (!buildOpenTypeHorizontalShapedGlyphs(
            shaping, positioned))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 4. GPOS.
        // ------------------------------------------------------------

        if (tables.gpos)
        {
            OpenTypeLayoutLookupPlan plan;
            bool selected = false;

            if (!selectOpenTypeHorizontalLayoutPlan(
                tables.gpos->data,
                request,
                request.gposFeatureTags,
                request.gposFeatureCount,
                plan,
                selected))
            {
                return false;
            }

            const bool runRightToLeft =
                (run->bidiLevel & 1u) != 0;

            if (selected &&
                !applyOpenTypeGposLookupPlan(
                    plan, tables.gdef, positioned,
                    runRightToLeft))
            {
                return false;
            }
        }


        output = std::move(positioned);
        return true;
    }


    // ====================================================================
    // shapeOpenTypeHorizontalRun
    //
    // Policy-driven path.
    //
    // The caller supplies the already-resolved OpenType script/language and
    // an explicit shaping policy.
    //
    // GSUB stages execute in policy order.
    //
    // Nominal metrics are loaded only after every GSUB stage completes.
    //
    // GPOS currently supports one policy stage so all attachment-producing
    // lookups continue to share one attachment graph and one final resolve.
    // ====================================================================

    [[nodiscard]]
    static inline bool shapeOpenTypeHorizontalRun(
        const FontRunView& input,
        uint32_t scriptTag, uint32_t languageTag,
        const OpenTypeShapingPolicy& policy,
        ShapedGlyphBuffer& output,
        bool fallbackToDefaultScript = true,
        bool fallbackToDefaultLanguage = true)
    {
        if (!input.face || scriptTag == 0)
            return false;

        if ((policy.gsubStageCount != 0 && !policy.gsubStages) ||
            (policy.gposStageCount != 0 && !policy.gposStages))
        {
            return false;
        }

        if (policy.gposStageCount > 1)
            return false;


        // ------------------------------------------------------------
        // 1. cmap.
        // ------------------------------------------------------------

        OpenTypeShapingBuffer shaping;

        if (!mapOpenTypeNominalGlyphs(input, shaping))
            return false;

        const FontRunView* run = shaping.input();

        if (!run || !run->face)
            return false;


        // ------------------------------------------------------------
        // Face layout tables.
        // ------------------------------------------------------------

        OpenTypeHorizontalFaceTables tables;

        if (!resolveOpenTypeHorizontalFaceTables(
            *run, tables))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 2. Ordered GSUB policy stages.
        // ------------------------------------------------------------

        if (tables.gsub &&
            !applyOpenTypeHorizontalGsubPolicy(
                tables.gsub->data,
                scriptTag,
                languageTag,
                policy,
                fallbackToDefaultScript,
                fallbackToDefaultLanguage,
                tables.gdef,
                shaping))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 3. Nominal horizontal metrics.
        //
        // All GSUB stages are complete before metric lookup.
        // ------------------------------------------------------------

        ShapedGlyphBuffer positioned;

        if (!buildOpenTypeHorizontalShapedGlyphs(
            shaping, positioned))
        {
            return false;
        }


        // ------------------------------------------------------------
        // 4. GPOS policy.
        // ------------------------------------------------------------

        if (tables.gpos)
        {
            const bool runRightToLeft =
                (run->bidiLevel & 1u) != 0;

            if (!applyOpenTypeHorizontalGposPolicy(
                tables.gpos->data,
                scriptTag,
                languageTag,
                policy,
                fallbackToDefaultScript,
                fallbackToDefaultLanguage,
                tables.gdef,
                positioned,
                runRightToLeft))
            {
                return false;
            }
        }


        output = std::move(positioned);
        return true;
    }


    // ====================================================================
    // shapeOpenTypeHorizontalRun
    //
    // Normal policy-driven entry point.
    //
    // Select the shaping policy directly from the OpenType script tag.
    // ====================================================================

    [[nodiscard]]
    static inline bool shapeOpenTypeHorizontalRun(
        const FontRunView& input,
        uint32_t scriptTag, uint32_t languageTag,
        ShapedGlyphBuffer& output,
        bool fallbackToDefaultScript = true,
        bool fallbackToDefaultLanguage = true)
    {
        if (scriptTag == 0)
            return false;

        const OpenTypeShapingPolicy& policy =
            openTypeShapingPolicyForScript(scriptTag);

        return shapeOpenTypeHorizontalRun(
            input,
            scriptTag,
            languageTag,
            policy,
            output,
            fallbackToDefaultScript,
            fallbackToDefaultLanguage);
    }

} // namespace waavs