// opentype_layout_view.h

#pragma once

#include "opentype_bytestream.h"

#include <cstddef>
#include <cstdint>


namespace waavs
{
    // ====================================================================
// OpenTypeLayoutFeatureView
//
// Non-owning lazy view of:
//
//   Offset16 featureParamsOffset
//   uint16   lookupIndexCount
//   uint16   lookupIndices[lookupIndexCount]
//
// lookupIndices refer into the layout table's LookupList.
// ====================================================================

    class OpenTypeLayoutFeatureView
    {
    public:
        OpenTypeLayoutFeatureView() noexcept = default;
        explicit OpenTypeLayoutFeatureView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t featureParamsOffset = 0;
            uint16_t lookupCount = 0;
            return readHeader(featureParamsOffset, lookupCount);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t featureParamsOffset() const noexcept
        {
            uint16_t featureParamsOffset = 0;
            uint16_t lookupCount = 0;

            if (!readHeader(featureParamsOffset, lookupCount))
                return 0;

            return featureParamsOffset;
        }

        [[nodiscard]] uint16_t lookupCount() const noexcept
        {
            uint16_t featureParamsOffset = 0;
            uint16_t lookupCount = 0;

            if (!readHeader(featureParamsOffset, lookupCount))
                return 0;

            return lookupCount;
        }

        [[nodiscard]] bool lookupIndex(size_t index, uint16_t& result) const noexcept
        {
            uint16_t featureParamsOffset = 0;
            uint16_t lookupCount = 0;

            if (!readHeader(featureParamsOffset, lookupCount) || index >= lookupCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(4 + index * 2))
                return false;

            return stream.readUInt16(result);
        }

    private:
        bool readHeader(uint16_t& featureParamsOffset, uint16_t& lookupCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readOffset16(featureParamsOffset))
                return false;

            if (!stream.readUInt16(lookupCount))
                return false;

            if (lookupCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeLayoutFeatureListView
    //
    // Non-owning lazy view of:
    //
    //   uint16        featureCount
    //   FeatureRecord featureRecords[featureCount]
    //
    // FeatureRecord:
    //
    //   Tag      featureTag
    //   Offset16 featureOffset
    //
    // featureOffset is relative to FeatureList.
    // ====================================================================

    class OpenTypeLayoutFeatureListView
    {
    public:
        OpenTypeLayoutFeatureListView() noexcept = default;
        explicit OpenTypeLayoutFeatureListView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count) ? count : 0;
        }

        [[nodiscard]] bool featureTag(size_t index, Tag& result) const noexcept
        {
            uint16_t offset = 0;
            return readFeatureRecord(index, result, offset);
        }

        [[nodiscard]] bool contains(Tag featureTag) const noexcept
        {
            const uint16_t count = size();

            for (uint16_t i = 0; i < count; ++i)
            {
                Tag tag = 0;
                uint16_t offset = 0;

                if (!readFeatureRecord(i, tag, offset))
                    return false;

                if (tag == featureTag)
                    return true;
            }

            return false;
        }

        [[nodiscard]] OpenTypeLayoutFeatureView feature(size_t index) const noexcept
        {
            Tag tag = 0;
            uint16_t offset = 0;

            if (!readFeatureRecord(index, tag, offset) || offset == 0)
                return {};

            OpenTypeByteStream stream(fData);
            auto feature = stream.subStream(offset);

            if (!feature.isValid())
                return {};

            return OpenTypeLayoutFeatureView(feature.remainingData());
        }

        [[nodiscard]] OpenTypeLayoutFeatureView find(Tag featureTag) const noexcept
        {
            const uint16_t count = size();

            for (uint16_t i = 0; i < count; ++i)
            {
                Tag tag = 0;
                uint16_t offset = 0;

                if (!readFeatureRecord(i, tag, offset))
                    return {};

                if (tag != featureTag)
                    continue;

                if (offset == 0)
                    return {};

                OpenTypeByteStream stream(fData);
                auto feature = stream.subStream(offset);

                if (!feature.isValid())
                    return {};

                return OpenTypeLayoutFeatureView(feature.remainingData());
            }

            return {};
        }

    private:
        bool readHeader(uint16_t& featureCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(featureCount))
                return false;

            if (featureCount > stream.remaining() / 6)
                return false;

            return true;
        }

        bool readFeatureRecord(size_t index, Tag& tag, uint16_t& offset) const noexcept
        {
            uint16_t featureCount = 0;

            if (!readHeader(featureCount) || index >= featureCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(2 + index * 6))
                return false;

            if (!stream.readUInt32(tag))
                return false;

            return stream.readOffset16(offset);
        }

    private:
        ByteSpan fData{};
    };

    //namespace opentype
    //{
        static constexpr uint16_t kOpenTypeNoRequiredFeature = 0xFFFFu;


        // ====================================================================
        // OpenTypeLayoutLangSysView
        //
        // Non-owning lazy view of:
        //
        //   Offset16 lookupOrder
        //   uint16   requiredFeatureIndex
        //   uint16   featureIndexCount
        //   uint16   featureIndices[featureIndexCount]
        // ====================================================================

        class OpenTypeLayoutLangSysView
        {
        public:
            OpenTypeLayoutLangSysView() noexcept = default;
            explicit OpenTypeLayoutLangSysView(ByteSpan data) noexcept : fData(data) {}

            [[nodiscard]] bool isValid() const noexcept
            {
                uint16_t requiredFeatureIndex = 0;
                uint16_t featureCount = 0;
                return readHeader(requiredFeatureIndex, featureCount);
            }

            explicit operator bool() const noexcept { return isValid(); }

            [[nodiscard]] uint16_t requiredFeatureIndex() const noexcept
            {
                uint16_t requiredFeatureIndex = kOpenTypeNoRequiredFeature;
                uint16_t featureCount = 0;

                if (!readHeader(requiredFeatureIndex, featureCount))
                    return kOpenTypeNoRequiredFeature;

                return requiredFeatureIndex;
            }

            [[nodiscard]] uint16_t featureCount() const noexcept
            {
                uint16_t requiredFeatureIndex = 0;
                uint16_t featureCount = 0;

                if (!readHeader(requiredFeatureIndex, featureCount))
                    return 0;

                return featureCount;
            }

            [[nodiscard]] bool featureIndex(size_t index, uint16_t& result) const noexcept
            {
                uint16_t requiredFeatureIndex = 0;
                uint16_t featureCount = 0;

                if (!readHeader(requiredFeatureIndex, featureCount) || index >= featureCount)
                    return false;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(6 + index * 2))
                    return false;

                return stream.readUInt16(result);
            }

        private:
            bool readHeader(uint16_t& requiredFeatureIndex, uint16_t& featureCount) const noexcept
            {
                OpenTypeByteStream stream(fData);

                uint16_t lookupOrder = 0;

                if (!stream.readOffset16(lookupOrder))
                    return false;

                if (!stream.readUInt16(requiredFeatureIndex))
                    return false;

                if (!stream.readUInt16(featureCount))
                    return false;

                if (lookupOrder != 0)
                    return false;

                if (featureCount > stream.remaining() / 2)
                    return false;

                return true;
            }

        private:
            ByteSpan fData{};
        };


        // ====================================================================
        // OpenTypeLayoutScriptView
        //
        // Non-owning lazy view of:
        //
        //   Offset16      defaultLangSysOffset
        //   uint16        langSysCount
        //   LangSysRecord langSysRecords[langSysCount]
        // ====================================================================

        class OpenTypeLayoutScriptView
        {
        public:
            OpenTypeLayoutScriptView() noexcept = default;
            explicit OpenTypeLayoutScriptView(ByteSpan data) noexcept : fData(data) {}

            [[nodiscard]] bool isValid() const noexcept
            {
                uint16_t defaultLangSysOffset = 0;
                uint16_t languageCount = 0;
                return readHeader(defaultLangSysOffset, languageCount);
            }

            explicit operator bool() const noexcept { return isValid(); }

            [[nodiscard]] bool hasDefaultLangSys() const noexcept
            {
                uint16_t defaultLangSysOffset = 0;
                uint16_t languageCount = 0;

                return readHeader(defaultLangSysOffset, languageCount) && defaultLangSysOffset != 0;
            }

            [[nodiscard]] uint16_t languageCount() const noexcept
            {
                uint16_t defaultLangSysOffset = 0;
                uint16_t languageCount = 0;

                if (!readHeader(defaultLangSysOffset, languageCount))
                    return 0;

                return languageCount;
            }

            [[nodiscard]] OpenTypeLayoutLangSysView defaultLangSys() const noexcept
            {
                uint16_t defaultLangSysOffset = 0;
                uint16_t languageCount = 0;

                if (!readHeader(defaultLangSysOffset, languageCount) || defaultLangSysOffset == 0)
                    return {};

                OpenTypeByteStream stream(fData);
                auto langSys = stream.subStream(defaultLangSysOffset);

                if (!langSys.isValid())
                    return {};

                return OpenTypeLayoutLangSysView(langSys.remainingData());
            }

            [[nodiscard]] bool languageTag(size_t index, Tag& result) const noexcept
            {
                uint16_t offset = 0;
                return readLanguageRecord(index, result, offset);
            }

            [[nodiscard]] OpenTypeLayoutLangSysView language(size_t index) const noexcept
            {
                Tag tag = 0;
                uint16_t offset = 0;

                if (!readLanguageRecord(index, tag, offset) || offset == 0)
                    return {};

                OpenTypeByteStream stream(fData);
                auto langSys = stream.subStream(offset);

                if (!langSys.isValid())
                    return {};

                return OpenTypeLayoutLangSysView(langSys.remainingData());
            }

            [[nodiscard]] OpenTypeLayoutLangSysView findLanguage(Tag languageTag) const noexcept
            {
                const uint16_t count = languageCount();

                for (uint16_t i = 0; i < count; ++i)
                {
                    Tag tag = 0;
                    uint16_t offset = 0;

                    if (!readLanguageRecord(i, tag, offset))
                        return {};

                    if (tag != languageTag)
                        continue;

                    if (offset == 0)
                        return {};

                    OpenTypeByteStream stream(fData);
                    auto langSys = stream.subStream(offset);

                    if (!langSys.isValid())
                        return {};

                    return OpenTypeLayoutLangSysView(langSys.remainingData());
                }

                return {};
            }

        private:
            bool readHeader(uint16_t& defaultLangSysOffset, uint16_t& languageCount) const noexcept
            {
                OpenTypeByteStream stream(fData);

                if (!stream.readOffset16(defaultLangSysOffset))
                    return false;

                if (!stream.readUInt16(languageCount))
                    return false;

                if (languageCount > stream.remaining() / 6)
                    return false;

                return true;
            }

            bool readLanguageRecord(size_t index, Tag& tag, uint16_t& offset) const noexcept
            {
                uint16_t defaultLangSysOffset = 0;
                uint16_t languageCount = 0;

                if (!readHeader(defaultLangSysOffset, languageCount) || index >= languageCount)
                    return false;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(4 + index * 6))
                    return false;

                if (!stream.readUInt32(tag))
                    return false;

                return stream.readOffset16(offset);
            }

        private:
            ByteSpan fData{};
        };


        // ====================================================================
        // OpenTypeLayoutScriptListView
        //
        // Non-owning lazy view of:
        //
        //   uint16       scriptCount
        //   ScriptRecord scriptRecords[scriptCount]
        // ====================================================================

        class OpenTypeLayoutScriptListView
        {
        public:
            OpenTypeLayoutScriptListView() noexcept = default;
            explicit OpenTypeLayoutScriptListView(ByteSpan data) noexcept : fData(data) {}

            [[nodiscard]] bool isValid() const noexcept
            {
                uint16_t count = 0;
                return readHeader(count);
            }

            explicit operator bool() const noexcept { return isValid(); }

            [[nodiscard]] uint16_t size() const noexcept
            {
                uint16_t count = 0;
                return readHeader(count) ? count : 0;
            }

            [[nodiscard]] bool scriptTag(size_t index, Tag& result) const noexcept
            {
                uint16_t offset = 0;
                return readScriptRecord(index, result, offset);
            }

            [[nodiscard]] bool contains(Tag scriptTag) const noexcept
            {
                const uint16_t count = size();

                for (uint16_t i = 0; i < count; ++i)
                {
                    Tag tag = 0;
                    uint16_t offset = 0;

                    if (!readScriptRecord(i, tag, offset))
                        return false;

                    if (tag == scriptTag)
                        return true;
                }

                return false;
            }

            [[nodiscard]] OpenTypeLayoutScriptView script(size_t index) const noexcept
            {
                Tag tag = 0;
                uint16_t offset = 0;

                if (!readScriptRecord(index, tag, offset) || offset == 0)
                    return {};

                OpenTypeByteStream stream(fData);
                auto script = stream.subStream(offset);

                if (!script.isValid())
                    return {};

                return OpenTypeLayoutScriptView(script.remainingData());
            }

            [[nodiscard]] OpenTypeLayoutScriptView find(Tag scriptTag) const noexcept
            {
                const uint16_t count = size();

                for (uint16_t i = 0; i < count; ++i)
                {
                    Tag tag = 0;
                    uint16_t offset = 0;

                    if (!readScriptRecord(i, tag, offset))
                        return {};

                    if (tag != scriptTag)
                        continue;

                    if (offset == 0)
                        return {};

                    OpenTypeByteStream stream(fData);
                    auto script = stream.subStream(offset);

                    if (!script.isValid())
                        return {};

                    return OpenTypeLayoutScriptView(script.remainingData());
                }

                return {};
            }

        private:
            bool readHeader(uint16_t& scriptCount) const noexcept
            {
                OpenTypeByteStream stream(fData);

                if (!stream.readUInt16(scriptCount))
                    return false;

                if (scriptCount > stream.remaining() / 6)
                    return false;

                return true;
            }

            bool readScriptRecord(size_t index, Tag& tag, uint16_t& offset) const noexcept
            {
                uint16_t scriptCount = 0;

                if (!readHeader(scriptCount) || index >= scriptCount)
                    return false;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(2 + index * 6))
                    return false;

                if (!stream.readUInt32(tag))
                    return false;

                return stream.readOffset16(offset);
            }

        private:
            ByteSpan fData{};
        };


        // ====================================================================
// OpenTypeLayoutLookupView
//
// Non-owning lazy view of:
//
//   uint16   lookupType
//   uint16   lookupFlag
//   uint16   subTableCount
//   Offset16 subTableOffsets[subTableCount]
//
// If lookupFlag has UseMarkFilteringSet:
//
//   uint16   markFilteringSet
//
// Subtable offsets are relative to the beginning of the Lookup table.
// ====================================================================

        static constexpr uint16_t kOpenTypeLookupFlagUseMarkFilteringSet = 0x0010u;


        class OpenTypeLayoutLookupView
        {
        public:
            OpenTypeLayoutLookupView() noexcept = default;
            explicit OpenTypeLayoutLookupView(ByteSpan data) noexcept : fData(data) {}

            [[nodiscard]] bool isValid() const noexcept
            {
                uint16_t lookupType = 0;
                uint16_t lookupFlag = 0;
                uint16_t subtableCount = 0;

                return readHeader(lookupType, lookupFlag, subtableCount);
            }

            explicit operator bool() const noexcept { return isValid(); }

            [[nodiscard]] uint16_t lookupType() const noexcept
            {
                uint16_t lookupType = 0;
                uint16_t lookupFlag = 0;
                uint16_t subtableCount = 0;

                if (!readHeader(lookupType, lookupFlag, subtableCount))
                    return 0;

                return lookupType;
            }

            [[nodiscard]] uint16_t lookupFlag() const noexcept
            {
                uint16_t lookupType = 0;
                uint16_t lookupFlag = 0;
                uint16_t subtableCount = 0;

                if (!readHeader(lookupType, lookupFlag, subtableCount))
                    return 0;

                return lookupFlag;
            }

            [[nodiscard]] uint16_t subtableCount() const noexcept
            {
                uint16_t lookupType = 0;
                uint16_t lookupFlag = 0;
                uint16_t subtableCount = 0;

                if (!readHeader(lookupType, lookupFlag, subtableCount))
                    return 0;

                return subtableCount;
            }

            [[nodiscard]] bool usesMarkFilteringSet() const noexcept
            {
                return (lookupFlag() & kOpenTypeLookupFlagUseMarkFilteringSet) != 0;
            }

            [[nodiscard]] bool subtableOffset(size_t index, uint16_t& result) const noexcept
            {
                uint16_t lookupType = 0;
                uint16_t lookupFlag = 0;
                uint16_t subtableCount = 0;

                if (!readHeader(lookupType, lookupFlag, subtableCount) || index >= subtableCount)
                    return false;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(6 + index * 2))
                    return false;

                return stream.readOffset16(result);
            }

            [[nodiscard]] ByteSpan subtable(size_t index) const noexcept
            {
                uint16_t offset = 0;

                if (!subtableOffset(index, offset) || offset == 0)
                    return {};

                OpenTypeByteStream stream(fData);
                auto subtable = stream.subStream(offset);

                if (!subtable.isValid() || subtable.empty())
                    return {};

                return subtable.remainingData();
            }

            [[nodiscard]] bool markFilteringSet(uint16_t& result) const noexcept
            {
                uint16_t lookupType = 0;
                uint16_t lookupFlag = 0;
                uint16_t subtableCount = 0;

                if (!readHeader(lookupType, lookupFlag, subtableCount))
                    return false;

                if ((lookupFlag & kOpenTypeLookupFlagUseMarkFilteringSet) == 0)
                    return false;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(6 + size_t(subtableCount) * 2))
                    return false;

                return stream.readUInt16(result);
            }

        private:
            bool readHeader(uint16_t& lookupType, uint16_t& lookupFlag, uint16_t& subtableCount) const noexcept
            {
                OpenTypeByteStream stream(fData);

                if (!stream.readUInt16(lookupType))
                    return false;

                if (!stream.readUInt16(lookupFlag))
                    return false;

                if (!stream.readUInt16(subtableCount))
                    return false;

                if (subtableCount > stream.remaining() / 2)
                    return false;

                const size_t subtableBytes = size_t(subtableCount) * 2;

                if (!stream.skip(subtableBytes))
                    return false;

                if ((lookupFlag & kOpenTypeLookupFlagUseMarkFilteringSet) != 0 && stream.remaining() < 2)
                    return false;

                return true;
            }

        private:
            ByteSpan fData{};
        };


        // ====================================================================
        // OpenTypeLayoutLookupListView
        //
        // Non-owning lazy view of:
        //
        //   uint16   lookupCount
        //   Offset16 lookupOffsets[lookupCount]
        //
        // lookupOffsets are relative to the beginning of LookupList.
        // ====================================================================

        class OpenTypeLayoutLookupListView
        {
        public:
            OpenTypeLayoutLookupListView() noexcept = default;
            explicit OpenTypeLayoutLookupListView(ByteSpan data) noexcept : fData(data) {}

            [[nodiscard]] bool isValid() const noexcept
            {
                uint16_t count = 0;
                return readHeader(count);
            }

            explicit operator bool() const noexcept { return isValid(); }

            [[nodiscard]] uint16_t size() const noexcept
            {
                uint16_t count = 0;
                return readHeader(count) ? count : 0;
            }

            [[nodiscard]] bool lookupOffset(size_t index, uint16_t& result) const noexcept
            {
                uint16_t count = 0;

                if (!readHeader(count) || index >= count)
                    return false;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(2 + index * 2))
                    return false;

                return stream.readOffset16(result);
            }

            [[nodiscard]] OpenTypeLayoutLookupView lookup(size_t index) const noexcept
            {
                uint16_t offset = 0;

                if (!lookupOffset(index, offset) || offset == 0)
                    return {};

                OpenTypeByteStream stream(fData);
                auto lookup = stream.subStream(offset);

                if (!lookup.isValid() || lookup.empty())
                    return {};

                return OpenTypeLayoutLookupView(lookup.remainingData());
            }

        private:
            bool readHeader(uint16_t& lookupCount) const noexcept
            {
                OpenTypeByteStream stream(fData);

                if (!stream.readUInt16(lookupCount))
                    return false;

                if (lookupCount > stream.remaining() / 2)
                    return false;

                return true;
            }

        private:
            ByteSpan fData{};
        };





        // ====================================================================
        // OpenTypeLayoutView
        //
        // Common top-level view for GSUB and GPOS.
        // ====================================================================

        class OpenTypeLayoutView
        {
        public:
            OpenTypeLayoutView() noexcept = default;
            explicit OpenTypeLayoutView(ByteSpan data) noexcept : fData(data) {}

            [[nodiscard]] bool isValid() const noexcept
            {
                uint16_t scriptListOffset = 0;
                uint16_t featureListOffset = 0;
                uint16_t lookupListOffset = 0;

                return readHeader(scriptListOffset, featureListOffset, lookupListOffset);
            }

            explicit operator bool() const noexcept { return isValid(); }

            [[nodiscard]] OpenTypeLayoutScriptListView scripts() const noexcept
            {
                uint16_t scriptListOffset = 0;
                uint16_t featureListOffset = 0;
                uint16_t lookupListOffset = 0;

                if (!readHeader(scriptListOffset, featureListOffset, lookupListOffset))
                    return {};

                OpenTypeByteStream stream(fData);
                auto scripts = stream.subStream(scriptListOffset);

                if (!scripts.isValid())
                    return {};

                return OpenTypeLayoutScriptListView(scripts.remainingData());
            }

            [[nodiscard]] bool hasScript(Tag scriptTag) const noexcept
            {
                const OpenTypeLayoutScriptListView scriptList = scripts();
                return scriptList && scriptList.contains(scriptTag);
            }

            [[nodiscard]] OpenTypeLayoutScriptView script(Tag scriptTag) const noexcept
            {
                const OpenTypeLayoutScriptListView scriptList = scripts();
                return scriptList ? scriptList.find(scriptTag) : OpenTypeLayoutScriptView{};
            }

            [[nodiscard]] OpenTypeLayoutFeatureListView features() const noexcept
            {
                uint16_t scriptListOffset = 0;
                uint16_t featureListOffset = 0;
                uint16_t lookupListOffset = 0;

                if (!readHeader(scriptListOffset, featureListOffset, lookupListOffset))
                    return {};

                OpenTypeByteStream stream(fData);
                auto features = stream.subStream(featureListOffset);

                if (!features.isValid())
                    return {};

                return OpenTypeLayoutFeatureListView(features.remainingData());
            }

            [[nodiscard]] OpenTypeLayoutLookupListView lookups() const noexcept
            {
                uint16_t scriptListOffset = 0;
                uint16_t featureListOffset = 0;
                uint16_t lookupListOffset = 0;

                if (!readHeader(scriptListOffset, featureListOffset, lookupListOffset))
                    return {};

                OpenTypeByteStream stream(fData);
                auto lookups = stream.subStream(lookupListOffset);

                if (!lookups.isValid())
                    return {};

                return OpenTypeLayoutLookupListView(lookups.remainingData());
            }

        private:
            bool readHeader(uint16_t& scriptListOffset, uint16_t& featureListOffset, uint16_t& lookupListOffset) const noexcept
            {
                OpenTypeByteStream stream(fData);

                uint16_t majorVersion = 0;
                uint16_t minorVersion = 0;

                if (!stream.readUInt16(majorVersion) || !stream.readUInt16(minorVersion))
                    return false;

                if (majorVersion != 1 || minorVersion > 1)
                    return false;

                if (!stream.readOffset16(scriptListOffset) || !stream.readOffset16(featureListOffset) || !stream.readOffset16(lookupListOffset))
                    return false;

                if (minorVersion == 1)
                {
                    uint32_t featureVariationsOffset = 0;

                    if (!stream.readOffset32(featureVariationsOffset))
                        return false;

                    if (featureVariationsOffset != 0 && featureVariationsOffset >= fData.size())
                        return false;
                }

                if (scriptListOffset == 0 || featureListOffset == 0 || lookupListOffset == 0)
                    return false;

                if (scriptListOffset >= fData.size() || featureListOffset >= fData.size() || lookupListOffset >= fData.size())
                    return false;

                return true;
            }

        private:
            ByteSpan fData{};
        };

    //} // namespace opentype


} // namespace waavs
