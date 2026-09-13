// opentype_layout_selection.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_bytestream.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeLayoutSelectionResult
    // ====================================================================

    enum class OpenTypeLayoutSelectionResult : uint8_t
    {
        Invalid = 0,
        NoScript,
        NoLanguageSystem,
        Success
    };


    // ====================================================================
    // OpenTypeLayoutFeatureRequest
    //
    // Feature policy lives above this layer. This request contains only the
    // feature tags that the caller has already decided should participate.
    //
    // languageTag == 0 means use the Script's DefaultLangSys directly.
    // ====================================================================

    struct OpenTypeLayoutFeatureRequest
    {
        uint32_t scriptTag{ 0 };
        uint32_t languageTag{ 0 };

        const uint32_t* featureTags{ nullptr };
        size_t featureTagCount{ 0 };
        
        bool includeRequiredFeature{ true };

        bool fallbackToDefaultScript{ true };
        bool fallbackToDefaultLanguage{ true };

    };


    // ====================================================================
    // OpenTypeLayoutSelectedFeature
    // ====================================================================

    struct OpenTypeLayoutSelectedFeature
    {
        uint32_t tag{ 0 };
        uint16_t featureIndex{ 0 };
        bool required{ false };
    };


    // ====================================================================
    // OpenTypeLayoutLookupPlan
    //
    // lookupIndices are always:
    //
    //   unique
    //   sorted by LookupList index
    //
    // This is the OpenType processing order for the selected feature stage.
    //
    // lookupListData is the original LookupList ByteSpan. The GSUB/GPOS
    // orchestrator can construct OpenTypeLayoutLookupListView directly from
    // this span.
    // ====================================================================

    struct OpenTypeLayoutLookupPlan
    {
        uint32_t scriptTag{ 0 };
        uint32_t languageTag{ 0 };

        bool usedDefaultScript{ false };
        bool usedDefaultLanguage{ false };

        ByteSpan lookupListData{};

        std::vector<OpenTypeLayoutSelectedFeature> features{};
        std::vector<uint16_t> lookupIndices{};

        void clear() noexcept
        {
            scriptTag = 0;
            languageTag = 0;

            usedDefaultScript = false;
            usedDefaultLanguage = false;

            lookupListData = {};

            features.clear();
            lookupIndices.clear();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return lookupIndices.empty();
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return lookupIndices.size();
        }
    };


    // ====================================================================
    // Internal table decomposition.
    // ====================================================================

    struct OpenTypeLayoutSelectionParts
    {
        ByteSpan scriptList{};
        ByteSpan featureList{};
        ByteSpan lookupList{};

        uint16_t featureCount{ 0 };
        uint16_t lookupCount{ 0 };
    };


    enum class OpenTypeLayoutFindResult : uint8_t
    {
        Invalid = 0,
        NotFound,
        Found
    };


    // ====================================================================
    // Small big-endian random-access readers.
    // ====================================================================

    static inline bool openTypeLayoutSelectionReadU16(
        ByteSpan data, size_t offset, uint16_t& value) noexcept
    {
        value = 0;

        if (offset > data.size() ||
            data.size() - offset < 2)
        {
            return false;
        }

        OpenTypeByteStream stream(data.subSpan(offset, 2));
        return stream.readUInt16(value);
    }


    static inline bool openTypeLayoutSelectionReadU32(
        ByteSpan data, size_t offset, uint32_t& value) noexcept
    {
        value = 0;

        if (offset > data.size() ||
            data.size() - offset < 4)
        {
            return false;
        }

        OpenTypeByteStream stream(data.subSpan(offset, 4));
        return stream.readUInt32(value);
    }


    // ====================================================================
    // openTypeLayoutSelectionParts
    //
    // Split a GSUB or GPOS table into its three common layout tables.
    //
    // FeatureVariations in version 1.1 is validated as an offset but is not
    // applied here. Variable-feature substitution belongs to a later layer.
    // ====================================================================

    static inline bool openTypeLayoutSelectionParts(
        ByteSpan tableData, OpenTypeLayoutSelectionParts& result) noexcept
    {
        result = {};

        OpenTypeByteStream stream(tableData);

        uint16_t majorVersion = 0;
        uint16_t minorVersion = 0;

        uint16_t scriptListOffset = 0;
        uint16_t featureListOffset = 0;
        uint16_t lookupListOffset = 0;

        if (!stream.readUInt16(majorVersion) ||
            !stream.readUInt16(minorVersion) ||
            !stream.readOffset16(scriptListOffset) ||
            !stream.readOffset16(featureListOffset) ||
            !stream.readOffset16(lookupListOffset))
        {
            return false;
        }

        if (majorVersion != 1 || minorVersion > 1)
            return false;

        if (minorVersion == 1)
        {
            uint32_t featureVariationsOffset = 0;

            if (!stream.readOffset32(featureVariationsOffset))
                return false;

            if (featureVariationsOffset != 0 &&
                featureVariationsOffset >= tableData.size())
            {
                return false;
            }
        }

        if (scriptListOffset == 0 ||
            featureListOffset == 0 ||
            lookupListOffset == 0 ||
            scriptListOffset >= tableData.size() ||
            featureListOffset >= tableData.size() ||
            lookupListOffset >= tableData.size())
        {
            return false;
        }

        result.scriptList = tableData.subSpan(scriptListOffset);
        result.featureList = tableData.subSpan(featureListOffset);
        result.lookupList = tableData.subSpan(lookupListOffset);

        if (!result.scriptList ||
            !result.featureList ||
            !result.lookupList)
        {
            return false;
        }


        // FeatureList.

        if (!openTypeLayoutSelectionReadU16(
            result.featureList, 0, result.featureCount))
        {
            return false;
        }

        if (size_t(result.featureCount) >
            (result.featureList.size() - 2) / 6)
        {
            return false;
        }


        // LookupList.

        if (!openTypeLayoutSelectionReadU16(
            result.lookupList, 0, result.lookupCount))
        {
            return false;
        }

        if (size_t(result.lookupCount) >
            (result.lookupList.size() - 2) / 2)
        {
            return false;
        }

        return true;
    }


    // ====================================================================
    // Script lookup.
    // ====================================================================

    static inline OpenTypeLayoutFindResult openTypeLayoutFindScript(
        ByteSpan scriptList, uint32_t scriptTag,
        ByteSpan& script) noexcept
    {
        script = {};

        uint16_t scriptCount = 0;

        if (!openTypeLayoutSelectionReadU16(
            scriptList, 0, scriptCount))
        {
            return OpenTypeLayoutFindResult::Invalid;
        }

        if (size_t(scriptCount) >
            (scriptList.size() - 2) / 6)
        {
            return OpenTypeLayoutFindResult::Invalid;
        }

        for (uint16_t i = 0; i < scriptCount; ++i)
        {
            const size_t recordOffset =
                2 + size_t(i) * 6;

            uint32_t tag = 0;
            uint16_t offset = 0;

            if (!openTypeLayoutSelectionReadU32(
                scriptList, recordOffset, tag) ||
                !openTypeLayoutSelectionReadU16(
                    scriptList, recordOffset + 4, offset))
            {
                return OpenTypeLayoutFindResult::Invalid;
            }

            if (offset == 0 ||
                offset >= scriptList.size())
            {
                return OpenTypeLayoutFindResult::Invalid;
            }

            if (tag != scriptTag)
                continue;

            script = scriptList.subSpan(offset);

            if (script.size() < 4)
                return OpenTypeLayoutFindResult::Invalid;

            return OpenTypeLayoutFindResult::Found;
        }

        return OpenTypeLayoutFindResult::NotFound;
    }


    // ====================================================================
    // Language-system lookup.
    // ====================================================================

    static inline OpenTypeLayoutFindResult openTypeLayoutFindDefaultLangSys(
        ByteSpan script, ByteSpan& langSys) noexcept
    {
        langSys = {};

        uint16_t defaultOffset = 0;
        uint16_t langSysCount = 0;

        if (!openTypeLayoutSelectionReadU16(
            script, 0, defaultOffset) ||
            !openTypeLayoutSelectionReadU16(
                script, 2, langSysCount))
        {
            return OpenTypeLayoutFindResult::Invalid;
        }

        if (size_t(langSysCount) >
            (script.size() - 4) / 6)
        {
            return OpenTypeLayoutFindResult::Invalid;
        }

        if (defaultOffset == 0)
            return OpenTypeLayoutFindResult::NotFound;

        if (defaultOffset >= script.size())
            return OpenTypeLayoutFindResult::Invalid;

        langSys = script.subSpan(defaultOffset);

        if (langSys.size() < 6)
            return OpenTypeLayoutFindResult::Invalid;

        return OpenTypeLayoutFindResult::Found;
    }


    static inline OpenTypeLayoutFindResult openTypeLayoutFindLangSys(
        ByteSpan script, uint32_t languageTag,
        ByteSpan& langSys) noexcept
    {
        langSys = {};

        uint16_t defaultOffset = 0;
        uint16_t langSysCount = 0;

        if (!openTypeLayoutSelectionReadU16(
            script, 0, defaultOffset) ||
            !openTypeLayoutSelectionReadU16(
                script, 2, langSysCount))
        {
            return OpenTypeLayoutFindResult::Invalid;
        }

        if (size_t(langSysCount) >
            (script.size() - 4) / 6)
        {
            return OpenTypeLayoutFindResult::Invalid;
        }

        for (uint16_t i = 0; i < langSysCount; ++i)
        {
            const size_t recordOffset =
                4 + size_t(i) * 6;

            uint32_t tag = 0;
            uint16_t offset = 0;

            if (!openTypeLayoutSelectionReadU32(
                script, recordOffset, tag) ||
                !openTypeLayoutSelectionReadU16(
                    script, recordOffset + 4, offset))
            {
                return OpenTypeLayoutFindResult::Invalid;
            }

            if (offset == 0 ||
                offset >= script.size())
            {
                return OpenTypeLayoutFindResult::Invalid;
            }

            if (tag != languageTag)
                continue;

            langSys = script.subSpan(offset);

            if (langSys.size() < 6)
                return OpenTypeLayoutFindResult::Invalid;

            return OpenTypeLayoutFindResult::Found;
        }

        return OpenTypeLayoutFindResult::NotFound;
    }


    // ====================================================================
    // LangSys decoding.
    // ====================================================================

    static inline bool openTypeLayoutReadLangSys(
        ByteSpan langSys, uint16_t& requiredFeatureIndex,
        std::vector<uint16_t>& optionalFeatureIndices) noexcept
    {
        requiredFeatureIndex = 0xFFFFu;
        optionalFeatureIndices.clear();

        OpenTypeByteStream stream(langSys);

        uint16_t lookupOrderOffset = 0;
        uint16_t featureCount = 0;

        if (!stream.readOffset16(lookupOrderOffset) ||
            !stream.readUInt16(requiredFeatureIndex) ||
            !stream.readUInt16(featureCount))
        {
            return false;
        }

        // Reserved by OpenType and required to be NULL.

        if (lookupOrderOffset != 0)
            return false;

        if (size_t(featureCount) >
            stream.remaining() / 2)
        {
            return false;
        }

        optionalFeatureIndices.reserve(featureCount);

        for (uint16_t i = 0; i < featureCount; ++i)
        {
            uint16_t featureIndex = 0;

            if (!stream.readUInt16(featureIndex))
                return false;

            optionalFeatureIndices.push_back(featureIndex);
        }

        return true;
    }


    // ====================================================================
    // FeatureList access.
    // ====================================================================

    static inline bool openTypeLayoutFeatureTag(
        ByteSpan featureList, uint16_t featureCount,
        uint16_t featureIndex, uint32_t& tag) noexcept
    {
        tag = 0;

        if (featureIndex >= featureCount)
            return false;

        const size_t recordOffset =
            2 + size_t(featureIndex) * 6;

        return openTypeLayoutSelectionReadU32(
            featureList, recordOffset, tag);
    }


    static inline bool openTypeLayoutFeatureTable(
        ByteSpan featureList, uint16_t featureCount,
        uint16_t featureIndex, ByteSpan& feature) noexcept
    {
        feature = {};

        if (featureIndex >= featureCount)
            return false;

        const size_t recordOffset =
            2 + size_t(featureIndex) * 6;

        uint16_t offset = 0;

        if (!openTypeLayoutSelectionReadU16(
            featureList, recordOffset + 4, offset))
        {
            return false;
        }

        if (offset == 0 ||
            offset >= featureList.size())
        {
            return false;
        }

        feature = featureList.subSpan(offset);

        return feature.size() >= 4;
    }


    // ====================================================================
    // Selected-feature helpers.
    // ====================================================================

    static inline bool openTypeLayoutPlanHasFeature(
        const OpenTypeLayoutLookupPlan& plan,
        uint16_t featureIndex) noexcept
    {
        for (const OpenTypeLayoutSelectedFeature& feature : plan.features)
        {
            if (feature.featureIndex == featureIndex)
                return true;
        }

        return false;
    }


    static inline bool openTypeLayoutAddSelectedFeature(
        const OpenTypeLayoutSelectionParts& parts,
        OpenTypeLayoutLookupPlan& plan,
        uint16_t featureIndex, bool required) noexcept
    {
        if (featureIndex >= parts.featureCount)
            return false;

        if (openTypeLayoutPlanHasFeature(
            plan, featureIndex))
        {
            return true;
        }

        uint32_t tag = 0;

        if (!openTypeLayoutFeatureTag(
            parts.featureList, parts.featureCount,
            featureIndex, tag))
        {
            return false;
        }

        ByteSpan featureTable;

        if (!openTypeLayoutFeatureTable(
            parts.featureList, parts.featureCount,
            featureIndex, featureTable))
        {
            return false;
        }

        plan.features.push_back({
            tag,
            featureIndex,
            required
            });

        return true;
    }


    // ====================================================================
    // Mark lookup indices referenced by one selected Feature table.
    // ====================================================================

    static inline bool openTypeLayoutMarkFeatureLookups(
        const OpenTypeLayoutSelectionParts& parts,
        uint16_t featureIndex,
        std::vector<uint8_t>& selectedLookups) noexcept
    {
        ByteSpan featureData;

        if (!openTypeLayoutFeatureTable(
            parts.featureList, parts.featureCount,
            featureIndex, featureData))
        {
            return false;
        }

        OpenTypeByteStream stream(featureData);

        uint16_t featureParamsOffset = 0;
        uint16_t lookupCount = 0;

        if (!stream.readOffset16(featureParamsOffset) ||
            !stream.readUInt16(lookupCount))
        {
            return false;
        }

        if (featureParamsOffset != 0 &&
            featureParamsOffset >= featureData.size())
        {
            return false;
        }

        if (size_t(lookupCount) >
            stream.remaining() / 2)
        {
            return false;
        }

        for (uint16_t i = 0; i < lookupCount; ++i)
        {
            uint16_t lookupIndex = 0;

            if (!stream.readUInt16(lookupIndex))
                return false;

            if (lookupIndex >= parts.lookupCount)
                return false;

            selectedLookups[lookupIndex] = 1;
        }

        return true;
    }


    // ====================================================================
    // selectOpenTypeLayoutLookups
    //
    // Resolve the common GSUB/GPOS hierarchy:
    //
    //   ScriptList
    //      -> Script
    //          -> LangSys
    //              -> required Feature
    //              -> requested optional Features
    //                  -> LookupList indices
    //
    // The required feature is structurally validated whenever present and is
    // selected when request.includeRequiredFeature is true.
    //
    // Requested feature tags that are not supported by the resolved LangSys
    // are simply ignored. Absence of an optional feature is not malformed.
    //
    // Lookup indices are emitted in LookupList order, not FeatureIndex order.
    // ====================================================================

    static inline OpenTypeLayoutSelectionResult selectOpenTypeLayoutLookups(
        ByteSpan tableData, const OpenTypeLayoutFeatureRequest& request,
        OpenTypeLayoutLookupPlan& plan)
    {
        plan.clear();

        if (request.scriptTag == 0 ||
            (request.featureTagCount != 0 &&
                request.featureTags == nullptr))
        {
            return OpenTypeLayoutSelectionResult::Invalid;
        }

        OpenTypeLayoutSelectionParts parts;

        if (!openTypeLayoutSelectionParts(
            tableData, parts))
        {
            return OpenTypeLayoutSelectionResult::Invalid;
        }


        // ------------------------------------------------------------
        // Resolve Script.
        // ------------------------------------------------------------

        ByteSpan script;

        OpenTypeLayoutFindResult findResult =
            openTypeLayoutFindScript(
                parts.scriptList,
                request.scriptTag,
                script);

        if (findResult == OpenTypeLayoutFindResult::Invalid)
            return OpenTypeLayoutSelectionResult::Invalid;

        uint32_t resolvedScriptTag =
            request.scriptTag;

        bool usedDefaultScript = false;

        if (findResult == OpenTypeLayoutFindResult::NotFound)
        {
            if (!request.fallbackToDefaultScript ||
                request.scriptTag == OTAG("DFLT"))
            {
                return OpenTypeLayoutSelectionResult::NoScript;
            }

            findResult =
                openTypeLayoutFindScript(
                    parts.scriptList,
                    OTAG("DFLT"),
                    script);

            if (findResult == OpenTypeLayoutFindResult::Invalid)
                return OpenTypeLayoutSelectionResult::Invalid;

            if (findResult == OpenTypeLayoutFindResult::NotFound)
                return OpenTypeLayoutSelectionResult::NoScript;

            resolvedScriptTag = OTAG("DFLT");
            usedDefaultScript = true;
        }


        // ------------------------------------------------------------
        // Resolve LangSys.
        // ------------------------------------------------------------

        ByteSpan langSys;

        uint32_t resolvedLanguageTag = 0;
        bool usedDefaultLanguage = false;

        if (request.languageTag != 0)
        {
            findResult =
                openTypeLayoutFindLangSys(
                    script,
                    request.languageTag,
                    langSys);

            if (findResult == OpenTypeLayoutFindResult::Invalid)
                return OpenTypeLayoutSelectionResult::Invalid;

            if (findResult == OpenTypeLayoutFindResult::Found)
            {
                resolvedLanguageTag =
                    request.languageTag;
            }
            else
            {
                if (!request.fallbackToDefaultLanguage)
                {
                    return OpenTypeLayoutSelectionResult::NoLanguageSystem;
                }

                findResult =
                    openTypeLayoutFindDefaultLangSys(
                        script, langSys);

                if (findResult == OpenTypeLayoutFindResult::Invalid)
                    return OpenTypeLayoutSelectionResult::Invalid;

                if (findResult == OpenTypeLayoutFindResult::NotFound)
                {
                    return OpenTypeLayoutSelectionResult::NoLanguageSystem;
                }

                usedDefaultLanguage = true;
            }
        }
        else
        {
            findResult =
                openTypeLayoutFindDefaultLangSys(
                    script, langSys);

            if (findResult == OpenTypeLayoutFindResult::Invalid)
                return OpenTypeLayoutSelectionResult::Invalid;

            if (findResult == OpenTypeLayoutFindResult::NotFound)
            {
                return OpenTypeLayoutSelectionResult::NoLanguageSystem;
            }

            usedDefaultLanguage = true;
        }


        // ------------------------------------------------------------
        // Decode LangSys feature references.
        // ------------------------------------------------------------

        uint16_t requiredFeatureIndex = 0xFFFFu;
        std::vector<uint16_t> optionalFeatureIndices;

        if (!openTypeLayoutReadLangSys(
            langSys,
            requiredFeatureIndex,
            optionalFeatureIndices))
        {
            return OpenTypeLayoutSelectionResult::Invalid;
        }

        if (requiredFeatureIndex != 0xFFFFu &&
            requiredFeatureIndex >= parts.featureCount)
        {
            return OpenTypeLayoutSelectionResult::Invalid;
        }

        for (uint16_t featureIndex :
        optionalFeatureIndices)
        {
            if (featureIndex >= parts.featureCount)
                return OpenTypeLayoutSelectionResult::Invalid;
        }


        // ------------------------------------------------------------
        // Required feature.
        //
        // The LangSys required feature is always structurally validated
        // above, but the caller may suppress its selection for later
        // shaping stages.
        // ------------------------------------------------------------

        if (request.includeRequiredFeature &&
            requiredFeatureIndex != 0xFFFFu)
        {
            if (!openTypeLayoutAddSelectedFeature(
                parts, plan,
                requiredFeatureIndex, true))
            {
                return OpenTypeLayoutSelectionResult::Invalid;
            }
        }


        // ------------------------------------------------------------
        // Requested optional features.
        //
        // LangSys FeatureIndex order is arbitrary. Match by feature tag.
        // Request order is retained only in plan.features for diagnostics.
        // Lookup execution order is normalized below.
        // ------------------------------------------------------------

        for (size_t requestIndex = 0;
            requestIndex < request.featureTagCount;
            ++requestIndex)
        {
            const uint32_t requestedTag =
                request.featureTags[requestIndex];

            for (uint16_t featureIndex :
            optionalFeatureIndices)
            {
                uint32_t featureTag = 0;

                if (!openTypeLayoutFeatureTag(
                    parts.featureList,
                    parts.featureCount,
                    featureIndex,
                    featureTag))
                {
                    return OpenTypeLayoutSelectionResult::Invalid;
                }

                if (featureTag != requestedTag)
                    continue;

                if (!openTypeLayoutAddSelectedFeature(
                    parts, plan,
                    featureIndex, false))
                {
                    return OpenTypeLayoutSelectionResult::Invalid;
                }

                break;
            }
        }


        // ------------------------------------------------------------
        // Union selected feature lookups.
        // ------------------------------------------------------------

        std::vector<uint8_t> selectedLookups(
            parts.lookupCount, 0);

        for (const OpenTypeLayoutSelectedFeature& feature :
            plan.features)
        {
            if (!openTypeLayoutMarkFeatureLookups(
                parts,
                feature.featureIndex,
                selectedLookups))
            {
                return OpenTypeLayoutSelectionResult::Invalid;
            }
        }


        // ------------------------------------------------------------
        // Emit in LookupList order.
        //
        // This simultaneously de-duplicates lookup references shared by
        // multiple selected features.
        // ------------------------------------------------------------

        plan.lookupIndices.reserve(parts.lookupCount);

        for (uint16_t lookupIndex = 0;
            lookupIndex < parts.lookupCount;
            ++lookupIndex)
        {
            if (selectedLookups[lookupIndex])
                plan.lookupIndices.push_back(lookupIndex);
        }


        plan.scriptTag = resolvedScriptTag;
        plan.languageTag = resolvedLanguageTag;

        plan.usedDefaultScript = usedDefaultScript;
        plan.usedDefaultLanguage = usedDefaultLanguage;

        plan.lookupListData = parts.lookupList;

        return OpenTypeLayoutSelectionResult::Success;
    }


    // ====================================================================
    // Convenience overload for std::vector feature tags.
    // ====================================================================

    static inline OpenTypeLayoutSelectionResult selectOpenTypeLayoutLookups(
        ByteSpan tableData, uint32_t scriptTag, uint32_t languageTag,
        const std::vector<uint32_t>& featureTags,
        OpenTypeLayoutLookupPlan& plan,
        bool fallbackToDefaultScript = true,
        bool fallbackToDefaultLanguage = true)
    {
        OpenTypeLayoutFeatureRequest request;
        request.scriptTag = scriptTag;
        request.languageTag = languageTag;
        request.featureTags = featureTags.data();
        request.featureTagCount = featureTags.size();
        request.fallbackToDefaultScript = fallbackToDefaultScript;
        request.fallbackToDefaultLanguage = fallbackToDefaultLanguage;

        return selectOpenTypeLayoutLookups(
            tableData, request, plan);
    }

} // namespace waavs