// font_run_itemizer.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "font_interfaces.h"
#include "font_run.h"
#include "font_support.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // FontRunItemizer
    //
    // Assign an ordered font candidate list to one ShapingRunView.
    //
    // Candidate order is priority order.
    //
    // For every shaping cluster:
    //
    //      1. Search candidates from first to last.
    //      2. Select the first face which fully supports the cluster.
    //      3. If no face supports it, assign the first valid candidate and
    //         mark the resulting FontRunView as incomplete coverage.
    //
    // Adjacent clusters assigned to the same face are coalesced into one
    // FontRunView.
    //
    // Candidate selection restarts at candidate zero for every cluster.
    // This allows fallback to return immediately to a preferred font:
    //
    //      candidates: A, B
    //
    //      cluster:    0 1 2 3 4
    //      A:          + + - + +
    //      B:          + + + + +
    //
    //      result:     A A | B | A A
    //
    // ShapingCluster records in the emitted FontRunView are copied and
    // rebased so scalarOffset is relative to FontRunView::scalars.
    //
    // The returned FontRunView remains valid until the next call to
    // operator().
    //
    // The source ShapingRunView and candidate provider must remain valid for
    // the lifetime of this itemizer.
    // ========================================================================

    class FontRunItemizer
    {
    public:
        FontRunItemizer(const ShapingRunView& run,
            const IProvideFontFaces& candidates,
            const UnicodeDatabase& database)
            : mRun(run)
            , mCandidates(&candidates)
            , mDatabase(&database)
        {
            initialize();
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]] TextStreamStatus status() const noexcept
        {
            return mStatus;
        }


        [[nodiscard]] bool ready() const noexcept
        {
            return mStatus == TextStreamStatus::Ready;
        }


        [[nodiscard]] bool ended() const noexcept
        {
            return mStatus == TextStreamStatus::End;
        }


        [[nodiscard]] bool failed() const noexcept
        {
            return mStatus == TextStreamStatus::InvalidInput;
        }


        // ====================================================================
        // operator()
        //
        // Produce the next contiguous font run.
        //
        // false:
        //
        //      status() == End
        //          normal completion
        //
        //      status() == InvalidInput
        //          malformed shaping-run geometry, invalid database, or
        //          absence of any usable font candidate
        // ====================================================================

        bool operator()(FontRunView& out)
        {
            out = {};


            if (mStatus != TextStreamStatus::Ready)
                return false;


            // ---------------------------------------------------------------
            // Previous call emitted the final run.
            // ---------------------------------------------------------------

            if (mClusterIndex >= mRun.clusterCount)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }


            // ---------------------------------------------------------------
            // Reusing this storage invalidates the previously returned view.
            // ---------------------------------------------------------------

            mRunClusters.clear();


            // ---------------------------------------------------------------
            // Select the face for the first cluster of this run.
            // ---------------------------------------------------------------

            const uint32_t firstClusterIndex =
                mClusterIndex;

            const ShapingCluster& firstCluster =
                mRun.clusters[firstClusterIndex];


            FontFace runFace{};

            bool firstComplete = false;


            if (!selectFace(
                firstCluster,
                runFace,
                firstComplete))
            {
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            const uint32_t scalarBegin =
                firstCluster.scalarOffset;


            uint32_t scalarEnd =
                scalarBegin;

            bool completeCoverage =
                true;

            SourceRange sourceRange{};

            ScalarIndex normalizedBegin =
                firstCluster.normalizedBegin;


            // ---------------------------------------------------------------
            // Extend while successive clusters select the same face.
            //
            // Notice that candidate selection restarts from the beginning for
            // every cluster. We do not simply ask whether runFace supports the
            // next cluster because a higher-priority candidate may again
            // become usable.
            // ---------------------------------------------------------------

            while (mClusterIndex < mRun.clusterCount)
            {
                const ShapingCluster& cluster =
                    mRun.clusters[mClusterIndex];


                FontFace selectedFace{};

                bool clusterComplete = false;


                if (!selectFace(
                    cluster,
                    selectedFace,
                    clusterComplete))
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return false;
                }


                if (mClusterIndex != firstClusterIndex &&
                    selectedFace != runFace)
                {
                    break;
                }


                // -----------------------------------------------------------
                // Copy and rebase cluster geometry.
                // -----------------------------------------------------------

                ShapingCluster runCluster =
                    cluster;

                runCluster.scalarOffset -=
                    scalarBegin;

                mRunClusters.push_back(
                    runCluster);


                // -----------------------------------------------------------
                // Run metadata.
                // -----------------------------------------------------------

                completeCoverage =
                    completeCoverage &&
                    clusterComplete;


                extendSourceRange(
                    sourceRange,
                    cluster.source);


                scalarEnd =
                    cluster.scalarOffset +
                    cluster.scalarCount;


                ++mClusterIndex;
            }


            // ---------------------------------------------------------------
            // Emit borrowed run view.
            // ---------------------------------------------------------------

            out.scalars =
                mRun.scalars + scalarBegin;

            out.scalarCount =
                scalarEnd - scalarBegin;


            out.clusters =
                mRunClusters.empty()
                ? nullptr
                : mRunClusters.data();

            out.clusterCount =
                static_cast<uint32_t>(
                    mRunClusters.size());


            out.face =
                runFace;

            out.script =
                mRun.script;

            out.bidiLevel =
                mRun.bidiLevel;

            out.normalizedBegin =
                normalizedBegin;

            out.source =
                sourceRange;

            out.completeCoverage =
                completeCoverage;


            return true;
        }


    private:
        // ====================================================================
        // initialize
        // ====================================================================

        void initialize()
        {
            // ---------------------------------------------------------------
            // Database
            //
            // fontFallbackScalarRequiresGlyph() will eventually depend on
            // database properties, so require a valid database now.
            // ---------------------------------------------------------------

            if (!mDatabase ||
                !*mDatabase)
            {
                mStatus = TextStreamStatus::InvalidInput;
                return;
            }


            // ---------------------------------------------------------------
            // Empty run
            // ---------------------------------------------------------------

            if (mRun.scalarCount == 0 &&
                mRun.clusterCount == 0)
            {
                mStatus = TextStreamStatus::End;
                return;
            }


            // ---------------------------------------------------------------
            // Required storage
            // ---------------------------------------------------------------

            if (!mRun.scalars ||
                !mRun.clusters ||
                mRun.scalarCount == 0 ||
                mRun.clusterCount == 0)
            {
                mStatus = TextStreamStatus::InvalidInput;
                return;
            }


            // ---------------------------------------------------------------
            // Validate cluster partition.
            //
            // A ShapingRunView must be completely partitioned into contiguous
            // non-empty clusters.
            // ---------------------------------------------------------------

            uint32_t expectedScalarOffset =
                0;


            for (uint32_t i = 0;
                i < mRun.clusterCount;
                ++i)
            {
                const ShapingCluster& cluster =
                    mRun.clusters[i];


                if (cluster.scalarCount == 0)
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return;
                }


                if (cluster.scalarOffset !=
                    expectedScalarOffset)
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return;
                }


                if (cluster.scalarOffset >
                    mRun.scalarCount)
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return;
                }


                if (cluster.scalarCount >
                    mRun.scalarCount -
                    cluster.scalarOffset)
                {
                    mStatus = TextStreamStatus::InvalidInput;
                    return;
                }


                expectedScalarOffset +=
                    cluster.scalarCount;
            }


            if (expectedScalarOffset !=
                mRun.scalarCount)
            {
                mStatus = TextStreamStatus::InvalidInput;
                return;
            }


            // ---------------------------------------------------------------
            // Find the first valid candidate.
            //
            // This is also the face used when no candidate completely covers
            // a cluster.
            // ---------------------------------------------------------------

            if (!mCandidates)
            {
                mStatus = TextStreamStatus::InvalidInput;
                return;
            }


            const size_t candidateCount =
                mCandidates->fontFaceCount();


            for (size_t i = 0;
                i < candidateCount;
                ++i)
            {
                FontFace face =
                    mCandidates->fontFace(i);


                if (!face)
                    continue;


                mFallbackFace =
                    face;

                break;
            }


            if (!mFallbackFace)
            {
                // TextStreamStatus currently has no distinct font
                // configuration failure state.
                mStatus = TextStreamStatus::InvalidInput;
                return;
            }


            mRunClusters.reserve(
                mRun.clusterCount);
        }


        // ====================================================================
        // selectFace
        //
        // Search in candidate priority order.
        //
        // completeCoverage:
        //
        //      true
        //          one candidate fully supports the cluster
        //
        //      false
        //          no candidate fully supports it; use the first valid face
        //          so downstream shaping may produce .notdef
        // ====================================================================

        [[nodiscard]]
        bool selectFace(const ShapingCluster& cluster,
            FontFace& outFace,
            bool& completeCoverage) const noexcept
        {
            outFace = {};
            completeCoverage = false;


            if (!mCandidates ||
                !mDatabase)
            {
                return false;
            }


            const size_t candidateCount =
                mCandidates->fontFaceCount();


            for (size_t i = 0;
                i < candidateCount;
                ++i)
            {
                FontFace face =
                    mCandidates->fontFace(i);


                if (!face)
                    continue;


                if (!fontSupportsCluster(
                    face,
                    mRun,
                    cluster,
                    *mDatabase))
                {
                    continue;
                }


                outFace =
                    face;

                completeCoverage =
                    true;

                return true;
            }


            // ---------------------------------------------------------------
            // No complete candidate.
            //
            // Preserve text and assign the first valid face. This is not
            // malformed Unicode. The resulting run simply records incomplete
            // coverage.
            // ---------------------------------------------------------------

            outFace =
                mFallbackFace;

            completeCoverage =
                false;


            return bool(outFace);
        }


        // ====================================================================
        // extendSourceRange
        // ====================================================================

        static void extendSourceRange(SourceRange& destination,
            const SourceRange& source) noexcept
        {
            if (!source.valid())
                return;


            if (!destination.valid())
            {
                destination =
                    source;

                return;
            }


            if (source.begin <
                destination.begin)
            {
                destination.begin =
                    source.begin;
            }


            if (source.end >
                destination.end)
            {
                destination.end =
                    source.end;
            }
        }


    private:
        ShapingRunView mRun{};

        const IProvideFontFaces* mCandidates{ nullptr };
        const UnicodeDatabase* mDatabase{ nullptr };


        // First valid candidate. Used only when no candidate fully supports
        // a cluster.
        FontFace mFallbackFace{};


        // Rebased cluster metadata owned by the itemizer. Reused for every
        // emitted FontRunView.
        std::vector<ShapingCluster> mRunClusters{};


        uint32_t mClusterIndex{ 0 };

        TextStreamStatus mStatus{
            TextStreamStatus::Ready
        };
    };

} // namespace waavs