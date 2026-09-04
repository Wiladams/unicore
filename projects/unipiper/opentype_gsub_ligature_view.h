// opentype_gsub_ligature_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_coverage_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGsubLigatureView
    //
    // Non-owning lazy view of:
    //
    //   uint16 ligatureGlyph
    //   uint16 componentCount
    //   uint16 componentGlyphIds[componentCount - 1]
    //
    // componentCount includes the first glyph, which is represented by
    // the parent LigatureSet's Coverage entry.
    // ====================================================================

    class OpenTypeGsubLigatureView
    {
    public:
        OpenTypeGsubLigatureView() noexcept = default;
        explicit OpenTypeGsubLigatureView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t ligatureGlyph = 0;
            uint16_t componentCount = 0;
            return readHeader(ligatureGlyph, componentCount);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t ligatureGlyph() const noexcept
        {
            uint16_t ligatureGlyph = 0;
            uint16_t componentCount = 0;
            return readHeader(ligatureGlyph, componentCount) ? ligatureGlyph : 0;
        }

        [[nodiscard]] uint16_t componentCount() const noexcept
        {
            uint16_t ligatureGlyph = 0;
            uint16_t componentCount = 0;
            return readHeader(ligatureGlyph, componentCount) ? componentCount : 0;
        }

        [[nodiscard]] uint16_t trailingComponentCount() const noexcept
        {
            const uint16_t count = componentCount();
            return count > 0 ? static_cast<uint16_t>(count - 1) : 0;
        }

        [[nodiscard]] bool componentGlyphId(size_t index, uint16_t& result) const noexcept
        {
            uint16_t ligatureGlyph = 0;
            uint16_t componentCount = 0;

            if (!readHeader(ligatureGlyph, componentCount))
                return false;

            const size_t trailingCount = size_t(componentCount - 1);

            if (index >= trailingCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(4 + index * 2))
                return false;

            return stream.readUInt16(result);
        }

    private:
        bool readHeader(uint16_t& ligatureGlyph, uint16_t& componentCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(ligatureGlyph))
                return false;

            if (!stream.readUInt16(componentCount))
                return false;

            // LookupType 4 represents multiple glyphs collapsing to one.
            if (componentCount < 2)
                return false;

            const size_t trailingCount = size_t(componentCount - 1);

            if (trailingCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubLigatureSetView
    //
    // Non-owning lazy view of:
    //
    //   uint16   ligatureCount
    //   Offset16 ligatureOffsets[ligatureCount]
    //
    // Offsets are relative to the beginning of LigatureSet.
    //
    // Ligature order is significant: it expresses preference.
    // ====================================================================

    class OpenTypeGsubLigatureSetView
    {
    public:
        OpenTypeGsubLigatureSetView() noexcept = default;
        explicit OpenTypeGsubLigatureSetView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t size() const noexcept
        {
            uint16_t count = 0;
            return readHeader(count) ? count : 0;
        }

        [[nodiscard]] bool ligatureOffset(size_t index, uint16_t& result) const noexcept
        {
            uint16_t count = 0;

            if (!readHeader(count) || index >= count)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(2 + index * 2))
                return false;

            return stream.readOffset16(result);
        }

        [[nodiscard]] OpenTypeGsubLigatureView ligature(size_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ligatureOffset(index, offset) || offset == 0)
                return {};

            OpenTypeByteStream stream(fData);
            auto ligatureStream = stream.subStream(offset);

            if (!ligatureStream.isValid() || ligatureStream.empty())
                return {};

            return OpenTypeGsubLigatureView(ligatureStream.remainingData());
        }

    private:
        bool readHeader(uint16_t& ligatureCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(ligatureCount))
                return false;

            if (ligatureCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };


    // ====================================================================
    // OpenTypeGsubLigatureSubstView
    //
    // GSUB LookupType 4: Ligature Substitution
    //
    // Format 1:
    //
    //   uint16   substFormat
    //   Offset16 coverageOffset
    //   uint16   ligatureSetCount
    //   Offset16 ligatureSetOffsets[ligatureSetCount]
    //
    // LigatureSet offsets are relative to this substitution subtable and
    // are ordered by Coverage index.
    // ====================================================================

    class OpenTypeGsubLigatureSubstView
    {
    public:
        OpenTypeGsubLigatureSubstView() noexcept = default;
        explicit OpenTypeGsubLigatureSubstView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t ligatureSetCount = 0;
            return readHeader(coverageOffset, ligatureSetCount);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            return isValid() ? 1 : 0;
        }

        [[nodiscard]] uint16_t coverageOffset() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t ligatureSetCount = 0;
            return readHeader(coverageOffset, ligatureSetCount) ? coverageOffset : 0;
        }

        [[nodiscard]] uint16_t ligatureSetCount() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t ligatureSetCount = 0;
            return readHeader(coverageOffset, ligatureSetCount) ? ligatureSetCount : 0;
        }


        // ================================================================
        // Coverage
        // ================================================================

        [[nodiscard]] OpenTypeCoverageView coverage() const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t ligatureSetCount = 0;

            if (!readHeader(coverageOffset, ligatureSetCount))
                return {};

            OpenTypeByteStream stream(fData);
            auto coverageStream = stream.subStream(coverageOffset);

            if (!coverageStream.isValid() || coverageStream.empty())
                return {};

            return OpenTypeCoverageView(coverageStream.remainingData());
        }


        // ================================================================
        // LigatureSet access
        // ================================================================

        [[nodiscard]] bool ligatureSetOffset(size_t index, uint16_t& result) const noexcept
        {
            uint16_t coverageOffset = 0;
            uint16_t ligatureSetCount = 0;

            if (!readHeader(coverageOffset, ligatureSetCount) || index >= ligatureSetCount)
                return false;

            OpenTypeByteStream stream(fData);

            if (!stream.seek(6 + index * 2))
                return false;

            return stream.readOffset16(result);
        }

        [[nodiscard]] OpenTypeGsubLigatureSetView ligatureSet(size_t index) const noexcept
        {
            uint16_t offset = 0;

            if (!ligatureSetOffset(index, offset) || offset == 0)
                return {};

            OpenTypeByteStream stream(fData);
            auto setStream = stream.subStream(offset);

            if (!setStream.isValid() || setStream.empty())
                return {};

            return OpenTypeGsubLigatureSetView(setStream.remainingData());
        }


        // ================================================================
        // Resolve the LigatureSet for a first-component glyph.
        //
        // Coverage Index directly selects the LigatureSet.
        // ================================================================

        [[nodiscard]] bool ligatureSetForGlyph(uint32_t glyphId,
            OpenTypeGsubLigatureSetView& result) const noexcept
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

            const OpenTypeGsubLigatureSetView set = ligatureSet(coverageIndex);

            if (!set)
                return false;

            result = set;
            return true;
        }

    private:
        bool readHeader(uint16_t& coverageOffset, uint16_t& ligatureSetCount) const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;

            if (!stream.readUInt16(format))
                return false;

            if (format != 1)
                return false;

            if (!stream.readOffset16(coverageOffset))
                return false;

            if (!stream.readUInt16(ligatureSetCount))
                return false;

            if (coverageOffset == 0 || coverageOffset >= fData.size())
                return false;

            if (ligatureSetCount > stream.remaining() / 2)
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs