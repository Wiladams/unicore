// opentype_feature_lookup.h
#pragma once

#include "opentype_feature_plan.h"
#include "opentype_layout_view.h"

namespace waavs
{
    struct OpenTypeFeatureLookupRef
    {
        Tag featureTag{ 0 };
        OpenTypeFeatureIndex featureIndex{ kOpenTypeFeatureIndexInvalid };
        uint16_t lookupIndex{ 0 };
        bool requiredFeature{ false };
    };


    // ====================================================================
    // Validate one selected Feature against FeatureList and LookupList.
    //
    // The Lookup itself is not dereferenced. We validate only that:
    //
    //   - the FeatureList index exists
    //   - the Feature table is structurally valid
    //   - every referenced lookup index exists in LookupList
    // ====================================================================

    static inline bool validateOpenTypeFeatureLookups(const OpenTypeSelectedFeature& selected, const OpenTypeLayoutFeatureListView& features, const OpenTypeLayoutLookupListView& lookups) noexcept
    {
        if (selected.featureIndex == kOpenTypeFeatureIndexInvalid || selected.featureIndex >= features.size())
            return false;

        Tag featureTag = 0;

        if (!features.featureTag(selected.featureIndex, featureTag))
            return false;

        if (featureTag != selected.tag)
            return false;

        const OpenTypeLayoutFeatureView feature = features.feature(selected.featureIndex);

        if (!feature)
            return false;

        const uint16_t lookupCount = feature.lookupCount();
        const uint16_t lookupListCount = lookups.size();

        for (uint16_t i = 0; i < lookupCount; ++i)
        {
            uint16_t lookupIndex = 0;

            if (!feature.lookupIndex(i, lookupIndex))
                return false;

            if (lookupIndex >= lookupListCount)
                return false;
        }

        return true;
    }


    // ====================================================================
    // Validate the complete FeaturePlan before producing any lookup refs.
    //
    // This prevents a malformed later Feature from leaving a caller with
    // a partially traversed plan.
    // ====================================================================

    static inline bool validateOpenTypeFeaturePlanLookups(const OpenTypeFeaturePlan& plan, const OpenTypeLayoutFeatureListView& features, const OpenTypeLayoutLookupListView& lookups) noexcept
    {
        if (!features || !lookups)
            return false;

        if (plan.hasRequiredFeature())
        {
            if (!validateOpenTypeFeatureLookups(plan.requiredFeature(), features, lookups))
                return false;
        }

        for (size_t i = 0; i < plan.size(); ++i)
        {
            if (!validateOpenTypeFeatureLookups(plan[i], features, lookups))
                return false;
        }

        return true;
    }


    // ====================================================================
    // Traverse one selected Feature.
    //
    // Callback signature:
    //
    //   bool(const OpenTypeFeatureLookupRef&)
    //
    // Returning false stops traversal and propagates failure.
    // ====================================================================

    template <typename Fn>
    static inline bool forEachOpenTypeFeatureLookup(const OpenTypeSelectedFeature& selected, bool requiredFeature, const OpenTypeLayoutFeatureListView& features, Fn&& fn)
    {
        const OpenTypeLayoutFeatureView feature = features.feature(selected.featureIndex);

        if (!feature)
            return false;

        const uint16_t lookupCount = feature.lookupCount();

        for (uint16_t i = 0; i < lookupCount; ++i)
        {
            uint16_t lookupIndex = 0;

            if (!feature.lookupIndex(i, lookupIndex))
                return false;

            OpenTypeFeatureLookupRef ref{};
            ref.featureTag = selected.tag;
            ref.featureIndex = selected.featureIndex;
            ref.lookupIndex = lookupIndex;
            ref.requiredFeature = requiredFeature;

            if (!fn(ref))
                return false;
        }

        return true;
    }


    // ====================================================================
    // Traverse all lookup references selected by an OpenTypeFeaturePlan.
    //
    // Order:
    //
    //   1. required Feature, when present
    //   2. optional Features in OpenTypeFeaturePlan order
    //   3. lookup indices in each Feature table's stored order
    //
    // All references are validated before the callback is invoked.
    // ====================================================================

    template <typename Fn>
    static inline bool forEachOpenTypeFeatureLookup(const OpenTypeFeaturePlan& plan, const OpenTypeLayoutFeatureListView& features, const OpenTypeLayoutLookupListView& lookups, Fn&& fn)
    {
        if (!validateOpenTypeFeaturePlanLookups(plan, features, lookups))
            return false;

        if (plan.hasRequiredFeature())
        {
            if (!forEachOpenTypeFeatureLookup(plan.requiredFeature(), true, features, fn))
                return false;
        }

        for (size_t i = 0; i < plan.size(); ++i)
        {
            if (!forEachOpenTypeFeatureLookup(plan[i], false, features, fn))
                return false;
        }

        return true;
    }

} // namespace waavs
