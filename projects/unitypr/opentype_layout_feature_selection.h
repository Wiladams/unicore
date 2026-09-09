// opentype_layout_feature_selection.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_feature_selection.h"
#include "opentype_layout_view.h"


namespace waavs
{
    // ====================================================================
    // Adapt lazy OpenType layout views to the pure feature-selection layer.
    //
    // Only FeatureRecords referenced by this LangSys are inspected.
    // Feature tables themselves are not dereferenced.
    // ====================================================================

    [[nodiscard]]
    static inline bool selectOpenTypeLayoutFeatures(const OpenTypeLayoutLangSysView& langSys,
        const OpenTypeLayoutFeatureListView& features,
        const Tag* desiredTags,
        size_t desiredTagCount,
        OpenTypeFeaturePlan& output)
    {
        output.clear();

        if (!langSys || !features)
            return false;

        if (desiredTagCount != 0 && !desiredTags)
            return false;


        const uint16_t directoryCount = features.size();
        const uint16_t featureCount = langSys.featureCount();
        const OpenTypeFeatureIndex requiredFeatureIndex = langSys.requiredFeatureIndex();


        // Validate all cross-table references before modifying the plan.

        if (requiredFeatureIndex != kOpenTypeFeatureIndexInvalid &&
            requiredFeatureIndex >= directoryCount)
        {
            return false;
        }


        std::vector<OpenTypeFeatureIndex> featureIndices;
        featureIndices.reserve(featureCount);

        for (uint16_t i = 0; i < featureCount; ++i)
        {
            uint16_t featureIndex = 0;

            if (!langSys.featureIndex(i, featureIndex))
                return false;

            if (featureIndex >= directoryCount)
                return false;

            featureIndices.push_back(featureIndex);
        }


        // The pure selector expects a FeatureList-indexed directory.
        //
        // Allocate the index space, but populate only entries actually
        // referenced by this LangSys.

        std::vector<OpenTypeFeatureDirectoryEntry> directory(directoryCount);


        if (requiredFeatureIndex != kOpenTypeFeatureIndexInvalid)
        {
            Tag tag = 0;

            if (!features.featureTag(requiredFeatureIndex, tag))
                return false;

            directory[requiredFeatureIndex].tag = tag;
        }


        for (OpenTypeFeatureIndex featureIndex : featureIndices)
        {
            Tag tag = 0;

            if (!features.featureTag(featureIndex, tag))
                return false;

            directory[featureIndex].tag = tag;
        }


        OpenTypeLangSysView selectionLangSys{};

        selectionLangSys.featureIndices = featureIndices.data();
        selectionLangSys.featureCount = static_cast<uint16_t>(featureIndices.size());
        selectionLangSys.requiredFeatureIndex = requiredFeatureIndex;


        return selectOpenTypeFeatures(
            selectionLangSys,
            directory.data(),
            directoryCount,
            desiredTags,
            desiredTagCount,
            output);
    }


    // ====================================================================
    // Generic GSUB feature policy from real layout views.
    // ====================================================================

    [[nodiscard]]
    static inline bool selectGenericOpenTypeGsubFeatures(const OpenTypeLayoutLangSysView& langSys,
        const OpenTypeLayoutFeatureListView& features,
        OpenTypeFeaturePlan& output)
    {
        static constexpr Tag tags[] = {
            OTAG("ccmp"),
            OTAG("locl"),
            OTAG("rlig"),
            OTAG("liga"),
            OTAG("clig"),
            OTAG("calt")
        };

        return selectOpenTypeLayoutFeatures(
            langSys,
            features,
            tags,
            sizeof(tags) / sizeof(tags[0]),
            output);
    }


    // ====================================================================
    // Convenience adapter for a complete layout table.
    //
    // Current policy:
    //
    //   requested script
    //       |
    //       v
    //   DFLT fallback
    //       |
    //       v
    //   DefaultLangSys
    //
    // Named language-system selection comes later.
    // ====================================================================

    [[nodiscard]]
    static inline bool selectGenericOpenTypeGsubFeatures(const OpenTypeLayoutView& layout,
        Tag scriptTag,
        OpenTypeFeaturePlan& output)
    {
        output.clear();

        if (!layout)
            return false;


        OpenTypeLayoutScriptView script = layout.script(scriptTag);

        if (!script)
            script = layout.script(OTAG("DFLT"));


        // No applicable script is not malformed. There are simply no
        // GSUB features to select from this table.

        if (!script)
            return true;


        // Likewise, a Script table is allowed to have no DefaultLangSys.

        if (!script.hasDefaultLangSys())
            return true;


        const OpenTypeLayoutLangSysView langSys = script.defaultLangSys();

        if (!langSys)
            return false;


        const OpenTypeLayoutFeatureListView features = layout.features();

        if (!features)
            return false;


        return selectGenericOpenTypeGsubFeatures(langSys, features, output);
    }

} // namespace waavs
