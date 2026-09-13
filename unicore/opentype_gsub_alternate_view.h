// opentype_gsub_alternate_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_coverage_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGsubAlternateSetView
    //
    // Non-owning lazy view of:
    //
    //   uint16 glyphCount
    //   uint16 alternateGlyphIds[glyphCount]
    //
    // Alternate glyphs are choices for one input glyph. They are not a
    // replacement sequence.
    // ====================================================================

    class OpenTypeGsubAlternateSetView
    {
    public:
        OpenTypeGsubAlternateSetView() noexcept = default;
        explicit OpenTypeGsubAlternateSetView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t glyphCount = 0;
            return readHeader(glyphCount);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            uint16_t glyphCount = 0;
            return readHeader(glyphCount) ? glyphCount : 0;
        }

        [[nodiscard]] bool glyphId(size_t index, uint16_t& result) const noexcept
        {
            uint16_t glyphCount = 0;

            if (!readHeader(glyphCount) || index >= glyphCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(2 + index * 2))
                return false;

            return stream.readUInt16(result);
        }

    private:
        bool readHeader(uint16_t& glyphCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(glyphCount))
                return false;

            if (glyphCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubAlternateSubstView
    //
    // GSUB LookupType 3: Alternate Substitution
    //
    // Format 1:
    //
    //   uint16   substFormat
    //   Offset16 coverageOffset
    //   uint16   alternateSetCount
    //   Offset16 alternateSetOffsets[alternateSetCount]
    //
    // AlternateSet offsets are relative to the beginning of this
    // substitution subtable and are ordered by Coverage index.
    // ====================================================================

    class OpenTypeGsubAlternateSubstView
    {
    public:
        OpenTypeGsubAlternateSubstView() noexcept = default;
        explicit OpenTypeGsubAlternateSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t alternateSetCount = 0;
            return readHeader(coverageOffset, alternateSetCount);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            return isValid() ? 1 : 0;
        }

        [[nodiscard]] uint16_t coverageOffset() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t alternateSetCount = 0;

            return readHeader(coverageOffset, alternateSetCount) ? coverageOffset : 0;
        }

        [[nodiscard]] uint16_t alternateSetCount() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t alternateSetCount = 0;

            return readHeader(coverageOffset, alternateSetCount) ? alternateSetCount : 0;
        }


        // ================================================================
        // Coverage
        // ================================================================

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t alternateSetCount = 0;

            if (!readHeader(coverageOffset, alternateSetCount))
                return {};

            OpenTypeByteStream stream(fData);
            auto coverageStream = stream.subStream(coverageOffset);

            if (!coverageStream.isValid() || coverageStream.empty())
                return {};

            return OpenTypeCoverageView(coverageStream.remainingData());
        }


        // ================================================================
        // AlternateSet access
        // ================================================================

        [[nodiscard]] bool alternateSetOffset(size_t index, uint16_t& result) const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t alternateSetCount = 0;

            if (!readHeader(coverageOffset, alternateSetCount) || index >= alternateSetCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(6 + index * 2))
                return false;

            return stream.readOffset16(result);
        }

        [[nodiscard]] OpenTypeGsubAlternateSetView alternateSet(size_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!alternateSetOffset(index, offset) || offset == 0)
                return {};

            OpenTypeByteStream stream(fData);
            auto setStream = stream.subStream(offset);

            if (!setStream.isValid() || setStream.empty())
                return {};

            return OpenTypeGsubAlternateSetView(setStream.remainingData());
        }


        // ================================================================
        // Resolve AlternateSet for an input glyph.
        //
        // Coverage Index directly selects AlternateSet.
        // ================================================================

        [[nodiscard]] bool alternateSetForGlyph(uint32_t glyphId,
            OpenTypeGsubAlternateSetView& result) const noexcept
        {
            result = {};

            if (glyphId > 0xFFFFu)
                return false;

            const OpenTypeCoverageView coverageView = coverage();

            if (!coverageView)
                return false;

            uint16_t coverageIndex = 0;

            if (!coverageView.find(glyphId, coverageIndex))
                return false;

            const OpenTypeGsubAlternateSetView set =
                alternateSet(coverageIndex);

            if (!set)
                return false;

            result = set;
            return true;
        }

    private:
        bool readHeader(uint16_t& coverageOffset, uint16_t& alternateSetCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;

            if (!stream.readUInt16(format))
                return false;

            if (format != 1)
                return false;

            if (!stream.readOffset16(coverageOffset))
                return false;

            if (!stream.readUInt16(alternateSetCount))
                return false;

            if (coverageOffset == 0 || coverageOffset >= fData.size())
                return false;

            if (alternateSetCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs