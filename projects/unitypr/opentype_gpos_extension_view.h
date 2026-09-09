// opentype_gpos_extension_view.h
#pragma once

#include <cstdint>

#include "opentype_bytestream.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGposExtensionPosView
    //
    // GPOS LookupType 9: Extension Positioning
    //
    // Format 1:
    //
    //   uint16   format
    //   uint16   extensionLookupType
    //   Offset32 extensionOffset
    //
    // extensionOffset is relative to the beginning of this ExtensionPos
    // subtable.
    //
    // Type 9 introduces no positioning semantics of its own. It redirects to
    // a positioning subtable for GPOS LookupType 1..8 using a 32-bit offset.
    // ====================================================================

    class OpenTypeGposExtensionPosView
    {
    public:
        OpenTypeGposExtensionPosView() noexcept = default;
        explicit OpenTypeGposExtensionPosView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            uint16_t lookupType = 0;
            uint32_t offset = 0;
            return readHeader(lookupType, offset);
        }

        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t format() const noexcept
        {
            return isValid() ? 1 : 0;
        }

        [[nodiscard]] uint16_t extensionLookupType() const noexcept
        {
            uint16_t lookupType = 0;
            uint32_t offset = 0;
            return readHeader(lookupType, offset) ? lookupType : 0;
        }

        [[nodiscard]] uint32_t extensionOffset() const noexcept
        {
            uint16_t lookupType = 0;
            uint32_t offset = 0;
            return readHeader(lookupType, offset) ? offset : 0;
        }


        // ================================================================
        // extensionSubtable
        //
        // Return a ByteSpan beginning at the actual GPOS positioning
        // subtable. Its interpretation is determined by extensionLookupType().
        // ================================================================

        [[nodiscard]] ByteSpan extensionSubtable() const noexcept
        {
            uint16_t lookupType = 0;
            uint32_t offset = 0;

            if (!readHeader(lookupType, offset))
                return {};

            OpenTypeByteStream stream(fData);
            auto extensionStream = stream.subStream(offset);

            if (!extensionStream.isValid() || extensionStream.empty())
                return {};

            return extensionStream.remainingData();
        }

    private:
        bool readHeader(uint16_t& lookupType, uint32_t& offset) const noexcept
        {
            OpenTypeByteStream stream(fData);

            uint16_t format = 0;

            if (!stream.readUInt16(format) || format != 1)
                return false;

            if (!stream.readUInt16(lookupType))
                return false;

            // ExtensionPos may target GPOS Types 1..8 only.
            // Recursive Type 9 extension is prohibited.

            if (lookupType < 1 || lookupType > 8)
                return false;

            if (!stream.readOffset32(offset))
                return false;

            // Target must begin after the eight-byte ExtensionPos header and
            // remain within this ExtensionPos subtable.

            if (offset < 8 || offset >= fData.size())
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs