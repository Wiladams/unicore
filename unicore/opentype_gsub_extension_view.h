// opentype_gsub_extension_view.h
#pragma once

#include <cstdint>

#include "opentype_bytestream.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGsubExtensionSubstView
    //
    // GSUB LookupType 7: Extension Substitution
    //
    // Format 1:
    //
    //   uint16   format
    //   uint16   extensionLookupType
    //   Offset32 extensionOffset
    //
    // extensionOffset is relative to the beginning of this
    // ExtensionSubst subtable.
    //
    // Type 7 introduces no substitution semantics of its own. It simply
    // redirects to a subtable for another GSUB lookup type using a 32-bit
    // offset.
    // ====================================================================

    class OpenTypeGsubExtensionSubstView
    {
    public:
        OpenTypeGsubExtensionSubstView() noexcept = default;
        explicit OpenTypeGsubExtensionSubstView(ByteSpan data) noexcept : fData(data) {}

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
        // Extension subtable
        //
        // The returned ByteSpan begins at the actual GSUB subtable.
        //
        // Its interpretation is determined by extensionLookupType().
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

            if (!stream.readUInt16(format))
                return false;

            if (format != 1)
                return false;

            if (!stream.readUInt16(lookupType))
                return false;

            // ExtensionSubst cannot recursively extend Type 7.

            if (lookupType == 0 || lookupType == 7)
                return false;

            if (!stream.readOffset32(offset))
                return false;

            // The target subtable must begin after this 8-byte header and
            // must remain within the supplied ExtensionSubst data.

            if (offset < 8 || offset >= fData.size())
                return false;

            return true;
        }

    private:
        ByteSpan fData{};
    };

} // namespace waavs