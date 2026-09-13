// opentype_gsub_reverse_chain_single_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_coverage_view.h"

namespace waavs
{
    // ====================================================================
    // Internal helpers.
    // ====================================================================

    static inline bool openTypeGsubReverseChainSingleReadUInt16(
        const ByteSpan& data, size_t offset, uint16_t& result) noexcept
    {
        result = 0;

        if (offset > data.size() || data.size() - offset < 2)
            return false;

        const uint8_t* p = data.begin() + offset;
        result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
        return true;
    }


    static inline bool openTypeGsubReverseChainSingleArrayFits(
        const ByteSpan& data, size_t offset, size_t count, size_t itemSize) noexcept
    {
        if (offset > data.size())
            return false;

        return count <= (data.size() - offset) / itemSize;
    }


    // ====================================================================
    // ReverseChainSingleSubstFormat1 variable geometry.
    //
    // uint16   substFormat
    // Offset16 coverageOffset
    // uint16   backtrackGlyphCount
    // Offset16 backtrackCoverageOffsets[backtrackGlyphCount]
    // uint16   lookaheadGlyphCount
    // Offset16 lookaheadCoverageOffsets[lookaheadGlyphCount]
    // uint16   glyphCount
    // uint16   substituteGlyphIDs[glyphCount]
    // ====================================================================

    struct OpenTypeGsubReverseChainSingleLayout
    {
        uint16_t coverageOffset{ 0 };
        uint16_t backtrackGlyphCount{ 0 };
        uint16_t lookaheadGlyphCount{ 0 };
        uint16_t glyphCount{ 0 };

        size_t backtrackCoverageOffset{ 0 };
        size_t lookaheadCoverageOffset{ 0 };
        size_t substituteGlyphOffset{ 0 };
    };


    static inline bool openTypeGsubReverseChainSingleReadLayout(
        const ByteSpan& data, OpenTypeGsubReverseChainSingleLayout& result) noexcept
    {
        result = {};

        uint16_t format = 0;

        if (!openTypeGsubReverseChainSingleReadUInt16(data, 0, format) || format != 1)
            return false;

        if (!openTypeGsubReverseChainSingleReadUInt16(data, 2, result.coverageOffset))
            return false;

        size_t offset = 4;


        // ------------------------------------------------------------
        // Backtrack Coverage array.
        // ------------------------------------------------------------

        if (!openTypeGsubReverseChainSingleReadUInt16(
            data, offset, result.backtrackGlyphCount))
        {
            return false;
        }

        offset += 2;
        result.backtrackCoverageOffset = offset;

        if (!openTypeGsubReverseChainSingleArrayFits(
            data, offset, result.backtrackGlyphCount, 2))
        {
            return false;
        }

        offset += size_t(result.backtrackGlyphCount) * 2;


        // ------------------------------------------------------------
        // Lookahead Coverage array.
        // ------------------------------------------------------------

        if (!openTypeGsubReverseChainSingleReadUInt16(
            data, offset, result.lookaheadGlyphCount))
        {
            return false;
        }

        offset += 2;
        result.lookaheadCoverageOffset = offset;

        if (!openTypeGsubReverseChainSingleArrayFits(
            data, offset, result.lookaheadGlyphCount, 2))
        {
            return false;
        }

        offset += size_t(result.lookaheadGlyphCount) * 2;


        // ------------------------------------------------------------
        // Substitute glyph array.
        // ------------------------------------------------------------

        if (!openTypeGsubReverseChainSingleReadUInt16(
            data, offset, result.glyphCount))
        {
            return false;
        }

        offset += 2;
        result.substituteGlyphOffset = offset;

        return openTypeGsubReverseChainSingleArrayFits(
            data, offset, result.glyphCount, 2);
    }


    // ====================================================================
    // OpenTypeGsubReverseChainSingleSubstView
    //
    // GSUB LookupType 8 Reverse Chaining Contextual Single Substitution.
    //
    // There is only one format.
    //
    // The input sequence contains exactly one glyph. Coverage selects that
    // input glyph and its Coverage index selects substituteGlyphIDs[index].
    //
    // Backtrack and lookahead Coverage arrays describe match-only context.
    //
    // backtrackCoverage(0) is nearest to the current glyph.
    // lookaheadCoverage(0) is nearest to the current glyph.
    //
    // Reverse processing changes lookup scan direction, not this encoded
    // context geometry.
    // ====================================================================

    class OpenTypeGsubReverseChainSingleSubstView
    {
    public:
        OpenTypeGsubReverseChainSingleSubstView() noexcept = default;
        explicit OpenTypeGsubReverseChainSingleSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            OpenTypeGsubReverseChainSingleLayout layout{};
            return openTypeGsubReverseChainSingleReadLayout(fData, layout);
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t result = 0;
            return openTypeGsubReverseChainSingleReadUInt16(fData, 0, result) ? result : 0;
        }


        // ================================================================
        // Input Coverage.
        // ================================================================

        [[nodiscard]] bool coverageOffset(uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubReverseChainSingleLayout layout{};

            if (!openTypeGsubReverseChainSingleReadLayout(fData, layout))
                return false;

            result = layout.coverageOffset;
            return true;
        }


        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            uint16_t offset = 0;

            if (!coverageOffset(offset) || offset == 0 || offset >= fData.size())
                return {};

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Backtrack Coverages.
        //
        // Index 0 is the nearest logical previous glyph.
        // ================================================================

        [[nodiscard]] uint16_t backtrackGlyphCount() const noexcept
        {
            OpenTypeGsubReverseChainSingleLayout layout{};

            return openTypeGsubReverseChainSingleReadLayout(fData, layout) ?
                layout.backtrackGlyphCount : 0;
        }


        [[nodiscard]] bool backtrackCoverageOffset(
            uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubReverseChainSingleLayout layout{};

            if (!openTypeGsubReverseChainSingleReadLayout(fData, layout) ||
                index >= layout.backtrackGlyphCount)
            {
                return false;
            }

            return openTypeGsubReverseChainSingleReadUInt16(
                fData, layout.backtrackCoverageOffset + size_t(index) * 2, result);
        }


        [[nodiscard]] OpenTypeCoverageView backtrackCoverage(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!backtrackCoverageOffset(index, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Lookahead Coverages.
        //
        // Index 0 is the nearest logical following glyph.
        // ================================================================

        [[nodiscard]] uint16_t lookaheadGlyphCount() const noexcept
        {
            OpenTypeGsubReverseChainSingleLayout layout{};

            return openTypeGsubReverseChainSingleReadLayout(fData, layout) ?
                layout.lookaheadGlyphCount : 0;
        }


        [[nodiscard]] bool lookaheadCoverageOffset(
            uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubReverseChainSingleLayout layout{};

            if (!openTypeGsubReverseChainSingleReadLayout(fData, layout) ||
                index >= layout.lookaheadGlyphCount)
            {
                return false;
            }

            return openTypeGsubReverseChainSingleReadUInt16(
                fData, layout.lookaheadCoverageOffset + size_t(index) * 2, result);
        }


        [[nodiscard]] OpenTypeCoverageView lookaheadCoverage(uint16_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!lookaheadCoverageOffset(index, offset) ||
                offset == 0 || offset >= fData.size())
            {
                return {};
            }

            return OpenTypeCoverageView(fData.subSpan(offset));
        }


        // ================================================================
        // Substitute glyphs.
        //
        // substituteGlyphId(i) corresponds directly to Coverage index i.
        // ================================================================

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            OpenTypeGsubReverseChainSingleLayout layout{};

            return openTypeGsubReverseChainSingleReadLayout(fData, layout) ?
                layout.glyphCount : 0;
        }


        [[nodiscard]] bool substituteGlyphId(uint16_t index, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeGsubReverseChainSingleLayout layout{};

            if (!openTypeGsubReverseChainSingleReadLayout(fData, layout) ||
                index >= layout.glyphCount)
            {
                return false;
            }

            return openTypeGsubReverseChainSingleReadUInt16(
                fData, layout.substituteGlyphOffset + size_t(index) * 2, result);
        }


    private:
        ByteSpan fData{};
    };

} // namespace waavs