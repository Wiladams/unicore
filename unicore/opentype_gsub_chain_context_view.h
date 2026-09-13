// opentype_gsub_chain_context_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"
#include "opentype_classdef_view.h"
#include "opentype_sequence_lookup.h"

namespace waavs
{
    // ====================================================================
    // Internal helpers.
    // ====================================================================

    static inline bool openTypeGsubChainContextReadUInt16(const ByteSpan& data, size_t offset, uint16_t& result) noexcept
    {
        result = 0;

        if (offset > data.size() || data.size() - offset < 2)
            return false;

        const uint8_t* p = data.begin() + offset;
        result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
        return true;
    }


    static inline bool openTypeGsubChainContextArrayFits(const ByteSpan& data, size_t offset, size_t count, size_t itemSize) noexcept
    {
        if (offset > data.size())
            return false;

        return count <= (data.size() - offset) / itemSize;
    }


    // ====================================================================
    // Variable ChainedSequenceRule geometry.
    //
    // Used by:
    //
    //   Format 1 ChainedSequenceRule
    //   Format 2 ChainedClassSequenceRule
    //
    // The binary geometry is identical; only the interpretation of the
    // uint16 sequence values differs.
    // ====================================================================

    struct OpenTypeGsubChainContextRuleLayout
    {
        uint16_t backtrackGlyphCount{ 0 };
        uint16_t inputGlyphCount{ 0 };
        uint16_t lookaheadGlyphCount{ 0 };
        uint16_t sequenceLookupCount{ 0 };

        size_t backtrackOffset{ 0 };
        size_t inputOffset{ 0 };
        size_t lookaheadOffset{ 0 };
        size_t sequenceLookupOffset{ 0 };
    };


    static inline bool openTypeGsubChainContextReadRuleLayout(const ByteSpan& data, OpenTypeGsubChainContextRuleLayout& result) noexcept
    {
        result = {};

        size_t offset = 0;

        if (!openTypeGsubChainContextReadUInt16(data, offset, result.backtrackGlyphCount))
            return false;

        offset += 2;
        result.backtrackOffset = offset;

        if (!openTypeGsubChainContextArrayFits(data, offset, result.backtrackGlyphCount, 2))
            return false;

        offset += size_t(result.backtrackGlyphCount) * 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.inputGlyphCount))
            return false;

        if (result.inputGlyphCount == 0)
            return false;

        offset += 2;
        result.inputOffset = offset;

        const size_t remainingInputCount = size_t(result.inputGlyphCount) - 1;

        if (!openTypeGsubChainContextArrayFits(data, offset, remainingInputCount, 2))
            return false;

        offset += remainingInputCount * 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.lookaheadGlyphCount))
            return false;

        offset += 2;
        result.lookaheadOffset = offset;

        if (!openTypeGsubChainContextArrayFits(data, offset, result.lookaheadGlyphCount, 2))
            return false;

        offset += size_t(result.lookaheadGlyphCount) * 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.sequenceLookupCount))
            return false;

        offset += 2;
        result.sequenceLookupOffset = offset;

        return openTypeGsubChainContextArrayFits(data, offset, result.sequenceLookupCount, 4);
    }


    // ====================================================================
    // OpenTypeGsubChainContextRuleView
    //
    // ChainedSequenceRule used by Format 1.
    //
    // uint16 backtrackGlyphCount
    // uint16 backtrackSequence[backtrackGlyphCount]
    // uint16 inputGlyphCount
    // uint16 inputSequence[inputGlyphCount - 1]
    // uint16 lookaheadGlyphCount
    // uint16 lookaheadSequence[lookaheadGlyphCount]
    // uint16 seqLookupCount
    // SequenceLookup seqLookupRecords[seqLookupCount]
    //
    // The first input glyph is supplied by the parent Coverage.
    //
    // Backtrack storage is reverse logical order:
    //
    //   backtrackGlyphId(0) is the glyph nearest the current glyph.
    //
    // inputGlyphId(0) describes input sequence position 1.
    // ====================================================================

    class OpenTypeGsubChainContextRuleView
    {
    public:
        OpenTypeGsubChainContextRuleView() noexcept = default;
        explicit OpenTypeGsubChainContextRuleView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout);
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t backtrackGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.backtrackGlyphCount : 0;
        }

        [[nodiscard]] uint16_t inputGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.inputGlyphCount : 0;
        }

        [[nodiscard]] uint16_t lookaheadGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.lookaheadGlyphCount : 0;
        }

        [[nodiscard]] uint16_t sequenceLookupCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.sequenceLookupCount : 0;
        }

        [[nodiscard]] bool backtrackGlyphId(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.backtrackGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.backtrackOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] bool inputGlyphId(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.inputGlyphCount - 1)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.inputOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] bool lookaheadGlyphId(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.lookaheadGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.lookaheadOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] bool sequenceLookup(uint16_t index, OpenTypeSequenceLookup& result) const noexcept
        {
            result = {};

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.sequenceLookupCount)
                return false;

            const size_t offset = layout.sequenceLookupOffset + size_t(index) * 4;

            if (!openTypeGsubChainContextReadUInt16(fData, offset, result.sequenceIndex))
                return false;

            return openTypeGsubChainContextReadUInt16(fData, offset + 2, result.lookupListIndex);
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubChainContextRuleSetView
    //
    // ChainedSequenceRuleSet used by Format 1.
    //
    // uint16 chainedSeqRuleCount
    // Offset16 chainedSeqRuleOffsets[chainedSeqRuleCount]
    //
    // Rule offsets are relative to this RuleSet.
    // Rule order is significant.
    // ====================================================================

    class OpenTypeGsubChainContextRuleSetView
    {
    public:
        OpenTypeGsubChainContextRuleSetView() noexcept = default;
        explicit OpenTypeGsubChainContextRuleSetView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;

            if (!openTypeGsubChainContextReadUInt16(fData, 0, count))
                return false;

            return openTypeGsubChainContextArrayFits(fData, 2, count, 2);
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return openTypeGsubChainContextReadUInt16(fData, 0, count) ? count : 0;
        }

        [[nodiscard]] bool ruleOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            const uint16_t count = size();

            if (!isValid() || index >= count)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 2 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubChainContextRuleView rule(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ruleOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubChainContextRuleView(fData.subSpan(offset));
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubChainContextClassRuleView
    //
    // ChainedClassSequenceRule used by Format 2.
    //
    // Binary geometry is identical to Format 1 ChainedSequenceRule, but
    // the sequence values are class values rather than glyph IDs.
    //
    // backtrackClass(0) is the nearest backtrack class.
    // inputClass(0) describes input sequence position 1.
    // ====================================================================

    class OpenTypeGsubChainContextClassRuleView
    {
    public:
        OpenTypeGsubChainContextClassRuleView() noexcept = default;
        explicit OpenTypeGsubChainContextClassRuleView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout);
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t backtrackGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.backtrackGlyphCount : 0;
        }

        [[nodiscard]] uint16_t inputGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.inputGlyphCount : 0;
        }

        [[nodiscard]] uint16_t lookaheadGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.lookaheadGlyphCount : 0;
        }

        [[nodiscard]] uint16_t sequenceLookupCount() const noexcept
        {
            OpenTypeGsubChainContextRuleLayout layout{};
            return openTypeGsubChainContextReadRuleLayout(fData, layout) ? layout.sequenceLookupCount : 0;
        }

        [[nodiscard]] bool backtrackClass(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.backtrackGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.backtrackOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] bool inputClass(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.inputGlyphCount - 1)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.inputOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] bool lookaheadClass(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.lookaheadGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.lookaheadOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] bool sequenceLookup(uint16_t index, OpenTypeSequenceLookup& result) const noexcept
        {
            result = {};

            OpenTypeGsubChainContextRuleLayout layout{};

            if (!openTypeGsubChainContextReadRuleLayout(fData, layout) || index >= layout.sequenceLookupCount)
                return false;

            const size_t offset = layout.sequenceLookupOffset + size_t(index) * 4;

            if (!openTypeGsubChainContextReadUInt16(fData, offset, result.sequenceIndex))
                return false;

            return openTypeGsubChainContextReadUInt16(fData, offset + 2, result.lookupListIndex);
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubChainContextClassSetView
    //
    // ChainedClassSequenceRuleSet used by Format 2.
    //
    // uint16 chainedClassSeqRuleCount
    // Offset16 chainedClassSeqRuleOffsets[chainedClassSeqRuleCount]
    //
    // Rule offsets are relative to this ClassRuleSet.
    // Rule order is significant.
    // ====================================================================

    class OpenTypeGsubChainContextClassSetView
    {
    public:
        OpenTypeGsubChainContextClassSetView() noexcept = default;
        explicit OpenTypeGsubChainContextClassSetView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;

            if (!openTypeGsubChainContextReadUInt16(fData, 0, count))
                return false;

            return openTypeGsubChainContextArrayFits(fData, 2, count, 2);
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return openTypeGsubChainContextReadUInt16(fData, 0, count) ? count : 0;
        }

        [[nodiscard]] bool ruleOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            const uint16_t count = size();

            if (!isValid() || index >= count)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 2 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubChainContextClassRuleView rule(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ruleOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubChainContextClassRuleView(fData.subSpan(offset));
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // Format 3 variable geometry.
    //
    // uint16 format
    // uint16 backtrackGlyphCount
    // Offset16 backtrackCoverageOffsets[backtrackGlyphCount]
    // uint16 inputGlyphCount
    // Offset16 inputCoverageOffsets[inputGlyphCount]
    // uint16 lookaheadGlyphCount
    // Offset16 lookaheadCoverageOffsets[lookaheadGlyphCount]
    // uint16 seqLookupCount
    // SequenceLookup seqLookupRecords[seqLookupCount]
    // ====================================================================

    struct OpenTypeGsubChainContextFormat3Layout
    {
        uint16_t backtrackGlyphCount{ 0 };
        uint16_t inputGlyphCount{ 0 };
        uint16_t lookaheadGlyphCount{ 0 };
        uint16_t sequenceLookupCount{ 0 };

        size_t backtrackCoverageOffset{ 0 };
        size_t inputCoverageOffset{ 0 };
        size_t lookaheadCoverageOffset{ 0 };
        size_t sequenceLookupOffset{ 0 };
    };


    static inline bool openTypeGsubChainContextReadFormat3Layout(const ByteSpan& data, OpenTypeGsubChainContextFormat3Layout& result) noexcept
    {
        result = {};

        uint16_t format = 0;

        if (!openTypeGsubChainContextReadUInt16(data, 0, format) || format != 3)
            return false;

        size_t offset = 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.backtrackGlyphCount))
            return false;

        offset += 2;
        result.backtrackCoverageOffset = offset;

        if (!openTypeGsubChainContextArrayFits(data, offset, result.backtrackGlyphCount, 2))
            return false;

        offset += size_t(result.backtrackGlyphCount) * 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.inputGlyphCount))
            return false;

        if (result.inputGlyphCount == 0)
            return false;

        offset += 2;
        result.inputCoverageOffset = offset;

        if (!openTypeGsubChainContextArrayFits(data, offset, result.inputGlyphCount, 2))
            return false;

        offset += size_t(result.inputGlyphCount) * 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.lookaheadGlyphCount))
            return false;

        offset += 2;
        result.lookaheadCoverageOffset = offset;

        if (!openTypeGsubChainContextArrayFits(data, offset, result.lookaheadGlyphCount, 2))
            return false;

        offset += size_t(result.lookaheadGlyphCount) * 2;


        if (!openTypeGsubChainContextReadUInt16(data, offset, result.sequenceLookupCount))
            return false;

        offset += 2;
        result.sequenceLookupOffset = offset;

        return openTypeGsubChainContextArrayFits(data, offset, result.sequenceLookupCount, 4);
    }


    // ====================================================================
    // OpenTypeGsubChainContextSubstView
    //
    // GSUB LookupType 6 Chaining Contextual Substitution.
    //
    // Format 1:
    //
    //   uint16 format
    //   Offset16 coverageOffset
    //   uint16 chainedSeqRuleSetCount
    //   Offset16 chainedSeqRuleSetOffsets[chainedSeqRuleSetCount]
    //
    // Format 2:
    //
    //   uint16 format
    //   Offset16 coverageOffset
    //   Offset16 backtrackClassDefOffset
    //   Offset16 inputClassDefOffset
    //   Offset16 lookaheadClassDefOffset
    //   uint16 chainedClassSeqRuleSetCount
    //   Offset16 chainedClassSeqRuleSetOffsets[chainedClassSeqRuleSetCount]
    //
    // Format 3 has variable Coverage-array geometry handled above.
    //
    // Child tables are validated lazily.
    // ====================================================================

    class OpenTypeGsubChainContextSubstView
    {
    public:
        OpenTypeGsubChainContextSubstView() noexcept = default;
        explicit OpenTypeGsubChainContextSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            switch (format())
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
            return openTypeGsubChainContextReadUInt16(fData, 0, result) ? result : 0;
        }


        // ================================================================
        // Formats 1 and 2 - initial input Coverage.
        // ================================================================

        [[nodiscard]] bool coverageOffset(uint16_t& result) const noexcept
        {
            result = 0;

            const uint16_t substFormat = format();

            if (substFormat != 1 && substFormat != 2)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 2, result);
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
            return openTypeGsubChainContextReadUInt16(fData, 4, count) ? count : 0;
        }

        [[nodiscard]] bool ruleSetOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (!isFormat1Valid())
                return false;

            const uint16_t count = ruleSetCount();

            if (index >= count)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 6 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubChainContextRuleSetView ruleSet(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ruleSetOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubChainContextRuleSetView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 2 ClassDefs.
        // ================================================================

        [[nodiscard]] bool backtrackClassDefOffset(uint16_t& result) const noexcept
        {
            result = 0;

            if (format() != 2)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 4, result);
        }

        [[nodiscard]] bool inputClassDefOffset(uint16_t& result) const noexcept
        {
            result = 0;

            if (format() != 2)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 6, result);
        }

        [[nodiscard]] bool lookaheadClassDefOffset(uint16_t& result) const noexcept
        {
            result = 0;

            if (format() != 2)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 8, result);
        }

        [[nodiscard]] OpenTypeClassDefView backtrackClassDef() const noexcept
        {
            uint16_t offset = 0;

            if (!backtrackClassDefOffset(offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeClassDefView(fData.subSpan(offset));
        }

        [[nodiscard]] OpenTypeClassDefView inputClassDef() const noexcept
        {
            uint16_t offset = 0;

            if (!inputClassDefOffset(offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeClassDefView(fData.subSpan(offset));
        }

        [[nodiscard]] OpenTypeClassDefView lookaheadClassDef() const noexcept
        {
            uint16_t offset = 0;

            if (!lookaheadClassDefOffset(offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeClassDefView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 2 ClassRuleSets.
        // ================================================================

        [[nodiscard]] uint16_t classSetCount() const noexcept
        {
            if (format() != 2)
                return 0;

            uint16_t count = 0;
            return openTypeGsubChainContextReadUInt16(fData, 10, count) ? count : 0;
        }

        [[nodiscard]] bool classSetOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            if (!isFormat2Valid())
                return false;

            const uint16_t count = classSetCount();

            if (index >= count)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, 12 + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeGsubChainContextClassSetView classSet(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!classSetOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeGsubChainContextClassSetView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 3 counts.
        // ================================================================

        [[nodiscard]] uint16_t backtrackGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextFormat3Layout layout{};
            return openTypeGsubChainContextReadFormat3Layout(fData, layout) ? layout.backtrackGlyphCount : 0;
        }

        [[nodiscard]] uint16_t inputGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextFormat3Layout layout{};
            return openTypeGsubChainContextReadFormat3Layout(fData, layout) ? layout.inputGlyphCount : 0;
        }

        [[nodiscard]] uint16_t lookaheadGlyphCount() const noexcept
        {
            OpenTypeGsubChainContextFormat3Layout layout{};
            return openTypeGsubChainContextReadFormat3Layout(fData, layout) ? layout.lookaheadGlyphCount : 0;
        }

        [[nodiscard]] uint16_t sequenceLookupCount() const noexcept
        {
            OpenTypeGsubChainContextFormat3Layout layout{};
            return openTypeGsubChainContextReadFormat3Layout(fData, layout) ? layout.sequenceLookupCount : 0;
        }


        // ================================================================
        // Format 3 backtrack Coverages.
        //
        // Index zero is the nearest backtrack position.
        // ================================================================

        [[nodiscard]] bool backtrackCoverageOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextFormat3Layout layout{};

            if (!openTypeGsubChainContextReadFormat3Layout(fData, layout) || index >= layout.backtrackGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.backtrackCoverageOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeCoverageView backtrackCoverage(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!backtrackCoverageOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 3 input Coverages.
        //
        // Unlike Formats 1 and 2, input position zero is explicitly
        // represented by inputCoverage(0).
        // ================================================================

        [[nodiscard]] bool inputCoverageOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextFormat3Layout layout{};

            if (!openTypeGsubChainContextReadFormat3Layout(fData, layout) || index >= layout.inputGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.inputCoverageOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeCoverageView inputCoverage(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!inputCoverageOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 3 lookahead Coverages.
        // ================================================================

        [[nodiscard]] bool lookaheadCoverageOffset(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubChainContextFormat3Layout layout{};

            if (!openTypeGsubChainContextReadFormat3Layout(fData, layout) || index >= layout.lookaheadGlyphCount)
                return false;

            return openTypeGsubChainContextReadUInt16(fData, layout.lookaheadCoverageOffset + size_t(index) * 2, result);
        }

        [[nodiscard]] OpenTypeCoverageView lookaheadCoverage(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!lookaheadCoverageOffset(index, offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Format 3 SequenceLookup records.
        //
        // sequenceIndex is intentionally not validated against the original
        // inputGlyphCount. Earlier nested substitutions may alter the current
        // input sequence before a later SequenceLookup executes.
        // ================================================================

        [[nodiscard]] bool sequenceLookup(uint16_t index, OpenTypeSequenceLookup& result) const noexcept
        {
            result = {};

            OpenTypeGsubChainContextFormat3Layout layout{};

            if (!openTypeGsubChainContextReadFormat3Layout(fData, layout) || index >= layout.sequenceLookupCount)
                return false;

            const size_t offset = layout.sequenceLookupOffset + size_t(index) * 4;

            if (!openTypeGsubChainContextReadUInt16(fData, offset, result.sequenceIndex))
                return false;

            return openTypeGsubChainContextReadUInt16(fData, offset + 2, result.lookupListIndex);
        }


    private:
        [[nodiscard]] bool isFormat1Valid() const noexcept
        {
            uint16_t substFormat = 0;
            uint16_t coverageOffset = 0;
            uint16_t ruleSetCount = 0;

            if (!openTypeGsubChainContextReadUInt16(fData, 0, substFormat) ||
                !openTypeGsubChainContextReadUInt16(fData, 2, coverageOffset) ||
                !openTypeGsubChainContextReadUInt16(fData, 4, ruleSetCount))
            {
                return false;
            }

            if (substFormat != 1)
                return false;

            return openTypeGsubChainContextArrayFits(fData, 6, ruleSetCount, 2);
        }

        [[nodiscard]] bool isFormat2Valid() const noexcept
        {
            uint16_t substFormat = 0;
            uint16_t coverageOffset = 0;
            uint16_t backtrackClassDefOffset = 0;
            uint16_t inputClassDefOffset = 0;
            uint16_t lookaheadClassDefOffset = 0;
            uint16_t classSetCount = 0;

            if (!openTypeGsubChainContextReadUInt16(fData, 0, substFormat) ||
                !openTypeGsubChainContextReadUInt16(fData, 2, coverageOffset) ||
                !openTypeGsubChainContextReadUInt16(fData, 4, backtrackClassDefOffset) ||
                !openTypeGsubChainContextReadUInt16(fData, 6, inputClassDefOffset) ||
                !openTypeGsubChainContextReadUInt16(fData, 8, lookaheadClassDefOffset) ||
                !openTypeGsubChainContextReadUInt16(fData, 10, classSetCount))
            {
                return false;
            }

            if (substFormat != 2)
                return false;

            return openTypeGsubChainContextArrayFits(fData, 12, classSetCount, 2);
        }

        [[nodiscard]] bool isFormat3Valid() const noexcept
        {
            OpenTypeGsubChainContextFormat3Layout layout{};
            return openTypeGsubChainContextReadFormat3Layout(fData, layout);
        }

        ByteSpan fData{};
    };

} // namespace waavs