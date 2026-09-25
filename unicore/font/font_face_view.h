// font_face_view.h
#pragma once

#include "font_interfaces.h"
#include "font_resource.h"
#include "opentype_bytestream.h"
#include "core_openhashmap.h"

namespace waavs
{
    class FontFaceView
    {
    private:
        FontResource fResource;
        size_t fFaceOffset{ 0 };

        WSOpenHashMap<Tag, TableRecord, WSHash32> fTables;

        bool fValid{ false };

    public:
        FontFaceView() = default;

        FontFaceView(FontResource resource, size_t faceOffset = 0) noexcept
            : fResource(std::move(resource))
            , fFaceOffset(faceOffset)
        {
            initialize();
        }

        [[nodiscard]] bool isValid() const noexcept
        {
            return fValid;
        }

        explicit operator bool() const noexcept
        {
            return isValid();
        }

        [[nodiscard]] const FontResource& resource() const noexcept
        {
            return fResource;
        }

        [[nodiscard]] FontName sourceLocation() const noexcept
        {
            return fResource.sourceLocation();
        }

        [[nodiscard]] size_t faceOffset() const noexcept
        {
            return fFaceOffset;
        }

        [[nodiscard]] const TableRecord* getTable(Tag tag) const noexcept
        {
            return fTables.getRef(tag);
        }

        [[nodiscard]] bool hasTable(Tag tag) const noexcept
        {
            return fTables.contains(tag);
        }

    private:

        bool initialize() noexcept
        {
            ByteSpan data = fResource.data();
            OpenTypeByteStream stream(data);

            // TTC table offsets remain relative to the complete resource.
            if (!stream.seek(fFaceOffset))
                return false;

            uint16_t numTables = 0;

            if (!parseOffsetTable(stream, numTables))
                return false;

            if (!parseTableDirectory(stream, numTables))
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


    };
}