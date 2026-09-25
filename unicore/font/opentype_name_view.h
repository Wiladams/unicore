// opentype_name_view.h
#pragma once

#include "core_nametable.h"
#include "core_openhashmap.h"
#include "core_utf8.h"
#include "font_face_view.h"
#include "opentype_bytestream.h"
#include "opentype_types.h"


namespace waavs
{
    // ========================================================================
    // NameView
    //
    // Parsed view of the OpenType 'name' table.
    //
    // Name strings are interned through WSNameSet and indexed by:
    //
    //     platformId
    //     encodingId
    //     languageId
    //     nameId
    //
    // The view preserves the existing preferred-name lookup policy while
    // allowing explicit access to individual name records.
    // ========================================================================

    class NameView
    {
    private:
        WSOpenHashMap<uint64_t, const char*, WSHash64> fNameStrings;
        bool fValid{ false };


    public:
        NameView() = default;


        explicit NameView(const FontFaceView& face) noexcept
        {
            reset(face);
        }


        // ====================================================================
        // reset
        // ====================================================================

        bool reset(const FontFaceView& face) noexcept
        {
            fNameStrings.clear();
            fValid = false;


            const TableRecord* table =
                face.getTable(TagConstants::NAME);

            if (!table)
                return false;


            const ByteSpan& data = table->data;

            if (data.size() < 6)
                return false;


            OpenTypeByteStream stream(data);


            uint16_t format = 0;

            if (!stream.readUInt16(format))
                return false;


            uint16_t count = 0;

            if (!stream.readUInt16(count))
                return false;


            uint16_t stringOffset = 0;

            if (!stream.readOffset16(stringOffset))
                return false;


            // stringOffset is relative to the beginning of the name table.
            auto stringStorage =
                stream.subStream(stringOffset);

            if (!stringStorage.isValid())
                return false;


            fNameStrings.reserve(count * 4);


            for (uint16_t i = 0; i < count; ++i)
            {
                uint16_t platformId = 0;
                uint16_t encodingId = 0;
                uint16_t languageId = 0;
                uint16_t nameId = 0;
                uint16_t length = 0;
                uint16_t offset = 0;


                if (!stream.readUInt16(platformId))
                    break;

                if (!stream.readUInt16(encodingId))
                    break;

                if (!stream.readUInt16(languageId))
                    break;

                if (!stream.readUInt16(nameId))
                    break;

                if (!stream.readUInt16(length))
                    break;

                if (!stream.readOffset16(offset))
                    break;


                // NameRecord.offset is relative to the beginning of the
                // string storage area.
                auto nameData = stringStorage.subStream(offset, length);

                if (!nameData.isValid())
                    continue;


                ByteSpan nameSpan = nameData.remainingData();

                const char* internedName = nullptr;


                if (platformId == PlatformIDs::PID_WINDOWS &&
                    (encodingId == EncodingIDs::Windows::EID_UNICODE_BMP ||
                        encodingId == EncodingIDs::Windows::EID_UNICODE_FULL))
                {
                    std::string utf8 =
                        convertUtf16BeToUtf8(nameSpan);

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
                    const uint64_t key =
                        makeFullNameKey(
                            platformId,
                            encodingId,
                            languageId,
                            nameId);

                    fNameStrings.put(key, internedName);
                }
            }


            fValid = true;
            return true;
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        bool isValid() const noexcept
        {
            return fValid;
        }


        explicit operator bool() const noexcept
        {
            return isValid();
        }


        // ====================================================================
        // Common font names
        // ====================================================================

        [[nodiscard]]
        FontName familyName() const noexcept
        {
            return getNameString(
                NameIDs::NID_FAMILY);
        }


        [[nodiscard]]
        FontName subfamilyName() const noexcept
        {
            return getNameString(
                NameIDs::NID_SUBFAMILY);
        }


        [[nodiscard]]
        FontName fullName() const noexcept
        {
            return getNameString(
                NameIDs::NID_FULL_NAME);
        }


        [[nodiscard]]
        FontName postScriptName() const noexcept
        {
            return getNameString(
                NameIDs::NID_POSTSCRIPT);
        }


        // ====================================================================
        // getNameString
        //
        // Return the preferred string for a NameID.
        // ====================================================================

        [[nodiscard]]
        const char* getNameString(uint16_t nameId) const noexcept
        {
            struct Candidate
            {
                uint16_t platform;
                uint16_t encoding;
                uint16_t language;
            };


            static const Candidate candidates[] =
            {
                // Windows Unicode BMP, English (United States).
                {
                    PlatformIDs::PID_WINDOWS,
                    EncodingIDs::Windows::EID_UNICODE_BMP,
                    0x0409
                },

                // Windows Unicode BMP, neutral.
                {
                    PlatformIDs::PID_WINDOWS,
                    EncodingIDs::Windows::EID_UNICODE_BMP,
                    0x0000
                },

                // Windows Unicode full, English.
                {
                    PlatformIDs::PID_WINDOWS,
                    EncodingIDs::Windows::EID_UNICODE_FULL,
                    0x0409
                },

                // Windows Unicode full, neutral.
                {
                    PlatformIDs::PID_WINDOWS,
                    EncodingIDs::Windows::EID_UNICODE_FULL,
                    0x0000
                },

                // Unicode platform.
                {
                    PlatformIDs::PID_UNICODE,
                    0,
                    0
                },

                // Macintosh MacRoman, English.
                {
                    PlatformIDs::PID_MACINTOSH,
                    0,
                    0
                }
            };


            for (const Candidate& candidate : candidates)
            {
                const uint64_t key =
                    makeFullNameKey(
                        candidate.platform,
                        candidate.encoding,
                        candidate.language,
                        nameId);

                const char* const* result =
                    fNameStrings.getRef(key);

                if (result && *result)
                    return *result;
            }


            // Final fallback: find any record for this NameID.
            const char* found = nullptr;

            fNameStrings.forEach(
                [&](uint64_t key, const char* value)
                {
                    if (found)
                        return;

                    const uint16_t storedNameId =
                        static_cast<uint16_t>(
                            key & 0xFFFFu);

                    if (storedNameId == nameId)
                        found = value;
                });


            return found;
        }


        // ====================================================================
        // getNameStringEx
        //
        // Exact platform / encoding / language / NameID lookup.
        // ====================================================================

        [[nodiscard]]
        const char* getNameStringEx(
            uint16_t platformId,
            uint16_t encodingId,
            uint16_t languageId,
            uint16_t nameId) const noexcept
        {
            const uint64_t key =
                makeFullNameKey(
                    platformId,
                    encodingId,
                    languageId,
                    nameId);

            const char* const* result =
                fNameStrings.getRef(key);

            return result
                ? *result
                : nullptr;
        }
    };
}