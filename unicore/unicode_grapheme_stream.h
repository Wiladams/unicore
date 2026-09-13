// unicode_grapheme_stream.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "unicode_grapheme_property_stream.h"
#include "unicode_scalar_stream.h"


namespace waavs
{
    // ========================================================================
    // GraphemeClusterView
    //
    // Borrowed view of one extended grapheme cluster.
    //
    // The view refers to storage owned by GraphemeStream and remains valid
    // until the next call to GraphemeStream::operator().
    //
    // scalarCount refers to positions in the normalized scalar stream.
    // source is the provenance envelope in the original UTF-8 source.
    // ========================================================================

    struct GraphemeClusterView
    {
        const UnicodeScalar* scalars{ nullptr };
        uint32_t scalarCount{ 0 };

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};


        [[nodiscard]]
        bool empty() const noexcept {
            return scalarCount == 0;
        }


        explicit operator bool() const noexcept {
            return !empty();
        }


        [[nodiscard]]
        const UnicodeScalar* begin() const noexcept {
            return scalars;
        }


        [[nodiscard]]
        const UnicodeScalar* end() const noexcept {
            return scalars + scalarCount;
        }
    };


    // ========================================================================
    // GraphemeStream
    //
    // Forward-only Unicode extended grapheme-cluster segmenter.
    //
    // Input:
    //
    //      GraphemeScalar
    //
    // Output:
    //
    //      GraphemeClusterView
    //
    // The source has already performed all Unicode database lookups. This
    // stream therefore implements only the UAX #29 boundary state machine.
    //
    // The cluster buffer contains UnicodeScalar rather than GraphemeScalar.
    // GCB, InCB, and Extended_Pictographic are transient segmentation metadata
    // and disappear once their boundary decisions have been made.
    //
    // Source must provide:
    //
    //      bool operator()(GraphemeScalar& out);
    //      TextStreamStatus status() const noexcept;
    //
    // operator() is not noexcept because grapheme clusters have no finite
    // maximum length and the transient cluster vector may allocate.
    // ========================================================================

    template<typename Source>
    class GraphemeStream
    {
        enum class IndicState : uint8_t
        {
            None = 0,

            // Suffix matches:
            //
            //      InCB=Consonant
            //      followed by zero or more InCB=Extend
            //
            Consonant,

            // Suffix matches:
            //
            //      InCB=Consonant
            //      followed by Extend/Linker values
            //      and at least one Linker
            //
            Linked
        };


    public:
        explicit GraphemeStream(Source& source) noexcept
            : mSource(source)
        {}


        // ====================================================================
        // operator()
        //
        // Produce one complete extended grapheme cluster.
        //
        // The returned view remains valid until the next call.
        //
        // false means no cluster was produced. status() determines whether
        // this was clean End or InvalidInput.
        // ====================================================================

        bool operator()(GraphemeClusterView& out)
        {
            if (mStatus != TextStreamStatus::Ready)
                return false;


            // ---------------------------------------------------------------
            // The previous call emitted the final cluster after observing
            // upstream End.
            // ---------------------------------------------------------------

            if (mInputEnded)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }


            // ---------------------------------------------------------------
            // Reusing this buffer invalidates the previously returned view.
            // Capacity is retained for subsequent clusters.
            // ---------------------------------------------------------------

            mCluster.clear();

            mClusterSource = {};
            mClusterBegin = mNextScalarIndex;

            resetRuleState();


            // ---------------------------------------------------------------
            // Obtain the first scalar of the cluster.
            //
            // It may already be present as lookahead from the previous call.
            // ---------------------------------------------------------------

            GraphemeScalar current;


            if (mHasLookahead)
            {
                current = mLookahead;
                mHasLookahead = false;
            }
            else
            {
                if (!mSource(current))
                {
                    const TextStreamStatus sourceStatus =
                        mSource.status();


                    if (sourceStatus == TextStreamStatus::End)
                    {
                        mInputEnded = true;
                        mStatus = TextStreamStatus::End;

                        return false;
                    }


                    mStatus = TextStreamStatus::InvalidInput;

                    return false;
                }
            }


            append(current);


            // ---------------------------------------------------------------
            // Continue consuming until a grapheme boundary is found.
            // ---------------------------------------------------------------

            while (true)
            {
                GraphemeScalar next;


                if (!mSource(next))
                {
                    const TextStreamStatus sourceStatus =
                        mSource.status();


                    // -------------------------------------------------------
                    // Clean EOF.
                    //
                    // GB2 establishes a boundary at end-of-text, so the
                    // current cluster is complete and may be emitted.
                    // -------------------------------------------------------

                    if (sourceStatus == TextStreamStatus::End)
                    {
                        mInputEnded = true;

                        return emitCluster(out);
                    }


                    // -------------------------------------------------------
                    // Do not flush an incomplete cluster after malformed
                    // upstream input.
                    // -------------------------------------------------------

                    mCluster.clear();
                    mStatus = TextStreamStatus::InvalidInput;

                    return false;
                }


                // -----------------------------------------------------------
                // Boundary before next.
                //
                // Keep next as one-scalar lookahead. It becomes the first
                // scalar of the cluster produced by the following call.
                // -----------------------------------------------------------

                if (shouldBreakBefore(next))
                {
                    mLookahead = next;
                    mHasLookahead = true;

                    return emitCluster(out);
                }


                append(next);
            }
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


    private:
        Source& mSource;


        // ====================================================================
        // Cluster storage
        //
        // Only UnicodeScalar survives segmentation.
        // ====================================================================

        std::vector<UnicodeScalar> mCluster;

        ScalarIndex mClusterBegin{ 0 };
        ScalarIndex mNextScalarIndex{ 0 };

        SourceRange mClusterSource{};


        // ====================================================================
        // Lookahead
        // ====================================================================

        GraphemeScalar mLookahead{};
        bool mHasLookahead{ false };


        // ====================================================================
        // Stream state
        // ====================================================================

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
        bool mInputEnded{ false };


        // ====================================================================
        // Boundary-rule state
        //
        // This is all the history required by Unicode 17 extended grapheme
        // cluster rules.
        // ====================================================================

        UnicodeGraphemeClusterBreak mPreviousGcb{
            UnicodeGraphemeClusterBreak::Other
        };


        // Consecutive RI count ending at the previous scalar.
        uint32_t mRegionalIndicatorCount{ 0 };


        // GB11:
        //
        // mEmojiBase means the current suffix matches:
        //
        //      Extended_Pictographic Extend*
        //
        // mEmojiZwjReady means the current suffix matches:
        //
        //      Extended_Pictographic Extend* ZWJ
        //
        bool mEmojiBase{ false };
        bool mEmojiZwjReady{ false };


        // GB9c state.
        IndicState mIndicState{ IndicState::None };


        // ====================================================================
        // resetRuleState
        // ====================================================================

        void resetRuleState() noexcept
        {
            mPreviousGcb =
                UnicodeGraphemeClusterBreak::Other;

            mRegionalIndicatorCount = 0;

            mEmojiBase = false;
            mEmojiZwjReady = false;

            mIndicState = IndicState::None;
        }


        // ====================================================================
        // append
        //
        // Append one scalar to the current grapheme and update all boundary
        // state so it describes the suffix ending at this scalar.
        // ====================================================================

        void append(const GraphemeScalar& item)
        {
            if (mCluster.empty())
            {
                mClusterSource =
                    item.scalar.source;
            }
            else
            {
                mClusterSource =
                    sourceRangeUnion(
                        mClusterSource,
                        item.scalar.source);
            }


            mCluster.push_back(
                item.scalar);

            ++mNextScalarIndex;


            updateRegionalIndicatorState(item);
            updateEmojiState(item);
            updateIndicState(item);

            mPreviousGcb =
                item.gcb;
        }


        // ====================================================================
        // shouldBreakBefore
        //
        // Apply UAX #29 extended grapheme-cluster rules in specification order.
        //
        // false:
        //
        //      no boundary before current
        //
        // true:
        //
        //      boundary before current
        // ====================================================================

        [[nodiscard]]
        bool shouldBreakBefore(
            const GraphemeScalar& current) const noexcept
        {
            using GCB =
                UnicodeGraphemeClusterBreak;


            const GCB previous =
                mPreviousGcb;

            const GCB next =
                current.gcb;


            // ---------------------------------------------------------------
            // GB3
            //
            // CR x LF
            // ---------------------------------------------------------------

            if (previous == GCB::CR &&
                next == GCB::LF)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB4
            //
            // (Control | CR | LF) /
            // ---------------------------------------------------------------

            if (isControl(previous))
                return true;


            // ---------------------------------------------------------------
            // GB5
            //
            // / (Control | CR | LF)
            // ---------------------------------------------------------------

            if (isControl(next))
                return true;


            // ---------------------------------------------------------------
            // GB6
            //
            // L x (L | V | LV | LVT)
            // ---------------------------------------------------------------

            if (previous == GCB::L &&
                (next == GCB::L ||
                    next == GCB::V ||
                    next == GCB::LV ||
                    next == GCB::LVT))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB7
            //
            // (LV | V) x (V | T)
            // ---------------------------------------------------------------

            if ((previous == GCB::LV ||
                previous == GCB::V) &&
                (next == GCB::V ||
                    next == GCB::T))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB8
            //
            // (LVT | T) x T
            // ---------------------------------------------------------------

            if ((previous == GCB::LVT ||
                previous == GCB::T) &&
                next == GCB::T)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB9
            //
            // x (Extend | ZWJ)
            // ---------------------------------------------------------------

            if (next == GCB::Extend ||
                next == GCB::ZWJ)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB9a
            //
            // x SpacingMark
            // ---------------------------------------------------------------

            if (next == GCB::SpacingMark)
                return false;


            // ---------------------------------------------------------------
            // GB9b
            //
            // Prepend x
            // ---------------------------------------------------------------

            if (previous == GCB::Prepend)
                return false;


            // ---------------------------------------------------------------
            // GB9c
            //
            // InCB=Consonant
            //   [InCB=Extend | InCB=Linker]*
            //   InCB=Linker
            //   [InCB=Extend | InCB=Linker]*
            // x
            // InCB=Consonant
            //
            // mIndicState == Linked means exactly that the required left-hand
            // suffix has already been recognized.
            // ---------------------------------------------------------------

            if (current.incb ==
                UnicodeIndicConjunctBreak::Consonant &&
                mIndicState == IndicState::Linked)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB11
            //
            // Extended_Pictographic Extend* ZWJ
            // x
            // Extended_Pictographic
            // ---------------------------------------------------------------

            if (current.extendedPictographic &&
                mEmojiZwjReady)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB12 / GB13
            //
            // Do not break between RI characters when an odd number of
            // consecutive RI characters precede the boundary.
            // ---------------------------------------------------------------

            if (previous == GCB::RegionalIndicator &&
                next == GCB::RegionalIndicator &&
                (mRegionalIndicatorCount & 1u) != 0)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // GB999
            //
            // Otherwise break everywhere.
            // ---------------------------------------------------------------

            return true;
        }


        // ====================================================================
        // isControl
        // ====================================================================

        [[nodiscard]]
        static bool isControl(
            UnicodeGraphemeClusterBreak value) noexcept
        {
            using GCB =
                UnicodeGraphemeClusterBreak;


            return
                value == GCB::Control ||
                value == GCB::CR ||
                value == GCB::LF;
        }


        // ====================================================================
        // updateRegionalIndicatorState
        //
        // Track the RI run ending at the current scalar.
        // ====================================================================

        void updateRegionalIndicatorState(
            const GraphemeScalar& item) noexcept
        {
            if (item.gcb ==
                UnicodeGraphemeClusterBreak::RegionalIndicator)
            {
                ++mRegionalIndicatorCount;
            }
            else
            {
                mRegionalIndicatorCount = 0;
            }
        }


        // ====================================================================
        // updateEmojiState
        //
        // Recognize only the suffix required by GB11:
        //
        //      Extended_Pictographic Extend* ZWJ
        //
        // No backward scan of the cluster is required.
        // ====================================================================

        void updateEmojiState(
            const GraphemeScalar& item) noexcept
        {
            using GCB =
                UnicodeGraphemeClusterBreak;


            // ---------------------------------------------------------------
            // ZWJ completes the left side of GB11 only if the preceding suffix
            // was Extended_Pictographic Extend*.
            // ---------------------------------------------------------------

            if (item.gcb == GCB::ZWJ)
            {
                mEmojiZwjReady =
                    mEmojiBase;

                mEmojiBase = false;

                return;
            }


            // Any scalar after the ZWJ consumes that ready state.
            mEmojiZwjReady = false;


            // ---------------------------------------------------------------
            // A new Extended_Pictographic starts a new possible GB11 prefix.
            // ---------------------------------------------------------------

            if (item.extendedPictographic)
            {
                mEmojiBase = true;
                return;
            }


            // ---------------------------------------------------------------
            // Extend preserves:
            //
            //      Extended_Pictographic Extend*
            // ---------------------------------------------------------------

            if (item.gcb == GCB::Extend)
                return;


            mEmojiBase = false;
        }


        // ====================================================================
        // updateIndicState
        //
        // Incrementally recognize the left side of GB9c.
        // ====================================================================

        void updateIndicState(
            const GraphemeScalar& item) noexcept
        {
            using InCB =
                UnicodeIndicConjunctBreak;


            switch (item.incb)
            {
            case InCB::Consonant:
                mIndicState =
                    IndicState::Consonant;

                break;


            case InCB::Extend:

                // Extend preserves either recognized prefix.
                break;


            case InCB::Linker:

                // A Linker is useful only when preceded by an InCB Consonant
                // prefix. Once a Linker has been seen, additional Linkers and
                // Extends preserve the Linked state.

                if (mIndicState != IndicState::None)
                {
                    mIndicState =
                        IndicState::Linked;
                }

                break;


            case InCB::None:
            default:
                mIndicState =
                    IndicState::None;

                break;
            }
        }


        // ====================================================================
        // emitCluster
        // ====================================================================

        bool emitCluster(
            GraphemeClusterView& out) noexcept
        {
            if (mCluster.empty())
                return false;


            GraphemeClusterView result;

            result.scalars =
                mCluster.data();

            result.scalarCount =
                static_cast<uint32_t>(
                    mCluster.size());

            result.normalizedBegin =
                mClusterBegin;

            result.source =
                mClusterSource;


            out = result;

            return true;
        }
    };

} // namespace waavs

