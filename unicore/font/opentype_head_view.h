// opentype_head_view.h
#pragma once

#include "font_face_view.h"
#include "opentype_bytestream.h"


namespace waavs
{
    // ========================================================================
    // HeadView
    //
    // Structural view of the OpenType 'head' table.
    //
    // The view validates and retains the core values currently required by
    // higher-level font processing:
    //
    //     unitsPerEm
    //     indexToLocFormat
    //
    // Construction does not modify the underlying FontFaceView.
    // ========================================================================

    class HeadView
    {
    private:
        uint16_t fUnitsPerEm{ 0 };
        int16_t fIndexToLocFormat{ 0 };

        bool fValid{ false };


    public:
        HeadView() = default;


        explicit HeadView(const FontFaceView& face) noexcept
        {
            reset(face);
        }


        // ====================================================================
        // reset
        // ====================================================================

        bool reset(const FontFaceView& face) noexcept
        {
            fUnitsPerEm = 0;
            fIndexToLocFormat = 0;
            fValid = false;


            const TableRecord* table =
                face.getTable(TagConstants::HEAD);

            if (!table)
                return false;


            const ByteSpan& data = table->data;

            if (data.size() < 54)
                return false;


            OpenTypeByteStream stream(data);


            // ------------------------------------------------------------
            // magicNumber
            // ------------------------------------------------------------

            if (!stream.seek(12))
                return false;


            uint32_t magic = 0;

            if (!stream.readUInt32(magic) ||
                magic != 0x5F0F3CF5)
            {
                return false;
            }


            // ------------------------------------------------------------
            // flags
            // ------------------------------------------------------------

            uint16_t flags = 0;

            if (!stream.readUInt16(flags))
                return false;


            // ------------------------------------------------------------
            // unitsPerEm
            // ------------------------------------------------------------

            if (!stream.readUInt16(fUnitsPerEm))
                return false;

            if (fUnitsPerEm < 16 ||
                fUnitsPerEm > 16384)
            {
                fUnitsPerEm = 0;
                return false;
            }


            // ------------------------------------------------------------
            // indexToLocFormat
            // ------------------------------------------------------------

            if (!stream.seek(50))
            {
                fUnitsPerEm = 0;
                return false;
            }

            if (!stream.readInt16(fIndexToLocFormat))
            {
                fUnitsPerEm = 0;
                return false;
            }

            if (fIndexToLocFormat != 0 &&
                fIndexToLocFormat != 1)
            {
                fUnitsPerEm = 0;
                fIndexToLocFormat = 0;
                return false;
            }


            // ------------------------------------------------------------
            // glyphDataFormat
            //
            // OpenType currently requires this to be zero.
            // ------------------------------------------------------------

            int16_t glyphDataFormat = 0;

            if (!stream.readInt16(glyphDataFormat) ||
                glyphDataFormat != 0)
            {
                fUnitsPerEm = 0;
                fIndexToLocFormat = 0;
                return false;
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
        // Access
        // ====================================================================

        [[nodiscard]]
        uint16_t unitsPerEm() const noexcept
        {
            return fUnitsPerEm;
        }


        [[nodiscard]]
        int16_t indexToLocFormat() const noexcept
        {
            return fIndexToLocFormat;
        }
    };
}