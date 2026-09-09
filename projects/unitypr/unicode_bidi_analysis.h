// unicode_bidi_analysis.h

#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>

#include "unicode_bidi_class.h"
#include "unicode_database.h"
#include "unicode_script_analysis.h"
#include "unicode_shaping_run.h"


namespace waavs
{
    // ========================================================================
    // Bidi levels
    // ========================================================================

    using UnicodeBidiLevel = uint8_t;

    static constexpr UnicodeBidiLevel kUnicodeBidiLevelInvalid = 0xFFu;
    static constexpr UnicodeBidiLevel kUnicodeBidiMaxDepth = 125u;


    // ========================================================================
    // UnicodeBidiParagraphDirection
    //
    // Auto:
    //
    //      Apply UAX #9 P2/P3.
    //
    // LeftToRight / RightToLeft:
    //
    //      Paragraph direction was supplied by a higher-level protocol.
    // ========================================================================

    enum class UnicodeBidiParagraphDirection : uint8_t
    {
        Auto = 0,
        LeftToRight,
        RightToLeft
    };

    enum class UnicodeBidiOverride : uint8_t
    {
        Neutral = 0,
        LeftToRight,
        RightToLeft
    };


    struct UnicodeBidiStatus
    {
        UnicodeBidiLevel level{ 0 };
        UnicodeBidiOverride overrideStatus{ UnicodeBidiOverride::Neutral };
        bool isolate{ false };
    };

    static constexpr uint32_t kBidiIndexInvalid = 0xFFFFFFFFu;


    // ========================================================================
    // BidiLevelRun
    //
    // A maximal contiguous range of post-X9 characters having the same
    // explicit embedding level.
    //
    // begin/end are positions in the post-X9 bidi index sequence, not scalar
    // indices in the original paragraph.
    // ========================================================================

    struct BidiLevelRun
    {
        uint32_t begin{ 0 };
        uint32_t end{ 0 };

        UnicodeBidiLevel level{ 0 };

        [[nodiscard]] bool empty() const noexcept
        {
            return begin == end;
        }

        [[nodiscard]] uint32_t size() const noexcept
        {
            return end - begin;
        }
    };


    // ========================================================================
    // BidiIsolatingRunSequence
    //
    // One UAX #9 BD13 isolating run sequence.
    //
    // The constituent level runs are stored in a parallel flattened array.
    // runOffset/runCount identify that range.
    // ========================================================================

    struct BidiIsolatingRunSequence
    {
        uint32_t runOffset{ 0 };
        uint32_t runCount{ 0 };

        UnicodeBidiLevel level{ 0 };

        UnicodeBidiClass sos{ UnicodeBidiClass::LeftToRight };
        UnicodeBidiClass eos{ UnicodeBidiClass::LeftToRight };
    };

    // ========================================================================
    // BidiBracketPair
    //
    // One BD16 paired-bracket match.
    //
    // Positions are positions in the post-X9 bidi sequence.
    // ========================================================================

    struct BidiBracketPair
    {
        uint32_t openPosition{ 0 };
        uint32_t closePosition{ 0 };
    };

    // ========================================================================
    // BidiBracketStrongTypes
    //
    // Strong-direction evidence found inside one N0 bracket pair.
    //
    // EN and AN contribute to rightToLeft, as required by N0.
    // ========================================================================

    struct BidiBracketStrongTypes
    {
        bool leftToRight{ false };
        bool rightToLeft{ false };

        [[nodiscard]] bool empty() const noexcept {
            return !leftToRight && !rightToLeft;
        }

        [[nodiscard]] bool mixed() const noexcept {
            return leftToRight && rightToLeft;
        }
    };


    // ========================================================================
    // ScriptClusterInfo
    //
    // Temporary Script information retained through bidi analysis for later
    // shaping-run itemization.
    //
    // This deliberately remains separate from ShapingCluster.
    // ========================================================================

    struct ScriptClusterInfo
    {
        UnicodeScriptSet candidates{};
        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };
    };


    // ========================================================================
    // BidiParagraphView
    //
    // Borrowed view of one complete bidi paragraph.
    //
    // Scalars remain in logical order.
    //
    // levels contains one bidi level for every scalar.
    //
    // clusters and scripts are parallel arrays:
    //
    //      clusters[i]
    //      scripts[i]
    //
    // The view remains valid until the next call to
    // UnicodeBidiStream::operator().
    // ========================================================================

    struct BidiParagraphView
    {
        const UnicodeScalar* scalars{ nullptr };
        const UnicodeBidiClass* originalTypes{ nullptr };
        const UnicodeBidiLevel* levels{ nullptr };
        uint32_t scalarCount{ 0 };

        const ShapingCluster* clusters{ nullptr };
        const ScriptClusterInfo* scripts{ nullptr };
        uint32_t clusterCount{ 0 };

        UnicodeBidiLevel paragraphLevel{ 0 };

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};


        [[nodiscard]] bool empty() const noexcept {
            return scalarCount == 0;
        }

        explicit operator bool() const noexcept {
            return !empty();
        }

        [[nodiscard]] bool rightToLeft() const noexcept {
            return (paragraphLevel & 1u) != 0;
        }

        [[nodiscard]] bool leftToRight() const noexcept {
            return !rightToLeft();
        }

        [[nodiscard]] const UnicodeScalar* begin() const noexcept {
            return scalars;
        }

        [[nodiscard]] const UnicodeScalar* end() const noexcept {
            return scalars + scalarCount;
        }
    };



    // ========================================================================
    // UAX #9 paragraph helpers
    // ========================================================================

    [[nodiscard]]
    static constexpr bool isBidiIsolateInitiator(
        UnicodeBidiClass type) noexcept
    {
        return
            type == UnicodeBidiClass::LeftToRightIsolate ||
            type == UnicodeBidiClass::RightToLeftIsolate ||
            type == UnicodeBidiClass::FirstStrongIsolate;
    }


    // ========================================================================
    // skipBidiIsolate
    //
    // Return the position of the PDI matching the isolate initiator at index.
    //
    // count is returned when no matching PDI exists.
    //
    // Matching follows the nesting behavior required by BD9/P2:
    //
    //      isolate initiator -> depth + 1
    //      PDI               -> depth - 1
    //
    // Other bidi formatting characters are irrelevant here.
    // ========================================================================

    [[nodiscard]]
    static inline uint32_t skipBidiIsolate(const UnicodeBidiClass* types,
        uint32_t count, uint32_t index) noexcept
    {
        if (!types ||
            index >= count ||
            !isBidiIsolateInitiator(types[index]))
        {
            return count;
        }

        uint32_t depth = 1;

        for (uint32_t i = index + 1u; i < count; ++i)
        {
            const UnicodeBidiClass type = types[i];

            if (isBidiIsolateInitiator(type))
            {
                ++depth;
                continue;
            }

            if (type != UnicodeBidiClass::PopDirectionalIsolate)
                continue;

            --depth;

            if (depth == 0)
                return i;
        }

        return count;
    }


    // ========================================================================
    // resolveBidiParagraphLevel
    //
    // UAX #9 P2/P3.
    //
    // Explicit higher-level paragraph direction bypasses P2/P3.
    //
    // Auto searches for the first L, R, or AL outside isolate contents.
    //
    // Embedding initiators themselves are naturally ignored because they are
    // not L, R, or AL, but their contents remain visible to P2.
    // ========================================================================

    [[nodiscard]]
    static inline UnicodeBidiLevel resolveBidiParagraphLevel(
        const UnicodeBidiClass* types, uint32_t count,
        UnicodeBidiParagraphDirection direction) noexcept
    {
        if (direction == UnicodeBidiParagraphDirection::LeftToRight)
            return 0;

        if (direction == UnicodeBidiParagraphDirection::RightToLeft)
            return 1;


        for (uint32_t i = 0; i < count; ++i)
        {
            const UnicodeBidiClass type = types[i];

            if (isBidiIsolateInitiator(type))
            {
                const uint32_t matchingPdi =
                    skipBidiIsolate(types, count, i);

                if (matchingPdi == count)
                    break;

                i = matchingPdi;
                continue;
            }


            if (type == UnicodeBidiClass::LeftToRight)
                return 0;

            if (type == UnicodeBidiClass::RightToLeft ||
                type == UnicodeBidiClass::ArabicLetter)
            {
                return 1;
            }
        }


        // P3 default when P2 finds no relevant strong character.

        return 0;
    }

    // ========================================================================
// nextOddBidiLevel
//
// Return the least odd embedding level greater than level.
//
// kUnicodeBidiLevelInvalid means that the required level exceeds the
// maximum explicit depth.
// ========================================================================

    [[nodiscard]]
    static constexpr UnicodeBidiLevel nextOddBidiLevel(
        UnicodeBidiLevel level) noexcept
    {
        const uint16_t next =
            static_cast<uint16_t>(level) +
            ((level & 1u) != 0 ? 2u : 1u);

        return next <= kUnicodeBidiMaxDepth
            ? static_cast<UnicodeBidiLevel>(next)
            : kUnicodeBidiLevelInvalid;
    }


    // ========================================================================
    // nextEvenBidiLevel
    //
    // Return the least even embedding level greater than level.
    // ========================================================================

    [[nodiscard]]
    static constexpr UnicodeBidiLevel nextEvenBidiLevel(
        UnicodeBidiLevel level) noexcept
    {
        const uint16_t next =
            static_cast<uint16_t>(level) +
            ((level & 1u) != 0 ? 1u : 2u);

        return next <= kUnicodeBidiMaxDepth
            ? static_cast<UnicodeBidiLevel>(next)
            : kUnicodeBidiLevelInvalid;
    }


    // ========================================================================
    // applyBidiOverride
    //
    // Apply the active directional override to one working bidi type.
    // ========================================================================

    static inline void applyBidiOverride(UnicodeBidiClass& type,
        UnicodeBidiOverride overrideStatus) noexcept
    {
        if (overrideStatus == UnicodeBidiOverride::LeftToRight)
            type = UnicodeBidiClass::LeftToRight;
        else if (overrideStatus == UnicodeBidiOverride::RightToLeft)
            type = UnicodeBidiClass::RightToLeft;
    }


    // ========================================================================
    // resolveExplicitBidiLevels
    //
    // Apply UAX #9 rules X1-X8 to one already-collected paragraph.
    //
    // types is intentionally mutable. It is now the algorithm's working bidi
    // type array rather than merely a cache of the original Bidi_Class values.
    // Directional overrides modify these types, and later W/N processing will
    // continue modifying the same working array.
    //
    // levels receives one explicit embedding level for every scalar.
    //
    // Formatting characters removed later by X9 are given a useful bookkeeping
    // level here even when X1-X8 do not require a persistent level for them.
    // X9 will prevent those values from affecting later implicit processing.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveExplicitBidiLevels(UnicodeBidiClass* types,
        UnicodeBidiLevel* levels, uint32_t count,
        UnicodeBidiLevel paragraphLevel) noexcept
    {
        if (paragraphLevel > kUnicodeBidiMaxDepth)
            return false;

        if (count == 0)
            return true;

        if (!types || !levels)
            return false;


        // --------------------------------------------------------------------
        // X1
        // --------------------------------------------------------------------

        std::array<
            UnicodeBidiStatus,
            static_cast<size_t>(kUnicodeBidiMaxDepth) + 2u> stack{};

        uint32_t stackSize = 1;

        stack[0] = UnicodeBidiStatus{
            paragraphLevel,
            UnicodeBidiOverride::Neutral,
            false
        };


        uint32_t overflowIsolateCount = 0;
        uint32_t overflowEmbeddingCount = 0;
        uint32_t validIsolateCount = 0;


        auto pushStatus =
            [&](UnicodeBidiLevel level,
                UnicodeBidiOverride overrideStatus,
                bool isolate) noexcept -> bool
            {
                if (stackSize >= stack.size())
                    return false;

                stack[stackSize++] = UnicodeBidiStatus{
                    level,
                    overrideStatus,
                    isolate
                };

                return true;
            };


        // --------------------------------------------------------------------
        // X2-X8
        // --------------------------------------------------------------------

        for (uint32_t i = 0; i < count; ++i)
        {
            UnicodeBidiClass& type = types[i];

            // Give every scalar a deterministic initial bookkeeping level.
            // Individual rules below replace this where required.
            levels[i] = stack[stackSize - 1u].level;


            switch (type)
            {
                // ================================================================
                // X2 - RLE
                // ================================================================

            case UnicodeBidiClass::RightToLeftEmbedding:
            {
                const UnicodeBidiLevel newLevel =
                    nextOddBidiLevel(
                        stack[stackSize - 1u].level);

                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::Neutral,
                        false))
                    {
                        return false;
                    }
                }
                else if (overflowIsolateCount == 0)
                {
                    ++overflowEmbeddingCount;
                }

                break;
            }


            // ================================================================
            // X3 - LRE
            // ================================================================

            case UnicodeBidiClass::LeftToRightEmbedding:
            {
                const UnicodeBidiLevel newLevel =
                    nextEvenBidiLevel(
                        stack[stackSize - 1u].level);

                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::Neutral,
                        false))
                    {
                        return false;
                    }
                }
                else if (overflowIsolateCount == 0)
                {
                    ++overflowEmbeddingCount;
                }

                break;
            }


            // ================================================================
            // X4 - RLO
            // ================================================================

            case UnicodeBidiClass::RightToLeftOverride:
            {
                const UnicodeBidiLevel newLevel =
                    nextOddBidiLevel(
                        stack[stackSize - 1u].level);

                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::RightToLeft,
                        false))
                    {
                        return false;
                    }
                }
                else if (overflowIsolateCount == 0)
                {
                    ++overflowEmbeddingCount;
                }

                break;
            }


            // ================================================================
            // X5 - LRO
            // ================================================================

            case UnicodeBidiClass::LeftToRightOverride:
            {
                const UnicodeBidiLevel newLevel =
                    nextEvenBidiLevel(
                        stack[stackSize - 1u].level);

                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::LeftToRight,
                        false))
                    {
                        return false;
                    }
                }
                else if (overflowIsolateCount == 0)
                {
                    ++overflowEmbeddingCount;
                }

                break;
            }


            // ================================================================
            // X5a - RLI
            // ================================================================

            case UnicodeBidiClass::RightToLeftIsolate:
            {
                const UnicodeBidiStatus outer =
                    stack[stackSize - 1u];

                levels[i] = outer.level;

                applyBidiOverride(
                    type,
                    outer.overrideStatus);


                const UnicodeBidiLevel newLevel =
                    nextOddBidiLevel(outer.level);

                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    ++validIsolateCount;

                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::Neutral,
                        true))
                    {
                        return false;
                    }
                }
                else
                {
                    ++overflowIsolateCount;
                }

                break;
            }


            // ================================================================
            // X5b - LRI
            // ================================================================

            case UnicodeBidiClass::LeftToRightIsolate:
            {
                const UnicodeBidiStatus outer =
                    stack[stackSize - 1u];

                levels[i] = outer.level;

                applyBidiOverride(
                    type,
                    outer.overrideStatus);


                const UnicodeBidiLevel newLevel =
                    nextEvenBidiLevel(outer.level);

                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    ++validIsolateCount;

                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::Neutral,
                        true))
                    {
                        return false;
                    }
                }
                else
                {
                    ++overflowIsolateCount;
                }

                break;
            }


            // ================================================================
            // X5c - FSI
            //
            // Determine the direction of the isolate contents using P2/P3,
            // then process the FSI as though it were RLI or LRI.
            // ================================================================

            case UnicodeBidiClass::FirstStrongIsolate:
            {
                const uint32_t matchingPdi =
                    skipBidiIsolate(
                        types,
                        count,
                        i);

                const uint32_t contentEnd =
                    matchingPdi < count
                    ? matchingPdi
                    : count;

                const uint32_t contentBegin =
                    i + 1u;

                const uint32_t contentCount =
                    contentEnd > contentBegin
                    ? contentEnd - contentBegin
                    : 0u;


                const UnicodeBidiLevel isolateDirection =
                    resolveBidiParagraphLevel(
                        contentCount != 0
                        ? types + contentBegin
                        : nullptr,
                        contentCount,
                        UnicodeBidiParagraphDirection::Auto);


                const bool rightToLeft =
                    isolateDirection == 1;


                const UnicodeBidiStatus outer =
                    stack[stackSize - 1u];

                levels[i] = outer.level;

                applyBidiOverride(
                    type,
                    outer.overrideStatus);


                const UnicodeBidiLevel newLevel =
                    rightToLeft
                    ? nextOddBidiLevel(outer.level)
                    : nextEvenBidiLevel(outer.level);


                if (newLevel != kUnicodeBidiLevelInvalid &&
                    overflowIsolateCount == 0 &&
                    overflowEmbeddingCount == 0)
                {
                    ++validIsolateCount;

                    if (!pushStatus(
                        newLevel,
                        UnicodeBidiOverride::Neutral,
                        true))
                    {
                        return false;
                    }
                }
                else
                {
                    ++overflowIsolateCount;
                }

                break;
            }


            // ================================================================
            // X6a - PDI
            // ================================================================

            case UnicodeBidiClass::PopDirectionalIsolate:
            {
                if (overflowIsolateCount != 0)
                {
                    --overflowIsolateCount;
                }
                else if (validIsolateCount != 0)
                {
                    overflowEmbeddingCount = 0;


                    while (stackSize > 1 &&
                        !stack[stackSize - 1u].isolate)
                    {
                        --stackSize;
                    }


                    if (stackSize <= 1 ||
                        !stack[stackSize - 1u].isolate)
                    {
                        return false;
                    }


                    --stackSize;
                    --validIsolateCount;
                }


                const UnicodeBidiStatus outer =
                    stack[stackSize - 1u];

                levels[i] = outer.level;

                applyBidiOverride(
                    type,
                    outer.overrideStatus);

                break;
            }


            // ================================================================
            // X7 - PDF
            // ================================================================

            case UnicodeBidiClass::PopDirectionalFormat:
            {
                if (overflowIsolateCount != 0)
                {
                    // PDF inside an overflow isolate is ignored.
                }
                else if (overflowEmbeddingCount != 0)
                {
                    --overflowEmbeddingCount;
                }
                else if (stackSize >= 2 &&
                    !stack[stackSize - 1u].isolate)
                {
                    --stackSize;
                }

                break;
            }


            // ================================================================
            // X8 - B
            // ================================================================

            case UnicodeBidiClass::ParagraphSeparator:
            {
                levels[i] = paragraphLevel;

                // P1 normally guarantees that B occurs at the end of this
                // paragraph. Resetting here also handles a CR/LF paragraph
                // separator represented by more than one B scalar.
                stackSize = 1;

                stack[0] = UnicodeBidiStatus{
                    paragraphLevel,
                    UnicodeBidiOverride::Neutral,
                    false
                };

                overflowIsolateCount = 0;
                overflowEmbeddingCount = 0;
                validIsolateCount = 0;

                break;
            }


            // ================================================================
            // BN
            //
            // X6 excludes BN. X9 will remove it from subsequent bidi
            // processing. The bookkeeping level assigned above is retained.
            // ================================================================

            case UnicodeBidiClass::BoundaryNeutral:
                break;


                // ================================================================
                // X6
                //
                // All remaining bidi types receive the current embedding level
                // and are changed to L or R when an override is active.
                // ================================================================

            default:
            {
                const UnicodeBidiStatus current = stack[stackSize - 1u];

                levels[i] = current.level;

                applyBidiOverride( type, current.overrideStatus);

                break;
            }
            }
        }


        return true;
    }

    // ========================================================================
// isBidiRemovedByX9
//
// UAX #9 X9 removes explicit embedding/override formatting characters,
// PDF, and BN from all subsequent implicit bidi processing.
//
// Isolate initiators and PDI are intentionally NOT removed.
// ========================================================================

    [[nodiscard]]
    static constexpr bool isBidiRemovedByX9(
        UnicodeBidiClass type) noexcept
    {
        switch (type)
        {
        case UnicodeBidiClass::RightToLeftEmbedding:
        case UnicodeBidiClass::LeftToRightEmbedding:
        case UnicodeBidiClass::RightToLeftOverride:
        case UnicodeBidiClass::LeftToRightOverride:
        case UnicodeBidiClass::PopDirectionalFormat:
        case UnicodeBidiClass::BoundaryNeutral:
            return true;

        default:
            return false;
        }
    }


    // ========================================================================
    // buildBidiX9Indices
    //
    // Build the scalar-index sequence used by all bidi processing after X9.
    //
    // The paragraph's actual scalar/type/level arrays are not modified or
    // compacted. Each entry in indices is an index into those original arrays.
    //
    // This preserves:
    //
    //      scalar storage
    //      source provenance
    //      grapheme/cluster geometry
    //      normalized scalar indices
    //
    // while allowing later UAX #9 stages to behave exactly as though the X9
    // formatting characters had been removed.
    // ========================================================================

    [[nodiscard]]
    static inline bool buildBidiX9Indices(const UnicodeBidiClass* types,  uint32_t count, std::vector<uint32_t>& indices)
    {
        indices.clear();

        if (count == 0)
            return true;

        if (!types)
            return false;


        indices.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            if (!isBidiRemovedByX9(types[i]))
                indices.push_back(i);
        }

        return true;
    }

    // ========================================================================
    // bidiTypeFromLevel
    //
    // Convert an embedding level into the corresponding strong bidi type.
    // ========================================================================

    [[nodiscard]]
    static constexpr UnicodeBidiClass bidiTypeFromLevel( UnicodeBidiLevel level) noexcept
    {
        return (level & 1u) != 0
            ? UnicodeBidiClass::RightToLeft
            : UnicodeBidiClass::LeftToRight;
    }

    // ========================================================================
    // buildBidiLevelRuns
    //
    // Divide the post-X9 character sequence into maximal ranges having the
    // same explicit embedding level.
    //
    // runForPosition provides the reverse mapping:
    //
    //      post-X9 position -> level-run index
    // ========================================================================

    [[nodiscard]]
    static inline bool buildBidiLevelRuns(
        const uint32_t* bidiIndices,
        uint32_t bidiCount,
        const UnicodeBidiLevel* levels,
        uint32_t scalarCount,
        std::vector<BidiLevelRun>& runs,
        std::vector<uint32_t>& runForPosition)
    {
        runs.clear();
        runForPosition.clear();

        if (bidiCount == 0)
            return true;

        if (!bidiIndices || !levels)
            return false;


        runForPosition.resize(
            bidiCount,
            kBidiIndexInvalid);


        const uint32_t firstScalarIndex =
            bidiIndices[0];

        if (firstScalarIndex >= scalarCount)
            return false;


        uint32_t runBegin = 0;

        UnicodeBidiLevel runLevel =
            levels[firstScalarIndex];


        for (uint32_t position = 1;
            position <= bidiCount;
            ++position)
        {
            bool endRun =
                position == bidiCount;


            UnicodeBidiLevel level = runLevel;


            if (!endRun)
            {
                const uint32_t scalarIndex =
                    bidiIndices[position];

                if (scalarIndex >= scalarCount)
                    return false;

                level = levels[scalarIndex];

                endRun =
                    level != runLevel;
            }


            if (!endRun)
                continue;


            const uint32_t runIndex =
                static_cast<uint32_t>(runs.size());


            runs.push_back(
                BidiLevelRun{
                    runBegin,
                    position,
                    runLevel
                });


            for (uint32_t p = runBegin; p < position; ++p)
                runForPosition[p] = runIndex;


            if (position < bidiCount)
            {
                runBegin = position;
                runLevel = level;
            }
        }


        return true;
    }

    // ========================================================================
// buildBidiIsolateMatches
//
// Build matching isolate-initiator/PDI pairs in the post-X9 sequence.
//
// matches[position] contains the matching post-X9 position for both sides
// of a pair, or kBidiIndexInvalid when unmatched.
//
// originalTypes is required because X1-X8 may have changed the working
// type of an isolate initiator or PDI under a directional override.
// ========================================================================

    [[nodiscard]]
    static inline bool buildBidiIsolateMatches(
        const uint32_t* bidiIndices,
        uint32_t bidiCount,
        const UnicodeBidiClass* originalTypes,
        uint32_t scalarCount,
        std::vector<uint32_t>& matches)
    {
        matches.clear();

        if (bidiCount == 0)
            return true;

        if (!bidiIndices || !originalTypes)
            return false;


        matches.assign(
            bidiCount,
            kBidiIndexInvalid);


        std::vector<uint32_t> isolateStack;
        isolateStack.reserve(bidiCount);


        for (uint32_t position = 0;
            position < bidiCount;
            ++position)
        {
            const uint32_t scalarIndex =
                bidiIndices[position];

            if (scalarIndex >= scalarCount)
                return false;


            const UnicodeBidiClass type =
                originalTypes[scalarIndex];


            if (isBidiIsolateInitiator(type))
            {
                isolateStack.push_back(position);
                continue;
            }


            if (type != UnicodeBidiClass::PopDirectionalIsolate)
                continue;


            if (isolateStack.empty())
                continue;


            const uint32_t initiatorPosition =
                isolateStack.back();

            isolateStack.pop_back();


            matches[initiatorPosition] = position;
            matches[position] = initiatorPosition;
        }


        return true;
    }


    // ========================================================================
    // buildBidiIsolatingRunSequences
    //
    // Apply BD13 and the structural portion of X10.
    //
    // sequenceRunIndices is a flattened list of level-run indices.
    //
    // Each BidiIsolatingRunSequence identifies its portion of that list with:
    //
    //      runOffset
    //      runCount
    //
    // sos/eos are computed from the original explicit embedding levels, as
    // required by X10.
    // ========================================================================

    [[nodiscard]]
    static inline bool buildBidiIsolatingRunSequences(
        const uint32_t* bidiIndices,
        uint32_t bidiCount,
        const UnicodeBidiClass* originalTypes,
        const UnicodeBidiLevel* levels,
        uint32_t scalarCount,
        UnicodeBidiLevel paragraphLevel,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& runForPosition,
        const std::vector<uint32_t>& isolateMatches,
        std::vector<uint32_t>& sequenceRunIndices,
        std::vector<BidiIsolatingRunSequence>& sequences)
    {
        sequenceRunIndices.clear();
        sequences.clear();


        if (bidiCount == 0)
            return runs.empty();

        if (!bidiIndices ||
            !originalTypes ||
            !levels)
        {
            return false;
        }


        if (runForPosition.size() != bidiCount ||
            isolateMatches.size() != bidiCount)
        {
            return false;
        }


        std::vector<uint8_t> runUsed(
            runs.size(),
            0);


        for (uint32_t firstRunIndex = 0;
            firstRunIndex < static_cast<uint32_t>(runs.size());
            ++firstRunIndex)
        {
            const BidiLevelRun& firstRun =
                runs[firstRunIndex];

            if (firstRun.begin >= firstRun.end ||
                firstRun.end > bidiCount)
            {
                return false;
            }


            const uint32_t firstPosition =
                firstRun.begin;

            const uint32_t firstScalarIndex =
                bidiIndices[firstPosition];

            if (firstScalarIndex >= scalarCount)
                return false;


            const UnicodeBidiClass firstOriginalType =
                originalTypes[firstScalarIndex];


            // A level run beginning with a matched PDI belongs to the
            // isolating run sequence that contains its matching initiator.
            if (firstOriginalType ==
                UnicodeBidiClass::PopDirectionalIsolate &&
                isolateMatches[firstPosition] != kBidiIndexInvalid)
            {
                continue;
            }


            BidiIsolatingRunSequence sequence{};

            sequence.runOffset =
                static_cast<uint32_t>(
                    sequenceRunIndices.size());

            sequence.level =
                firstRun.level;


            uint32_t currentRunIndex =
                firstRunIndex;


            for (;;)
            {
                if (currentRunIndex >= runs.size())
                    return false;

                if (runUsed[currentRunIndex] != 0)
                    return false;


                const BidiLevelRun& currentRun =
                    runs[currentRunIndex];

                if (currentRun.begin >= currentRun.end ||
                    currentRun.end > bidiCount)
                {
                    return false;
                }


                if (currentRun.level != sequence.level)
                    return false;


                runUsed[currentRunIndex] = 1;

                sequenceRunIndices.push_back(
                    currentRunIndex);

                ++sequence.runCount;


                const uint32_t lastPosition =
                    currentRun.end - 1u;

                const uint32_t lastScalarIndex =
                    bidiIndices[lastPosition];

                if (lastScalarIndex >= scalarCount)
                    return false;


                const UnicodeBidiClass lastOriginalType =
                    originalTypes[lastScalarIndex];


                if (!isBidiIsolateInitiator(
                    lastOriginalType))
                {
                    break;
                }


                const uint32_t matchingPdiPosition =
                    isolateMatches[lastPosition];

                if (matchingPdiPosition ==
                    kBidiIndexInvalid)
                {
                    break;
                }


                if (matchingPdiPosition >= bidiCount)
                    return false;


                const uint32_t nextRunIndex =
                    runForPosition[matchingPdiPosition];

                if (nextRunIndex == kBidiIndexInvalid ||
                    nextRunIndex >= runs.size())
                {
                    return false;
                }


                // BD13 guarantees that the matching PDI starts the next
                // level run in this isolating run sequence.
                if (runs[nextRunIndex].begin !=
                    matchingPdiPosition)
                {
                    return false;
                }


                currentRunIndex =
                    nextRunIndex;
            }


            // ---------------------------------------------------------------
            // sos
            // ---------------------------------------------------------------

            const uint32_t sequenceFirstRunIndex =
                sequenceRunIndices[sequence.runOffset];

            const BidiLevelRun& sequenceFirstRun =
                runs[sequenceFirstRunIndex];

            const uint32_t sequenceFirstPosition =
                sequenceFirstRun.begin;


            UnicodeBidiLevel precedingLevel =
                paragraphLevel;


            if (sequenceFirstPosition != 0)
            {
                const uint32_t precedingScalarIndex =
                    bidiIndices[
                        sequenceFirstPosition - 1u];

                if (precedingScalarIndex >= scalarCount)
                    return false;

                precedingLevel =
                    levels[precedingScalarIndex];
            }


            const UnicodeBidiLevel sosLevel =
                sequence.level > precedingLevel
                ? sequence.level
                : precedingLevel;

            sequence.sos =
                bidiTypeFromLevel(sosLevel);


            // ---------------------------------------------------------------
            // eos
            // ---------------------------------------------------------------

            const uint32_t sequenceLastRunIndex =
                sequenceRunIndices[
                    sequence.runOffset +
                        sequence.runCount -
                        1u];

            const BidiLevelRun& sequenceLastRun =
                runs[sequenceLastRunIndex];

            const uint32_t sequenceLastPosition =
                sequenceLastRun.end - 1u;

            const uint32_t sequenceLastScalarIndex =
                bidiIndices[sequenceLastPosition];

            if (sequenceLastScalarIndex >= scalarCount)
                return false;


            const UnicodeBidiClass sequenceLastOriginalType =
                originalTypes[sequenceLastScalarIndex];


            UnicodeBidiLevel followingLevel =
                paragraphLevel;


            const bool unmatchedIsolateAtEnd =
                isBidiIsolateInitiator(
                    sequenceLastOriginalType) &&
                isolateMatches[sequenceLastPosition] ==
                kBidiIndexInvalid;


            if (!unmatchedIsolateAtEnd &&
                sequenceLastPosition + 1u < bidiCount)
            {
                const uint32_t followingScalarIndex =
                    bidiIndices[
                        sequenceLastPosition + 1u];

                if (followingScalarIndex >= scalarCount)
                    return false;

                followingLevel =
                    levels[followingScalarIndex];
            }


            const UnicodeBidiLevel eosLevel =
                sequence.level > followingLevel
                ? sequence.level
                : followingLevel;

            sequence.eos =
                bidiTypeFromLevel(eosLevel);


            sequences.push_back(sequence);
        }


        // Every level run must belong to exactly one isolating run sequence.
        for (uint8_t used : runUsed)
        {
            if (used == 0)
                return false;
        }


        return true;
    }


    // ========================================================================
// resolveBidiWeakTypesW1
//
// UAX #9 W1.
//
// Resolve NSM characters one isolating run sequence at a time.
//
//      NSM at start of sequence
//          -> sos
//
//      NSM after isolate initiator or PDI
//          -> ON
//
//      otherwise
//          -> resolved type of previous character
//
// Constituent level runs of an isolating run sequence are treated as
// logically adjacent.
//
// originalTypes is required to recognize isolate initiators and PDI even
// when X1-X8 directional overrides changed their working bidi types.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW1(
        const uint32_t* bidiIndices,
        uint32_t bidiCount,
        const UnicodeBidiClass* originalTypes,
        UnicodeBidiClass* types,
        uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !originalTypes ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            UnicodeBidiClass previousType =
                sequence.sos;

            UnicodeBidiClass previousOriginalType =
                UnicodeBidiClass::OtherNeutral;

            bool hasPreviousCharacter = false;


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    if (type == UnicodeBidiClass::NonspacingMark)
                    {
                        if (hasPreviousCharacter &&
                            (isBidiIsolateInitiator(
                                previousOriginalType) ||
                                previousOriginalType ==
                                UnicodeBidiClass::PopDirectionalIsolate))
                        {
                            type =
                                UnicodeBidiClass::OtherNeutral;
                        }
                        else
                        {
                            type =
                                previousType;
                        }
                    }


                    previousType =
                        type;

                    previousOriginalType =
                        originalTypes[scalarIndex];

                    hasPreviousCharacter =
                        true;
                }
            }
        }


        return true;
    }


    // ========================================================================
// resolveBidiWeakTypesW2
//
// UAX #9 W2.
//
// For each EN, search backward through its isolating run sequence until
// the first strong type R, L, AL, or sos is found.
//
// If that strong type is AL:
//
//      EN -> AN
//
// A forward pass maintaining the most recent strong type is equivalent
// to performing a separate backward search for each EN.
//
// W1 must already have been applied to the working type array.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW2(const uint32_t* bidiIndices,
        uint32_t bidiCount, UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            // sos is always L or R and acts as the initial strong type
            // for W2 when no preceding strong character exists.
            UnicodeBidiClass previousStrong =
                sequence.sos;


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    // --------------------------------------------------------
                    // W2
                    // --------------------------------------------------------

                    if (type == UnicodeBidiClass::EuropeanNumber &&
                        previousStrong == UnicodeBidiClass::ArabicLetter)
                    {
                        type =
                            UnicodeBidiClass::ArabicNumber;
                    }


                    // --------------------------------------------------------
                    // Remember only strong types relevant to W2.
                    //
                    // This intentionally examines the current working type.
                    // X1-X8 directional overrides may already have changed a
                    // character to L or R, and W2 operates on those altered
                    // types.
                    // --------------------------------------------------------

                    if (type == UnicodeBidiClass::LeftToRight ||
                        type == UnicodeBidiClass::RightToLeft ||
                        type == UnicodeBidiClass::ArabicLetter)
                    {
                        previousStrong =
                            type;
                    }
                }
            }
        }


        return true;
    }


    // ========================================================================
    // resolveBidiWeakTypesW3
    //
    // UAX #9 W3.
    //
    // Change every remaining Arabic Letter type to Right-to-Left:
    //
    //      AL -> R
    //
    // W2 must already have been applied, because W2 still needs to distinguish
    // AL from R when deciding whether EN becomes AN.
    //
    // This rule has no contextual dependency, so scanning the post-X9 scalar
    // sequence is equivalent to processing each isolating run sequence
    // separately.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW3(const uint32_t* bidiIndices,
        uint32_t bidiCount, UnicodeBidiClass* types,
        uint32_t scalarCount) noexcept
    {
        if (bidiCount == 0)
            return true;

        if (!bidiIndices || !types)
            return false;


        for (uint32_t position = 0; position < bidiCount; ++position)
        {
            const uint32_t scalarIndex =
                bidiIndices[position];

            if (scalarIndex >= scalarCount)
                return false;


            if (types[scalarIndex] == UnicodeBidiClass::ArabicLetter)
                types[scalarIndex] = UnicodeBidiClass::RightToLeft;
        }


        return true;
    }


    // ========================================================================
// resolveBidiWeakTypesW4
//
// UAX #9 W4.
//
// Resolve numeric separators inside each isolating run sequence:
//
//      EN ES EN -> EN EN EN
//      EN CS EN -> EN EN EN
//      AN CS AN -> AN AN AN
//
// The previous and next characters are those in the isolating run
// sequence, so adjacency may cross constituent level-run boundaries.
//
// W1-W3 must already have been applied to the working type array.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW4(const uint32_t* bidiIndices,
        uint32_t bidiCount, UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            UnicodeBidiClass previousType =
                sequence.sos;

            bool hasPreviousCharacter = false;


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    // --------------------------------------------------------
                    // Locate the next character in this isolating run
                    // sequence. It may be in the same level run or at the
                    // beginning of the next constituent level run.
                    // --------------------------------------------------------

                    UnicodeBidiClass nextType =
                        sequence.eos;

                    bool hasNextCharacter = false;


                    if (position + 1u < run.end)
                    {
                        const uint32_t nextScalarIndex =
                            bidiIndices[position + 1u];

                        if (nextScalarIndex >= scalarCount)
                            return false;

                        nextType =
                            types[nextScalarIndex];

                        hasNextCharacter =
                            true;
                    }
                    else if (sequenceRun + 1u < sequence.runCount)
                    {
                        const uint32_t nextRunListIndex =
                            sequence.runOffset +
                            sequenceRun +
                            1u;

                        const uint32_t nextRunIndex =
                            sequenceRunIndices[nextRunListIndex];

                        if (nextRunIndex >= runs.size())
                            return false;


                        const BidiLevelRun& nextRun =
                            runs[nextRunIndex];

                        if (nextRun.begin >= nextRun.end ||
                            nextRun.end > bidiCount)
                        {
                            return false;
                        }


                        const uint32_t nextScalarIndex =
                            bidiIndices[nextRun.begin];

                        if (nextScalarIndex >= scalarCount)
                            return false;


                        nextType =
                            types[nextScalarIndex];

                        hasNextCharacter =
                            true;
                    }


                    // --------------------------------------------------------
                    // W4
                    // --------------------------------------------------------

                    if (hasPreviousCharacter &&
                        hasNextCharacter)
                    {
                        if (type == UnicodeBidiClass::EuropeanSeparator)
                        {
                            if (previousType ==
                                UnicodeBidiClass::EuropeanNumber &&
                                nextType ==
                                UnicodeBidiClass::EuropeanNumber)
                            {
                                type =
                                    UnicodeBidiClass::EuropeanNumber;
                            }
                        }
                        else if (type == UnicodeBidiClass::CommonSeparator)
                        {
                            if (previousType ==
                                UnicodeBidiClass::EuropeanNumber &&
                                nextType ==
                                UnicodeBidiClass::EuropeanNumber)
                            {
                                type =
                                    UnicodeBidiClass::EuropeanNumber;
                            }
                            else if (previousType ==
                                UnicodeBidiClass::ArabicNumber &&
                                nextType ==
                                UnicodeBidiClass::ArabicNumber)
                            {
                                type =
                                    UnicodeBidiClass::ArabicNumber;
                            }
                        }
                    }


                    previousType =
                        type;

                    hasPreviousCharacter =
                        true;
                }
            }
        }


        return true;
    }


    // ========================================================================
// resolveBidiWeakTypesW5
//
// UAX #9 W5.
//
// A sequence of European Terminators adjacent to a European Number
// becomes European Number:
//
//      ET ET EN -> EN EN EN
//      EN ET ET -> EN EN EN
//
// A forward pass resolves ET sequences following EN. A backward pass
// resolves ET sequences preceding EN.
//
// Processing is performed independently for each isolating run sequence.
//
// W1-W4 must already have been applied to the working type array.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW5(const uint32_t* bidiIndices,
        uint32_t bidiCount, UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Forward pass.
            //
            //      EN ET ET -> EN EN EN
            // ---------------------------------------------------------------

            UnicodeBidiClass previousType =
                sequence.sos;


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    if (type == UnicodeBidiClass::EuropeanTerminator &&
                        previousType == UnicodeBidiClass::EuropeanNumber)
                    {
                        type =
                            UnicodeBidiClass::EuropeanNumber;
                    }


                    previousType =
                        type;
                }
            }


            // ---------------------------------------------------------------
            // Backward pass.
            //
            //      ET ET EN -> EN EN EN
            // ---------------------------------------------------------------

            UnicodeBidiClass nextType =
                sequence.eos;


            for (uint32_t sequenceRun = sequence.runCount;
                sequenceRun-- > 0;)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.end;
                    position-- > run.begin;)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    if (type == UnicodeBidiClass::EuropeanTerminator &&
                        nextType == UnicodeBidiClass::EuropeanNumber)
                    {
                        type =
                            UnicodeBidiClass::EuropeanNumber;
                    }


                    nextType =
                        type;
                }
            }
        }


        return true;
    }


    // ========================================================================
// resolveBidiWeakTypesW6
//
// UAX #9 W6.
//
// All separators and terminators remaining after W4 and W5 become
// Other Neutral:
//
//      ES -> ON
//      ET -> ON
//      CS -> ON
//
// W4 and W5 must already have been applied, because those rules consume
// the numeric separators and terminators that should retain numeric types.
//
// This rule has no contextual dependency, so scanning the post-X9 scalar
// sequence is equivalent to processing each isolating run sequence
// separately.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW6(const uint32_t* bidiIndices,
        uint32_t bidiCount, UnicodeBidiClass* types,
        uint32_t scalarCount) noexcept
    {
        if (bidiCount == 0)
            return true;

        if (!bidiIndices || !types)
            return false;


        for (uint32_t position = 0; position < bidiCount; ++position)
        {
            const uint32_t scalarIndex =
                bidiIndices[position];

            if (scalarIndex >= scalarCount)
                return false;


            UnicodeBidiClass& type =
                types[scalarIndex];


            if (type == UnicodeBidiClass::EuropeanSeparator ||
                type == UnicodeBidiClass::EuropeanTerminator ||
                type == UnicodeBidiClass::CommonSeparator)
            {
                type =
                    UnicodeBidiClass::OtherNeutral;
            }
        }


        return true;
    }


    // ========================================================================
    // resolveBidiWeakTypesW7
    //
    // UAX #9 W7.
    //
    // For each European Number, search backward through its isolating run
    // sequence until the first strong type R, L, or sos is found.
    //
    // If that strong type is L:
    //
    //      EN -> L
    //
    // A forward pass maintaining the most recent strong L or R is equivalent
    // to performing a separate backward search for each EN.
    //
    // W1-W6 must already have been applied to the working type array.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiWeakTypesW7(const uint32_t* bidiIndices,
        uint32_t bidiCount, UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            // sos is always L or R and therefore provides the initial
            // strong direction for W7.

            UnicodeBidiClass previousStrong =
                sequence.sos;


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    // --------------------------------------------------------
                    // W7
                    // --------------------------------------------------------

                    if (type == UnicodeBidiClass::EuropeanNumber &&
                        previousStrong == UnicodeBidiClass::LeftToRight)
                    {
                        type =
                            UnicodeBidiClass::LeftToRight;
                    }


                    // --------------------------------------------------------
                    // Remember the most recent strong L or R.
                    //
                    // AL no longer exists here because W3 converted AL to R.
                    // --------------------------------------------------------

                    if (type == UnicodeBidiClass::LeftToRight ||
                        type == UnicodeBidiClass::RightToLeft)
                    {
                        previousStrong =
                            type;
                    }
                }
            }
        }


        return true;
    }


    // ========================================================================
    // bidiBracketCodePointsEquivalent
    //
    // BD16 bracket matching normally compares code points directly.
    //
    // U+3009 RIGHT ANGLE BRACKET and U+232A RIGHT-POINTING ANGLE BRACKET are
    // canonically equivalent for BD16 matching.
    // ========================================================================

    [[nodiscard]]
    static constexpr bool bidiBracketCodePointsEquivalent(
        uint32_t a, uint32_t b) noexcept
    {
        if (a == b)
            return true;

        return
            (a == 0x3009u && b == 0x232Au) ||
            (a == 0x232Au && b == 0x3009u);
    }


    // ========================================================================
    // buildBidiBracketPairs
    //
    // UAX #9 BD14-BD16.
    //
    // Discover paired brackets in one isolating run sequence.
    //
    // This performs pair discovery only. It does not apply N0 or modify the
    // working bidi types.
    //
    // Brackets participate only when their current working type is ON.
    //
    // BD16 requires a fixed stack of exactly 63 entries. If another opening
    // bracket is encountered when that stack is full, the resulting pair list
    // for this isolating run sequence is empty.
    // ========================================================================

    [[nodiscard]]
    static inline bool buildBidiBracketPairs(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        const UnicodeScalar* scalars, const UnicodeBidiClass* types,
        uint32_t scalarCount, const UnicodeDatabase& database,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const BidiIsolatingRunSequence& sequence,
        std::vector<BidiBracketPair>& pairs)
    {
        pairs.clear();

        if (bidiCount == 0)
            return true;

        if (!bidiIndices ||
            !scalars ||
            !types ||
            !database.hasBidiBrackets())
        {
            return false;
        }

        if (sequence.runCount == 0)
            return false;

        if (sequence.runOffset > sequenceRunIndices.size() ||
            sequence.runCount >
            sequenceRunIndices.size() - sequence.runOffset)
        {
            return false;
        }


        struct StackEntry
        {
            uint32_t pairedCodePoint{ 0 };
            uint32_t position{ 0 };
        };


        std::array<StackEntry, 63> stack{};
        uint32_t stackSize = 0;


        for (uint32_t sequenceRun = 0;
            sequenceRun < sequence.runCount;
            ++sequenceRun)
        {
            const uint32_t runListIndex =
                sequence.runOffset + sequenceRun;

            const uint32_t runIndex =
                sequenceRunIndices[runListIndex];

            if (runIndex >= runs.size())
                return false;


            const BidiLevelRun& run =
                runs[runIndex];

            if (run.begin >= run.end ||
                run.end > bidiCount)
            {
                return false;
            }


            for (uint32_t position = run.begin;
                position < run.end;
                ++position)
            {
                const uint32_t scalarIndex =
                    bidiIndices[position];

                if (scalarIndex >= scalarCount)
                    return false;


                // BD14/BD15 require the current bidi type to be ON.

                if (types[scalarIndex] !=
                    UnicodeBidiClass::OtherNeutral)
                {
                    continue;
                }


                const uint32_t cp =
                    scalars[scalarIndex].value;

                const UnicodeBidiBracketRecord* bracket =
                    database.bidiBracket(cp);

                if (!bracket)
                    continue;


                const UnicodeBidiPairedBracketType bracketType =
                    static_cast<UnicodeBidiPairedBracketType>(
                        bracket->type);


                // ------------------------------------------------------------
                // Opening paired bracket
                // ------------------------------------------------------------

                if (bracketType ==
                    UnicodeBidiPairedBracketType::Open)
                {
                    if (stackSize >= stack.size())
                    {
                        // BD16 specifies an empty list for the sequence when
                        // the fixed 63-entry stack overflows.

                        pairs.clear();
                        return true;
                    }


                    stack[stackSize++] = StackEntry{
                        bracket->pairedCodePoint,
                        position
                    };

                    continue;
                }


                // ------------------------------------------------------------
                // Closing paired bracket
                // ------------------------------------------------------------

                if (bracketType !=
                    UnicodeBidiPairedBracketType::Close)
                {
                    return false;
                }

                if (stackSize == 0)
                    continue;


                uint32_t stackIndex =
                    stackSize;

                while (stackIndex != 0)
                {
                    --stackIndex;

                    const StackEntry& entry =
                        stack[stackIndex];

                    if (!bidiBracketCodePointsEquivalent(
                        cp,
                        entry.pairedCodePoint))
                    {
                        continue;
                    }


                    pairs.push_back(BidiBracketPair{
                        entry.position,
                        position
                        });


                    // Pop the matched opener and everything above it.

                    stackSize =
                        stackIndex;

                    break;
                }
            }
        }


        // BD16 requires ordering by opening bracket position. Nested pairs are
        // discovered in closing-bracket order, so this sort is necessary.

        std::sort(
            pairs.begin(),
            pairs.end(),
            [](const BidiBracketPair& a,
                const BidiBracketPair& b) noexcept
            {
                return a.openPosition < b.openPosition;
            });


        return true;
    }


    // ========================================================================
    // inspectBidiBracketPairContents
    //
    // N0 enclosed-strong inspection for one BD16 bracket pair.
    //
    // Scan only characters belonging to this isolating run sequence between
    // the opening and closing bracket.
    //
    // Strong types contribute as follows:
    //
    //      L       -> leftToRight
    //      R       -> rightToLeft
    //      EN, AN  -> rightToLeft
    //
    // All other current working bidi types are ignored.
    //
    // This helper performs inspection only. It does not modify bidi types.
    // ========================================================================

    [[nodiscard]]
    static inline bool inspectBidiBracketPairContents(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        const UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const BidiIsolatingRunSequence& sequence,
        const BidiBracketPair& pair,
        BidiBracketStrongTypes& strongTypes)
    {
        strongTypes = {};

        if (!bidiIndices || !types)
            return false;

        if (pair.openPosition >= pair.closePosition ||
            pair.closePosition >= bidiCount)
        {
            return false;
        }

        if (sequence.runCount == 0)
            return false;

        if (sequence.runOffset > sequenceRunIndices.size() ||
            sequence.runCount >
            sequenceRunIndices.size() - sequence.runOffset)
        {
            return false;
        }


        bool foundOpen = false;
        bool foundClose = false;


        for (uint32_t sequenceRun = 0;
            sequenceRun < sequence.runCount;
            ++sequenceRun)
        {
            const uint32_t runListIndex =
                sequence.runOffset + sequenceRun;

            const uint32_t runIndex =
                sequenceRunIndices[runListIndex];

            if (runIndex >= runs.size())
                return false;


            const BidiLevelRun& run =
                runs[runIndex];

            if (run.begin >= run.end ||
                run.end > bidiCount)
            {
                return false;
            }


            for (uint32_t position = run.begin;
                position < run.end;
                ++position)
            {
                if (!foundOpen)
                {
                    if (position == pair.openPosition)
                        foundOpen = true;

                    continue;
                }


                if (position == pair.closePosition)
                {
                    foundClose = true;
                    break;
                }


                const uint32_t scalarIndex =
                    bidiIndices[position];

                if (scalarIndex >= scalarCount)
                    return false;


                const UnicodeBidiClass type =
                    types[scalarIndex];


                switch (type)
                {
                case UnicodeBidiClass::LeftToRight:
                    strongTypes.leftToRight = true;
                    break;

                case UnicodeBidiClass::RightToLeft:
                case UnicodeBidiClass::EuropeanNumber:
                case UnicodeBidiClass::ArabicNumber:
                    strongTypes.rightToLeft = true;
                    break;

                default:
                    break;
                }


                // Once both directions have been found, additional characters
                // cannot change the result of this inspection.

                if (strongTypes.mixed())
                    return true;
            }


            if (foundClose)
                break;
        }


        return foundOpen && foundClose;
    }


    // ========================================================================
// resolveBidiBracketPairN0
//
// UAX #9 N0 resolution for one BD16 bracket pair.
//
// The pair list must be processed in ascending opening-position order.
//
// This helper:
//
//      1. Inspects strong types enclosed by the pair.
//      2. Resolves immediately to the embedding direction when that
//         direction occurs inside the pair.
//      3. Otherwise, when only the opposite direction occurs inside,
//         finds the preceding strong context.
//      4. Leaves the pair unchanged when no strong type occurs inside.
//
// EN and AN are treated as R throughout N0.
//
// Trailing original-NSM handling required by N0 is intentionally not
// performed here.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiBracketPairN0(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const BidiIsolatingRunSequence& sequence,
        const BidiBracketPair& pair)
    {
        if (!bidiIndices || !types)
            return false;

        if (pair.openPosition >= pair.closePosition ||
            pair.closePosition >= bidiCount)
        {
            return false;
        }


        const uint32_t openScalarIndex =
            bidiIndices[pair.openPosition];

        const uint32_t closeScalarIndex =
            bidiIndices[pair.closePosition];

        if (openScalarIndex >= scalarCount ||
            closeScalarIndex >= scalarCount)
        {
            return false;
        }


        // BD16 pairs consist only of brackets whose current type was ON.
        // A pair position should therefore still be ON when N0 reaches it.

        if (types[openScalarIndex] != UnicodeBidiClass::OtherNeutral ||
            types[closeScalarIndex] != UnicodeBidiClass::OtherNeutral)
        {
            return false;
        }


        // --------------------------------------------------------------------
        // N0a - inspect enclosed strong types.
        // --------------------------------------------------------------------

        BidiBracketStrongTypes strongTypes{};

        if (!inspectBidiBracketPairContents(
            bidiIndices,
            bidiCount,
            types,
            scalarCount,
            runs,
            sequenceRunIndices,
            sequence,
            pair,
            strongTypes))
        {
            return false;
        }


        // No strong type enclosed by the pair.
        //
        // N0 leaves both brackets unchanged. N1/N2 will resolve them later.

        if (strongTypes.empty())
            return true;


        const UnicodeBidiClass embeddingType =
            bidiTypeFromLevel(sequence.level);

        const UnicodeBidiClass oppositeType =
            embeddingType == UnicodeBidiClass::LeftToRight
            ? UnicodeBidiClass::RightToLeft
            : UnicodeBidiClass::LeftToRight;


        const bool containsEmbeddingType =
            embeddingType == UnicodeBidiClass::LeftToRight
            ? strongTypes.leftToRight
            : strongTypes.rightToLeft;


        // --------------------------------------------------------------------
        // N0b
        //
        // If the enclosed text contains a strong type matching the embedding
        // direction, both brackets take the embedding direction.
        // --------------------------------------------------------------------

        if (containsEmbeddingType)
        {
            types[openScalarIndex] = embeddingType;
            types[closeScalarIndex] = embeddingType;

            return true;
        }


        // --------------------------------------------------------------------
        // N0c
        //
        // There is enclosed strong text, but none matches the embedding
        // direction. Therefore the enclosed strong direction is opposite.
        //
        // Find the preceding strong type in this isolating run sequence.
        // sos supplies the context when no earlier strong type exists.
        //
        // EN and AN act as R.
        // --------------------------------------------------------------------

        UnicodeBidiClass precedingStrong =
            sequence.sos;

        bool foundOpen = false;


        if (sequence.runCount == 0)
            return false;

        if (sequence.runOffset > sequenceRunIndices.size() ||
            sequence.runCount >
            sequenceRunIndices.size() - sequence.runOffset)
        {
            return false;
        }


        for (uint32_t sequenceRun = 0;
            sequenceRun < sequence.runCount;
            ++sequenceRun)
        {
            const uint32_t runListIndex =
                sequence.runOffset + sequenceRun;

            const uint32_t runIndex =
                sequenceRunIndices[runListIndex];

            if (runIndex >= runs.size())
                return false;


            const BidiLevelRun& run =
                runs[runIndex];

            if (run.begin >= run.end ||
                run.end > bidiCount)
            {
                return false;
            }


            for (uint32_t position = run.begin;
                position < run.end;
                ++position)
            {
                if (position == pair.openPosition)
                {
                    foundOpen = true;
                    break;
                }


                const uint32_t scalarIndex =
                    bidiIndices[position];

                if (scalarIndex >= scalarCount)
                    return false;


                const UnicodeBidiClass type =
                    types[scalarIndex];


                if (type == UnicodeBidiClass::LeftToRight)
                {
                    precedingStrong =
                        UnicodeBidiClass::LeftToRight;
                }
                else if (type == UnicodeBidiClass::RightToLeft ||
                    type == UnicodeBidiClass::EuropeanNumber ||
                    type == UnicodeBidiClass::ArabicNumber)
                {
                    precedingStrong =
                        UnicodeBidiClass::RightToLeft;
                }
            }


            if (foundOpen)
                break;
        }


        if (!foundOpen)
            return false;


        // --------------------------------------------------------------------
        // N0c1 / N0c2
        //
        // If preceding context is opposite the embedding direction, use that
        // opposite direction. Otherwise use the embedding direction.
        // --------------------------------------------------------------------

        const UnicodeBidiClass resolvedType =
            precedingStrong == oppositeType
            ? oppositeType
            : embeddingType;


        types[openScalarIndex] = resolvedType;
        types[closeScalarIndex] = resolvedType;


        return true;
    }


    // ========================================================================
    // applyBidiBracketTrailingNsmN0
    //
    // Apply the trailing-NSM portion of UAX #9 N0 for one resolved bracket.
    //
    // Any immediately following characters whose original bidi type was NSM
    // take the L or R type assigned to the preceding bracket by N0.
    //
    // Adjacency is adjacency in the isolating run sequence, so the scan may
    // continue across constituent level-run boundaries.
    //
    // originalTypes must contain the Bidi_Class values from before W1.
    // ========================================================================

    [[nodiscard]]
    static inline bool applyBidiBracketTrailingNsmN0(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        const UnicodeBidiClass* originalTypes,
        UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const BidiIsolatingRunSequence& sequence,
        uint32_t bracketPosition)
    {
        if (!bidiIndices ||
            !originalTypes ||
            !types ||
            bracketPosition >= bidiCount)
        {
            return false;
        }

        if (sequence.runCount == 0)
            return false;

        if (sequence.runOffset > sequenceRunIndices.size() ||
            sequence.runCount >
            sequenceRunIndices.size() - sequence.runOffset)
        {
            return false;
        }


        const uint32_t bracketScalarIndex =
            bidiIndices[bracketPosition];

        if (bracketScalarIndex >= scalarCount)
            return false;


        const UnicodeBidiClass bracketType =
            types[bracketScalarIndex];

        if (bracketType != UnicodeBidiClass::LeftToRight &&
            bracketType != UnicodeBidiClass::RightToLeft)
        {
            return false;
        }


        bool foundBracket = false;


        for (uint32_t sequenceRun = 0;
            sequenceRun < sequence.runCount;
            ++sequenceRun)
        {
            const uint32_t runListIndex =
                sequence.runOffset + sequenceRun;

            const uint32_t runIndex =
                sequenceRunIndices[runListIndex];

            if (runIndex >= runs.size())
                return false;


            const BidiLevelRun& run =
                runs[runIndex];

            if (run.begin >= run.end ||
                run.end > bidiCount)
            {
                return false;
            }


            for (uint32_t position = run.begin;
                position < run.end;
                ++position)
            {
                if (!foundBracket)
                {
                    if (position == bracketPosition)
                        foundBracket = true;

                    continue;
                }


                const uint32_t scalarIndex =
                    bidiIndices[position];

                if (scalarIndex >= scalarCount)
                    return false;


                // N0 refers specifically to the bidi type prior to W1.

                if (originalTypes[scalarIndex] !=
                    UnicodeBidiClass::NonspacingMark)
                {
                    return true;
                }


                types[scalarIndex] =
                    bracketType;
            }
        }


        return foundBracket;
    }

    // ========================================================================
    // resolveBidiNeutralTypesN0
    //
    // UAX #9 N0.
    //
    // Process paired brackets independently for each isolating run sequence.
    //
    // For each sequence:
    //
    //      1. Discover the complete BD16 bracket-pair list.
    //      2. Process pairs in ascending opening-position order.
    //      3. Apply trailing original-NSM propagation to brackets which
    //         changed to L or R under N0.
    //
    // Pair discovery is completed before any pair is resolved. This preserves
    // the BD16 pair list while allowing earlier N0 resolutions to influence
    // the processing of later pairs.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiNeutralTypesN0(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        const UnicodeScalar* scalars,
        const UnicodeBidiClass* originalTypes,
        UnicodeBidiClass* types, uint32_t scalarCount,
        const UnicodeDatabase& database,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !scalars ||
            !originalTypes ||
            !types ||
            runs.empty() ||
            sequences.empty() ||
            !database.hasBidiBrackets())
        {
            return false;
        }


        std::vector<BidiBracketPair> pairs;


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            // ---------------------------------------------------------------
            // BD16
            //
            // Discover all pairs before changing any bracket types.
            // buildBidiBracketPairs() returns them in ascending opening-
            // position order, as required by N0.
            // ---------------------------------------------------------------

            if (!buildBidiBracketPairs(
                bidiIndices,
                bidiCount,
                scalars,
                types,
                scalarCount,
                database,
                runs,
                sequenceRunIndices,
                sequence,
                pairs))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // N0
            //
            // Process pairs sequentially. Changes made by an earlier pair
            // remain visible while resolving later pairs.
            // ---------------------------------------------------------------

            for (const BidiBracketPair& pair : pairs)
            {
                if (pair.openPosition >= bidiCount ||
                    pair.closePosition >= bidiCount)
                {
                    return false;
                }


                const uint32_t openScalarIndex =
                    bidiIndices[pair.openPosition];

                const uint32_t closeScalarIndex =
                    bidiIndices[pair.closePosition];

                if (openScalarIndex >= scalarCount ||
                    closeScalarIndex >= scalarCount)
                {
                    return false;
                }


                if (!resolveBidiBracketPairN0(
                    bidiIndices,
                    bidiCount,
                    types,
                    scalarCount,
                    runs,
                    sequenceRunIndices,
                    sequence,
                    pair))
                {
                    return false;
                }


                const UnicodeBidiClass resolvedType =
                    types[openScalarIndex];


                // -----------------------------------------------------------
                // No enclosed strong type.
                //
                // N0 left the pair as ON. N1/N2 will resolve it later.
                // -----------------------------------------------------------

                if (resolvedType ==
                    UnicodeBidiClass::OtherNeutral)
                {
                    if (types[closeScalarIndex] !=
                        UnicodeBidiClass::OtherNeutral)
                    {
                        return false;
                    }

                    continue;
                }


                // -----------------------------------------------------------
                // A BD16 pair began as ON/ON. Therefore L or R here means
                // this pair changed under N0.
                // -----------------------------------------------------------

                if (resolvedType != UnicodeBidiClass::LeftToRight &&
                    resolvedType != UnicodeBidiClass::RightToLeft)
                {
                    return false;
                }

                if (types[closeScalarIndex] != resolvedType)
                    return false;


                // -----------------------------------------------------------
                // N0 trailing original-NSM handling.
                // -----------------------------------------------------------

                if (!applyBidiBracketTrailingNsmN0(
                    bidiIndices,
                    bidiCount,
                    originalTypes,
                    types,
                    scalarCount,
                    runs,
                    sequenceRunIndices,
                    sequence,
                    pair.openPosition))
                {
                    return false;
                }


                if (!applyBidiBracketTrailingNsmN0(
                    bidiIndices,
                    bidiCount,
                    originalTypes,
                    types,
                    scalarCount,
                    runs,
                    sequenceRunIndices,
                    sequence,
                    pair.closePosition))
                {
                    return false;
                }
            }
        }


        return true;
    }



    // ========================================================================
// isBidiNeutralOrIsolateFormatting
//
// UAX #9 NI:
//
//      B, S, WS, ON, FSI, LRI, RLI, PDI
// ========================================================================

    [[nodiscard]]
    static constexpr bool isBidiNeutralOrIsolateFormatting(
        UnicodeBidiClass type) noexcept
    {
        switch (type)
        {
        case UnicodeBidiClass::ParagraphSeparator:
        case UnicodeBidiClass::SegmentSeparator:
        case UnicodeBidiClass::WhiteSpace:
        case UnicodeBidiClass::OtherNeutral:
        case UnicodeBidiClass::FirstStrongIsolate:
        case UnicodeBidiClass::LeftToRightIsolate:
        case UnicodeBidiClass::RightToLeftIsolate:
        case UnicodeBidiClass::PopDirectionalIsolate:
            return true;

        default:
            return false;
        }
    }

    // ========================================================================
// bidiStrongTypeForN1
//
// Return the strong directional influence used by N1:
//
//      L      -> L
//      R      -> R
//      EN/AN  -> R
//
// false means that type does not provide N1 strong context.
// ========================================================================

    [[nodiscard]]
    static constexpr bool bidiStrongTypeForN1(
        UnicodeBidiClass type,
        UnicodeBidiClass& strongType) noexcept
    {
        if (type == UnicodeBidiClass::LeftToRight)
        {
            strongType = UnicodeBidiClass::LeftToRight;
            return true;
        }

        if (type == UnicodeBidiClass::RightToLeft ||
            type == UnicodeBidiClass::EuropeanNumber ||
            type == UnicodeBidiClass::ArabicNumber)
        {
            strongType = UnicodeBidiClass::RightToLeft;
            return true;
        }

        return false;
    }


    // ========================================================================
// resolveBidiNeutralTypesN1
//
// UAX #9 N1.
//
// For each isolating run sequence, resolve every maximal sequence of NI
// characters when the strong directional influence on both sides agrees.
//
// EN and AN act as R for their influence on NI characters.
//
// sos and eos supply the strong context at isolating-run-sequence
// boundaries.
//
// NI sequences whose surrounding directions disagree remain unchanged
// for N2.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiNeutralTypesN1(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        std::vector<uint32_t> neutralScalars;


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            if (sequence.sos != UnicodeBidiClass::LeftToRight &&
                sequence.sos != UnicodeBidiClass::RightToLeft)
            {
                return false;
            }

            if (sequence.eos != UnicodeBidiClass::LeftToRight &&
                sequence.eos != UnicodeBidiClass::RightToLeft)
            {
                return false;
            }


            neutralScalars.clear();

            UnicodeBidiClass precedingStrong =
                sequence.sos;


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    const UnicodeBidiClass type =
                        types[scalarIndex];


                    // --------------------------------------------------------
                    // Continue the current NI sequence.
                    // --------------------------------------------------------

                    if (isBidiNeutralOrIsolateFormatting(type))
                    {
                        neutralScalars.push_back(
                            scalarIndex);

                        continue;
                    }


                    // --------------------------------------------------------
                    // A non-NI character must provide the strong directional
                    // influence relevant to N1 at this stage:
                    //
                    //      L
                    //      R
                    //      EN -> R
                    //      AN -> R
                    // --------------------------------------------------------

                    UnicodeBidiClass followingStrong{};

                    if (!bidiStrongTypeForN1(
                        type,
                        followingStrong))
                    {
                        return false;
                    }


                    // --------------------------------------------------------
                    // Resolve the pending NI sequence only when both sides
                    // agree.
                    //
                    // Otherwise leave it unchanged for N2.
                    // --------------------------------------------------------

                    if (!neutralScalars.empty())
                    {
                        if (precedingStrong ==
                            followingStrong)
                        {
                            for (uint32_t neutralScalarIndex :
                            neutralScalars)
                            {
                                types[neutralScalarIndex] =
                                    precedingStrong;
                            }
                        }

                        neutralScalars.clear();
                    }


                    precedingStrong =
                        followingStrong;
                }
            }


            // ---------------------------------------------------------------
            // eos supplies the following strong context for a trailing NI
            // sequence.
            // ---------------------------------------------------------------

            if (!neutralScalars.empty())
            {
                if (precedingStrong == sequence.eos)
                {
                    for (uint32_t neutralScalarIndex :
                    neutralScalars)
                    {
                        types[neutralScalarIndex] =
                            precedingStrong;
                    }
                }

                neutralScalars.clear();
            }
        }


        return true;
    }


    // ========================================================================
    // resolveBidiNeutralTypesN2
    //
    // UAX #9 N2.
    //
    // Any NI characters remaining after N1 take the embedding direction of
    // their isolating run sequence:
    //
    //      even level -> L
    //      odd level  -> R
    //
    // N1 must already have been applied. Therefore any NI still present is one
    // whose surrounding directional influences did not agree.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiNeutralTypesN2(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        UnicodeBidiClass* types, uint32_t scalarCount,
        const std::vector<BidiLevelRun>& runs,
        const std::vector<uint32_t>& sequenceRunIndices,
        const std::vector<BidiIsolatingRunSequence>& sequences)
    {
        if (bidiCount == 0)
            return sequences.empty();

        if (!bidiIndices ||
            !types ||
            runs.empty() ||
            sequences.empty())
        {
            return false;
        }


        for (const BidiIsolatingRunSequence& sequence : sequences)
        {
            if (sequence.runCount == 0)
                return false;

            if (sequence.runOffset > sequenceRunIndices.size() ||
                sequence.runCount >
                sequenceRunIndices.size() - sequence.runOffset)
            {
                return false;
            }


            const UnicodeBidiClass embeddingType =
                bidiTypeFromLevel(sequence.level);


            for (uint32_t sequenceRun = 0;
                sequenceRun < sequence.runCount;
                ++sequenceRun)
            {
                const uint32_t runListIndex =
                    sequence.runOffset + sequenceRun;

                const uint32_t runIndex =
                    sequenceRunIndices[runListIndex];

                if (runIndex >= runs.size())
                    return false;


                const BidiLevelRun& run =
                    runs[runIndex];

                if (run.begin >= run.end ||
                    run.end > bidiCount)
                {
                    return false;
                }


                if (run.level != sequence.level)
                    return false;


                for (uint32_t position = run.begin;
                    position < run.end;
                    ++position)
                {
                    const uint32_t scalarIndex =
                        bidiIndices[position];

                    if (scalarIndex >= scalarCount)
                        return false;


                    UnicodeBidiClass& type =
                        types[scalarIndex];


                    if (isBidiNeutralOrIsolateFormatting(type))
                        type = embeddingType;
                }
            }
        }


        return true;
    }



    // ========================================================================
// resolveBidiImplicitLevelsI1
//
// UAX #9 I1.
//
// For characters whose explicit embedding level is even:
//
//      R      -> level + 1
//      EN, AN -> level + 2
//
// L remains at its current level.
//
// X9-removed characters are absent from bidiIndices and are therefore not
// modified by this rule.
//
// W1-W7 and N0-N2 must already have been applied. At this point every
// post-X9 working bidi type must be one of:
//
//      L, R, EN, AN
//
// I2 will subsequently process characters whose embedding level was odd.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiImplicitLevelsI1(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        const UnicodeBidiClass* types,
        UnicodeBidiLevel* levels, uint32_t scalarCount) noexcept
    {
        if (bidiCount == 0)
            return true;

        if (!bidiIndices || !types || !levels)
            return false;


        for (uint32_t position = 0; position < bidiCount; ++position)
        {
            const uint32_t scalarIndex =
                bidiIndices[position];

            if (scalarIndex >= scalarCount)
                return false;


            const UnicodeBidiClass type =
                types[scalarIndex];

            UnicodeBidiLevel& level =
                levels[scalarIndex];


            // I1 receives the explicit embedding levels established by X1-X8.
            //
            // Explicit levels may not exceed max_depth. I1 can raise the
            // resulting implicit level as high as max_depth + 1.

            if (level > kUnicodeBidiMaxDepth)
                return false;


            // After W1-W7 and N0-N2, these are the only bidi types which may
            // remain in the post-X9 sequence.

            if (type != UnicodeBidiClass::LeftToRight &&
                type != UnicodeBidiClass::RightToLeft &&
                type != UnicodeBidiClass::EuropeanNumber &&
                type != UnicodeBidiClass::ArabicNumber)
            {
                return false;
            }


            // I1 applies only to even embedding levels.

            if ((level & 1u) != 0)
                continue;


            if (type == UnicodeBidiClass::RightToLeft)
            {
                level =
                    static_cast<UnicodeBidiLevel>(
                        level + 1u);
            }
            else if (type == UnicodeBidiClass::EuropeanNumber ||
                type == UnicodeBidiClass::ArabicNumber)
            {
                level =
                    static_cast<UnicodeBidiLevel>(
                        level + 2u);
            }
        }


        return true;
    }


    // ========================================================================
// resolveBidiImplicitLevelsI2
//
// UAX #9 I2.
//
// For characters whose embedding level is odd:
//
//      L      -> level + 1
//      EN, AN -> level + 1
//
// R remains at its current level.
//
// I1 must already have been applied.
//
// X9-removed characters are absent from bidiIndices and are therefore not
// modified by this rule.
//
// After I1, implicit levels may already be as high as max_depth + 1.
// ========================================================================

    [[nodiscard]]
    static inline bool resolveBidiImplicitLevelsI2(
        const uint32_t* bidiIndices, uint32_t bidiCount,
        const UnicodeBidiClass* types,
        UnicodeBidiLevel* levels, uint32_t scalarCount) noexcept
    {
        if (bidiCount == 0)
            return true;

        if (!bidiIndices || !types || !levels)
            return false;


        for (uint32_t position = 0; position < bidiCount; ++position)
        {
            const uint32_t scalarIndex =
                bidiIndices[position];

            if (scalarIndex >= scalarCount)
                return false;


            const UnicodeBidiClass type =
                types[scalarIndex];

            UnicodeBidiLevel& level =
                levels[scalarIndex];


            // I1 may already have produced max_depth + 1.

            if (static_cast<uint16_t>(level) >
                static_cast<uint16_t>(kUnicodeBidiMaxDepth) + 1u)
            {
                return false;
            }


            // After W1-W7 and N0-N2, these are the only bidi types which may
            // remain in the post-X9 sequence.

            if (type != UnicodeBidiClass::LeftToRight &&
                type != UnicodeBidiClass::RightToLeft &&
                type != UnicodeBidiClass::EuropeanNumber &&
                type != UnicodeBidiClass::ArabicNumber)
            {
                return false;
            }


            // I2 applies only to odd embedding levels.

            if ((level & 1u) == 0)
                continue;


            if (type == UnicodeBidiClass::LeftToRight ||
                type == UnicodeBidiClass::EuropeanNumber ||
                type == UnicodeBidiClass::ArabicNumber)
            {
                level =
                    static_cast<UnicodeBidiLevel>(
                        level + 1u);
            }
        }


        return true;
    }









    // ========================================================================
    // UnicodeBidiStream
    //
    // Pull stream:
    //
    //      ScriptGrapheme
    //              |
    //              v
    //      collect one paragraph
    //              |
    //              v
    //      own all paragraph storage
    //              |
    //              v
    //      P2/P3 paragraph level
    //              |
    //              v
    //      BidiParagraphView
    //
    //
    // Source must provide:
    //
    //      bool operator()(ScriptGrapheme& out);
    //      TextStreamStatus status() const noexcept;
    //
    //
    // P1:
    //
    // A scalar whose Bidi_Class is ParagraphSeparator terminates the current
    // paragraph and remains part of that paragraph.
    //
    //
    // Current milestone:
    //
    // UAX #9 processing currently includes:
    //
    //      P1-P3
    //      X1-X10
    //      W1-W7
    //      N0-N2
    //      I1-I2
    //
    // ========================================================================

    template<typename Source>
    class UnicodeBidiStream
    {
    public:
        UnicodeBidiStream(Source& source, const UnicodeDatabase& database,
            UnicodeBidiParagraphDirection direction =
            UnicodeBidiParagraphDirection::Auto) noexcept
            : mSource(&source),
            mDatabase(&database),
            mParagraphDirection(direction)
        {
            // The database needs to support both Bidi_Class and 
            // Bidi_Bracket properties for UAX #9 processing.
            if (!database.hasBidiClass() ||
                !database.hasBidiBrackets())
                mStatus = TextStreamStatus::InvalidInput;
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]] TextStreamStatus status() const noexcept {
            return mStatus;
        }

        [[nodiscard]] bool ready() const noexcept {
            return mStatus == TextStreamStatus::Ready;
        }

        [[nodiscard]] bool ended() const noexcept {
            return mStatus == TextStreamStatus::End;
        }

        [[nodiscard]] bool failed() const noexcept {
            return mStatus == TextStreamStatus::InvalidInput;
        }


        // ====================================================================
        // operator()
        //
        // Produce one complete bidi paragraph.
        //
        // The returned view remains valid until the next call.
        // ====================================================================

        bool operator()(BidiParagraphView& out)
        {
            out = {};

            if (mStatus != TextStreamStatus::Ready)
                return false;


            // Reusing paragraph storage invalidates the previously returned
            // BidiParagraphView.

            clearParagraph();


            if (mSourceEnded)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }


            if (!collectParagraph())
                return false;


            mParagraphLevel =
                resolveBidiParagraphLevel(
                    mTypes.empty() ? nullptr : mTypes.data(),
                    static_cast<uint32_t>(mTypes.size()),
                    mParagraphDirection);


            // ================================================================
            // X1-X8
            //
            // Resolve explicit embeddings, overrides, and isolates.
            //
            // mTypes becomes the working bidi-type array from this point
            // forward. Later W/N processing will continue operating on it.
            // ================================================================

            mLevels.resize(mScalars.size());


            if (!resolveExplicitBidiLevels(
                mTypes.empty() ? nullptr : mTypes.data(),
                mLevels.empty() ? nullptr : mLevels.data(),
                static_cast<uint32_t>(mTypes.size()),
                mParagraphLevel))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // X9
            //
            // Virtually remove RLE, LRE, RLO, LRO, PDF, and BN from all
            // subsequent bidi processing.
            //
            // The actual paragraph storage remains unchanged. mBidiIndices
            // identifies the scalar positions that remain visible to X10 and
            // the later W/N/I rules.
            // ================================================================

            if (!buildBidiX9Indices(
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mTypes.size()),
                mBidiIndices))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // X10 - level runs
            // ================================================================

            if (!buildBidiLevelRuns(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mLevels.empty() ? nullptr : mLevels.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mRunForBidiPosition))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // X10 - isolate matching
            // ================================================================

            if (!buildBidiIsolateMatches(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mOriginalTypes.empty() ? nullptr : mOriginalTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mIsolateMatches))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // X10 - isolating run sequences + sos/eos
            // ================================================================

            if (!buildBidiIsolatingRunSequences(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mOriginalTypes.empty() ? nullptr : mOriginalTypes.data(),
                mLevels.empty() ? nullptr : mLevels.data(),
                static_cast<uint32_t>(mScalars.size()),
                mParagraphLevel,
                mLevelRuns,
                mRunForBidiPosition,
                mIsolateMatches,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // W1
            //
            // Resolve nonspacing marks from the preceding character in each
            // isolating run sequence.
            // ================================================================

            if (!resolveBidiWeakTypesW1(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mOriginalTypes.empty() ? nullptr : mOriginalTypes.data(),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // W2
            //
            // Change EN to AN when the preceding strong type in the isolating
            // run sequence is AL.
            // ================================================================

            if (!resolveBidiWeakTypesW2(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }

            // ================================================================
            // W3
            //
            // Change all remaining AL types to R.
            // ================================================================

            if (!resolveBidiWeakTypesW3(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size())))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // W4
            //
            // Resolve ES and CS when they occur between compatible numbers.
            // ================================================================

            if (!resolveBidiWeakTypesW4(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // W5
            //
            // Change ET sequences adjacent to EN into EN.
            // ================================================================

            if (!resolveBidiWeakTypesW5(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // W6
            //
            // Change all remaining ES, ET, and CS types to ON.
            // ================================================================

            if (!resolveBidiWeakTypesW6(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size())))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // W7
            //
            // Change EN to L when the preceding strong type in the isolating
            // run sequence is L.
            // ================================================================

            if (!resolveBidiWeakTypesW7(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // N0
            //
            // Resolve paired brackets independently within each isolating run
            // sequence.
            //
            // BD16 pair discovery, sequential bracket resolution, and trailing
            // original-NSM handling are all performed by this stage.
            // ================================================================

            if (!resolveBidiNeutralTypesN0(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mScalars.empty() ? nullptr : mScalars.data(),
                mOriginalTypes.empty() ? nullptr : mOriginalTypes.data(),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                *mDatabase,
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // N1
            //
            // Resolve NI sequences when their surrounding strong directional
            // influences agree.
            //
            // EN and AN act as R for their influence on NI characters.
            // ================================================================

            if (!resolveBidiNeutralTypesN1(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // N2
            //
            // Any NI characters remaining after N1 take the embedding
            // direction of their isolating run sequence.
            // ================================================================

            if (!resolveBidiNeutralTypesN2(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                static_cast<uint32_t>(mScalars.size()),
                mLevelRuns,
                mSequenceRunIndices,
                mRunSequences))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // I1
            //
            // Resolve implicit levels for characters whose embedding level is
            // even:
            //
            //      R      -> level + 1
            //      EN, AN -> level + 2
            // ================================================================

            if (!resolveBidiImplicitLevelsI1(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                mLevels.empty() ? nullptr : mLevels.data(),
                static_cast<uint32_t>(mScalars.size())))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            // ================================================================
            // I2
            //
            // Resolve implicit levels for characters whose embedding level is
            // odd:
            //
            //      L, EN, AN -> level + 1
            //      R         -> unchanged
            // ================================================================

            if (!resolveBidiImplicitLevelsI2(
                mBidiIndices.empty() ? nullptr : mBidiIndices.data(),
                static_cast<uint32_t>(mBidiIndices.size()),
                mTypes.empty() ? nullptr : mTypes.data(),
                mLevels.empty() ? nullptr : mLevels.data(),
                static_cast<uint32_t>(mScalars.size())))
            {
                clearParagraph();
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }








            return emitParagraph(out);

        }


    private:
        Source* mSource{ nullptr };
        const UnicodeDatabase* mDatabase{ nullptr };

        UnicodeBidiParagraphDirection mParagraphDirection{
            UnicodeBidiParagraphDirection::Auto
        };


        // ====================================================================
        // Owned paragraph storage
        // ====================================================================

        std::vector<UnicodeScalar> mScalars{};

        // Original Unicode Bidi_Class values. These never change.
        std::vector<UnicodeBidiClass> mOriginalTypes{};

        // Mutable UAX #9 working types.
        std::vector<UnicodeBidiClass> mTypes{};

        std::vector<UnicodeBidiLevel> mLevels{};

        // Post-X9 scalar indices.
        std::vector<uint32_t> mBidiIndices{};

        // X10 level runs.
        std::vector<BidiLevelRun> mLevelRuns{};

        // Post-X9 position -> level-run index.
        std::vector<uint32_t> mRunForBidiPosition{};

        // Post-X9 isolate initiator <-> PDI matching positions.
        std::vector<uint32_t> mIsolateMatches{};

        // Flattened level-run membership for isolating run sequences.
        std::vector<uint32_t> mSequenceRunIndices{};

        // X10 isolating run sequences.
        std::vector<BidiIsolatingRunSequence> mRunSequences{};

        std::vector<ShapingCluster> mClusters{};
        std::vector<ScriptClusterInfo> mScripts{};

        UnicodeBidiLevel mParagraphLevel{ 0 };

        ScalarIndex mNormalizedBegin{ 0 };
        SourceRange mSourceRange{};


        // ====================================================================
        // Stream state
        // ====================================================================

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
        bool mSourceEnded{ false };


        // ====================================================================
        // clearParagraph
        // ====================================================================

        void clearParagraph()
        {
            mScalars.clear();
            mOriginalTypes.clear();
            mTypes.clear();

            mLevels.clear();
            mBidiIndices.clear();

            mLevelRuns.clear();
            mRunForBidiPosition.clear();
            mIsolateMatches.clear();
            mSequenceRunIndices.clear();
            mRunSequences.clear();

            mClusters.clear();
            mScripts.clear();

            mParagraphLevel = 0;
            mNormalizedBegin = 0;
            mSourceRange = {};
        }


        // ====================================================================
        // extendSourceRange
        //
        // Preserve the provenance envelope covering the complete paragraph.
        // ====================================================================

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


        // ====================================================================
        // appendGrapheme
        //
        // Promote one borrowed ScriptGrapheme into paragraph-owned storage.
        //
        // paragraphEnded becomes true when the grapheme contains B.
        // ====================================================================

        bool appendGrapheme(const ScriptGrapheme& input, bool& paragraphEnded)
        {
            paragraphEnded = false;

            if (!input.grapheme)
                return false;


            const uint32_t scalarOffset =
                static_cast<uint32_t>(mScalars.size());


            // ---------------------------------------------------------------
            // Preserve paragraph-level provenance.
            // ---------------------------------------------------------------

            if (mClusters.empty())
            {
                mNormalizedBegin =
                    input.grapheme.normalizedBegin;

                mSourceRange =
                    input.grapheme.source;
            }
            else
            {
                extendSourceRange(
                    mSourceRange,
                    input.grapheme.source);
            }


            // ---------------------------------------------------------------
            // Cluster metadata.
            // ---------------------------------------------------------------

            ShapingCluster cluster{};

            cluster.scalarOffset = scalarOffset;
            cluster.scalarCount = input.grapheme.scalarCount;
            cluster.normalizedBegin = input.grapheme.normalizedBegin;
            cluster.source = input.grapheme.source;

            mClusters.push_back(cluster);


            // ---------------------------------------------------------------
            // Temporary Script metadata.
            // ---------------------------------------------------------------

            ScriptClusterInfo script{};

            script.candidates = input.candidates;
            script.script = input.script;

            mScripts.push_back(script);


            // ---------------------------------------------------------------
            // Scalars and initial bidi types.
            // ---------------------------------------------------------------

            for (const UnicodeScalar& scalar : input.grapheme)
            {
                mScalars.push_back(scalar);

                const UnicodeBidiClass type =
                    mDatabase->bidiClass(scalar.value);

                mOriginalTypes.push_back(type);
                mTypes.push_back(type);

                if (type == UnicodeBidiClass::ParagraphSeparator)
                    paragraphEnded = true;
            }


            return true;
        }


        // ====================================================================
        // collectParagraph
        //
        // UAX #9 P1.
        //
        // B remains with the paragraph it terminates.
        //
        // Clean upstream End flushes a non-empty final paragraph.
        //
        // Invalid upstream input does not flush a partial paragraph.
        // ====================================================================

        bool collectParagraph()
        {
            while (true)
            {
                ScriptGrapheme input{};

                if (!(*mSource)(input))
                {
                    const TextStreamStatus sourceStatus =
                        mSource->status();


                    if (sourceStatus == TextStreamStatus::End)
                    {
                        mSourceEnded = true;

                        if (mScalars.empty())
                        {
                            mStatus = TextStreamStatus::End;
                            return false;
                        }

                        return true;
                    }


                    clearParagraph();
                    mStatus = TextStreamStatus::InvalidInput;

                    return false;
                }


                bool paragraphEnded = false;

                if (!appendGrapheme(
                    input,
                    paragraphEnded))
                {
                    clearParagraph();
                    mStatus = TextStreamStatus::InvalidInput;

                    return false;
                }


                if (paragraphEnded)
                    return true;
            }
        }


        // ====================================================================
        // emitParagraph
        // ====================================================================

        bool emitParagraph(BidiParagraphView& out) const noexcept
        {
            if (mScalars.empty())
                return false;
            

            out.scalars = mScalars.data();
            out.originalTypes = mOriginalTypes.data();
            out.levels = mLevels.data();
            out.scalarCount =
                static_cast<uint32_t>(mScalars.size());

            out.clusters = mClusters.data();
            out.scripts = mScripts.data();
            out.clusterCount =
                static_cast<uint32_t>(mClusters.size());

            out.paragraphLevel = mParagraphLevel;

            out.normalizedBegin = mNormalizedBegin;
            out.source = mSourceRange;

            return true;
        }
    };

} // namespace waavs