// unicode_coverage_storage.h

#pragma once

#include <utility>
#include <vector>

#include "unicode_coverage.h"
#include "unicode_coverage_builder.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCoverageStorage
    //
    // Owning storage for a dynamically constructed UnicodeCoverage.
    //
    // UnicodeCoverage itself is only a non-owning view. This object owns:
    //
    //      UnicodeCoverageData
    //      UnicodeMasterPage pool
    //      UnicodeBitPage pool
    //
    // and binds a UnicodeCoverage view to that storage.
    //
    // ========================================================================

    struct UnicodeCoverageStorage
    {
    public:
        UnicodeCoverageStorage() noexcept
        {
            reset();
        }


        UnicodeCoverageStorage(const UnicodeCoverageStorage&) = delete;
        UnicodeCoverageStorage& operator=(const UnicodeCoverageStorage&) = delete;

        UnicodeCoverageStorage(UnicodeCoverageStorage&&) = delete;
        UnicodeCoverageStorage& operator=(UnicodeCoverageStorage&&) = delete;


        // ====================================================================
        // coverage
        // ====================================================================

        [[nodiscard]]
        const UnicodeCoverage& coverage() const noexcept
        {
            return mCoverage;
        }


        // ====================================================================
        // build
        //
        // Finalize a mutable coverage builder into storage owned by this
        // object.
        //
        // ====================================================================

        [[nodiscard]]
        bool build(const UnicodeCoverageBuilder& builder)
        {
            UnicodePagePoolBuilder pool;
            UnicodeCoverageData data{};


            if (!builder.finalize(pool, data))
                return false;


            std::vector<UnicodeMasterPage> masterPages = pool.masterPages();

            std::vector<UnicodeBitPage> bitPages = pool.bitPages();


            mData = data;
            mMasterPages = std::move(masterPages);
            mBitPages = std::move(bitPages);


            bind();


            return true;
        }


        // ====================================================================
        // reset
        // ====================================================================

        void reset() noexcept
        {
            for (uint32_t i = 0; i < kUnicodeMasterCount; ++i)
                mData.masters[i] = kUnicodePageEmpty;


            mMasterPages.clear();
            mBitPages.clear();


            bind();
        }


    private:
        // ====================================================================
        // bind
        //
        // Rebuild all non-owning pointers after the owned storage changes.
        // ====================================================================

        void bind() noexcept
        {
            mPools.masterPages =
                mMasterPages.empty()
                ? nullptr
                : mMasterPages.data();

            mPools.bitPages =
                mBitPages.empty()
                ? nullptr
                : mBitPages.data();


            mPools.masterPageCount =
                static_cast<uint32_t>(mMasterPages.size());

            mPools.bitPageCount = static_cast<uint32_t>(mBitPages.size());


            mCoverage = UnicodeCoverage(&mData, &mPools);
        }


    private:
        UnicodeCoverageData mData{};

        std::vector<UnicodeMasterPage> mMasterPages{};
        std::vector<UnicodeBitPage> mBitPages{};

        UnicodeCoveragePools mPools{};
        UnicodeCoverage mCoverage{};
    };
}