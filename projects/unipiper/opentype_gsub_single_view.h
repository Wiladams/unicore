// opentype_gsub_single_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_coverage_view.h"


namespace waavs
{
    // ====================================================================
    // OpenTypeGsubSingleSubstView
    //
    // GSUB LookupType 1: Single Substitution
    //
    // Format 1:
    //
    //   uint16   substFormat
    //   Offset16 coverageOffset
    //   int16    deltaGlyphId
    //
    // Format 2:
    //
    //   uint16   substFormat
    //   Offset16 coverageOffset
    //   uint16   glyphCount
    //   uint16   substituteGlyphIds[glyphCount]
    //
    // The Coverage table is not dereferenced until coverage() or
    // substitute() is called.
    // ====================================================================

    class OpenTypeGsubSingleSubstView
    {
    public:
        OpenTypeGsubSingleSubstView() noexcept = default;
        explicit OpenTypeGsubSingleSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t format = 0;
            uint16_t coverageOffset = 0;
            return readHeader(format, coverageOffset);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            uint16_t format = 0;
            uint16_t coverageOffset = 0;
            return readHeader(format, coverageOffset) ? format : 0;
        }

        [[nodiscard]] uint16_t coverageOffset() const noexcept
        {
            uint16_t format = 0;
            uint16_t coverageOffset = 0;
            return readHeader(format, coverageOffset) ? coverageOffset : 0;
        }


        // ================================================================
        // Coverage
        // ================================================================

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            uint16_t format = 0;
            uint16_t offset = 0;

            if (!readHeader(format, offset))
                return {};

            OpenTypeByteStream stream(fData);
            auto coverageStream = stream.subStream(offset);

            if (!coverageStream.isValid() || coverageStream.empty())
                return {};

            return OpenTypeCoverageView(coverageStream.remainingData());
        }


        // ================================================================
        // Format 1
        // ================================================================

        [[nodiscard]] bool deltaGlyphId(int32_t& result) const noexcept
        {
            uint16_t format = 0;
            uint16_t coverageOffset = 0;

            if (!readHeader(format, coverageOffset) || format != 1)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(4))
                return false;

            uint16_t rawDelta = 0;

            if (!stream.readUInt16(rawDelta))
                return false;

            result = rawDelta < 0x8000u
                ? static_cast<int32_t>(rawDelta)
                : static_cast<int32_t>(rawDelta) - 0x10000;

            return true;
        }


        // ================================================================
        // Format 2
        // ================================================================

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            uint16_t format = 0;
            uint16_t coverageOffset = 0;

            if (!readHeader(format, coverageOffset) || format != 2)
                return 0;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(4))
                return 0;

            uint16_t count = 0;
            return stream.readUInt16(count) ? count : 0;
        }

        [[nodiscard]] bool substituteGlyphId(size_t coverageIndex, uint16_t& result) const noexcept
        {
            uint16_t format = 0;
            uint16_t coverageOffset = 0;

            if (!readHeader(format, coverageOffset) || format != 2)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(4))
                return false;

            uint16_t count = 0;

            if (!stream.readUInt16(count) || coverageIndex >= count)
                return false;

            if (!stream.seek(6 + coverageIndex * 2))
                return false;

            return stream.readUInt16(result);
        }


        // ================================================================
        // Substitution
        //
        // Returns true only when the input glyph is covered and a
        // replacement glyph was produced.
        // ================================================================

        [[nodiscard]] bool substitute(uint32_t glyphId, uint16_t& result) const noexcept
        {
            if (glyphId > 0xFFFFu)
                return false;

            uint16_t format = 0;
            uint16_t coverageOffset = 0;

            if (!readHeader(format, coverageOffset))
                return false;

            const OpenTypeCoverageView coverageView = coverage();

            if (!coverageView)
                return false;

            uint16_t coverageIndex = 0;

            if (!coverageView.find(glyphId, coverageIndex))
                return false;


            // ------------------------------------------------------------
            // Format 1
            //
            // Addition is modulo 65536. Using the raw 16-bit delta gives
            // exactly that behavior without depending on signed overflow.
            // ------------------------------------------------------------

            if (format == 1)
            {
                OpenTypeByteStream stream(fData);

                if (!stream.seek(4))
                    return false;

                uint16_t rawDelta = 0;

                if (!stream.readUInt16(rawDelta))
                    return false;

                result = static_cast<uint16_t>(
                    static_cast<uint32_t>(glyphId) +
                    static_cast<uint32_t>(rawDelta));

                return true;
            }


            // ------------------------------------------------------------
            // Format 2
            //
            // Coverage Index directly indexes substituteGlyphIds[].
            // ------------------------------------------------------------

            return substituteGlyphId(coverageIndex, result);
        }


    private:
        bool readHeader(uint16_t& format, uint16_t& coverageOffset) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(format))
                return false;

            if (!stream.readOffset16(coverageOffset))
                return false;

            if (coverageOffset == 0 || coverageOffset >= fData.size())
                return false;


            if (format == 1)
            {
                // deltaGlyphId
                return stream.remaining() >= 2;
            }


            if (format == 2)
            {
                uint16_t glyphCount = 0;

                if (!stream.readUInt16(glyphCount))
                    return false;

                if (glyphCount > stream.remaining() / 2)
                    return false;

                return true;
            }


            return false;
        }


    private:
        ByteSpan fData{};
    };

} // namespace waavs