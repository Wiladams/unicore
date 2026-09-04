// opentype_gsub_context_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"
#include "opentype_classdef_view.h"
#include "opentype_sequence_lookup.h"

namespace waavs
{

    // ====================================================================
    // Internal UInt16 reader.
    // ====================================================================

    static inline bool openTypeGsubContextReadUInt16(
        const ByteSpan& data, size_t offset, uint16_t& result) noexcept
    {
        result = 0;

        if (offset > data.size() || data.size() - offset < 2)
            return false;

        const uint8_t* p = data.begin() + offset;
        result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
        return true;
    }


    // ====================================================================
    // OpenTypeGsubContextRuleView
    //
    // SequenceRule table used by ContextSubst Format 1.
    //
    // uint16 glyphCount
    // uint16 seqLookupCount
    // uint16 inputSequence[glyphCount - 1]
    // SequenceLookupRecord seqLookupRecords[seqLookupCount]
    //
    // The first glyph is supplied by the parent Coverage table.
    // inputGlyphId(0) therefore describes sequence position 1.
    // ====================================================================

    class OpenTypeGsubContextRuleView
    {
    public:
        OpenTypeGsubContextRuleView() noexcept = default;
        explicit OpenTypeGsubContextRuleView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!readHeader(glyphCount, seqLookupCount))
                return false;

            if (glyphCount == 0)
                return false;

            const size_t inputCount = size_t(glyphCount) - 1;
            const size_t inputBytes = inputCount * 2;

            if (inputBytes > fData.size() - 4)
                return false;

            const size_t recordOffset = 4 + inputBytes;
            const size_t recordBytes = size_t(seqLookupCount) * 4;

            return recordBytes <= fData.size() - recordOffset;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;
            return readHeader(glyphCount, seqLookupCount) ? glyphCount : 0;
        }

        [[nodiscard]] uint16_t sequenceLookupCount() const noexcept
        {
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;
            return readHeader(glyphCount, seqLookupCount) ? seqLookupCount : 0;
        }

        [[nodiscard]] bool inputGlyphId(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!readHeader(glyphCount, seqLookupCount) || glyphCount == 0)
                return false;

            if (index >= glyphCount - 1)
                return false;

            return openTypeGsubContextReadUInt16(fData, 4 + size_t(index) * 2, result);
        }

        [[nodiscard]] bool sequenceLookup(
            uint16_t index, OpenTypeSequenceLookup& result) const noexcept
        {
            result = {};

            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!readHeader(glyphCount, seqLookupCount) || glyphCount == 0)
                return false;

            if (index >= seqLookupCount)
                return false;

            const size_t recordOffset =
                4 + size_t(glyphCount - 1) * 2 + size_t(index) * 4;

            if (!openTypeGsubContextReadUInt16(fData, recordOffset, result.sequenceIndex))
                return false;

            if (!openTypeGsubContextReadUInt16(fData, recordOffset + 2, result.lookupListIndex))
                return false;

            if (result.sequenceIndex >= glyphCount)
                return false;

            return true;
        }

    private:
        [[nodiscard]] bool readHeader(
            uint16_t& glyphCount, uint16_t& seqLookupCount) const noexcept
        {
            glyphCount = 0;
            seqLookupCount = 0;

            return openTypeGsubContextReadUInt16(fData, 0, glyphCount) &&
                openTypeGsubContextReadUInt16(fData, 2, seqLookupCount);
        }

        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubContextRuleSetView
    //
    // SequenceRuleSet table used by ContextSubst Format 1.
    //
    // uint16 seqRuleCount
    // Offset16 seqRuleOffsets[seqRuleCount]
    //
    // Rule order is significant.
    // ====================================================================

    class OpenTypeGsubContextRuleSetView
    {
    public:
        OpenTypeGsubContextRuleSetView() noexcept = default;
        explicit OpenTypeGsubContextRuleSetView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;

            if (!openTypeGsubContextReadUInt16(fData, 0, count))
                return false;

            return size_t(count) * 2 <= fData.size() - 2;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return openTypeGsubContextReadUInt16(fData, 0, count) ? count : 0;
        }

        [[nodiscard]] bool ruleOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            const uint16_t count = size();

            if (!isValid() || index >= count)
                return false;

            return openTypeGsubContextReadUInt16(fData, 2 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubContextRuleView rule(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ruleOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubContextRuleView(fData.subSpan(offset));
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubContextClassRuleView
    //
    // ClassSequenceRule table used by ContextSubst Format 2.
    //
    // uint16 glyphCount
    // uint16 seqLookupCount
    // uint16 inputSequence[glyphCount - 1]
    // SequenceLookupRecord seqLookupRecords[seqLookupCount]
    //
    // inputClass(0) describes the class at sequence position 1.
    // ====================================================================

    class OpenTypeGsubContextClassRuleView
    {
    public:
        OpenTypeGsubContextClassRuleView() noexcept = default;
        explicit OpenTypeGsubContextClassRuleView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!readHeader(glyphCount, seqLookupCount))
                return false;

            if (glyphCount == 0)
                return false;

            const size_t inputCount = size_t(glyphCount) - 1;
            const size_t inputBytes = inputCount * 2;

            if (inputBytes > fData.size() - 4)
                return false;

            const size_t recordOffset = 4 + inputBytes;
            const size_t recordBytes = size_t(seqLookupCount) * 4;

            return recordBytes <= fData.size() - recordOffset;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;
            return readHeader(glyphCount, seqLookupCount) ? glyphCount : 0;
        }

        [[nodiscard]] uint16_t sequenceLookupCount() const noexcept
        {
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;
            return readHeader(glyphCount, seqLookupCount) ? seqLookupCount : 0;
        }

        [[nodiscard]] bool inputClass(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!readHeader(glyphCount, seqLookupCount) || glyphCount == 0)
                return false;

            if (index >= glyphCount - 1)
                return false;

            return openTypeGsubContextReadUInt16(fData, 4 + size_t(index) * 2, result);
        }

        [[nodiscard]] bool sequenceLookup(
            uint16_t index, OpenTypeSequenceLookup& result) const noexcept
        {
            result = {};

            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!readHeader(glyphCount, seqLookupCount) || glyphCount == 0)
                return false;

            if (index >= seqLookupCount)
                return false;

            const size_t recordOffset =
                4 + size_t(glyphCount - 1) * 2 + size_t(index) * 4;

            if (!openTypeGsubContextReadUInt16(fData, recordOffset, result.sequenceIndex))
                return false;

            if (!openTypeGsubContextReadUInt16(fData, recordOffset + 2, result.lookupListIndex))
                return false;

            if (result.sequenceIndex >= glyphCount)
                return false;

            return true;
        }

    private:
        [[nodiscard]] bool readHeader(
            uint16_t& glyphCount, uint16_t& seqLookupCount) const noexcept
        {
            glyphCount = 0;
            seqLookupCount = 0;

            return openTypeGsubContextReadUInt16(fData, 0, glyphCount) &&
                openTypeGsubContextReadUInt16(fData, 2, seqLookupCount);
        }

        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubContextClassSetView
    //
    // ClassSequenceRuleSet table used by ContextSubst Format 2.
    //
    // uint16 classSeqRuleCount
    // Offset16 classSeqRuleOffsets[classSeqRuleCount]
    //
    // Rule order is significant.
    // ====================================================================

    class OpenTypeGsubContextClassSetView
    {
    public:
        OpenTypeGsubContextClassSetView() noexcept = default;
        explicit OpenTypeGsubContextClassSetView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;

            if (!openTypeGsubContextReadUInt16(fData, 0, count))
                return false;

            return size_t(count) * 2 <= fData.size() - 2;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return openTypeGsubContextReadUInt16(fData, 0, count) ? count : 0;
        }

        [[nodiscard]] bool ruleOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            const uint16_t count = size();

            if (!isValid() || index >= count)
                return false;

            return openTypeGsubContextReadUInt16(fData, 2 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubContextClassRuleView rule(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ruleOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubContextClassRuleView(fData.subSpan(offset));
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubContextSubstView
    //
    // GSUB LookupType 5 ContextSubst.
    //
    // Format 1:
    //
    //   uint16 format
    //   Offset16 coverageOffset
    //   uint16 seqRuleSetCount
    //   Offset16 seqRuleSetOffsets[seqRuleSetCount]
    //
    // Format 2:
    //
    //   uint16 format
    //   Offset16 coverageOffset
    //   Offset16 classDefOffset
    //   uint16 classSeqRuleSetCount
    //   Offset16 classSeqRuleSetOffsets[classSeqRuleSetCount]
    //
    // Format 3:
    //
    //   uint16 format
    //   uint16 glyphCount
    //   uint16 seqLookupCount
    //   Offset16 coverageOffsets[glyphCount]
    //   SequenceLookupRecord seqLookupRecords[seqLookupCount]
    // ====================================================================

    class OpenTypeGsubContextSubstView
    {
    public:
        OpenTypeGsubContextSubstView() noexcept = default;
        explicit OpenTypeGsubContextSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t substFormat = 0;

            if (!openTypeGsubContextReadUInt16(fData, 0, substFormat))
                return false;

            switch (substFormat)
            {
            case 1:
                return isFormat1Valid();

            case 2:
                return isFormat2Valid();

            case 3:
                return isFormat3Valid();

            default:
                return false;
            }
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t result = 0;
            return openTypeGsubContextReadUInt16(fData, 0, result) ? result : 0;
        }


        // ================================================================
        // Formats 1 and 2 - initial Coverage.
        // ================================================================

        [[nodiscard]] bool coverageOffset(uint16_t& result) const noexcept
        {
            result = 0;

            const uint16_t substFormat = format();

            if (substFormat != 1 && substFormat != 2)
                return false;

            return openTypeGsubContextReadUInt16(fData, 2, result);
        }

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            uint16_t offset = 0;

            if (!coverageOffset(offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 1.
        // ================================================================

        [[nodiscard]] uint16_t ruleSetCount() const noexcept
        {
            if (format() != 1)
                return 0;

            uint16_t count = 0;
            return openTypeGsubContextReadUInt16(fData, 4, count) ? count : 0;
        }

        [[nodiscard]] bool ruleSetOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (!isFormat1Valid())
                return false;

            const uint16_t count = ruleSetCount();

            if (index >= count)
                return false;

            return openTypeGsubContextReadUInt16(fData, 6 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubContextRuleSetView ruleSet(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ruleSetOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubContextRuleSetView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 2.
        // ================================================================

        [[nodiscard]] bool classDefOffset(uint16_t& result) const noexcept
        {
            result = 0;

            if (format() != 2)
                return false;

            return openTypeGsubContextReadUInt16(fData, 4, result);
        }

        [[nodiscard]] OpenTypeClassDefView classDef() const noexcept
        {
            uint16_t offset = 0;

            if (!classDefOffset(offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeClassDefView(fData.subSpan(offset));
        }

        [[nodiscard]] uint16_t classSetCount() const noexcept
        {
            if (format() != 2)
                return 0;

            uint16_t count = 0;
            return openTypeGsubContextReadUInt16(fData, 6, count) ? count : 0;
        }

        [[nodiscard]] bool classSetOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (!isFormat2Valid())
                return false;

            const uint16_t count = classSetCount();

            if (index >= count)
                return false;

            return openTypeGsubContextReadUInt16(fData, 8 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubContextClassSetView classSet(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!classSetOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubContextClassSetView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 3.
        // ================================================================

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            if (format() != 3)
                return 0;

            uint16_t count = 0;
            return openTypeGsubContextReadUInt16(fData, 2, count) ? count : 0;
        }

        [[nodiscard]] uint16_t sequenceLookupCount() const noexcept
        {
            if (format() != 3)
                return 0;

            uint16_t count = 0;
            return openTypeGsubContextReadUInt16(fData, 4, count) ? count : 0;
        }

        [[nodiscard]] bool inputCoverageOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (!isFormat3Valid())
                return false;

            const uint16_t count = glyphCount();

            if (index >= count)
                return false;

            return openTypeGsubContextReadUInt16(fData, 6 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeCoverageView inputCoverage(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!inputCoverageOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }

        [[nodiscard]] bool sequenceLookup(
            uint16_t index, OpenTypeSequenceLookup& result) const noexcept
        {
            result = {};

            if (!isFormat3Valid())
                return false;

            const uint16_t inputCount = glyphCount();
            const uint16_t lookupCount = sequenceLookupCount();

            if (index >= lookupCount)
                return false;

            const size_t recordOffset =
                6 + size_t(inputCount) * 2 + size_t(index) * 4;

            if (!openTypeGsubContextReadUInt16(fData, recordOffset, result.sequenceIndex))
                return false;

            if (!openTypeGsubContextReadUInt16(fData, recordOffset + 2, result.lookupListIndex))
                return false;

            if (result.sequenceIndex >= inputCount)
                return false;

            return true;
        }

    private:
        [[nodiscard]] bool isFormat1Valid() const noexcept
        {
            uint16_t substFormat = 0;
            uint16_t coverageOffset = 0;
            uint16_t ruleSetCount = 0;

            if (!openTypeGsubContextReadUInt16(fData, 0, substFormat) ||
                !openTypeGsubContextReadUInt16(fData, 2, coverageOffset) ||
                !openTypeGsubContextReadUInt16(fData, 4, ruleSetCount))
            {
                return false;
            }

            if (substFormat != 1)
                return false;

            return size_t(ruleSetCount) * 2 <= fData.size() - 6;
        }

        [[nodiscard]] bool isFormat2Valid() const noexcept
        {
            uint16_t substFormat = 0;
            uint16_t coverageOffset = 0;
            uint16_t classDefOffset = 0;
            uint16_t classSetCount = 0;

            if (!openTypeGsubContextReadUInt16(fData, 0, substFormat) ||
                !openTypeGsubContextReadUInt16(fData, 2, coverageOffset) ||
                !openTypeGsubContextReadUInt16(fData, 4, classDefOffset) ||
                !openTypeGsubContextReadUInt16(fData, 6, classSetCount))
            {
                return false;
            }

            if (substFormat != 2)
                return false;

            return size_t(classSetCount) * 2 <= fData.size() - 8;
        }

        [[nodiscard]] bool isFormat3Valid() const noexcept
        {
            uint16_t substFormat = 0;
            uint16_t glyphCount = 0;
            uint16_t seqLookupCount = 0;

            if (!openTypeGsubContextReadUInt16(fData, 0, substFormat) ||
                !openTypeGsubContextReadUInt16(fData, 2, glyphCount) ||
                !openTypeGsubContextReadUInt16(fData, 4, seqLookupCount))
            {
                return false;
            }

            if (substFormat != 3 || glyphCount == 0)
                return false;

            const size_t coverageBytes = size_t(glyphCount) * 2;

            if (coverageBytes > fData.size() - 6)
                return false;

            const size_t recordOffset = 6 + coverageBytes;
            const size_t recordBytes = size_t(seqLookupCount) * 4;

            return recordBytes <= fData.size() - recordOffset;
        }

        ByteSpan fData{};
    };

} // namespace waavs