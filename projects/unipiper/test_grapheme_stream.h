// test_grapheme_stream.h

#pragma once


#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "unicode_grapheme_stream.h"


namespace waavs
{
    // ========================================================================
    // Synthetic GraphemeScalar source
    //
    // GraphemeStream should be tested independently of UTF-8 decoding, NFC,
    // and Unicode-property lookup. Those stages already have their own tests.
    // ========================================================================

    class GraphemeScalarTestSource
    {
    public:
        GraphemeScalarTestSource(const GraphemeScalar* items, size_t count) noexcept
            : mItems(items)
            , mCount(count)
        {
            if (count == 0)
                mStatus = TextStreamStatus::End;
        }


        bool operator()(GraphemeScalar& out) noexcept
        {
            if (mStatus != TextStreamStatus::Ready)
                return false;


            if (mIndex >= mCount)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }


            out = mItems[mIndex++];

            return true;
        }


        [[nodiscard]]
        TextStreamStatus status() const noexcept {
            return mStatus;
        }


    private:
        const GraphemeScalar* mItems{ nullptr };
        size_t mCount{ 0 };
        size_t mIndex{ 0 };

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
    };


    // ========================================================================
    // Test helpers
    // ========================================================================

    static GraphemeScalar makeGraphemeTestScalar(
        uint32_t value,
        TextOffset begin,
        TextOffset end,
        UnicodeGraphemeClusterBreak gcb,
        UnicodeIndicConjunctBreak incb = UnicodeIndicConjunctBreak::None,
        bool extendedPictographic = false) noexcept
    {
        GraphemeScalar result;

        result.scalar.value = value;
        result.scalar.source.begin = begin;
        result.scalar.source.end = end;

        result.gcb = gcb;
        result.incb = incb;
        result.extendedPictographic = extendedPictographic;

        return result;
    }


    struct GraphemeStreamExpectedCluster
    {
        const uint32_t* values{ nullptr };
        uint32_t count{ 0 };

        ScalarIndex normalizedBegin{ 0 };

        TextOffset sourceBegin{ 0 };
        TextOffset sourceEnd{ 0 };
    };


    static bool testGraphemeStreamCase(
        const char* name,
        const GraphemeScalar* input,
        size_t inputCount,
        const GraphemeStreamExpectedCluster* expected,
        size_t expectedCount)
    {
        GraphemeScalarTestSource source(input, inputCount);
        GraphemeStream<GraphemeScalarTestSource> stream(source);

        GraphemeClusterView cluster;


        for (size_t ci = 0; ci < expectedCount; ++ci)
        {
            if (!stream(cluster))
            {
                std::printf(
                    "  %-24s FAIL - premature end at cluster %zu\n",
                    name,
                    ci);

                return false;
            }


            const GraphemeStreamExpectedCluster& e =
                expected[ci];


            if (cluster.scalarCount != e.count)
            {
                std::printf(
                    "  %-24s FAIL - cluster %zu count %u expected %u\n",
                    name,
                    ci,
                    cluster.scalarCount,
                    e.count);

                return false;
            }


            if (cluster.normalizedBegin != e.normalizedBegin)
            {
                std::printf(
                    "  %-24s FAIL - cluster %zu normalizedBegin %u expected %u\n",
                    name,
                    ci,
                    cluster.normalizedBegin,
                    e.normalizedBegin);

                return false;
            }


            if (cluster.source.begin != e.sourceBegin ||
                cluster.source.end != e.sourceEnd)
            {
                std::printf(
                    "  %-24s FAIL - cluster %zu source [%u,%u) expected [%u,%u)\n",
                    name,
                    ci,
                    cluster.source.begin,
                    cluster.source.end,
                    e.sourceBegin,
                    e.sourceEnd);

                return false;
            }


            for (uint32_t si = 0; si < e.count; ++si)
            {
                if (cluster.scalars[si].value != e.values[si])
                {
                    std::printf(
                        "  %-24s FAIL - cluster %zu scalar %u U+%04X expected U+%04X\n",
                        name,
                        ci,
                        si,
                        cluster.scalars[si].value,
                        e.values[si]);

                    return false;
                }
            }
        }


        if (stream(cluster))
        {
            std::printf(
                "  %-24s FAIL - unexpected additional cluster\n",
                name);

            return false;
        }


        if (stream.status() != TextStreamStatus::End)
        {
            std::printf(
                "  %-24s FAIL - stream did not reach End\n",
                name);

            return false;
        }


        if (source.status() != TextStreamStatus::End)
        {
            std::printf(
                "  %-24s FAIL - source did not reach End\n",
                name);

            return false;
        }


        std::printf(
            "  %-24s PASS  scalars=%zu clusters=%zu\n",
            name,
            inputCount,
            expectedCount);

        return true;
    }


    // ========================================================================
    // testGraphemeStream
    // ========================================================================

    static bool testGraphemeStream()
    {
        using GCB = UnicodeGraphemeClusterBreak;
        using InCB = UnicodeIndicConjunctBreak;


        // ====================================================================
        // Case 1: CR / LF
        //
        //      A / CR x LF / B
        //
        // Expected:
        //
        //      [A]
        //      [CR LF]
        //      [B]
        //
        // Exercises GB3, GB4, and GB5.
        // ====================================================================

        {
            const GraphemeScalar input[] = {
                makeGraphemeTestScalar(0x0041u, 0u, 1u, GCB::Other),
                makeGraphemeTestScalar(0x000Du, 1u, 2u, GCB::CR),
                makeGraphemeTestScalar(0x000Au, 2u, 3u, GCB::LF),
                makeGraphemeTestScalar(0x0042u, 3u, 4u, GCB::Other)
            };

            static const uint32_t c0[] = { 0x0041u };
            static const uint32_t c1[] = { 0x000Du, 0x000Au };
            static const uint32_t c2[] = { 0x0042u };

            const GraphemeStreamExpectedCluster expected[] = {
                { c0, 1u, 0u, 0u, 1u },
                { c1, 2u, 1u, 1u, 3u },
                { c2, 1u, 3u, 3u, 4u }
            };


            if (!testGraphemeStreamCase(
                "CR/LF",
                input,
                sizeof(input) / sizeof(input[0]),
                expected,
                sizeof(expected) / sizeof(expected[0])))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 2: Combining marks and provenance
        //
        // Normalized scalar order:
        //
        //      q
        //      U+0300
        //      U+0315
        //      r
        //
        // Provenance is deliberately non-monotonic:
        //
        //      q           [0,1)
        //      U+0300      [3,5)
        //      U+0315      [1,3)
        //
        // Expected cluster provenance:
        //
        //      [0,5)
        //
        // Exercises GB9 and verifies sourceRangeUnion() behavior.
        // ====================================================================

        {
            const GraphemeScalar input[] = {
                makeGraphemeTestScalar(
                    0x0071u, 0u, 1u,
                    GCB::Other),

                makeGraphemeTestScalar(
                    0x0300u, 3u, 5u,
                    GCB::Extend,
                    InCB::Extend),

                makeGraphemeTestScalar(
                    0x0315u, 1u, 3u,
                    GCB::Extend,
                    InCB::Extend),

                makeGraphemeTestScalar(
                    0x0072u, 5u, 6u,
                    GCB::Other)
            };

            static const uint32_t c0[] = {
                0x0071u,
                0x0300u,
                0x0315u
            };

            static const uint32_t c1[] = {
                0x0072u
            };

            const GraphemeStreamExpectedCluster expected[] = {
                { c0, 3u, 0u, 0u, 5u },
                { c1, 1u, 3u, 5u, 6u }
            };


            if (!testGraphemeStreamCase(
                "Combining marks",
                input,
                sizeof(input) / sizeof(input[0]),
                expected,
                sizeof(expected) / sizeof(expected[0])))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 3: Hangul
        //
        //      L x V x T x T / A
        //
        // Exercises GB6, GB7, and GB8.
        //
        // This intentionally bypasses NFC so the grapheme rules themselves
        // see the decomposed Hangul sequence.
        // ====================================================================

        {
            const GraphemeScalar input[] = {
                makeGraphemeTestScalar(0x1100u, 0u, 3u, GCB::L),
                makeGraphemeTestScalar(0x1161u, 3u, 6u, GCB::V),
                makeGraphemeTestScalar(0x11A8u, 6u, 9u, GCB::T),
                makeGraphemeTestScalar(0x11A9u, 9u, 12u, GCB::T),
                makeGraphemeTestScalar(0x0041u, 12u, 13u, GCB::Other)
            };

            static const uint32_t c0[] = {
                0x1100u,
                0x1161u,
                0x11A8u,
                0x11A9u
            };

            static const uint32_t c1[] = {
                0x0041u
            };

            const GraphemeStreamExpectedCluster expected[] = {
                { c0, 4u, 0u, 0u, 12u },
                { c1, 1u, 4u, 12u, 13u }
            };


            if (!testGraphemeStreamCase(
                "Hangul",
                input,
                sizeof(input) / sizeof(input[0]),
                expected,
                sizeof(expected) / sizeof(expected[0])))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 4: Extended pictographic ZWJ sequence
        //
        //      EP Extend ZWJ x EP
        //
        // Example:
        //
        //      WOMAN
        //      VS16
        //      ZWJ
        //      PERSONAL COMPUTER
        //
        // Exercises GB9 and GB11.
        // ====================================================================

        {
            const GraphemeScalar input[] = {
                makeGraphemeTestScalar(
                    0x1F469u, 0u, 4u,
                    GCB::Other,
                    InCB::None,
                    true),

                makeGraphemeTestScalar(
                    0xFE0Fu, 4u, 7u,
                    GCB::Extend,
                    InCB::Extend),

                makeGraphemeTestScalar(
                    0x200Du, 7u, 10u,
                    GCB::ZWJ,
                    InCB::Extend),

                makeGraphemeTestScalar(
                    0x1F4BBu, 10u, 14u,
                    GCB::Other,
                    InCB::None,
                    true),

                makeGraphemeTestScalar(
                    0x0041u, 14u, 15u,
                    GCB::Other)
            };

            static const uint32_t c0[] = {
                0x1F469u,
                0xFE0Fu,
                0x200Du,
                0x1F4BBu
            };

            static const uint32_t c1[] = {
                0x0041u
            };

            const GraphemeStreamExpectedCluster expected[] = {
                { c0, 4u, 0u, 0u, 14u },
                { c1, 1u, 4u, 14u, 15u }
            };


            if (!testGraphemeStreamCase(
                "Emoji ZWJ",
                input,
                sizeof(input) / sizeof(input[0]),
                expected,
                sizeof(expected) / sizeof(expected[0])))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 5: Regional Indicator pairing
        //
        //      RI RI / RI RI / A
        //
        // Exercises GB12 and GB13.
        // ====================================================================

        {
            const GraphemeScalar input[] = {
                makeGraphemeTestScalar(
                    0x1F1E6u, 0u, 4u,
                    GCB::RegionalIndicator),

                makeGraphemeTestScalar(
                    0x1F1E7u, 4u, 8u,
                    GCB::RegionalIndicator),

                makeGraphemeTestScalar(
                    0x1F1E8u, 8u, 12u,
                    GCB::RegionalIndicator),

                makeGraphemeTestScalar(
                    0x1F1E9u, 12u, 16u,
                    GCB::RegionalIndicator),

                makeGraphemeTestScalar(
                    0x0041u, 16u, 17u,
                    GCB::Other)
            };

            static const uint32_t c0[] = {
                0x1F1E6u,
                0x1F1E7u
            };

            static const uint32_t c1[] = {
                0x1F1E8u,
                0x1F1E9u
            };

            static const uint32_t c2[] = {
                0x0041u
            };

            const GraphemeStreamExpectedCluster expected[] = {
                { c0, 2u, 0u, 0u, 8u },
                { c1, 2u, 2u, 8u, 16u },
                { c2, 1u, 4u, 16u, 17u }
            };


            if (!testGraphemeStreamCase(
                "Regional Indicators",
                input,
                sizeof(input) / sizeof(input[0]),
                expected,
                sizeof(expected) / sizeof(expected[0])))
            {
                return false;
            }
        }


        // ====================================================================
        // Case 6: Indic conjunct
        //
        //      Consonant
        //      Extend
        //      Linker
        //      Extend
        //      x Consonant
        //
        // Example values:
        //
        //      U+0915      DEVANAGARI LETTER KA
        //      U+093C      DEVANAGARI SIGN NUKTA
        //      U+094D      DEVANAGARI SIGN VIRAMA
        //      U+200D      ZERO WIDTH JOINER
        //      U+0915      DEVANAGARI LETTER KA
        //
        // The Extend/Linker scalars already remain attached through GB9.
        // GB9c is what prevents the boundary before the final consonant.
        // ====================================================================

        {
            const GraphemeScalar input[] = {
                makeGraphemeTestScalar(
                    0x0915u, 0u, 3u,
                    GCB::Other,
                    InCB::Consonant),

                makeGraphemeTestScalar(
                    0x093Cu, 3u, 6u,
                    GCB::Extend,
                    InCB::Extend),

                makeGraphemeTestScalar(
                    0x094Du, 6u, 9u,
                    GCB::Extend,
                    InCB::Linker),

                makeGraphemeTestScalar(
                    0x200Du, 9u, 12u,
                    GCB::ZWJ,
                    InCB::Extend),

                makeGraphemeTestScalar(
                    0x0915u, 12u, 15u,
                    GCB::Other,
                    InCB::Consonant),

                makeGraphemeTestScalar(
                    0x0041u, 15u, 16u,
                    GCB::Other,
                    InCB::None)
            };

            static const uint32_t c0[] = {
                0x0915u,
                0x093Cu,
                0x094Du,
                0x200Du,
                0x0915u
            };

            static const uint32_t c1[] = {
                0x0041u
            };

            const GraphemeStreamExpectedCluster expected[] = {
                { c0, 5u, 0u, 0u, 15u },
                { c1, 1u, 5u, 15u, 16u }
            };


            if (!testGraphemeStreamCase(
                "Indic conjunct",
                input,
                sizeof(input) / sizeof(input[0]),
                expected,
                sizeof(expected) / sizeof(expected[0])))
            {
                return false;
            }
        }


        // ====================================================================
        // Success
        // ====================================================================

        std::printf("GraphemeStream: PASS\n");
        std::printf("  CR/LF rules:             PASS\n");
        std::printf("  Combining marks:         PASS\n");
        std::printf("  Provenance envelope:     PASS\n");
        std::printf("  Hangul rules:            PASS\n");
        std::printf("  Emoji ZWJ rule:          PASS\n");
        std::printf("  Regional Indicator:      PASS\n");
        std::printf("  Indic conjunct rule:     PASS\n");
        std::printf("  Scalar indexing:         PASS\n");
        std::printf("  Stream status:           PASS\n");

        return true;
    }

} // namespace waavs