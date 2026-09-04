// opentype_mark_glyph_sets_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_coverage_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeMarkGlyphSetsView
    //
    // Lazy non-owning view of the GDEF MarkGlyphSetsDef table.
    //
    // Format 1:
    //
    //   uint16   format
    //   uint16   markGlyphSetCount
    //   Offset32 coverageOffsets[markGlyphSetCount]
    //
    // Each Coverage table describes one mark glyph set.
    //
    // Coverage offsets are relative to the beginning of this
    // MarkGlyphSetsDef table.
    //
    // The offset array uses Offset32, not Offset16.
    // ====================================================================

    class OpenTypeMarkGlyphSetsView
    {
    public:
        OpenTypeMarkGlyphSetsView() noexcept = default;
        explicit OpenTypeMarkGlyphSetsView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count);
        }

        explicit operator bool() const noexcept { return isValid(); }


        // ================================================================
        // Header
        // ================================================================

        [[nodiscard]] uint16_t format() const noexcept
        {
            return isValid() ? 1 : 0;
        }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count) ? count : 0;
        }

        [[nodiscard]] uint16_t markGlyphSetCount() const noexcept
        {
            return size();
        }


        // ================================================================
        // Coverage offset access
        //
        // This returns the raw Offset32 stored by the font.
        //
        // Child validity is deliberately not checked here. That keeps an
        // invalid Coverage offset from invalidating unrelated mark sets.
        // ================================================================

        [[nodiscard]] bool coverageOffset(size_t index, uint32_t& result) const noexcept
        {
            result = 0;

            uint16_t count = 0;

            if (!readHeader(count) || index >= count)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(4 + index * 4))
                return false;

            return stream.readOffset32(result);
        }


        // ================================================================
        // Coverage access
        //
        // Dereference one mark set lazily.
        // ================================================================

        [[nodiscard]] OpenTypeCoverageView coverage(size_t index) const noexcept
        {
            uint16_t count = 0;

            if (!readHeader(count) || index >= count)
                return {};

            uint32_t offset = 0;

            if (!coverageOffset(index, offset))
                return {};

            const size_t headerSize = 4 + size_t(count) * 4;

            // Coverage offsets are not nullable and must point beyond the
            // complete MarkGlyphSetsDef header/offset array.

            if (offset < headerSize || offset >= fData.size())
                return {};

            OpenTypeByteStream stream(fData);
            auto coverageStream = stream.subStream(offset);

            if (!coverageStream.isValid() || coverageStream.empty())
                return {};

            return OpenTypeCoverageView(coverageStream.remainingData());
        }


        // ================================================================
        // contains
        //
        // Test membership in one mark glyph set.
        //
        // Return value:
        //
        //   true  -> operation was structurally usable; result contains
        //            membership
        //
        //   false -> invalid set index, invalid glyph ID, malformed offset,
        //            or invalid Coverage
        //
        // This distinction is useful for shaping: malformed font data should
        // not be silently treated as "glyph is not in this set".
        // ================================================================

        [[nodiscard]] bool contains(size_t setIndex, uint32_t glyphId, bool& result) const noexcept
        {
            result = false;

            if (glyphId > 0xFFFFu)
                return false;

            const OpenTypeCoverageView coverageView = coverage(setIndex);

            if (!coverageView)
                return false;

            result = coverageView.contains(glyphId);
            return true;
        }


    private:
        // ================================================================
        // readHeader
        //
        // Validate only the local MarkGlyphSetsDef structure:
        //
        //   format
        //   markGlyphSetCount
        //   coverageOffsets[]
        //
        // Individual Coverage offsets and Coverage contents remain lazy.
        // ================================================================

        bool readHeader(uint16_t& count) const noexcept
        {
            count = 0;

            OpenTypeByteStream stream(fData);

            uint16_t format = 0;

            if (!stream.readUInt16(format))
                return false;

            if (format != 1)
                return false;

            if (!stream.readUInt16(count))
                return false;

            if (count > stream.remaining() / 4)
                return false;

            return true;
        }


    private:
        ByteSpan fData{};
    };

} // namespace waavs