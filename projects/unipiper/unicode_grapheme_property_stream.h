// unicode_grapheme_property_stream.h

#pragma once

#include <cstdint>

#include "unicode_database.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_indic_conjunct_break.h"
#include "unicode_scalar_stream.h"


namespace waavs
{
    // ========================================================================
    // GraphemeScalar
    //
    // Unicode scalar annotated with the properties required for extended
    // grapheme-cluster segmentation.
    //
    // These properties are transient pipeline metadata. They exist only to
    // support grapheme segmentation and do not need to survive beyond that
    // stage unless a later consumer explicitly wants them.
    // ========================================================================

    struct GraphemeScalar
    {
        UnicodeScalar scalar{};

        UnicodeGraphemeClusterBreak gcb{
            UnicodeGraphemeClusterBreak::Other
        };

        UnicodeIndicConjunctBreak incb{
            UnicodeIndicConjunctBreak::None
        };

        bool extendedPictographic{ false };
    };


    // ========================================================================
    // GraphemePropertyStream
    //
    // Forward-only Unicode grapheme-property annotation stream.
    //
    // Input:
    //
    //      UnicodeScalar
    //
    // Output:
    //
    //      GraphemeScalar
    //
    // For every input scalar this stage performs three immutable database
    // lookups:
    //
    //      Grapheme_Cluster_Break
    //      Indic_Conjunct_Break
    //      Extended_Pictographic
    //
    // There is no buffering and no transformation of the scalar value or
    // source provenance.
    //
    // Source must provide:
    //
    //      bool operator()(UnicodeScalar& out);
    //      TextStreamStatus status() const noexcept;
    //
    // ========================================================================

    template<typename Source>
    class GraphemePropertyStream
    {
    public:
        GraphemePropertyStream(Source& source, const UnicodeDatabase& database) noexcept
            : mSource(source)
            , mDatabase(&database)
        {
            if (!database.valid() ||
                !database.hasGraphemeClusterBreak() ||
                !database.hasIndicConjunctBreak() ||
                !database.hasExtendedPictographic())
            {
                mStatus = TextStreamStatus::InvalidInput;
            }
        }


        // ====================================================================
        // operator()
        //
        // Produce one annotated scalar.
        //
        // This stage has no internal buffering, so a clean upstream End becomes
        // an immediate End here.
        // ====================================================================

        bool operator()(GraphemeScalar& out)
        {
            if (mStatus != TextStreamStatus::Ready)
                return false;


            UnicodeScalar scalar;


            if (!mSource(scalar))
            {
                const TextStreamStatus sourceStatus =
                    mSource.status();


                if (sourceStatus == TextStreamStatus::End)
                {
                    mStatus = TextStreamStatus::End;
                    return false;
                }


                if (sourceStatus == TextStreamStatus::InvalidInput)
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return false;
                }


                // A synchronous producer returning false while still Ready
                // violates the stream convention.
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            GraphemeScalar result;

            result.scalar =
                scalar;

            result.gcb =
                mDatabase->graphemeClusterBreak(
                    scalar.value);

            result.incb =
                mDatabase->indicConjunctBreak(
                    scalar.value);

            result.extendedPictographic =
                mDatabase->isExtendedPictographic(
                    scalar.value);


            out = result;

            return true;
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        TextStreamStatus status() const noexcept {
            return mStatus;
        }


        [[nodiscard]]
        bool ready() const noexcept {
            return mStatus == TextStreamStatus::Ready;
        }


        [[nodiscard]]
        bool ended() const noexcept {
            return mStatus == TextStreamStatus::End;
        }


        [[nodiscard]]
        bool failed() const noexcept {
            return mStatus == TextStreamStatus::InvalidInput;
        }


        [[nodiscard]]
        bool valid() const noexcept
        {
            return
                mDatabase != nullptr &&
                mDatabase->valid() &&
                mDatabase->hasGraphemeClusterBreak() &&
                mDatabase->hasIndicConjunctBreak() &&
                mDatabase->hasExtendedPictographic();
        }


    private:
        Source& mSource;
        const UnicodeDatabase* mDatabase{ nullptr };

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
    };


    static_assert(
        sizeof(UnicodeGraphemeClusterBreak) == 1,
        "UnicodeGraphemeClusterBreak must remain one byte");

    static_assert(
        sizeof(UnicodeIndicConjunctBreak) == 1,
        "UnicodeIndicConjunctBreak must remain one byte");

} // namespace waavs
