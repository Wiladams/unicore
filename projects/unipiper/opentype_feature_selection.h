// opentype_feature_selection.h

#pragma once

#include "opentype_feature_plan.h"



namespace waavs
{
    static inline bool openTypeFeaturePlanContainsIndex(const OpenTypeFeaturePlan& plan, OpenTypeFeatureIndex index) noexcept
    {
        if (plan.hasRequiredFeature() && plan.requiredFeature().featureIndex == index)
            return true;

        for (size_t i = 0; i < plan.size(); ++i)
        {
            if (plan[i].featureIndex == index)
                return true;
        }

        return false;
    }


    [[nodiscard]]
    static inline bool selectOpenTypeFeatures(const OpenTypeLangSysView& langSys, const OpenTypeFeatureDirectoryEntry* directory, uint16_t directoryCount, const Tag* desiredTags, size_t desiredTagCount, OpenTypeFeaturePlan& output)
    {
        output.clear();

        if (directoryCount != 0 && !directory)
            return false;

        if (langSys.featureCount != 0 && !langSys.featureIndices)
            return false;

        if (desiredTagCount != 0 && !desiredTags)
            return false;


        // Validate all references before modifying the output plan.

        if (langSys.requiredFeatureIndex != kOpenTypeFeatureIndexInvalid &&
            langSys.requiredFeatureIndex >= directoryCount)
        {
            return false;
        }

        for (uint16_t i = 0; i < langSys.featureCount; ++i)
        {
            if (langSys.featureIndices[i] >= directoryCount)
                return false;
        }


        // Required feature.

        if (langSys.requiredFeatureIndex != kOpenTypeFeatureIndexInvalid)
        {
            output.setRequiredFeature(
                directory[langSys.requiredFeatureIndex].tag,
                langSys.requiredFeatureIndex);
        }


        // Optional features in policy order.

        for (size_t desiredIndex = 0; desiredIndex < desiredTagCount; ++desiredIndex)
        {
            const Tag desiredTag = desiredTags[desiredIndex];

            for (uint16_t i = 0; i < langSys.featureCount; ++i)
            {
                const OpenTypeFeatureIndex featureIndex = langSys.featureIndices[i];

                if (directory[featureIndex].tag != desiredTag)
                    continue;

                if (openTypeFeaturePlanContainsIndex(output, featureIndex))
                    continue;

                output.addFeature(desiredTag, featureIndex);
            }
        }

        return true;
    }


    [[nodiscard]]
    static inline bool selectGenericOpenTypeGsubFeatures(const OpenTypeLangSysView& langSys, const OpenTypeFeatureDirectoryEntry* directory, uint16_t directoryCount, OpenTypeFeaturePlan& output)
    {
        static constexpr Tag tags[] = {
            OTAG("ccmp"),
            OTAG("locl"),
            OTAG("rlig"),
            OTAG("liga"),
            OTAG("clig"),
            OTAG("calt")
        };

        return selectOpenTypeFeatures(langSys, directory, directoryCount, tags, sizeof(tags) / sizeof(tags[0]), output);
    }

} // namespace waavs