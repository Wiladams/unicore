// opentype_gdef_view.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_bytestream.h"
#include "opentype_classdef_view.h"
#include "opentype_mark_glyph_sets_view.h"

namespace waavs
{
    // ====================================================================
    // OpenTypeGdefView
    //
    // Lazy non-owning view of the OpenType GDEF table.
    //
    // Version 1.0:
    //
    //   uint16   majorVersion
    //   uint16   minorVersion
    //   Offset16 glyphClassDefOffset
    //   Offset16 attachListOffset
    //   Offset16 ligCaretListOffset
    //   Offset16 markAttachClassDefOffset
    //
    // Version 1.2 adds:
    //
    //   Offset16 markGlyphSetsDefOffset
    //
    // Version 1.3 adds:
    //
    //   Offset32 itemVarStoreOffset
    //
    // Child tables are interpreted lazily.
    // ====================================================================

    class OpenTypeGdefView
    {
    public:
        OpenTypeGdefView() noexcept = default;
        explicit OpenTypeGdefView(ByteSpan data) noexcept : fData(data) {}

        [[nodiscard]] bool isValid() const noexcept
        {
            Header header{};
            return readHeader(header);
        }

        explicit operator bool() const noexcept { return isValid(); }


        // ================================================================
        // Version
        // ================================================================

        [[nodiscard]] uint16_t majorVersion() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.majorVersion : 0;
        }

        [[nodiscard]] uint16_t minorVersion() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.minorVersion : 0;
        }


        // ================================================================
        // Header offsets
        // ================================================================

        [[nodiscard]] uint16_t glyphClassDefOffset() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.glyphClassDefOffset : 0;
        }

        [[nodiscard]] uint16_t attachListOffset() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.attachListOffset : 0;
        }

        [[nodiscard]] uint16_t ligCaretListOffset() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.ligCaretListOffset : 0;
        }

        [[nodiscard]] uint16_t markAttachClassDefOffset() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.markAttachClassDefOffset : 0;
        }

        [[nodiscard]] uint16_t markGlyphSetsDefOffset() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.markGlyphSetsDefOffset : 0;
        }

        [[nodiscard]] uint32_t itemVarStoreOffset() const noexcept
        {
            Header header{};
            return readHeader(header) ? header.itemVarStoreOffset : 0;
        }


        // ================================================================
        // Class definitions
        // ================================================================

        [[nodiscard]] OpenTypeClassDefView glyphClassDef() const noexcept
        {
            Header header{};

            if (!readHeader(header) || header.glyphClassDefOffset == 0)
                return {};

            return OpenTypeClassDefView(subtable(header.glyphClassDefOffset));
        }

        [[nodiscard]] OpenTypeClassDefView markAttachClassDef() const noexcept
        {
            Header header{};

            if (!readHeader(header) || header.markAttachClassDefOffset == 0)
                return {};

            return OpenTypeClassDefView(subtable(header.markAttachClassDefOffset));
        }


        // ================================================================
        // Other GDEF children
        //
        // These remain raw ByteSpan views until their dedicated views are
        // implemented.
        // ================================================================

        [[nodiscard]] ByteSpan attachList() const noexcept
        {
            Header header{};

            if (!readHeader(header) || header.attachListOffset == 0)
                return {};

            return subtable(header.attachListOffset);
        }

        [[nodiscard]] ByteSpan ligCaretList() const noexcept
        {
            Header header{};

            if (!readHeader(header) || header.ligCaretListOffset == 0)
                return {};

            return subtable(header.ligCaretListOffset);
        }

        [[nodiscard]] OpenTypeMarkGlyphSetsView markGlyphSetsDef() const noexcept
        {
            Header header{};

            if (!readHeader(header) || header.markGlyphSetsDefOffset == 0)
                return {};

            return OpenTypeMarkGlyphSetsView(subtable(header.markGlyphSetsDefOffset));
        }

        [[nodiscard]] ByteSpan itemVarStore() const noexcept
        {
            Header header{};

            if (!readHeader(header) || header.itemVarStoreOffset == 0)
                return {};

            return subtable(header.itemVarStoreOffset);
        }


        // ================================================================
        // Convenience class lookups
        //
        // Absence of the relevant ClassDef means class zero.
        // ================================================================

        [[nodiscard]] bool glyphClass(uint32_t glyphId, uint16_t& result) const noexcept
        {
            result = 0;

            Header header{};

            if (!readHeader(header) || glyphId > 0xFFFFu)
                return false;

            if (header.glyphClassDefOffset == 0)
                return true;

            const OpenTypeClassDefView classes =
                OpenTypeClassDefView(subtable(header.glyphClassDefOffset));

            if (!classes)
                return false;

            return classes.classValue(glyphId, result);
        }

        [[nodiscard]] bool markAttachClass(uint32_t glyphId, uint16_t& result) const noexcept
        {
            result = 0;

            Header header{};

            if (!readHeader(header) || glyphId > 0xFFFFu)
                return false;

            if (header.markAttachClassDefOffset == 0)
                return true;

            const OpenTypeClassDefView classes =
                OpenTypeClassDefView(subtable(header.markAttachClassDefOffset));

            if (!classes)
                return false;

            return classes.classValue(glyphId, result);
        }


    private:
        struct Header
        {
            uint16_t majorVersion{ 0 };
            uint16_t minorVersion{ 0 };

            uint16_t glyphClassDefOffset{ 0 };
            uint16_t attachListOffset{ 0 };
            uint16_t ligCaretListOffset{ 0 };
            uint16_t markAttachClassDefOffset{ 0 };

            uint16_t markGlyphSetsDefOffset{ 0 };
            uint32_t itemVarStoreOffset{ 0 };

            size_t headerSize{ 0 };
        };


        // ================================================================
        // readHeader
        //
        // Validate only GDEF header structure and child offset geometry.
        // Child table contents are not interpreted here.
        // ================================================================

        bool readHeader(Header& result) const noexcept
        {
            result = {};

            OpenTypeByteStream stream(fData);

            if (!stream.readUInt16(result.majorVersion) ||
                !stream.readUInt16(result.minorVersion))
            {
                return false;
            }

            if (result.majorVersion != 1)
                return false;

            if (result.minorVersion != 0 &&
                result.minorVersion != 2 &&
                result.minorVersion != 3)
            {
                return false;
            }


            if (!stream.readOffset16(result.glyphClassDefOffset) ||
                !stream.readOffset16(result.attachListOffset) ||
                !stream.readOffset16(result.ligCaretListOffset) ||
                !stream.readOffset16(result.markAttachClassDefOffset))
            {
                return false;
            }

            result.headerSize = 12;


            if (result.minorVersion >= 2)
            {
                if (!stream.readOffset16(result.markGlyphSetsDefOffset))
                    return false;

                result.headerSize = 14;
            }


            if (result.minorVersion >= 3)
            {
                if (!stream.readOffset32(result.itemVarStoreOffset))
                    return false;

                result.headerSize = 18;
            }


            if (!validOffset(result.glyphClassDefOffset, result.headerSize) ||
                !validOffset(result.attachListOffset, result.headerSize) ||
                !validOffset(result.ligCaretListOffset, result.headerSize) ||
                !validOffset(result.markAttachClassDefOffset, result.headerSize) ||
                !validOffset(result.markGlyphSetsDefOffset, result.headerSize) ||
                !validOffset(result.itemVarStoreOffset, result.headerSize))
            {
                return false;
            }

            return true;
        }


        // ================================================================
        // Offset validation
        //
        // Zero is the OpenType NULL offset.
        // ================================================================

        bool validOffset(uint32_t offset, size_t headerSize) const noexcept
        {
            if (offset == 0)
                return true;

            return offset >= headerSize && offset < fData.size();
        }


        // ================================================================
        // Child span
        //
        // Offsets in the GDEF header are relative to the beginning of GDEF.
        // ================================================================

        ByteSpan subtable(uint32_t offset) const noexcept
        {
            if (offset == 0 || offset >= fData.size())
                return {};

            OpenTypeByteStream stream(fData);
            auto child = stream.subStream(offset);

            if (!child.isValid() || child.empty())
                return {};

            return child.remainingData();
        }


    private:
        ByteSpan fData{};
    };

} // namespace waavs