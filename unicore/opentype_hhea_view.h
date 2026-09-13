// opentype_hhea_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "lang_span.h"

namespace waavs
{
    static inline bool openTypeHheaReadUInt16(
        const ByteSpan& data, size_t offset, uint16_t& result) noexcept
    {
        result = 0;

        if (offset > data.size() || data.size() - offset < 2)
            return false;

        const uint8_t* p = data.begin() + offset;
        result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
        return true;
    }


    static inline bool openTypeHheaReadInt16(
        const ByteSpan& data, size_t offset, int16_t& result) noexcept
    {
        uint16_t value = 0;

        if (!openTypeHheaReadUInt16(data, offset, value))
            return false;

        result = static_cast<int16_t>(value);
        return true;
    }


    // ====================================================================
    // OpenTypeHheaView
    //
    // Lazy view of the OpenType 'hhea' horizontal-header table.
    //
    // The current hhea format is version 1.0 and is 36 bytes long.
    // numberOfHMetrics determines the number of longHorMetric records in
    // the associated 'hmtx' table.
    // ====================================================================

    class OpenTypeHheaView
    {
    public:
        OpenTypeHheaView() noexcept = default;
        explicit OpenTypeHheaView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            if (fData.size() < 36)
                return false;

            uint16_t majorVersion = 0;
            uint16_t minorVersion = 0;
            int16_t metricDataFormat = 0;
            uint16_t numberOfHMetrics = 0;

            if (!openTypeHheaReadUInt16(fData, 0, majorVersion) ||
                !openTypeHheaReadUInt16(fData, 2, minorVersion) ||
                !openTypeHheaReadInt16(fData, 32, metricDataFormat) ||
                !openTypeHheaReadUInt16(fData, 34, numberOfHMetrics))
            {
                return false;
            }

            return majorVersion == 1 &&
                minorVersion == 0 &&
                metricDataFormat == 0 &&
                numberOfHMetrics != 0;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t size() const noexcept { return fData.size(); }

        [[nodiscard]] int16_t ascender() const noexcept
        {
            int16_t result = 0;
            return openTypeHheaReadInt16(fData, 4, result) ? result : 0;
        }

        [[nodiscard]] int16_t descender() const noexcept
        {
            int16_t result = 0;
            return openTypeHheaReadInt16(fData, 6, result) ? result : 0;
        }

        [[nodiscard]] int16_t lineGap() const noexcept
        {
            int16_t result = 0;
            return openTypeHheaReadInt16(fData, 8, result) ? result : 0;
        }

        [[nodiscard]] uint16_t advanceWidthMax() const noexcept
        {
            uint16_t result = 0;
            return openTypeHheaReadUInt16(fData, 10, result) ? result : 0;
        }

        [[nodiscard]] uint16_t numberOfHMetrics() const noexcept
        {
            uint16_t result = 0;
            return openTypeHheaReadUInt16(fData, 34, result) ? result : 0;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs