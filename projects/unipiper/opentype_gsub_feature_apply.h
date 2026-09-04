// opentype_gsub_feature_apply.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_feature_plan.h"
#include "opentype_gsub_lookup_apply.h"
#include "opentype_layout_view.h"

namespace waavs
{
    // ====================================================================
    // markOpenTypeFeatureLookups
    //
    // Add one selected Feature's lookup indices to the execution set.
    //
    // selectedLookups is indexed directly by LookupList index.
    // ====================================================================

    static inline bool markOpenTypeFeatureLookups(const OpenTypeSelectedFeature& selected,
        const OpenTypeLayoutFeatureListView& features, uint16_t lookupCount,
        std::vector<uint8_t>& selectedLookups) noexcept
    {
        if (selected.featureIndex == kOpenTypeFeatureIndexInvalid ||
            selected.featureIndex >= features.size())
        {
            return false;
        }

        Tag featureTag = 0;

        if (!features.featureTag(selected.featureIndex, featureTag))
            return false;

        if (featureTag != selected.tag)
            return false;

        const OpenTypeLayoutFeatureView feature = features.feature(selected.featureIndex);

        if (!feature)
            return false;

        const uint16_t count = feature.lookupCount();

        for (uint16_t i = 0; i < count; ++i)
        {
            uint16_t lookupIndex = 0;

            if (!feature.lookupIndex(i, lookupIndex))
                return false;

            if (lookupIndex >= lookupCount)
                return false;

            selectedLookups[lookupIndex] = 1;
        }

        return true;
    }


    // ====================================================================
    // buildOpenTypeLookupSelection
    //
    // Build the union of LookupList indices referenced by the FeaturePlan.
    //
    // The mask automatically deduplicates lookups referenced by multiple
    // selected Features.
    // ====================================================================

    static inline bool buildOpenTypeLookupSelection(const OpenTypeFeaturePlan& plan,
        const OpenTypeLayoutFeatureListView& features,
        const OpenTypeLayoutLookupListView& lookups,
        std::vector<uint8_t>& selectedLookups)
    {
        selectedLookups.clear();

        if (!features || !lookups)
            return false;

        const uint16_t lookupCount = lookups.size();

        selectedLookups.resize(lookupCount, 0);


        if (plan.hasRequiredFeature())
        {
            if (!markOpenTypeFeatureLookups(
                plan.requiredFeature(), features, lookupCount, selectedLookups))
            {
                return false;
            }
        }


        for (size_t i = 0; i < plan.size(); ++i)
        {
            if (!markOpenTypeFeatureLookups(
                plan[i], features, lookupCount, selectedLookups))
            {
                return false;
            }
        }


        return true;
    }


    // ====================================================================
    // applyOpenTypeGsubFeaturePlan
    //
    // Apply the union of lookups referenced by the selected Features.
    //
    // Lookup execution order is LookupList order, independent of Feature
    // ordering in OpenTypeFeaturePlan.
    //
    // The operation is transactional with respect to the caller's buffer.
    // ====================================================================

    static inline bool applyOpenTypeGsubFeaturePlan(const OpenTypeFeaturePlan& plan,
        const OpenTypeLayoutFeatureListView& features,
        const OpenTypeLayoutLookupListView& lookups,
        OpenTypeShapingBuffer& buffer)
    {
        if (plan.empty())
            return true;

        std::vector<uint8_t> selectedLookups;

        if (!buildOpenTypeLookupSelection(plan, features, lookups, selectedLookups))
            return false;


        // Work on a copy. If any selected lookup fails, leave the caller's
        // buffer unchanged.

        OpenTypeShapingBuffer working = buffer;


        // LookupList order is the execution order.

        for (uint16_t lookupIndex = 0; lookupIndex < lookups.size(); ++lookupIndex)
        {
            if (!selectedLookups[lookupIndex])
                continue;

            const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

            if (!lookup)
                return false;

            if (!applyOpenTypeGsubLookup(lookup, working))
                return false;
        }


        buffer = std::move(working);

        return true;
    }


    // ====================================================================
    // Complete-layout convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGsubFeaturePlan(const OpenTypeFeaturePlan& plan,
        const OpenTypeLayoutView& layout, OpenTypeShapingBuffer& buffer)
    {
        if (!layout)
            return false;

        if (plan.empty())
            return true;

        const OpenTypeLayoutFeatureListView features = layout.features();
        const OpenTypeLayoutLookupListView lookups = layout.lookups();

        if (!features || !lookups)
            return false;

        return applyOpenTypeGsubFeaturePlan(plan, features, lookups, buffer);
    }

} // namespace waavs