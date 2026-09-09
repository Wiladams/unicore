// unicode_nfc_stream.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "unicode_canonical_decomposer.h"
#include "unicode_composition.h"
#include "unicode_database.h"
#include "unicode_scalar_stream.h"


namespace waavs
{
    // ========================================================================
    // UnicodeNfcStream
    //
    // Forward-only NFC normalization stream.
    //
    // Input:
    //
    //      UnicodeScalar
    //
    // Output:
    //
    //      UnicodeScalar
    //
    // Internally this performs the correlated NFC operations:
    //
    //      recursive canonical decomposition
    //          ->
    //      canonical combining-class ordering
    //          ->
    //      canonical composition
    //
    // Source provenance remains attached to each scalar throughout.
    //
    // Decomposition:
    //
    //      Every decomposed scalar inherits the source range of its parent.
    //
    // Ordering:
    //
    //      The complete scalar record moves with the code point.
    //
    // Composition:
    //
    //      The resulting scalar receives the union of the source ranges of
    //      the two participating scalars.
    //
    // The stream owns only transient normalization buffers. It owns no Unicode
    // database storage and does not own its source producer.
    //
    // Source must provide:
    //
    //      bool operator()(UnicodeScalar& out);
    //      TextStreamStatus status() const noexcept;
    //
    // operator() is intentionally not noexcept because normalization sequences
    // are unbounded and the transient vectors may allocate.
    // ========================================================================

    template<typename Source>
    class UnicodeNfcStream
    {
        struct NormalizationScalar
        {
            UnicodeScalar scalar;
            UnicodeCombiningClass ccc{ kUnicodeCombiningClassNotReordered };
        };


    public:
        UnicodeNfcStream(Source& source, const UnicodeDatabase& database) noexcept
            : mSource(source)
            , mDatabase(&database)
            , mDecomposer(database.decomposition())
            , mComposition(database.composition())
        {
            if (!database.valid() ||
                !database.hasDecomposition() ||
                !database.hasCombiningClass() ||
                !database.hasComposition() ||
                !mDecomposer.valid() ||
                !mComposition.valid())
            {
                mStatus = TextStreamStatus::InvalidInput;
            }
        }


        // ====================================================================
        // operator()
        //
        // Produce the next NFC scalar.
        // ====================================================================

        bool operator()(UnicodeScalar& out)
        {
            if (mStatus != TextStreamStatus::Ready)
                return false;


            // ---------------------------------------------------------------
            // First drain output already known to be final.
            // ---------------------------------------------------------------

            if (emitReady(out))
                return true;


            while (mStatus == TextStreamStatus::Ready)
            {
                // -----------------------------------------------------------
                // Upstream has already ended. All remaining normalization
                // output has been drained.
                // -----------------------------------------------------------

                if (mInputEnded)
                {
                    mStatus = TextStreamStatus::End;
                    return false;
                }


                UnicodeScalar input;


                if (mSource(input))
                {
                    if (!consumeInput(input))
                    {
                        mStatus = TextStreamStatus::InvalidInput;
                        return false;
                    }


                    if (emitReady(out))
                        return true;


                    continue;
                }


                // -----------------------------------------------------------
                // Source produced nothing. Its status explains why.
                // -----------------------------------------------------------

                const TextStreamStatus sourceStatus =
                    mSource.status();


                if (sourceStatus == TextStreamStatus::InvalidInput)
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return false;
                }


                if (sourceStatus != TextStreamStatus::End)
                {
                    // A synchronous producer returning false while still
                    // reporting Ready violates the stream contract.
                    mStatus = TextStreamStatus::InvalidInput;
                    return false;
                }


                // -----------------------------------------------------------
                // Clean upstream EOF.
                //
                // The pending normalization segment may still contain output.
                // -----------------------------------------------------------

                flushPending();

                mInputEnded = true;


                if (emitReady(out))
                    return true;


                mStatus = TextStreamStatus::End;
                return false;
            }


            return false;
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
                mDecomposer.valid() &&
                mComposition.valid();
        }


    private:
        Source& mSource;

        const UnicodeDatabase* mDatabase{ nullptr };

        UnicodeCanonicalDecomposer mDecomposer{};
        UnicodeComposition mComposition{};


        // ---------------------------------------------------------------
        // mPending
        //
        // Current normalization segment whose final NFC representation may
        // still be affected by subsequent input.
        //
        // Elements are kept in canonical combining-class order.
        // ---------------------------------------------------------------

        std::vector<NormalizationScalar> mPending;


        // ---------------------------------------------------------------
        // mReady
        //
        // Fully normalized scalars which are safe to emit.
        //
        // NormalizationScalar is retained here rather than converting to a
        // separate vector of UnicodeScalar solely to avoid another transient
        // representation and copy.
        // ---------------------------------------------------------------

        std::vector<NormalizationScalar> mReady;
        size_t mReadyIndex{ 0 };


        TextStreamStatus mStatus{ TextStreamStatus::Ready };
        bool mInputEnded{ false };


        // ====================================================================
        // consumeInput
        //
        // Fully canonically decompose one input scalar and feed every resulting
        // scalar into the incremental ordering/composition state.
        // ====================================================================

        bool consumeInput(const UnicodeScalar& input)
        {
            UnicodeCanonicalDecomposition decomposition;


            if (!mDecomposer.decompose(
                input.value,
                decomposition))
            {
                return false;
            }


            for (uint32_t i = 0; i < decomposition.count; ++i)
            {
                const uint32_t cp =
                    decomposition.codePoints[i];


                NormalizationScalar item;

                item.scalar.value = cp;
                item.scalar.source = input.source;

                item.ccc =
                    mDatabase->combiningClass(cp);


                consumeDecomposed(item);
            }


            return true;
        }


        // ====================================================================
        // consumeDecomposed
        //
        // Feed one fully decomposed scalar into the current normalization
        // segment.
        //
        // Non-starters are inserted directly into canonical CCC order.
        //
        // A CCC=0 scalar establishes an ordering boundary. Before deciding
        // whether that boundary is also an NFC output boundary, the previous
        // pending sequence is composed.
        //
        // This is important because all preceding non-starters may themselves
        // compose away. If that leaves only one starter, the new CCC=0 scalar
        // may still compose with it.
        // ====================================================================

        void consumeDecomposed(const NormalizationScalar& item)
        {
            if (item.ccc != kUnicodeCombiningClassNotReordered)
            {
                insertOrdered(item);
                return;
            }


            // ---------------------------------------------------------------
            // First scalar of a new stream/segment.
            // ---------------------------------------------------------------

            if (mPending.empty())
            {
                mPending.push_back(item);
                return;
            }


            // ---------------------------------------------------------------
            // CCC=0 is a canonical-ordering boundary.
            //
            // Everything preceding it is now known, so canonical composition
            // of the current pending segment is safe.
            // ---------------------------------------------------------------

            composePending();


            // ---------------------------------------------------------------
            // A CCC=0 scalar can still compose with the current starter when
            // no uncomposed scalar remains between them.
            //
            // Hangul L + V and LV + T are the important algorithmic examples.
            // Explicit composition pairs are handled as well.
            // ---------------------------------------------------------------

            if (mPending.size() == 1 &&
                mPending[0].ccc == kUnicodeCombiningClassNotReordered)
            {
                uint32_t composite = 0;


                if (composePair(
                    mPending[0].scalar.value,
                    item.scalar.value,
                    composite))
                {
                    mPending[0].scalar.value =
                        composite;

                    mPending[0].scalar.source =
                        sourceRangeUnion(
                            mPending[0].scalar.source,
                            item.scalar.source);

                    return;
                }
            }


            // ---------------------------------------------------------------
            // No composition crosses this boundary.
            //
            // The complete previous segment is now final.
            // ---------------------------------------------------------------

            movePendingToReady();

            mPending.push_back(item);
        }


        // ====================================================================
        // insertOrdered
        //
        // Insert one non-starter into canonical combining-class order.
        //
        // Equal CCC values retain their original relative order.
        //
        // A CCC=0 starter is never crossed.
        // ====================================================================

        void insertOrdered(const NormalizationScalar& item)
        {
            mPending.push_back(item);


            size_t index =
                mPending.size() - 1;


            while (index != 0)
            {
                const UnicodeCombiningClass previousClass =
                    mPending[index - 1].ccc;


                if (previousClass == kUnicodeCombiningClassNotReordered)
                    break;


                if (previousClass <= item.ccc)
                    break;


                mPending[index] =
                    mPending[index - 1];

                --index;
            }


            mPending[index] = item;
        }


        // ====================================================================
        // composePending
        //
        // Perform canonical composition in place over the already canonically
        // ordered pending sequence.
        //
        // The standard blocking rule is:
        //
        //      compose when
        //
        //          lastClass < currentClass
        //
        //      or
        //
        //          lastClass == 0
        //
        // When composition succeeds, lastClass is deliberately unchanged.
        // The composed-away scalar therefore does not block a subsequent
        // composition.
        // ====================================================================

        void composePending()
        {
            if (mPending.empty())
                return;


            size_t writeIndex = 0;
            size_t starterIndex = static_cast<size_t>(-1);

            UnicodeCombiningClass lastClass =
                kUnicodeCombiningClassNotReordered;


            for (size_t readIndex = 0;
                readIndex < mPending.size();
                ++readIndex)
            {
                const NormalizationScalar current =
                    mPending[readIndex];


                // -----------------------------------------------------------
                // First output scalar.
                // -----------------------------------------------------------

                if (writeIndex == 0)
                {
                    mPending[writeIndex] =
                        current;


                    if (current.ccc ==
                        kUnicodeCombiningClassNotReordered)
                    {
                        starterIndex = writeIndex;
                    }


                    ++writeIndex;

                    lastClass = current.ccc;

                    continue;
                }


                bool composed = false;


                // -----------------------------------------------------------
                // Attempt composition with the current starter.
                // -----------------------------------------------------------

                if (starterIndex != static_cast<size_t>(-1) &&
                    (lastClass < current.ccc ||
                        lastClass ==
                        kUnicodeCombiningClassNotReordered))
                {
                    uint32_t composite = 0;


                    if (composePair(
                        mPending[starterIndex].scalar.value,
                        current.scalar.value,
                        composite))
                    {
                        UnicodeScalar& starter =
                            mPending[starterIndex].scalar;


                        starter.value =
                            composite;

                        starter.source =
                            sourceRangeUnion(
                                starter.source,
                                current.scalar.source);


                        composed = true;
                    }
                }


                // -----------------------------------------------------------
                // A successfully composed scalar disappears from the stream.
                //
                // Critically, lastClass is not updated in this case.
                // -----------------------------------------------------------

                if (composed)
                    continue;


                // -----------------------------------------------------------
                // Scalar survives composition.
                // -----------------------------------------------------------

                if (current.ccc ==
                    kUnicodeCombiningClassNotReordered)
                {
                    starterIndex = writeIndex;
                }


                mPending[writeIndex++] =
                    current;

                lastClass =
                    current.ccc;
            }


            mPending.resize(writeIndex);
        }


        // ====================================================================
        // composePair
        //
        // Canonically compose one pair.
        //
        // Explicit non-Hangul pairs come from UnicodeComposition.
        // Hangul composition is algorithmic.
        // ====================================================================

        [[nodiscard]]
        bool composePair(
            uint32_t first,
            uint32_t second,
            uint32_t& outComposite) const noexcept
        {
            uint32_t composite = 0;


            if (composeHangul(
                first,
                second,
                composite))
            {
                outComposite = composite;
                return true;
            }


            composite =
                mComposition.composite(
                    first,
                    second);


            if (composite ==
                kUnicodeCompositionNone)
            {
                return false;
            }


            outComposite = composite;

            return true;
        }


        // ====================================================================
        // composeHangul
        //
        // Algorithmic Hangul canonical composition:
        //
        //      L  + V -> LV
        //      LV + T -> LVT
        //
        // TBase itself represents no trailing consonant and therefore does not
        // participate as T.
        // ====================================================================

        [[nodiscard]]
        static bool composeHangul(
            uint32_t first,
            uint32_t second,
            uint32_t& outComposite) noexcept
        {
            static constexpr uint32_t SBase = 0xAC00u;
            static constexpr uint32_t LBase = 0x1100u;
            static constexpr uint32_t VBase = 0x1161u;
            static constexpr uint32_t TBase = 0x11A7u;

            static constexpr uint32_t LCount = 19u;
            static constexpr uint32_t VCount = 21u;
            static constexpr uint32_t TCount = 28u;

            static constexpr uint32_t NCount =
                VCount * TCount;

            static constexpr uint32_t SCount =
                LCount * NCount;


            // ---------------------------------------------------------------
            // L + V -> LV
            // ---------------------------------------------------------------

            const uint32_t lIndex =
                first - LBase;

            const uint32_t vIndex =
                second - VBase;


            if (lIndex < LCount &&
                vIndex < VCount)
            {
                outComposite =
                    SBase +
                    (lIndex * VCount + vIndex) *
                    TCount;

                return true;
            }


            // ---------------------------------------------------------------
            // LV + T -> LVT
            // ---------------------------------------------------------------

            const uint32_t sIndex =
                first - SBase;

            const uint32_t tIndex =
                second - TBase;


            if (sIndex < SCount &&
                (sIndex % TCount) == 0 &&
                tIndex > 0 &&
                tIndex < TCount)
            {
                outComposite =
                    first + tIndex;

                return true;
            }


            return false;
        }


        // ====================================================================
        // flushPending
        //
        // End-of-input makes the current pending segment final.
        // ====================================================================

        void flushPending()
        {
            if (mPending.empty())
                return;

            composePending();
            movePendingToReady();
        }


        // ====================================================================
        // movePendingToReady
        //
        // Append the completed pending sequence to the ready queue.
        //
        // Usually mReady is empty here. Appending rather than swapping also
        // handles the uncommon case where one decomposition produces multiple
        // independently final normalization segments.
        // ====================================================================

        void movePendingToReady()
        {
            if (mPending.empty())
                return;


            mReady.insert(
                mReady.end(),
                mPending.begin(),
                mPending.end());


            mPending.clear();
        }


        // ====================================================================
        // emitReady
        // ====================================================================

        bool emitReady(UnicodeScalar& out)
        {
            if (mReadyIndex >= mReady.size())
            {
                mReady.clear();
                mReadyIndex = 0;

                return false;
            }


            out =
                mReady[mReadyIndex++].scalar;


            if (mReadyIndex == mReady.size())
            {
                mReady.clear();
                mReadyIndex = 0;
            }


            return true;
        }
    };

} // namespace waavs