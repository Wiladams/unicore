// opentype_hmtx_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "lang_span.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeHorizontalMetric
    //
    // Raw horizontal metrics in font design units.
    // ====================================================================

    struct OpenTypeHorizontalMetric
    {
        uint16_t advanceWidth{ 0 };
        int16_t leftSideBearing{ 0 };
    };


    static inline bool openTypeHmtxReadUInt16(
        const ByteSpan& data, size_t offset, uint16_t& result) noexcept
    {
        result = 0;

        if (offset > data.size() || data.size() - offset < 2)
            return false;

        const uint8_t* p = data.begin() + offset;
        result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
        return true;
    }


    static inline bool openTypeHmtxReadInt16(
        const ByteSpan& data, size_t offset, int16_t& result) noexcept
    {
        uint16_t value = 0;

        if (!openTypeHmtxReadUInt16(data, offset, value))
            return false;

        result = static_cast<int16_t>(value);
        return true;
    }


    // ====================================================================
    // OpenTypeHmtxView
    //
    // Lazy view of the OpenType 'hmtx' horizontal-metrics table.
    //
    // Layout:
    //
    //   longHorMetric hMetrics[numberOfHMetrics]
    //
    // where each longHorMetric is:
    //
    //   uint16 advanceWidth
    //   int16  leftSideBearing
    //
    // followed by:
    //
    //   int16 leftSideBearings[glyphCount - numberOfHMetrics]
    //
    // Glyphs after the final longHorMetric reuse its advanceWidth.
    // ====================================================================

    class OpenTypeHmtxView
    {
    public:
        OpenTypeHmtxView() noexcept = default;

        OpenTypeHmtxView(ByteSpan data, uint32_t glyphCount, uint16_t numberOfHMetrics) noexcept
            : fData(data)
            , fGlyphCount(glyphCount)
            , fNumberOfHMetrics(numberOfHMetrics)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fGlyphCount == 0 ||
                fGlyphCount > 0xFFFFu ||
                fNumberOfHMetrics == 0 ||
                fNumberOfHMetrics > fGlyphCount)
            {
                return false;
            }

            const size_t longMetricBytes = size_t(fNumberOfHMetrics) * 4;
            const size_t trailingCount = size_t(fGlyphCount - fNumberOfHMetrics);
            const size_t trailingBytes = trailingCount * 2;

            if (longMetricBytes > fData.size())
                return false;

            return trailingBytes <= fData.size() - longMetricBytes;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] uint32_t glyphCount() const noexcept { return fGlyphCount; }
        [[nodiscard]] uint16_t numberOfHMetrics() const noexcept { return fNumberOfHMetrics; }

        [[nodiscard]] bool metric(uint32_t glyphId, OpenTypeHorizontalMetric& result) const noexcept
        {
            result = {};

            if (!isValid() || glyphId >= fGlyphCount)
                return false;

            if (glyphId < fNumberOfHMetrics)
            {
                const size_t offset = size_t(glyphId) * 4;

                return openTypeHmtxReadUInt16(fData, offset, result.advanceWidth) &&
                    openTypeHmtxReadInt16(fData, offset + 2, result.leftSideBearing);
            }

            // Every glyph after numberOfHMetrics uses the advance width of
            // the final longHorMetric record.

            const size_t finalMetricOffset = size_t(fNumberOfHMetrics - 1) * 4;

            if (!openTypeHmtxReadUInt16(fData, finalMetricOffset, result.advanceWidth))
                return false;

            const size_t lsbOffset =
                size_t(fNumberOfHMetrics) * 4 +
                size_t(glyphId - fNumberOfHMetrics) * 2;

            return openTypeHmtxReadInt16(fData, lsbOffset, result.leftSideBearing);
        }

        [[nodiscard]] bool advanceWidth(uint32_t glyphId, uint16_t& result) const noexcept
        {
            result = 0;

            OpenTypeHorizontalMetric metricResult;

            if (!metric(glyphId, metricResult))
                return false;

            result = metricResult.advanceWidth;
            return true;
        }

    private:
        ByteSpan fData{};
        uint32_t fGlyphCount{ 0 };
        uint16_t fNumberOfHMetrics{ 0 };
    };

} // namespace waavs