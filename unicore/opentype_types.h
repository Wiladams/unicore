// opentype_types.h - OpenType font types
#pragma once

#include "lang_span.h"
#include "lang_memory.h"
#include "font_interfaces.h"

#include <vector>



namespace waavs {

        // ============================================================================
        // Basic OpenType Data Types
        // ============================================================================

        using Tag = uint32_t;
        using Offset = uint32_t;
        using Version = uint32_t;

        // ============================================================================
        // Table Tags (constant values for fast comparison)
        // ============================================================================
        // Construct a tag from a 4-character string literal (big-endian)
        // This one is used at compile time for constant tags, e.g., OTAG("cmap")
        consteval Tag OTAG(const char(&s)[5]) noexcept
        {
            return
                (static_cast<Tag>(static_cast<uint8_t>(s[0])) << 24) |
                (static_cast<Tag>(static_cast<uint8_t>(s[1])) << 16) |
                (static_cast<Tag>(static_cast<uint8_t>(s[2])) << 8) |
                static_cast<Tag>(static_cast<uint8_t>(s[3]));
        }

        // Construct a tag from a pointer to 4 bytes (big-endian)
        // This one is used at runtime when reading tags from a font file.
        // The pointer must point to at least 4 bytes of valid memory.
        constexpr Tag OSIG(const uint8_t* p) noexcept
        {
            return
                (static_cast<Tag>(p[0]) << 24) |
                (static_cast<Tag>(p[1]) << 16) |
                (static_cast<Tag>(p[2]) << 8) |
                static_cast<Tag>(p[3]);
        }

        constexpr bool isSupportedFontContainer(Tag signature) noexcept
        {
            return signature == 0x00010000 ||
                signature == OTAG("true") ||
                signature == OTAG("typ1") ||
                signature == OTAG("OTTO") ||
                signature == OTAG("ttcf");
        }



        namespace TagConstants {
            inline constexpr Tag CMAP = OTAG("cmap");
            inline constexpr Tag GLYF = OTAG("glyf");
            inline constexpr Tag HEAD = OTAG("head");
            inline constexpr Tag HHEA = OTAG("hhea");
            inline constexpr Tag HMTX = OTAG("hmtx");
            inline constexpr Tag LOCA = OTAG("loca");
            inline constexpr Tag MAXP = OTAG("maxp");
            inline constexpr Tag NAME = OTAG("name");
            inline constexpr Tag OS2 = OTAG("OS/2");
            inline constexpr Tag POST = OTAG("post");
            inline constexpr Tag KERN = OTAG("kern");

            inline constexpr Tag GDEF = OTAG("GDEF");
            inline constexpr Tag GSUB = OTAG("GSUB");
            inline constexpr Tag GPOS = OTAG("GPOS");

            inline constexpr Tag SVG = OTAG("SVG ");
            inline constexpr Tag COLR = OTAG("COLR");
            inline constexpr Tag CPAL = OTAG("CPAL");
            inline constexpr Tag CBDT = OTAG("CBDT");
            inline constexpr Tag CBLC = OTAG("CBLC");

            inline constexpr Tag TTCF = OTAG("ttcf");
        }


        // ============================================================================
        // Feature Flags (bitmask for quick feature detection)
        // ============================================================================

        namespace FeatureFlags {
            inline constexpr uint32_t SVG = 1u << 0;
            inline constexpr uint32_t COLR = 1u << 1;
            inline constexpr uint32_t CPAL = 1u << 2;
            inline constexpr uint32_t CBDT = 1u << 3;
            inline constexpr uint32_t CBLC = 1u << 4;
            inline constexpr uint32_t GSUB = 1u << 5;
            inline constexpr uint32_t GPOS = 1u << 6;
            inline constexpr uint32_t KERN = 1u << 7;
            inline constexpr uint32_t COLOR = COLR | CBDT | CPAL | CBLC;
            inline constexpr uint32_t LAYOUT = GSUB | GPOS | KERN;
        }

        // ============================================================================
        // Table Record (with ByteSpan data view)
        // ============================================================================
        /*
        struct TableRecord {
            Tag tag{ 0 };              // numeric tag for fast comparison
            uint32_t checksum{ 0 };
            Offset offset{ 0 };
            uint32_t length{ 0 };
            ByteSpan data;                // cached view of table data
        };
        */

        struct IProvideOpenTypeTables
        {
            virtual ~IProvideOpenTypeTables() = default;

            virtual const TableRecord* getTable(Tag tag) const noexcept = 0;
            virtual bool hasTable(Tag tag) const noexcept = 0;
        };

        // ============================================================================
        // TTC (TrueType Collection) Header
        // ============================================================================

        struct TTCHeader {
            //const char* tag{ nullptr };        // "ttcf" (interned)
            uint32_t version{ 0 };              // 0x00010000 or 0x00020000
            uint32_t numFonts{ 0 };
            std::vector<uint32_t> offsets;    // Offsets to each font in the collection
        };

        // ============================================================================
        // CMAP Encoding Record
        // ============================================================================

        struct CmapEncoding {
            uint16_t platformId{ 0 };
            uint16_t encodingId{ 0 };
            uint32_t offset{ 0 };                // Offset to encoding subtable
            uint16_t format{ 0 };                // CMAP format (0, 4, 12, etc.)
            uint32_t length{ 0 };                // Length of encoding subtable
            uint32_t language{ 0 };              // Language for formats that have it
        };

        // ============================================================================
        // Name Record
        // ============================================================================

        struct NameRecord {
            uint16_t platformId{ 0 };
            uint16_t encodingId{ 0 };
            uint16_t languageId{ 0 };
            uint16_t nameId{ 0 };                // 1=Family, 2=Subfamily, 4=Full, etc.
            uint16_t length{ 0 };                // Length in bytes
            uint16_t offset{ 0 };                // Offset from start of name table
            const char* internedName{ nullptr }; // Cached interned string (from WSNameSet)
        };

        // ============================================================================
        // Common Name IDs (prefixed with NID_)
        // ============================================================================

        namespace NameIDs {
            inline constexpr uint16_t NID_COPYRIGHT = 0;
            inline constexpr uint16_t NID_FAMILY = 1;
            inline constexpr uint16_t NID_SUBFAMILY = 2;
            inline constexpr uint16_t NID_UNIQUE_ID = 3;
            inline constexpr uint16_t NID_FULL_NAME = 4;
            inline constexpr uint16_t NID_VERSION = 5;
            inline constexpr uint16_t NID_POSTSCRIPT = 6;
            inline constexpr uint16_t NID_TRADEMARK = 7;
            inline constexpr uint16_t NID_MANUFACTURER = 8;
            inline constexpr uint16_t NID_DESIGNER = 9;
            inline constexpr uint16_t NID_DESCRIPTION = 10;
            inline constexpr uint16_t NID_VENDOR_URL = 11;
            inline constexpr uint16_t NID_DESIGNER_URL = 12;
            inline constexpr uint16_t NID_LICENSE = 13;
            inline constexpr uint16_t NID_LICENSE_URL = 14;
            inline constexpr uint16_t NID_PREFERRED_FAMILY = 16;
            inline constexpr uint16_t NID_PREFERRED_SUBFAMILY = 17;
            inline constexpr uint16_t NID_COMPATIBLE_FULL = 18;
            inline constexpr uint16_t NID_SAMPLE_TEXT = 19;
            inline constexpr uint16_t NID_POSTSCRIPT_CID = 20;
            inline constexpr uint16_t NID_WWS_FAMILY = 21;
            inline constexpr uint16_t NID_WWS_SUBFAMILY = 22;
            inline constexpr uint16_t NID_LIGHT_BG_PALETTE = 23;
            inline constexpr uint16_t NID_DARK_BG_PALETTE = 24;
            inline constexpr uint16_t NID_VARIATIONS_POSTSCRIPT = 25;
        }

        // ============================================================================
        // Common Platform IDs (prefixed with PID_)
        // ============================================================================

        namespace PlatformIDs {
            inline constexpr uint16_t PID_UNICODE = 0;
            inline constexpr uint16_t PID_MACINTOSH = 1;
            inline constexpr uint16_t PID_ISO = 2;
            inline constexpr uint16_t PID_WINDOWS = 3;
            inline constexpr uint16_t PID_CUSTOM = 4;
        }

        // ============================================================================
        // Common Encoding IDs (prefixed with EID_)
        // ============================================================================

        namespace EncodingIDs {
            // Windows encodings
            namespace Windows {
                inline constexpr uint16_t EID_SYMBOL = 0;
                inline constexpr uint16_t EID_UNICODE_BMP = 1;
                inline constexpr uint16_t EID_SHIFT_JIS = 2;
                inline constexpr uint16_t EID_PRC = 3;
                inline constexpr uint16_t EID_BIG5 = 4;
                inline constexpr uint16_t EID_WANSUNG = 5;
                inline constexpr uint16_t EID_JOHAB = 6;
                inline constexpr uint16_t EID_UNICODE_FULL = 10;
            }

            // Unicode encodings
            namespace Unicode {
                inline constexpr uint16_t EID_DEFAULT = 0;
                inline constexpr uint16_t EID_V1_1 = 1;
                inline constexpr uint16_t EID_ISO_10646 = 2;
                inline constexpr uint16_t EID_V2_0_BMP = 3;
                inline constexpr uint16_t EID_V2_0_FULL = 4;
                inline constexpr uint16_t EID_V4_0 = 5;
                inline constexpr uint16_t EID_V5_0 = 6;
                inline constexpr uint16_t EID_V6_0 = 7;
                inline constexpr uint16_t EID_V7_0 = 8;
                inline constexpr uint16_t EID_V8_0 = 9;
                inline constexpr uint16_t EID_V9_0 = 10;
                inline constexpr uint16_t EID_V10_0 = 11;
                inline constexpr uint16_t EID_V11_0 = 12;
                inline constexpr uint16_t EID_V12_0 = 13;
                inline constexpr uint16_t EID_V13_0 = 14;
                inline constexpr uint16_t EID_V14_0 = 15;
                inline constexpr uint16_t EID_V15_0 = 16;
            }
        }

        // ============================================================================
        // Glyph Metrics
        // ============================================================================
        // Note: xMin, yMin, xMax, yMax can be negative, as they are in font units.  
        // advanceWidth and leftSideBearing are always positive.
        //
        struct GlyphMetrics {
            uint16_t advanceWidth{ };
            int16_t leftSideBearing{ };
            int16_t xMin{ };
            int16_t yMin{ };
            int16_t xMax{ };
            int16_t yMax{ };
        };

        // ============================================================================
        // SVG Table Support
        // ============================================================================

        struct OpenTypeSVGDocument {
            uint32_t startOffset{ 0 };
            uint32_t endOffset{ 0 };
            uint32_t startGlyph{ 0 };
            uint32_t endGlyph{ 0 };
            ByteSpan data;                    // Raw SVG data view
            bool isCompressed{ false };
            bool isBase64{ false };
            bool isParsed{ false };
            SharedMemBuff decodedData;        // Decoded/decompressed SVG data
        };

        struct OpenTypeSVGTable {
            uint16_t version{ 0 };
            uint32_t numEntries{ 0 };
            std::vector<OpenTypeSVGDocument> documents;
        };

        // ============================================================================
        // COLR Table Support
        // ============================================================================
        struct OpenTypeColorLayer
        {
            uint16_t glyphId{ 0 };
            uint16_t paletteIndex{ 0 };
        };

        struct OpenTypeColorGlyph
        {
            uint16_t glyphId{ 0 };
            uint16_t firstLayerIndex{ 0 };
            uint16_t layerCount{ 0 };
        };

        struct OpenTypeBaseGlyphPaintRecord
        {
            uint16_t glyphId{ 0 };

            // Relative to the beginning of BaseGlyphList.
            uint32_t paintOffset{ 0 };
        };

        struct OpenTypeCOLRTable
        {
            uint16_t version{ 0 };

            // COLR v0
            std::vector<OpenTypeColorGlyph> colorGlyphs;
            std::vector<OpenTypeColorLayer> layers;

            // COLR v1
            uint32_t baseGlyphListOffset{ 0 };
            uint32_t layerListOffset{ 0 };
            uint32_t clipListOffset{ 0 };
            uint32_t varIndexMapOffset{ 0 };
            uint32_t itemVariationStoreOffset{ 0 };

            std::vector<OpenTypeBaseGlyphPaintRecord>
                baseGlyphPaintRecords;

            // Offsets relative to beginning of LayerList.
            std::vector<uint32_t> layerPaintOffsets;
        };

        // ======================================
        // CMAP Table Support
        // ======================================
        struct OpenTypeCPALColor
        {
            uint8_t blue{ 0 };
            uint8_t green{ 0 };
            uint8_t red{ 0 };
            uint8_t alpha{ 0 };
        };

        struct OpenTypeCPALTable
        {
            uint16_t version{ 0 };
            uint16_t numPaletteEntries{ 0 };

            // Index into colors[] at which each palette begins.
            std::vector<uint16_t> paletteIndices;

            // Shared ColorRecord array.
            std::vector<OpenTypeCPALColor> colors;

            // ------------------------------------------------------------
            // CPAL v1 optional metadata
            // ------------------------------------------------------------

            // One uint32 flag value per palette.
            // Empty if paletteTypesArrayOffset == 0.
            std::vector<uint32_t> paletteTypes;

            // Name-table IDs, one per palette.
            // 0xFFFF means no label.
            std::vector<uint16_t> paletteLabels;

            // Name-table IDs, one per palette entry.
            // 0xFFFF means no label.
            std::vector<uint16_t> paletteEntryLabels;
        };


        
        
        // ============================================================================
        // Helper Functions for Key Generation
        // ============================================================================

        // actual key generators
        constexpr uint32_t makeCmapKey(uint16_t platformId, uint16_t encodingId) noexcept {
            return (static_cast<uint32_t>(platformId) << 16) | encodingId;
        }

        constexpr uint64_t makeFullNameKey(
            uint16_t platformId,
            uint16_t encodingId,
            uint16_t languageId,
            uint16_t nameId) noexcept
        {
            return
                (static_cast<uint64_t>(platformId) << 48) |
                (static_cast<uint64_t>(encodingId) << 32) |
                (static_cast<uint64_t>(languageId) << 16) |
                static_cast<uint64_t>(nameId);
        }

} // namespace waavs