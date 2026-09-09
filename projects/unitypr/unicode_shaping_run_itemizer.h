// unicode_shaping_run_itemizer.h

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "unicode_bidi_analysis.h"
#include "unicode_database.h"

namespace waavs
{
    class ShapingRunItemizer
    {
    public:
        ShapingRunItemizer() noexcept = default;

        ShapingRunItemizer(const BidiParagraphView& paragraph,
            const UnicodeDatabase& database)
        {
            reset(paragraph, database);
        }

        [[nodiscard]] TextStreamStatus status() const noexcept { return mStatus; }
        [[nodiscard]] bool ready() const noexcept { return mStatus == TextStreamStatus::Ready; }
        [[nodiscard]] bool ended() const noexcept { return mStatus == TextStreamStatus::End; }
        [[nodiscard]] bool failed() const noexcept { return mStatus == TextStreamStatus::InvalidInput; }

        bool reset(const BidiParagraphView& paragraph,
            const UnicodeDatabase& database)
        {
            mParagraph = paragraph;
            mDatabase = &database;
            mNextCluster = 0;
            mRunClusters.clear();

            mCommonScript = findScript("Zyyy");
            mInheritedScript = findScript("Zinh");

            if (mCommonScript == kUnicodeScriptIndexInvalid ||
                mInheritedScript == kUnicodeScriptIndexInvalid ||
                !validateParagraph())
            {
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }

            mStatus = paragraph.empty()
                ? TextStreamStatus::End
                : TextStreamStatus::Ready;

            return true;
        }

        bool operator()(ShapingRunView& out)
        {
            out = {};

            if (mStatus != TextStreamStatus::Ready)
                return false;

            if (mNextCluster >= mParagraph.clusterCount)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }

            mRunClusters.clear();

            const uint32_t firstClusterIndex = mNextCluster;
            const ShapingCluster& firstCluster = mParagraph.clusters[firstClusterIndex];
            const UnicodeBidiLevel runLevel = clusterLevel(firstClusterIndex);
            const UnicodeScriptIndex runScript = startingRunScript(firstClusterIndex, runLevel);

            if (runScript == kUnicodeScriptIndexInvalid)
            {
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }

            const uint32_t runScalarOffset = firstCluster.scalarOffset;
            uint32_t runScalarEnd = firstCluster.scalarOffset;
            const ScalarIndex normalizedBegin = firstCluster.normalizedBegin;
            SourceRange source{};

            uint32_t clusterIndex = firstClusterIndex;

            while (clusterIndex < mParagraph.clusterCount)
            {
                const UnicodeBidiLevel level = clusterLevel(clusterIndex);

                if (level != runLevel)
                    break;

                const UnicodeScriptIndex script = mParagraph.scripts[clusterIndex].script;

                if (!isContextualScript(script))
                {
                    if (script != runScript)
                        break;

                    appendRunCluster(clusterIndex, runScalarOffset, runScalarEnd, source);
                    ++clusterIndex;
                    continue;
                }

                if (isContextualScript(runScript))
                {
                    if (script != runScript)
                        break;

                    appendRunCluster(clusterIndex, runScalarOffset, runScalarEnd, source);
                    ++clusterIndex;
                    continue;
                }

                const uint32_t spanEnd = contextualSpanEnd(clusterIndex, runLevel);
                const UnicodeScriptIndex rightScript = strongScriptAt(spanEnd, runLevel);

                if (rightScript != kUnicodeScriptIndexInvalid &&
                    rightScript != runScript)
                {
                    break;
                }

                for (uint32_t i = clusterIndex; i < spanEnd; ++i)
                    appendRunCluster(i, runScalarOffset, runScalarEnd, source);

                clusterIndex = spanEnd;
            }

            if (mRunClusters.empty())
            {
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }

            mNextCluster = clusterIndex;

            out.scalars = mParagraph.scalars + runScalarOffset;
            out.scalarCount = runScalarEnd - runScalarOffset;
            out.clusters = mRunClusters.data();
            out.clusterCount = static_cast<uint32_t>(mRunClusters.size());
            out.script = runScript;
            out.bidiLevel = runLevel;
            out.normalizedBegin = normalizedBegin;
            out.source = source;

            return true;
        }

    private:
        BidiParagraphView mParagraph{};
        const UnicodeDatabase* mDatabase{ nullptr };
        uint32_t mNextCluster{ 0 };
        std::vector<ShapingCluster> mRunClusters{};
        UnicodeScriptIndex mCommonScript{ kUnicodeScriptIndexInvalid };
        UnicodeScriptIndex mInheritedScript{ kUnicodeScriptIndexInvalid };
        TextStreamStatus mStatus{ TextStreamStatus::End };

        [[nodiscard]] UnicodeScriptIndex findScript(const char* iso15924) const noexcept
        {
            if (!mDatabase || !iso15924)
                return kUnicodeScriptIndexInvalid;

            for (uint32_t i = 0; i < mDatabase->scriptCount(); ++i)
            {
                InternedKey tag = mDatabase->scriptISO15924(i);

                if (tag && std::strcmp(tag, iso15924) == 0)
                    return static_cast<UnicodeScriptIndex>(i);
            }

            return kUnicodeScriptIndexInvalid;
        }

        [[nodiscard]] bool isContextualScript(UnicodeScriptIndex script) const noexcept
        {
            return script == mCommonScript || script == mInheritedScript;
        }

        [[nodiscard]] UnicodeBidiLevel clusterLevel(uint32_t clusterIndex) const noexcept
        {
            const ShapingCluster& cluster = mParagraph.clusters[clusterIndex];

            for (uint32_t i = 0; i < cluster.scalarCount; ++i)
            {
                const uint32_t scalarIndex =
                    cluster.scalarOffset + i;

                if (!isBidiRemovedByX9(
                    mParagraph.originalTypes[scalarIndex]))
                {
                    return mParagraph.levels[scalarIndex];
                }
            }

            // Entire cluster consists of X9-removed characters.
            // Preserve its existing bookkeeping level.
            return mParagraph.levels[cluster.scalarOffset];
        }

        [[nodiscard]] uint32_t contextualSpanEnd(uint32_t begin,
            UnicodeBidiLevel level) const noexcept
        {
            uint32_t index = begin;

            while (index < mParagraph.clusterCount)
            {
                if (clusterLevel(index) != level)
                    break;

                const UnicodeScriptIndex script = mParagraph.scripts[index].script;

                if (!isContextualScript(script))
                    break;

                ++index;
            }

            return index;
        }

        [[nodiscard]] UnicodeScriptIndex strongScriptBefore(uint32_t index,
            UnicodeBidiLevel level) const noexcept
        {
            while (index > 0)
            {
                --index;

                if (clusterLevel(index) != level)
                    return kUnicodeScriptIndexInvalid;

                const UnicodeScriptIndex script = mParagraph.scripts[index].script;

                if (!isContextualScript(script))
                    return script;
            }

            return kUnicodeScriptIndexInvalid;
        }

        [[nodiscard]] UnicodeScriptIndex strongScriptAt(uint32_t index,
            UnicodeBidiLevel level) const noexcept
        {
            if (index >= mParagraph.clusterCount)
                return kUnicodeScriptIndexInvalid;

            if (clusterLevel(index) != level)
                return kUnicodeScriptIndexInvalid;

            const UnicodeScriptIndex script = mParagraph.scripts[index].script;

            return isContextualScript(script)
                ? kUnicodeScriptIndexInvalid
                : script;
        }

        [[nodiscard]] UnicodeScriptIndex startingRunScript(uint32_t clusterIndex,
            UnicodeBidiLevel level) const noexcept
        {
            const UnicodeScriptIndex script = mParagraph.scripts[clusterIndex].script;

            if (!isContextualScript(script))
                return script;

            const uint32_t spanEnd = contextualSpanEnd(clusterIndex, level);
            const UnicodeScriptIndex leftScript = strongScriptBefore(clusterIndex, level);
            const UnicodeScriptIndex rightScript = strongScriptAt(spanEnd, level);

            if (leftScript != kUnicodeScriptIndexInvalid &&
                rightScript != kUnicodeScriptIndexInvalid)
            {
                if (leftScript == rightScript)
                    return leftScript;

                return script;
            }

            if (leftScript == kUnicodeScriptIndexInvalid &&
                rightScript != kUnicodeScriptIndexInvalid)
            {
                return rightScript;
            }

            if (leftScript != kUnicodeScriptIndexInvalid &&
                rightScript == kUnicodeScriptIndexInvalid)
            {
                return leftScript;
            }

            return script;
        }

        void appendRunCluster(uint32_t clusterIndex,
            uint32_t runScalarOffset,
            uint32_t& runScalarEnd,
            SourceRange& source)
        {
            const ShapingCluster& cluster = mParagraph.clusters[clusterIndex];

            ShapingCluster runCluster = cluster;
            runCluster.scalarOffset -= runScalarOffset;
            mRunClusters.push_back(runCluster);

            runScalarEnd = cluster.scalarOffset + cluster.scalarCount;
            extendSourceRange(source, cluster.source);
        }

        [[nodiscard]] bool validateParagraph() const noexcept
        {
            if (!mDatabase || !(*mDatabase))
                return false;

            if (mParagraph.scalarCount == 0)
                return mParagraph.clusterCount == 0;

            if (!mParagraph.scalars ||
                !mParagraph.originalTypes ||
                !mParagraph.levels ||
                !mParagraph.clusters ||
                !mParagraph.scripts ||
                mParagraph.clusterCount == 0)
            {
                return false;
            }

            uint32_t expectedScalarOffset = 0;

            for (uint32_t clusterIndex = 0;
                clusterIndex < mParagraph.clusterCount;
                ++clusterIndex)
            {
                const ShapingCluster& cluster = mParagraph.clusters[clusterIndex];
                const ScriptClusterInfo& script = mParagraph.scripts[clusterIndex];

                if (cluster.scalarCount == 0)
                    return false;

                if (cluster.scalarOffset != expectedScalarOffset)
                    return false;

                if (cluster.scalarOffset > mParagraph.scalarCount)
                    return false;

                if (cluster.scalarCount > mParagraph.scalarCount - cluster.scalarOffset)
                    return false;

                if (script.script == kUnicodeScriptIndexInvalid)
                    return false;

                const UnicodeBidiLevel level = clusterLevel(clusterIndex);

                for (uint32_t scalar = 0; scalar < cluster.scalarCount; ++scalar)
                {
                    const uint32_t scalarIndex = cluster.scalarOffset + scalar;

                    if (isBidiRemovedByX9(mParagraph.originalTypes[scalarIndex]))
                    {
                        continue;
                    }

                    if (mParagraph.levels[scalarIndex] != level)
                        return false;
                }

                /*
                const UnicodeBidiLevel level = mParagraph.levels[cluster.scalarOffset];

                for (uint32_t scalar = 1; scalar < cluster.scalarCount; ++scalar)
                {
                    if (mParagraph.levels[cluster.scalarOffset + scalar] != level)
                        return false;
                }
                */

                expectedScalarOffset += cluster.scalarCount;
            }

            return expectedScalarOffset == mParagraph.scalarCount;
        }

        static void extendSourceRange(SourceRange& destination,
            const SourceRange& source) noexcept
        {
            if (!source.valid())
                return;

            if (!destination.valid())
            {
                destination = source;
                return;
            }

            if (source.begin < destination.begin)
                destination.begin = source.begin;

            if (source.end > destination.end)
                destination.end = source.end;
        }
    };

} // namespace waavs
