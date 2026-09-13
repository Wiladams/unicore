// opentype_gsub_multiple_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_coverage_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGsubMultipleSequenceView
    //
    // Non-owning lazy view of:
    //
    //   uint16 glyphCount
    //   uint16 substituteGlyphIds[glyphCount]
    //
    // glyphCount must be greater than zero.
    // ====================================================================

    class OpenTypeGsubMultipleSequenceView
    {
    public:
        OpenTypeGsubMultipleSequenceView() noexcept = default;
        explicit OpenTypeGsubMultipleSequenceView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t glyphCount() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count) ? count : 0;
        }

        [[nodiscard]] bool glyphId(size_t index, uint16_t& result) const noexcept
        {
            uint16_t count = 0;

            if (!readHeader(count) || index >= count)
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

            if (glyphCount == 0)
                return false;

            if (glyphCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubMultipleSubstView
    //
    // GSUB LookupType 2: Multiple Substitution
    //
    // Format 1:
    //
    //   uint16   substFormat
    //   Offset16 coverageOffset
    //   uint16   sequenceCount
    //   Offset16 sequenceOffsets[sequenceCount]
    //
    // Sequence offsets are relative to the beginning of this subtable and
    // are ordered by Coverage index.
    // ====================================================================

    class OpenTypeGsubMultipleSubstView
    {
    public:
        OpenTypeGsubMultipleSubstView() noexcept = default;
        explicit OpenTypeGsubMultipleSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t sequenceCount = 0;
            return readHeader(coverageOffset, sequenceCount);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            return isValid() ? 1 : 0;
        }

        [[nodiscard]] uint16_t coverageOffset() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t sequenceCount = 0;

            return readHeader(coverageOffset, sequenceCount) ? coverageOffset : 0;
        }

        [[nodiscard]] uint16_t sequenceCount() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t sequenceCount = 0;

            return readHeader(coverageOffset, sequenceCount) ? sequenceCount : 0;
        }


        // ================================================================
        // Coverage
        // ================================================================

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t sequenceCount = 0;

            if (!readHeader(coverageOffset, sequenceCount))
                return {};

            OpenTypeByteStream stream(fData);
            auto coverageStream = stream.subStream(coverageOffset);

            if (!coverageStream.isValid() || coverageStream.empty())
                return {};

            return OpenTypeCoverageView(coverageStream.remainingData());
        }


        // ================================================================
        // Sequence access
        // ================================================================

        [[nodiscard]] bool sequenceOffset(size_t index, uint16_t& result) const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t sequenceCount = 0;

            if (!readHeader(coverageOffset, sequenceCount) || index >= sequenceCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(6 + index * 2))
                return false;

            return stream.readOffset16(result);
        }

        [[nodiscard]] OpenTypeGsubMultipleSequenceView sequence(size_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!sequenceOffset(index, offset) || offset == 0)
                return {};

            OpenTypeByteStream stream(fData);
            auto sequenceStream = stream.subStream(offset);

            if (!sequenceStream.isValid() || sequenceStream.empty())
                return {};

            return OpenTypeGsubMultipleSequenceView(sequenceStream.remainingData());
        }


        // ================================================================
        // Find the replacement sequence for one glyph.
        //
        // Returns false when:
        //
        //   - the subtable/Coverage is malformed
        //   - the glyph is not covered
        //   - Coverage index does not map to a Sequence
        //   - the selected Sequence is malformed
        //
        // The executor will distinguish those cases explicitly when
        // mutation semantics matter.
        // ================================================================

        [[nodiscard]] bool sequenceForGlyph(uint32_t glyphId,
            OpenTypeGsubMultipleSequenceView& result) const noexcept
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

            const OpenTypeGsubMultipleSequenceView replacement =
                sequence(coverageIndex);

            if (!replacement)
                return false;

            result = replacement;

            return true;
        }


    private:
        bool readHeader(uint16_t& coverageOffset, uint16_t& sequenceCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;

            if (!stream.readUInt16(format))
                return false;

            if (format != 1)
                return false;

            if (!stream.readOffset16(coverageOffset))
                return false;

            if (!stream.readUInt16(sequenceCount))
                return false;

            if (coverageOffset == 0 || coverageOffset >= fData.size())
                return false;

            if (sequenceCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs