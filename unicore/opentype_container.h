
// opentype_container.h
#pragma once

#include "font_face.h"
#include "opentype_bytestream.h"
#include "opentype_face.h"


namespace waavs {
    // opentype {

        // ============================================================
        // OpenTypeContainer
        //
        // Lazy, forward-only generator of FontFace objects from one
        // OpenType resource.
        //
        // A resource may contain:
        //
        //     TTF / OTF   -> one face
        //     TTC         -> multiple faces
        //
        // Generator contract:
        //
        //     bool operator()(FontFace& face);
        //
        // Each successful call produces the next FontFace.
        // Once exhausted, all subsequent calls return false.
        // ============================================================

        class OpenTypeContainer
        {
        private:
            SharedMemBuff fSource;
            FontName fSourceLocation{ nullptr };

            // For TTC traversal this remains positioned at the next
            // Offset32 entry in the TTC offset array.
            OpenTypeByteStream fStream;

            uint32_t fFacesRemaining{ 0 };

            bool fSingleFacePending{ false };
            bool fValid{ false };
            bool fExhausted{ false };


        public:
            OpenTypeContainer() = default;


            explicit OpenTypeContainer(
                const SharedMemBuff& source,
                FontName sourceLocation = nullptr) noexcept
                : fSource(source)
                , fSourceLocation(sourceLocation)
                , fStream(ByteSpan(
                    source.data(),
                    source.size()))
            {
                initialize();
            }


            // ========================================================
            // Generator contract
            // ========================================================

            bool operator()(FontFace& face)
            {
                face = {};

                if (!fValid || fExhausted)
                    return false;


                // ----------------------------------------------------
                // Ordinary TTF / OTF
                // ----------------------------------------------------

                if (fSingleFacePending)
                {
                    fSingleFacePending = false;
                    fExhausted = true;

                    return makeFace(
                        0,
                        face);
                }


                // ----------------------------------------------------
                // TTC
                // ----------------------------------------------------

                while (fFacesRemaining > 0)
                {
                    uint32_t faceOffset = 0;

                    if (!fStream.readOffset32(faceOffset))
                    {
                        fValid = false;
                        fExhausted = true;
                        return false;
                    }

                    --fFacesRemaining;

                    if (makeFace(
                        faceOffset,
                        face))
                    {
                        if (fFacesRemaining == 0)
                            fExhausted = true;

                        return true;
                    }

                    // A malformed face need not necessarily prevent us
                    // from trying the remaining faces in the collection.
                }


                fExhausted = true;
                return false;
            }


            // ========================================================
            // State
            // ========================================================

            bool isValid() const noexcept
            {
                return fValid;
            }


            bool exhausted() const noexcept
            {
                return fExhausted;
            }


            FontName sourceLocation() const noexcept
            {
                return fSourceLocation;
            }


        private:

            // ========================================================
            // Parse only enough of the container to establish
            // generator state.
            //
            // Individual FontFace objects are NOT created here.
            // ========================================================

            bool initialize() noexcept
            {
                if (fSource.size() < 4)
                    return false;


                Tag signature = 0;

                if (!fStream.readUInt32(signature))
                    return false;


                // ----------------------------------------------------
                // TrueType Collection
                // ----------------------------------------------------

                if (signature == TagConstants::TTCF)
                {
                    uint32_t version = 0;
                    uint32_t numFonts = 0;

                    if (!fStream.readUInt32(version))
                        return false;

                    if (!fStream.readUInt32(numFonts))
                        return false;


                    // TTC versions currently defined are:
                    //
                    //     0x00010000
                    //     0x00020000
                    //
                    if (version != 0x00010000 &&
                        version != 0x00020000)
                    {
                        return false;
                    }


                    if (numFonts == 0)
                        return false;


                    // The stream is now positioned immediately before:
                    //
                    //     Offset32 tableDirectoryOffsets[numFonts]
                    //
                    // Validate that the complete offset array is present.
                    if (numFonts >
                        fStream.remaining() / sizeof(uint32_t))
                    {
                        return false;
                    }


                    fFacesRemaining = numFonts;
                    fValid = true;

                    return true;
                }


                // ----------------------------------------------------
                // Single-face OpenType resource
                // ----------------------------------------------------

                if (!isSupportedFontContainer(signature))
                    return false;


                fSingleFacePending = true;
                fValid = true;

                return true;
            }


            // ========================================================
            // Construct one FontFace on demand.
            // ========================================================

            bool makeFace(size_t faceOffset, FontFace& face)
            {
                auto data = std::make_shared<OpenTypeFaceData>(
                        fSource,
                        faceOffset,
                        fSourceLocation);

                if (!data->isValid())
                    return false;

                face = FontFace( std::move(data));

                return true;
            }
        };

    //} // namespace opentype
} // namespace waavs


