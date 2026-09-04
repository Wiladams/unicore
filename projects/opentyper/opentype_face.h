#pragma once

#include "core_openhashmap.h"
#include "core_utf8.h"
#include "font_interfaces.h"
#include "opentype_types.h"
#include "opentype_bytestream.h"
#include "unicode_coverage_storage.h"
#include "unicode_coverage_builder.h"


namespace waavs {
    namespace opentype {
        class OpenTypeFaceData final : public IProvideFontFaceData, public IProvideOpenTypeTables
        {
        private:
            SharedMemBuff fSource;
            FontName fSourceLocation{ nullptr };

            size_t fFaceOffset{ 0 };

            WSOpenHashMap<Tag, TableRecord, WSHash32> fTables;
            WSOpenHashMap<uint64_t, const char*, WSHash64> fNameStrings;
            WSOpenHashMap<uint32_t, CmapEncoding, WSHash32> fCmapEncodings;

            UnicodeCoverageStorage fUnicodeCoverageStorage{};
            uint32_t fGlyphCount{ 0 };
            uint16_t fUnitsPerEm{ 0 };

            FontFaceProperties fProperties{};
            bool fValid{ false };



        public:
            OpenTypeFaceData(
                const SharedMemBuff& source,
                size_t faceOffset,
                FontName srcLocation = nullptr) noexcept
                : fSource(source)
                , fFaceOffset(faceOffset)
                , fSourceLocation(srcLocation)
            {
                parse();
            }

            bool isValid() const noexcept
            {
                return fValid;
            }

            // IProvideFontFaceData
            FontName sourceLocation() const noexcept override
            {
                return fSourceLocation;
            }

            FontName familyName() const noexcept override
            {
                return getNameString(NameIDs::NID_FAMILY);
            }

            FontName subfamilyName() const noexcept override
            {
                return getNameString(NameIDs::NID_SUBFAMILY);
            }

            FontName fullName() const noexcept override
            {
                return getNameString(NameIDs::NID_FULL_NAME);
            }

            FontName postScriptName() const noexcept override
            {
                return getNameString(NameIDs::NID_POSTSCRIPT);
            }

            FontFaceProperties properties() const noexcept override
            {
                return fProperties;
            }

            uint32_t glyphCount() const noexcept override
            {
                return fGlyphCount;
            }

            uint16_t unitsPerEm() const noexcept override
            {
                return fUnitsPerEm;
            }

            uint32_t glyphIndex(uint32_t codepoint) const noexcept override
            {
                if (codepoint >= kUnicodeLimit)
                    return 0;


                // Prefer Windows full-Unicode cmap.
                uint32_t key = makeCmapKey(
                    PlatformIDs::PID_WINDOWS,
                    EncodingIDs::Windows::EID_UNICODE_FULL);

                const CmapEncoding* encoding = fCmapEncodings.getRef(key);

                if (encoding)
                {
                    const uint32_t glyph = glyphIndexForEncoding(codepoint, *encoding);

                    if (glyph != 0)
                        return glyph;
                }


                // Then Windows BMP cmap.
                key = makeCmapKey(
                    PlatformIDs::PID_WINDOWS,
                    EncodingIDs::Windows::EID_UNICODE_BMP);

                encoding =
                    fCmapEncodings.getRef(key);

                if (encoding)
                {
                    const uint32_t glyph =
                        glyphIndexForEncoding(codepoint, *encoding);

                    if (glyph != 0)
                        return glyph;
                }


                // Finally try Unicode-platform cmap subtables.
                //
                // A font may use any of several Unicode platform encoding IDs, so do
                // not restrict this fallback to encoding ID zero.
                uint32_t bestGlyph = 0;
                uint32_t bestScore = 0;
                uint16_t bestEncoding = 0;


                fCmapEncodings.forEach(
                    [&](uint32_t cmapKey, const CmapEncoding& candidate) noexcept
                    {
                        const uint16_t platformId =
                            static_cast<uint16_t>(cmapKey >> 16);

                        if (platformId != PlatformIDs::PID_UNICODE)
                            return;


                        const uint32_t score =
                            cmapLookupFormatScore(candidate.format);

                        if (score == 0)
                            return;


                        const uint32_t glyph =
                            glyphIndexForEncoding(codepoint, candidate);

                        if (glyph == 0)
                            return;


                        const uint16_t encodingId =
                            static_cast<uint16_t>(cmapKey & 0xFFFFu);


                        if (score > bestScore ||
                            (score == bestScore && encodingId > bestEncoding))
                        {
                            bestGlyph = glyph;
                            bestScore = score;
                            bestEncoding = encodingId;
                        }
                    });


                return bestGlyph;
            }

            const UnicodeCoverage& unicodeCoverage() const noexcept override
            {
                return fUnicodeCoverageStorage.coverage();
            }

            // OpenType-specific access

            const TableRecord* getTable(Tag tag) const noexcept override
            {
                return fTables.getRef(tag);
            }

            bool hasTable(Tag tag) const noexcept override
            {
                return fTables.contains(tag);
            }

        private:

            inline bool parse() noexcept
            {
                ByteSpan data(fSource.data(), fSource.size());
                OpenTypeByteStream stream(data);

                // Do NOT slice data at fFaceOffset.
                // TTC table offsets remain relative to the complete source.

                if (!stream.seek(fFaceOffset))
                    return false;

                uint16_t numTables = 0;

                if (!parseOffsetTable(stream, numTables))
                    return false;

                if (!parseTableDirectory(stream, numTables))
                    return false;

                if (!parseCoreTables())
                    return false;

                fValid = true;
                return true;
            }

            inline bool parseOffsetTable(OpenTypeByteStream& stream, uint16_t& numTables) noexcept
            {
                uint32_t version;

                if (!stream.readUInt32(version))
                    return false;


                if (!stream.readUInt16(numTables))
                    return false;

                if (numTables == 0)
                    return false;

                // searchRange, entrySelector, rangeShift
                if (!stream.skip(6))
                    return false;

                // Each table-directory entry is 16 bytes.
                if (numTables > stream.remaining() / 16)
                    return false;

                return true;
            }

            inline bool parseTableDirectory(OpenTypeByteStream& stream, uint16_t numTables) noexcept
            {
                fTables.reserve(numTables);

                for (uint16_t i = 0; i < numTables; ++i)
                {
                    Tag tag;
                    uint32_t checksum;
                    Offset offset;
                    uint32_t length;

                    if (!stream.readUInt32(tag))
                        return false;

                    if (!stream.readUInt32(checksum))
                        return false;

                    if (!stream.readOffset32(offset))
                        return false;

                    if (!stream.readUInt32(length))
                        return false;

                    // THIS is the critical TTC behavior:
                    //
                    // mStream's base is still the beginning of the entire file.
                    // Therefore the table offset is resolved against the correct
                    // file-level coordinate system.
                    auto tableData = stream.subStream(offset, length);

                    if (!tableData.isValid())
                        return false;

                    TableRecord record{};
                    record.tag = tag;
                    record.checksum = checksum;
                    record.offset = offset;
                    record.length = length;
                    record.data = tableData.remainingData();

                    fTables.put(tag, std::move(record));
                }

                return true;
            }

            inline bool parseCoreTables() noexcept {
                const TableRecord* maxpTable = getTable(TagConstants::MAXP);
                if (maxpTable) parseMaxpTable(*maxpTable);

                const TableRecord* nameTable = getTable(TagConstants::NAME);
                if (nameTable) parseNameTable(*nameTable);

                const TableRecord* cmapTable = getTable(TagConstants::CMAP);
                if (cmapTable && !parseCmapTable(*cmapTable))
                    return false;

                const TableRecord* headTable = getTable(TagConstants::HEAD);
                if (headTable && !parseHeadTable(*headTable))
                    return false;

                return true;
            }

            // ==============================================
            // parseMaxpTable
            // ==============================================

            inline bool parseMaxpTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;
                if (data.size() < 6)
                    return false;

                OpenTypeByteStream stream(data);
                stream.skip(4); // Skip version
                uint16_t numGlyphs;
                if (!stream.readUInt16(numGlyphs))
                    return false;
                fGlyphCount = numGlyphs;

                return true;
            }

            // ==============================================
            // parseHeadTable
            // ==============================================
            inline bool parseHeadTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;
                if (data.size() < 12)
                    return false;

                OpenTypeByteStream stream(data);
                if (!stream.skip(8)) // Skip version and revision
                    return false;

                uint32_t magic=0;

                if (!stream.readUInt32(magic)) 
                    return false;

                return magic == 0x5F0F3CF5;
            }

            // ==============================================
            // parseCmapTable - not an official table, but substantial
            // enough to warrant its own parsing function.
            // =============================================
            inline bool parseCmapCoverage(const OpenTypeByteStream& subtable, uint16_t format,
                UnicodeCoverageBuilder& coverage) noexcept
            {
                switch (format)
                {
                    // ============================================================
                    // Format 0
                    //
                    // uint16 format
                    // uint16 length
                    // uint16 language
                    // uint8  glyphIdArray[256]
                    // ============================================================
                case 0:
                {
                    auto glyphs = subtable.subStream(6);

                    if (!glyphs.isValid())
                        return false;

                    for (uint32_t cp = 0; cp < 256; ++cp)
                    {
                        uint8_t glyphId = 0;

                        if (!glyphs.readUInt8(glyphId))
                            return false;

                        if (glyphId != 0)
                            coverage.add(cp);
                    }

                    break;
                }

                // ============================================================
                // Format 4
                //
                // uint16 format
                // uint16 length
                // uint16 language
                // uint16 segCountX2
                // uint16 searchRange
                // uint16 entrySelector
                // uint16 rangeShift
                //
                // uint16 endCode[segCount]
                // uint16 reservedPad
                // uint16 startCode[segCount]
                // int16  idDelta[segCount]
                // uint16 idRangeOffset[segCount]
                // uint16 glyphIdArray[]
                // ============================================================
                case 4:
                {
                    auto header = subtable.subStream(6);

                    if (!header.isValid())
                        return false;

                    uint16_t segCountX2 = 0;

                    if (!header.readUInt16(segCountX2))
                        return false;

                    if ((segCountX2 & 1u) != 0)
                        return false;

                    const uint32_t segCount =
                        uint32_t(segCountX2) >> 1;

                    if (segCount == 0)
                        return false;

                    // Arrays begin after the 14-byte fixed header.
                    const size_t endCodeOffset = 14;

                    const size_t reservedPadOffset =
                        endCodeOffset +
                        size_t(segCount) * 2;

                    const size_t startCodeOffset =
                        reservedPadOffset + 2;

                    const size_t idDeltaOffset =
                        startCodeOffset +
                        size_t(segCount) * 2;

                    const size_t idRangeOffsetOffset =
                        idDeltaOffset +
                        size_t(segCount) * 2;

                    const size_t glyphArrayOffset =
                        idRangeOffsetOffset +
                        size_t(segCount) * 2;

                    // This validates that all four arrays fit.
                    auto glyphArray =
                        subtable.subStream(glyphArrayOffset);

                    if (!glyphArray.isValid())
                        return false;

                    auto endCodes =
                        subtable.subStream(endCodeOffset);

                    auto startCodes =
                        subtable.subStream(startCodeOffset);

                    auto deltas =
                        subtable.subStream(idDeltaOffset);

                    auto rangeOffsets =
                        subtable.subStream(idRangeOffsetOffset);

                    if (!endCodes.isValid() ||
                        !startCodes.isValid() ||
                        !deltas.isValid() ||
                        !rangeOffsets.isValid())
                        return false;

                    for (uint32_t i = 0; i < segCount; ++i)
                    {
                        uint16_t endCode = 0;
                        uint16_t startCode = 0;
                        uint16_t idDelta = 0;
                        uint16_t idRangeOffset = 0;

                        if (!endCodes.readUInt16(endCode))
                            return false;

                        if (!startCodes.readUInt16(startCode))
                            return false;

                        if (!deltas.readUInt16(idDelta))
                            return false;

                        if (!rangeOffsets.readUInt16(idRangeOffset))
                            return false;

                        if (startCode > endCode)
                            continue;

                        // Standard terminal segment.
                        if (startCode == 0xFFFFu &&
                            endCode == 0xFFFFu)
                            continue;

                        // ----------------------------------------------------
                        // idRangeOffset == 0
                        //
                        // glyph =
                        //   (codepoint + idDelta) mod 65536
                        //
                        // This means the entire segment is covered except
                        // possibly one codepoint whose result becomes glyph 0.
                        // ----------------------------------------------------
                        if (idRangeOffset == 0)
                        {
                            const uint16_t zeroCodePoint =
                                static_cast<uint16_t>(0u - idDelta);

                            if (zeroCodePoint < startCode ||
                                zeroCodePoint > endCode)
                            {
                                coverage.addRange(
                                    startCode,
                                    endCode);
                            }
                            else
                            {
                                if (startCode < zeroCodePoint)
                                {
                                    coverage.addRange(
                                        startCode,
                                        uint32_t(zeroCodePoint) - 1u);
                                }

                                if (zeroCodePoint < endCode)
                                {
                                    coverage.addRange(
                                        uint32_t(zeroCodePoint) + 1u,
                                        endCode);
                                }
                            }

                            continue;
                        }

                        // ----------------------------------------------------
                        // idRangeOffset != 0
                        //
                        // The offset is relative to the address of the
                        // particular idRangeOffset[i] word itself.
                        // ----------------------------------------------------

                        const size_t rangeOffsetWord =
                            idRangeOffsetOffset +
                            size_t(i) * 2;

                        for (uint32_t cp = startCode;
                            cp <= endCode;
                            ++cp)
                        {
                            const size_t glyphOffset =
                                rangeOffsetWord +
                                size_t(idRangeOffset) +
                                size_t(cp - startCode) * 2;

                            auto glyphStream =
                                subtable.subStream(glyphOffset);

                            if (!glyphStream.isValid())
                                break;

                            uint16_t glyphId = 0;

                            if (!glyphStream.readUInt16(glyphId))
                                break;

                            // A zero glyphIdArray entry remains zero.
                            // idDelta is only applied to a nonzero entry.
                            if (glyphId != 0)
                            {
                                glyphId =
                                    static_cast<uint16_t>(
                                        uint32_t(glyphId) +
                                        uint32_t(idDelta));

                                if (glyphId != 0)
                                    coverage.add(cp);
                            }

                            // Avoid wrap when endCode == 0xFFFF.
                            if (cp == 0xFFFFu)
                                break;
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 6
                //
                // uint16 format
                // uint16 length
                // uint16 language
                // uint16 firstCode
                // uint16 entryCount
                // uint16 glyphIdArray[entryCount]
                // ============================================================
                case 6:
                {
                    auto stream = subtable.subStream(6);

                    if (!stream.isValid())
                        return false;

                    uint16_t firstCode = 0;
                    uint16_t entryCount = 0;

                    if (!stream.readUInt16(firstCode))
                        return false;

                    if (!stream.readUInt16(entryCount))
                        return false;

                    if (entryCount > stream.remaining() / 2)
                        return false;

                    for (uint32_t i = 0; i < entryCount; ++i)
                    {
                        uint16_t glyphId = 0;

                        if (!stream.readUInt16(glyphId))
                            return false;

                        if (glyphId != 0)
                        {
                            coverage.add(
                                uint32_t(firstCode) + i);
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 10
                //
                // uint16 format
                // uint16 reserved
                // uint32 length
                // uint32 language
                // uint32 startCharCode
                // uint32 numChars
                // uint16 glyphIdArray[numChars]
                // ============================================================
                case 10:
                {
                    auto stream = subtable.subStream(12);

                    if (!stream.isValid())
                        return false;

                    uint32_t startCharCode = 0;
                    uint32_t numChars = 0;

                    if (!stream.readUInt32(startCharCode))
                        return false;

                    if (!stream.readUInt32(numChars))
                        return false;

                    if (numChars > stream.remaining() / 2)
                        return false;

                    for (uint32_t i = 0; i < numChars; ++i)
                    {
                        uint16_t glyphId = 0;

                        if (!stream.readUInt16(glyphId))
                            return false;

                        const uint64_t cp =
                            uint64_t(startCharCode) +
                            uint64_t(i);

                        if (cp >= kUnicodeLimit)
                            continue;

                        if (glyphId != 0)
                        {
                            coverage.add(static_cast<uint32_t>(cp));
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 12
                //
                // uint16 format
                // uint16 reserved
                // uint32 length
                // uint32 language
                // uint32 numGroups
                //
                // groups:
                //   uint32 startCharCode
                //   uint32 endCharCode
                //   uint32 startGlyphID
                // ============================================================
                case 12:
                {
                    auto stream = subtable.subStream(12);

                    if (!stream.isValid())
                        return false;

                    uint32_t numGroups = 0;

                    if (!stream.readUInt32(numGroups))
                        return false;

                    if (numGroups > stream.remaining() / 12)
                        return false;

                    for (uint32_t i = 0; i < numGroups; ++i)
                    {
                        uint32_t startCharCode = 0;
                        uint32_t endCharCode = 0;
                        uint32_t startGlyphId = 0;

                        if (!stream.readUInt32(startCharCode))
                            return false;

                        if (!stream.readUInt32(endCharCode))
                            return false;

                        if (!stream.readUInt32(startGlyphId))
                            return false;

                        if (startCharCode > endCharCode)
                            continue;

                        if (startCharCode >= kUnicodeLimit)
                            continue;

                        if (endCharCode >= kUnicodeLimit)
                        {
                            endCharCode = kUnicodeLimit - 1u;
                        }

                        // Format 12 increments the glyph ID for each codepoint.
                        //
                        // If startGlyphId == 0, only the first codepoint maps
                        // to .notdef; the remainder are valid mappings.
                        if (startGlyphId == 0)
                        {
                            if (startCharCode < endCharCode)
                            {
                                coverage.addRange(startCharCode + 1u, endCharCode);
                            }
                        }
                        else
                        {
                            coverage.addRange(startCharCode, endCharCode);
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 13
                //
                // uint16 format
                // uint16 reserved
                // uint32 length
                // uint32 language
                // uint32 numGroups
                //
                // groups:
                //   uint32 startCharCode
                //   uint32 endCharCode
                //   uint32 glyphID
                // ============================================================
                case 13:
                {
                    auto stream = subtable.subStream(12);

                    if (!stream.isValid())
                        return false;

                    uint32_t numGroups = 0;

                    if (!stream.readUInt32(numGroups))
                        return false;

                    if (numGroups > stream.remaining() / 12)
                        return false;

                    for (uint32_t i = 0; i < numGroups; ++i)
                    {
                        uint32_t startCharCode = 0;
                        uint32_t endCharCode = 0;
                        uint32_t glyphId = 0;

                        if (!stream.readUInt32(startCharCode))
                            return false;

                        if (!stream.readUInt32(endCharCode))
                            return false;

                        if (!stream.readUInt32(glyphId))
                            return false;

                        if (glyphId == 0)
                            continue;

                        if (startCharCode > endCharCode)
                            continue;

                        if (startCharCode >= kUnicodeLimit)
                            continue;

                        if (endCharCode >= kUnicodeLimit)
                        {
                            endCharCode = kUnicodeLimit - 1u;
                        }

                        coverage.addRange(startCharCode, endCharCode);
                    }

                    break;
                }

                // Format 14 represents variation-selector sequences rather than
                // standalone Unicode scalar coverage.
                //
                // Formats 2 and 8 can be added later if they prove useful for
                // fonts encountered in the wild.
                case 2:
                case 8:
                case 14:
                default:
                    return false;
                }

                return true;
            }

            // ==============================================
            // parseCmapTable
            // ==============================================

            inline bool parseCmapTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;

                if (data.size() < 4)
                    return false;

                OpenTypeByteStream stream(data);

                uint16_t version = 0;
                uint16_t numEncodings = 0;

                if (!stream.readUInt16(version))
                    return false;

                if (!stream.readUInt16(numEncodings))
                    return false;

                // cmap table version is always zero.
                if (version != 0)
                    return false;

                // Each EncodingRecord is exactly 8 bytes:
                //
                //   uint16   platformID
                //   uint16   encodingID
                //   Offset32 subtableOffset
                if (numEncodings > stream.remaining() / 8)
                    return false;

                fCmapEncodings.reserve(numEncodings);

                UnicodeCoverageBuilder coverageBuilder;

                for (uint16_t i = 0; i < numEncodings; ++i)
                {
                    CmapEncoding encoding{};

                    if (!stream.readUInt16(encoding.platformId))
                        return false;

                    if (!stream.readUInt16(encoding.encodingId))
                        return false;

                    if (!stream.readOffset32(encoding.offset))
                        return false;

                    // EncodingRecord.subtableOffset is relative to the
                    // beginning of the cmap table.
                    auto subtable = stream.subStream(encoding.offset);

                    if (!subtable.isValid())
                        continue;

                    if (!subtable.readUInt16(encoding.format))
                        continue;

                    size_t minimumHeaderSize = 0;

                    switch (encoding.format)
                    {
                        // --------------------------------------------------------
                        // 16-bit length cmap formats.
                        // --------------------------------------------------------
                    case 0:
                    case 2:
                    case 4:
                    case 6:
                    {
                        uint16_t length = 0;
                        uint16_t language = 0;

                        if (!subtable.readUInt16(length))
                            continue;

                        if (!subtable.readUInt16(language))
                            continue;

                        encoding.length = length;
                        encoding.language = language;

                        minimumHeaderSize = 6;
                        break;
                    }

                    // --------------------------------------------------------
                    // 32-bit length cmap formats.
                    // --------------------------------------------------------
                    case 8:
                    case 10:
                    case 12:
                    case 13:
                    {
                        uint16_t reserved = 0;

                        if (!subtable.readUInt16(reserved))
                            continue;

                        if (reserved != 0)
                            continue;

                        if (!subtable.readUInt32(encoding.length))
                            continue;

                        if (!subtable.readUInt32(encoding.language))
                            continue;

                        minimumHeaderSize = 12;
                        break;
                    }

                    // --------------------------------------------------------
                    // Format 14 has no language field.
                    // --------------------------------------------------------
                    case 14:
                    {
                        uint32_t numVarSelectorRecords = 0;

                        if (!subtable.readUInt32(encoding.length))
                            continue;

                        if (!subtable.readUInt32(
                            numVarSelectorRecords))
                            continue;

                        encoding.language = 0;

                        minimumHeaderSize = 10;
                        break;
                    }

                    default:
                        continue;
                    }

                    if (encoding.length < minimumHeaderSize)
                        continue;

                    // Create a bounded stream containing exactly this cmap subtable.
                    //
                    // Importantly, all subsequent coverage parsing is constrained
                    // to the subtable's declared length.
                    auto boundedSubtable = stream.subStream(encoding.offset, encoding.length);

                    if (!boundedSubtable.isValid())
                        continue;

                    // Accumulate Unicode coverage before moving the encoding.
                    parseCmapCoverage(boundedSubtable, encoding.format, coverageBuilder);

                    const uint32_t key = makeCmapKey(encoding.platformId, encoding.encodingId);

                    fCmapEncodings.put(key, std::move(encoding));
                }

                // Produce the compact immutable representation retained by the face.
                return fUnicodeCoverageStorage.build(coverageBuilder);
            }

            // ==============================================
            // parseNameTable
            // ==============================================
            inline void parseNameTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;
                if (data.size() < 6)
                    return;

                OpenTypeByteStream stream(data);

                uint16_t format;
                if (!stream.readUInt16(format))
                    return;

                uint16_t count;
                if (!stream.readUInt16(count))
                    return;

                uint16_t stringOffset;
                if (!stream.readOffset16(stringOffset))
                    return;

                // stringOffset is relative to the beginning of the 'name' table.
                auto stringStorage = stream.subStream(stringOffset);
                if (!stringStorage.isValid())
                    return;

                fNameStrings.reserve(count * 4);

                for (uint16_t i = 0; i < count; ++i)
                {
                    uint16_t platformId;
                    uint16_t encodingId;
                    uint16_t languageId;
                    uint16_t nameId;
                    uint16_t length;
                    uint16_t offset;

                    if (!stream.readUInt16(platformId)) break;
                    if (!stream.readUInt16(encodingId)) break;
                    if (!stream.readUInt16(languageId)) break;
                    if (!stream.readUInt16(nameId)) break;
                    if (!stream.readUInt16(length)) break;
                    if (!stream.readOffset16(offset)) break;

                    // NameRecord.offset is relative to the beginning
                    // of the string storage area.
                    auto nameData = stringStorage.subStream(offset, length);
                    if (!nameData.isValid())
                        continue;

                    ByteSpan nameSpan = nameData.remainingData();

                    const char* internedName = nullptr;

                    if (platformId == PlatformIDs::PID_WINDOWS &&
                        (encodingId == EncodingIDs::Windows::EID_UNICODE_BMP ||
                            encodingId == EncodingIDs::Windows::EID_UNICODE_FULL))
                    {
                        std::string utf8 = convertUtf16BeToUtf8(nameSpan);
                        internedName = WSNameSet::INTERN(utf8.c_str());
                    }
                    else if (platformId == PlatformIDs::PID_UNICODE)
                    {
                        std::string utf8 = convertUtf16BeToUtf8(nameSpan);
                        internedName = WSNameSet::INTERN(utf8.c_str());
                    }
                    else
                    {
                        internedName = WSNameSet::INTERN(nameSpan);
                    }

                    if (internedName)
                    {
                        auto key = makeFullNameKey(
                            platformId,
                            encodingId,
                            languageId,
                            nameId);

                        fNameStrings.put(key, internedName);
                    }
                }
            }

            // ========================================================================
            // Name Table Access
            // ========================================================================

            inline const char* getNameString(uint16_t nameId) const noexcept {
                // Predefined fallback order (most preferred first)
                struct Candidate {
                    uint16_t platform;
                    uint16_t encoding;
                    uint16_t language;
                };

                static const Candidate candidates[] = {
                    // 1. Windows Unicode BMP, English (United States)
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_BMP, 0x0409},
                    // 2. Windows Unicode BMP, Neutral (language 0)
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_BMP, 0x0000},
                    // 3. Windows Unicode Full, English
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_FULL, 0x0409},
                    // 4. Windows Unicode Full, Neutral
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_FULL, 0x0000},
                    // 5. Unicode platform (any encoding, usually default 0)
                    {PlatformIDs::PID_UNICODE, 0, 0},
                    // 6. Macintosh (MacRoman), English
                    {PlatformIDs::PID_MACINTOSH, 0, 0},
                    // 7. Any other platform (fallback) - we can scan all, but that's slow.
                    // We'll just return nullptr if none of the above match.
                };

                for (const auto& cand : candidates) {
                    uint64_t key = makeFullNameKey(cand.platform, cand.encoding, cand.language, nameId);
                    const char* const* result = fNameStrings.getRef(key);
                    if (result && *result) {
                        return *result;
                    }
                }

                // Final fallback: try to find any record with this nameId by scanning the map.
                // This is O(N) but only if no preferred record is found.
                const char* found = nullptr;
                fNameStrings.forEach([&](uint64_t key, const char* value)
                    {
                        if (!found)
                        {
                            uint16_t storedNameId = static_cast<uint16_t>(key & 0xFFFFu);
                            if (storedNameId == nameId) {
                                found = value;
                            }
                        }
                    });

                return found;
            }

            inline const char* getNameStringEx(uint16_t platformId, uint16_t encodingId, uint16_t languageId, uint16_t nameId) const noexcept {
                auto key = makeFullNameKey(platformId, encodingId, languageId, nameId);
                const char* const* result = fNameStrings.getRef(key);

                return result ? *result : nullptr;
            }

        private:
            // ========================================================================
            // cmap format 0
            // ========================================================================

            uint32_t glyphIndexFormat0(uint32_t codepoint, OpenTypeByteStream subtable) const noexcept
            {
                if (codepoint > 0xFFu)
                    return 0;


                uint16_t format = 0;
                uint16_t length = 0;
                uint16_t language = 0;


                if (!subtable.readUInt16(format) ||
                    !subtable.readUInt16(length) ||
                    !subtable.readUInt16(language))
                {
                    return 0;
                }


                if (format != 0 || length < 262)
                    return 0;


                auto glyph =
                    subtable.subStream(
                        6u + codepoint,
                        1);

                if (!glyph.isValid())
                    return 0;


                uint8_t glyphId = 0;

                if (!glyph.readUInt8(glyphId))
                    return 0;


                return glyphId;
            }
            
            // ========================================================================
// cmap format 4
// ========================================================================

            uint32_t glyphIndexFormat4(uint32_t codepoint,
                OpenTypeByteStream subtable) const noexcept
            {
                if (codepoint > 0xFFFFu)
                    return 0;


                auto header =
                    subtable.subStream(0, 14);

                if (!header.isValid())
                    return 0;


                uint16_t format = 0;
                uint16_t length = 0;
                uint16_t language = 0;
                uint16_t segCountX2 = 0;
                uint16_t searchRange = 0;
                uint16_t entrySelector = 0;
                uint16_t rangeShift = 0;


                if (!header.readUInt16(format) ||
                    !header.readUInt16(length) ||
                    !header.readUInt16(language) ||
                    !header.readUInt16(segCountX2) ||
                    !header.readUInt16(searchRange) ||
                    !header.readUInt16(entrySelector) ||
                    !header.readUInt16(rangeShift))
                {
                    return 0;
                }


                if (format != 4 ||
                    segCountX2 == 0 ||
                    (segCountX2 & 1u) != 0)
                {
                    return 0;
                }


                const size_t segCount =
                    size_t(segCountX2) >> 1;


                const size_t endCodeOffset = 14;

                const size_t reservedPadOffset =
                    endCodeOffset +
                    segCount * 2;

                const size_t startCodeOffset =
                    reservedPadOffset + 2;

                const size_t idDeltaOffset =
                    startCodeOffset +
                    segCount * 2;

                const size_t idRangeOffsetOffset =
                    idDeltaOffset +
                    segCount * 2;


                // Four arrays plus reservedPad must fit.
                if (subtable.remaining() < 16u)
                    return 0;

                if (segCount > (subtable.remaining() - 16u) / 8u)
                    return 0;


                auto endCodes =
                    subtable.subStream(
                        endCodeOffset,
                        segCount * 2);

                auto startCodes =
                    subtable.subStream(
                        startCodeOffset,
                        segCount * 2);

                auto idDeltas =
                    subtable.subStream(
                        idDeltaOffset,
                        segCount * 2);

                auto idRangeOffsets =
                    subtable.subStream(
                        idRangeOffsetOffset,
                        segCount * 2);


                if (!endCodes.isValid() ||
                    !startCodes.isValid() ||
                    !idDeltas.isValid() ||
                    !idRangeOffsets.isValid())
                {
                    return 0;
                }


                auto readUInt16At =
                    [](const OpenTypeByteStream& array,
                        size_t index, uint16_t& value) noexcept -> bool
                    {
                        auto item =
                            array.subStream(index * 2, 2);

                        return
                            item.isValid() &&
                            item.readUInt16(value);
                    };


                auto readInt16At =
                    [](const OpenTypeByteStream& array,
                        size_t index, int16_t& value) noexcept -> bool
                    {
                        auto item =
                            array.subStream(index * 2, 2);

                        return
                            item.isValid() &&
                            item.readInt16(value);
                    };


                // endCode[] is sorted. Find the first segment whose endCode is
                // greater than or equal to the requested code point.
                size_t left = 0;
                size_t right = segCount;


                while (left < right)
                {
                    const size_t middle =
                        left + (right - left) / 2;


                    uint16_t endCode = 0;

                    if (!readUInt16At(
                        endCodes,
                        middle,
                        endCode))
                    {
                        return 0;
                    }


                    if (codepoint > endCode)
                        left = middle + 1;
                    else
                        right = middle;
                }


                if (left >= segCount)
                    return 0;


                const size_t segment = left;


                uint16_t startCode = 0;
                uint16_t endCode = 0;
                int16_t idDelta = 0;
                uint16_t idRangeOffset = 0;


                if (!readUInt16At(startCodes, segment, startCode) ||
                    !readUInt16At(endCodes, segment, endCode) ||
                    !readInt16At(idDeltas, segment, idDelta) ||
                    !readUInt16At(idRangeOffsets, segment, idRangeOffset))
                {
                    return 0;
                }


                if (codepoint < startCode ||
                    codepoint > endCode)
                {
                    return 0;
                }


                // Simple mapping:
                //
                // glyph = (codepoint + idDelta) mod 65536
                if (idRangeOffset == 0)
                {
                    return static_cast<uint16_t>(
                        static_cast<int32_t>(codepoint) +
                        static_cast<int32_t>(idDelta));
                }


                // idRangeOffset is relative to the address of this particular
                // idRangeOffset word.
                const size_t rangeWordOffset =
                    idRangeOffsetOffset +
                    segment * 2;


                const size_t glyphOffset =
                    rangeWordOffset +
                    size_t(idRangeOffset) +
                    size_t(codepoint - startCode) * 2;


                auto glyphEntry =
                    subtable.subStream(
                        glyphOffset,
                        2);

                if (!glyphEntry.isValid())
                    return 0;


                uint16_t glyphId = 0;

                if (!glyphEntry.readUInt16(glyphId))
                    return 0;


                // Zero remains .notdef. idDelta is not applied.
                if (glyphId == 0)
                    return 0;


                return static_cast<uint16_t>(
                    static_cast<int32_t>(glyphId) +
                    static_cast<int32_t>(idDelta));
            }


            // ========================================================================
// cmap format 6
// ========================================================================

            uint32_t glyphIndexFormat6(uint32_t codepoint,
                OpenTypeByteStream subtable) const noexcept
            {
                if (codepoint > 0xFFFFu)
                    return 0;


                uint16_t format = 0;
                uint16_t length = 0;
                uint16_t language = 0;
                uint16_t firstCode = 0;
                uint16_t entryCount = 0;


                if (!subtable.readUInt16(format) ||
                    !subtable.readUInt16(length) ||
                    !subtable.readUInt16(language) ||
                    !subtable.readUInt16(firstCode) ||
                    !subtable.readUInt16(entryCount))
                {
                    return 0;
                }


                if (format != 6 ||
                    codepoint < firstCode)
                {
                    return 0;
                }


                const uint32_t index =
                    codepoint -
                    uint32_t(firstCode);


                if (index >= entryCount)
                    return 0;


                auto glyph =
                    subtable.subStream(
                        10u + size_t(index) * 2u,
                        2);

                if (!glyph.isValid())
                    return 0;


                uint16_t glyphId = 0;

                if (!glyph.readUInt16(glyphId))
                    return 0;


                return glyphId;
            }

            // ========================================================================
// cmap format 10
// ========================================================================

            uint32_t glyphIndexFormat10(uint32_t codepoint,
                OpenTypeByteStream subtable) const noexcept
            {
                uint16_t format = 0;
                uint16_t reserved = 0;
                uint32_t length = 0;
                uint32_t language = 0;
                uint32_t startCharCode = 0;
                uint32_t numChars = 0;


                if (!subtable.readUInt16(format) ||
                    !subtable.readUInt16(reserved) ||
                    !subtable.readUInt32(length) ||
                    !subtable.readUInt32(language) ||
                    !subtable.readUInt32(startCharCode) ||
                    !subtable.readUInt32(numChars))
                {
                    return 0;
                }


                if (format != 10 ||
                    reserved != 0 ||
                    codepoint < startCharCode)
                {
                    return 0;
                }


                const uint64_t index =
                    uint64_t(codepoint) -
                    uint64_t(startCharCode);


                if (index >= numChars)
                    return 0;


                auto glyph =
                    subtable.subStream(
                        20u + size_t(index) * 2u,
                        2);

                if (!glyph.isValid())
                    return 0;


                uint16_t glyphId = 0;

                if (!glyph.readUInt16(glyphId))
                    return 0;


                return glyphId;
            }

            // ========================================================================
// cmap format 12
// ========================================================================

            uint32_t glyphIndexFormat12(uint32_t codepoint,
                OpenTypeByteStream subtable) const noexcept
            {
                uint16_t format = 0;
                uint16_t reserved = 0;
                uint32_t length = 0;
                uint32_t language = 0;
                uint32_t numGroups = 0;


                if (!subtable.readUInt16(format) ||
                    !subtable.readUInt16(reserved) ||
                    !subtable.readUInt32(length) ||
                    !subtable.readUInt32(language) ||
                    !subtable.readUInt32(numGroups))
                {
                    return 0;
                }


                if (format != 12 ||
                    reserved != 0)
                {
                    return 0;
                }


                if (numGroups > subtable.remaining() / 12u)
                    return 0;


                auto groups =
                    subtable.subStream(16);

                if (!groups.isValid())
                    return 0;


                size_t left = 0;
                size_t right =
                    static_cast<size_t>(numGroups);


                while (left < right)
                {
                    const size_t middle =
                        left + (right - left) / 2;


                    auto group =
                        groups.subStream(
                            middle * 12u,
                            12);

                    if (!group.isValid())
                        return 0;


                    uint32_t startCharCode = 0;
                    uint32_t endCharCode = 0;
                    uint32_t startGlyphId = 0;


                    if (!group.readUInt32(startCharCode) ||
                        !group.readUInt32(endCharCode) ||
                        !group.readUInt32(startGlyphId))
                    {
                        return 0;
                    }


                    if (codepoint < startCharCode)
                    {
                        right = middle;
                    }
                    else if (codepoint > endCharCode)
                    {
                        left = middle + 1;
                    }
                    else
                    {
                        return
                            startGlyphId +
                            (codepoint - startCharCode);
                    }
                }


                return 0;
            }

            // ========================================================================
// cmap format 13
// ========================================================================

            uint32_t glyphIndexFormat13(uint32_t codepoint,
                OpenTypeByteStream subtable) const noexcept
            {
                uint16_t format = 0;
                uint16_t reserved = 0;
                uint32_t length = 0;
                uint32_t language = 0;
                uint32_t numGroups = 0;


                if (!subtable.readUInt16(format) ||
                    !subtable.readUInt16(reserved) ||
                    !subtable.readUInt32(length) ||
                    !subtable.readUInt32(language) ||
                    !subtable.readUInt32(numGroups))
                {
                    return 0;
                }


                if (format != 13 ||
                    reserved != 0)
                {
                    return 0;
                }


                if (numGroups > subtable.remaining() / 12u)
                    return 0;


                auto groups =
                    subtable.subStream(16);

                if (!groups.isValid())
                    return 0;


                size_t left = 0;
                size_t right =
                    static_cast<size_t>(numGroups);


                while (left < right)
                {
                    const size_t middle =
                        left + (right - left) / 2;


                    auto group =
                        groups.subStream(
                            middle * 12u,
                            12);

                    if (!group.isValid())
                        return 0;


                    uint32_t startCharCode = 0;
                    uint32_t endCharCode = 0;
                    uint32_t glyphId = 0;


                    if (!group.readUInt32(startCharCode) ||
                        !group.readUInt32(endCharCode) ||
                        !group.readUInt32(glyphId))
                    {
                        return 0;
                    }


                    if (codepoint < startCharCode)
                    {
                        right = middle;
                    }
                    else if (codepoint > endCharCode)
                    {
                        left = middle + 1;
                    }
                    else
                    {
                        return glyphId;
                    }
                }


                return 0;
            }


            // ========================================================================
// cmapLookupFormatScore
//
// Prefer the more capable cmap representations when several Unicode
// platform subtables are available.
//
// Format 14 is deliberately excluded. It maps Unicode variation sequences,
// not standalone Unicode scalars.
            // ========================================================================

            static uint32_t cmapLookupFormatScore(uint16_t format) noexcept
            {
                switch (format)
                {
                case 12: return 600;
                case 13: return 550;
                case 10: return 500;
                case 4:  return 400;
                case 6:  return 300;
                case 0:  return 200;
                default: return 0;
                }
            }


            // ========================================================================
            // glyphIndexForEncoding
            // ========================================================================

            uint32_t glyphIndexForEncoding(uint32_t codepoint,
                const CmapEncoding& encoding) const noexcept
            {
                const TableRecord* cmapTable =
                    getTable(TagConstants::CMAP);

                if (!cmapTable)
                    return 0;


                OpenTypeByteStream cmap(cmapTable->data);


                // EncodingRecord.offset is relative to the beginning of cmap.
                //
                // encoding.length was validated when parseCmapTable() stored the
                // encoding record.
                auto subtable =
                    cmap.subStream(
                        encoding.offset,
                        encoding.length);

                if (!subtable.isValid())
                    return 0;


                uint32_t glyph = 0;


                switch (encoding.format)
                {
                case 0:
                    glyph = glyphIndexFormat0(codepoint, subtable);
                    break;

                case 4:
                    glyph = glyphIndexFormat4(codepoint, subtable);
                    break;

                case 6:
                    glyph = glyphIndexFormat6(codepoint, subtable);
                    break;

                case 10:
                    glyph = glyphIndexFormat10(codepoint, subtable);
                    break;

                case 12:
                    glyph = glyphIndexFormat12(codepoint, subtable);
                    break;

                case 13:
                    glyph = glyphIndexFormat13(codepoint, subtable);
                    break;

                default:
                    return 0;
                }


                // maxp gives the authoritative glyph count when available.
                if (fGlyphCount != 0 && glyph >= fGlyphCount)
                    return 0;


                return glyph;
            }
        };


    }
}