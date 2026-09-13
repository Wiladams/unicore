// opentype_coverage_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"


namespace waavs
{
    // ====================================================================
    // OpenTypeCoverageView
    //
    // Non-owning lazy view of an OpenType Coverage table.
    //
    // Format 1:
    //
    //   uint16 coverageFormat
    //   uint16 glyphCount
    //   uint16 glyphArray[glyphCount]
    //
    // Format 2:
    //
    //   uint16 coverageFormat
    //   uint16 rangeCount
    //   RangeRecord ranges[rangeCount]
    //
    // RangeRecord:
    //
    //   uint16 startGlyphId
    //   uint16 endGlyphId
    //   uint16 startCoverageIndex
    //
    // The table is searched lazily. isValid() validates only the table
    // header and array geometry; records are inspected only when queried.
    // ====================================================================

    class OpenTypeCoverageView
    {
    public:
        OpenTypeCoverageView() noexcept = default;
        explicit OpenTypeCoverageView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t format = 0;
            uint16_t count = 0;
            return readHeader(format, count);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t format = 0;
            uint16_t count = 0;
            return readHeader(format, count) ? format : 0;
        }

        [[nodiscard]] uint16_t recordCount() const noexcept
        {
            uint16_t format = 0;
            uint16_t count = 0;
            return readHeader(format, count) ? count : 0;
        }

        [[nodiscard]] bool contains(uint32_t glyphId) const noexcept
        {
            uint16_t coverageIndex = 0;
            return find(glyphId, coverageIndex);
        }

        [[nodiscard]] bool find(uint32_t glyphId, uint16_t& coverageIndex) const noexcept
        {
            if (glyphId > 0xFFFFu)
                return false;

            uint16_t format = 0;
            uint16_t count = 0;

            if (!readHeader(format, count))
                return false;

            if (format == 1)
                return findFormat1(static_cast<uint16_t>(glyphId), count, coverageIndex);

            return findFormat2(static_cast<uint16_t>(glyphId), count, coverageIndex);
        }

    private:
        bool readHeader(uint16_t& format, uint16_t& count) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(format) || !stream.readUInt16(count))
                return false;

            if (format == 1)
                return count <= stream.remaining() / 2;

            if (format == 2)
                return count <= stream.remaining() / 6;

            return false;
        }

        bool findFormat1(uint16_t glyphId, uint16_t count, uint16_t& coverageIndex) const noexcept
        {
            size_t first = 0;
            size_t last = count;

            while (first < last)
            {
                const size_t middle = first + (last - first) / 2;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(4 + middle * 2))
                    return false;

                uint16_t currentGlyph = 0;

                if (!stream.readUInt16(currentGlyph))
                    return false;

                if (glyphId < currentGlyph)
                {
                    last = middle;
                }
                else if (glyphId > currentGlyph)
                {
                    first = middle + 1;
                }
                else
                {
                    coverageIndex = static_cast<uint16_t>(middle);
                    return true;
                }
            }

            return false;
        }

        bool findFormat2(uint16_t glyphId, uint16_t count, uint16_t& coverageIndex) const noexcept
        {
            size_t first = 0;
            size_t last = count;

            while (first < last)
            {
                const size_t middle = first + (last - first) / 2;

                OpenTypeByteStream stream(fData);

                if (!stream.seek(4 + middle * 6))
                    return false;

                uint16_t startGlyphId = 0;
                uint16_t endGlyphId = 0;
                uint16_t startCoverageIndex = 0;

                if (!stream.readUInt16(startGlyphId) ||
                    !stream.readUInt16(endGlyphId) ||
                    !stream.readUInt16(startCoverageIndex))
                {
                    return false;
                }

                if (startGlyphId > endGlyphId)
                    return false;

                if (glyphId < startGlyphId)
                {
                    last = middle;
                }
                else if (glyphId > endGlyphId)
                {
                    first = middle + 1;
                }
                else
                {
                    const uint32_t index =
                        uint32_t(startCoverageIndex) +
                        uint32_t(glyphId - startGlyphId);

                    if (index > 0xFFFFu)
                        return false;

                    coverageIndex = static_cast<uint16_t>(index);
                    return true;
                }
            }

            return false;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs