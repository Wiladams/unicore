// opentype_feature_plan.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_types.h"


namespace waavs
{
    using OpenTypeFeatureIndex = uint16_t;

    static constexpr OpenTypeFeatureIndex kOpenTypeFeatureIndexInvalid = 0xFFFFu;


    struct OpenTypeFeatureDirectoryEntry
    {
        Tag tag{ 0 };
    };


    struct OpenTypeLangSysView
    {
        const OpenTypeFeatureIndex* featureIndices{ nullptr };
        uint16_t featureCount{ 0 };
        OpenTypeFeatureIndex requiredFeatureIndex{ kOpenTypeFeatureIndexInvalid };
    };


    struct OpenTypeSelectedFeature
    {
        Tag tag{ 0 };
        OpenTypeFeatureIndex featureIndex{ kOpenTypeFeatureIndexInvalid };
    };


    class OpenTypeFeaturePlan
    {
    public:
        void clear() noexcept
        {
            mRequiredFeature = {};
            mFeatures.clear();
        }

        [[nodiscard]] bool hasRequiredFeature() const noexcept { return mRequiredFeature.featureIndex != kOpenTypeFeatureIndexInvalid; }
        [[nodiscard]] const OpenTypeSelectedFeature& requiredFeature() const noexcept { return mRequiredFeature; }

        [[nodiscard]] size_t size() const noexcept { return mFeatures.size(); }
        [[nodiscard]] bool empty() const noexcept { return mFeatures.empty() && !hasRequiredFeature(); }

        [[nodiscard]] const OpenTypeSelectedFeature& operator[](size_t index) const noexcept { return mFeatures[index]; }

        void setRequiredFeature(Tag tag, OpenTypeFeatureIndex index) noexcept
        {
            mRequiredFeature.tag = tag;
            mRequiredFeature.featureIndex = index;
        }

        void addFeature(Tag tag, OpenTypeFeatureIndex index)
        {
            mFeatures.push_back(OpenTypeSelectedFeature{ tag, index });
        }

    private:
        OpenTypeSelectedFeature mRequiredFeature{};
        std::vector<OpenTypeSelectedFeature> mFeatures{};
    };

} // namespace waavs