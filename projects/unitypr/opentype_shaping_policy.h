// opentype_shaping_policy.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_types.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeShapingFeatureStage
    //
    // One ordered shaping stage.
    //
    // All features in one stage are selected together. Their referenced
    // lookups are unioned and executed in LookupList order.
    //
    // includeRequiredFeature:
    //
    //   The resolved LangSys ReqFeatureIndex belongs to the shaping
    //   pipeline as a whole, not independently to every stage.
    //
    //   Therefore only the first stage normally enables it.
    // ====================================================================

    struct OpenTypeShapingFeatureStage
    {
        const uint32_t* featureTags{ nullptr };
        size_t featureTagCount{ 0 };
        bool includeRequiredFeature{ false };

        [[nodiscard]] bool empty() const noexcept
        {
            return featureTagCount == 0;
        }

        [[nodiscard]] uint32_t featureTag(size_t index) const noexcept
        {
            return index < featureTagCount ? featureTags[index] : 0;
        }
    };


    // ====================================================================
    // OpenTypeShapingPolicy
    // ====================================================================

    struct OpenTypeShapingPolicy
    {
        const OpenTypeShapingFeatureStage* gsubStages{ nullptr };
        size_t gsubStageCount{ 0 };

        const OpenTypeShapingFeatureStage* gposStages{ nullptr };
        size_t gposStageCount{ 0 };

        [[nodiscard]] const OpenTypeShapingFeatureStage* gsubStage(size_t index) const noexcept
        {
            return index < gsubStageCount ? &gsubStages[index] : nullptr;
        }

        [[nodiscard]] const OpenTypeShapingFeatureStage* gposStage(size_t index) const noexcept
        {
            return index < gposStageCount ? &gposStages[index] : nullptr;
        }
    };


    // ====================================================================
    // Generic policy
    //
    // Keep this deliberately conservative.
    //
    // GSUB:
    //
    //   stage 0: ccmp locl
    //   stage 1: rlig
    //
    // GPOS:
    //
    //   stage 0: kern mark mkmk
    //
    // Complex scripts will eventually receive their own policies rather
    // than inheriting Latin-specific discretionary shaping.
    // ====================================================================

    static constexpr uint32_t kOpenTypeGenericGsubStage0Features[] =
    {
        OTAG("ccmp"),
        OTAG("locl")
    };

    static constexpr uint32_t kOpenTypeGenericGsubStage1Features[] =
    {
        OTAG("rlig")
    };

    static constexpr uint32_t kOpenTypeGenericGposStage0Features[] =
    {
        OTAG("kern"),
        OTAG("mark"),
        OTAG("mkmk")
    };

    static constexpr OpenTypeShapingFeatureStage kOpenTypeGenericGsubStages[] =
    {
        {
            kOpenTypeGenericGsubStage0Features,
            sizeof(kOpenTypeGenericGsubStage0Features) / sizeof(kOpenTypeGenericGsubStage0Features[0]),
            true
        },
        {
            kOpenTypeGenericGsubStage1Features,
            sizeof(kOpenTypeGenericGsubStage1Features) / sizeof(kOpenTypeGenericGsubStage1Features[0]),
            false
        }
    };

    static constexpr OpenTypeShapingFeatureStage kOpenTypeGenericGposStages[] =
    {
        {
            kOpenTypeGenericGposStage0Features,
            sizeof(kOpenTypeGenericGposStage0Features) / sizeof(kOpenTypeGenericGposStage0Features[0]),
            true
        }
    };

    static constexpr OpenTypeShapingPolicy kOpenTypeGenericShapingPolicy =
    {
        kOpenTypeGenericGsubStages,
        sizeof(kOpenTypeGenericGsubStages) / sizeof(kOpenTypeGenericGsubStages[0]),

        kOpenTypeGenericGposStages,
        sizeof(kOpenTypeGenericGposStages) / sizeof(kOpenTypeGenericGposStages[0])
    };


    // ====================================================================
    // Latin policy
    //
    // GSUB:
    //
    //   stage 0: ccmp locl
    //   stage 1: rlig
    //   stage 2: liga clig calt
    //
    // GPOS:
    //
    //   stage 0: kern mark mkmk
    //
    // Keep mark and mkmk in one GPOS stage so they participate in the same
    // attachment graph and are resolved together by the GPOS orchestrator.
    // ====================================================================

    static constexpr uint32_t kOpenTypeLatinGsubStage0Features[] =
    {
        OTAG("ccmp"),
        OTAG("locl")
    };

    static constexpr uint32_t kOpenTypeLatinGsubStage1Features[] =
    {
        OTAG("rlig")
    };

    static constexpr uint32_t kOpenTypeLatinGsubStage2Features[] =
    {
        OTAG("liga"),
        OTAG("clig"),
        OTAG("calt")
    };

    static constexpr uint32_t kOpenTypeLatinGposStage0Features[] =
    {
        OTAG("kern"),
        OTAG("mark"),
        OTAG("mkmk")
    };

    static constexpr OpenTypeShapingFeatureStage kOpenTypeLatinGsubStages[] =
    {
        {
            kOpenTypeLatinGsubStage0Features,
            sizeof(kOpenTypeLatinGsubStage0Features) / sizeof(kOpenTypeLatinGsubStage0Features[0]),
            true
        },
        {
            kOpenTypeLatinGsubStage1Features,
            sizeof(kOpenTypeLatinGsubStage1Features) / sizeof(kOpenTypeLatinGsubStage1Features[0]),
            false
        },
        {
            kOpenTypeLatinGsubStage2Features,
            sizeof(kOpenTypeLatinGsubStage2Features) / sizeof(kOpenTypeLatinGsubStage2Features[0]),
            false
        }
    };

    static constexpr OpenTypeShapingFeatureStage kOpenTypeLatinGposStages[] =
    {
        {
            kOpenTypeLatinGposStage0Features,
            sizeof(kOpenTypeLatinGposStage0Features) / sizeof(kOpenTypeLatinGposStage0Features[0]),
            true
        }
    };

    static constexpr OpenTypeShapingPolicy kOpenTypeLatinShapingPolicy =
    {
        kOpenTypeLatinGsubStages,
        sizeof(kOpenTypeLatinGsubStages) / sizeof(kOpenTypeLatinGsubStages[0]),

        kOpenTypeLatinGposStages,
        sizeof(kOpenTypeLatinGposStages) / sizeof(kOpenTypeLatinGposStages[0])
    };


    // ====================================================================
    // openTypeShapingPolicyForScript
    //
    // Start narrowly.
    //
    // Complex-script policies will be added explicitly as their shaping
    // algorithms are implemented. Until then they receive the conservative
    // generic policy rather than Latin-specific discretionary features.
    // ====================================================================

    [[nodiscard]]
    static inline const OpenTypeShapingPolicy& openTypeShapingPolicyForScript(uint32_t scriptTag) noexcept
    {
        switch (scriptTag)
        {
        case OTAG("latn"):
            return kOpenTypeLatinShapingPolicy;

        default:
            return kOpenTypeGenericShapingPolicy;
        }
    }

} // namespace waavs