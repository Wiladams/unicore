// test_opentype_horizontal_metrics_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_hhea_view.h"
#include "opentype_hmtx_view.h"

namespace waavs
{
    static void appendHorizontalMetricTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendHorizontalMetricTestS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendHorizontalMetricTestU16(data, static_cast<uint16_t>(value));
    }


    static std::vector<uint8_t> makeHorizontalMetricTestHhea(uint16_t numberOfHMetrics)
    {
        std::vector<uint8_t> data;

        appendHorizontalMetricTestU16(data, 1);
        appendHorizontalMetricTestU16(data, 0);

        appendHorizontalMetricTestS16(data, 800);
        appendHorizontalMetricTestS16(data, -200);
        appendHorizontalMetricTestS16(data, 100);

        appendHorizontalMetricTestU16(data, 1200);

        appendHorizontalMetricTestS16(data, -50);
        appendHorizontalMetricTestS16(data, -40);
        appendHorizontalMetricTestS16(data, 1100);

        appendHorizontalMetricTestS16(data, 1);
        appendHorizontalMetricTestS16(data, 0);
        appendHorizontalMetricTestS16(data, 0);

        appendHorizontalMetricTestS16(data, 0);
        appendHorizontalMetricTestS16(data, 0);
        appendHorizontalMetricTestS16(data, 0);
        appendHorizontalMetricTestS16(data, 0);

        appendHorizontalMetricTestS16(data, 0);
        appendHorizontalMetricTestU16(data, numberOfHMetrics);

        return data;
    }


    // glyphCount = 6
    // numberOfHMetrics = 3
    //
    // glyph  advance  lsb
    //
    //   0      500     10
    //   1      600    -20
    //   2      700     30
    //   3      700     40
    //   4      700    -50
    //   5      700     60

    static std::vector<uint8_t> makeHorizontalMetricTestHmtx()
    {
        std::vector<uint8_t> data;

        appendHorizontalMetricTestU16(data, 500);
        appendHorizontalMetricTestS16(data, 10);

        appendHorizontalMetricTestU16(data, 600);
        appendHorizontalMetricTestS16(data, -20);

        appendHorizontalMetricTestU16(data, 700);
        appendHorizontalMetricTestS16(data, 30);

        appendHorizontalMetricTestS16(data, 40);
        appendHorizontalMetricTestS16(data, -50);
        appendHorizontalMetricTestS16(data, 60);

        return data;
    }


    static bool testOpenTypeHorizontalMetricsView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType horizontal metrics view: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ================================================================
        // Case 1 - hhea geometry.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeHorizontalMetricTestHhea(3);
            const OpenTypeHheaView hhea(ByteSpan(data.data(), data.size()));

            if (!hhea ||
                hhea.size() != 36 ||
                hhea.ascender() != 800 ||
                hhea.descender() != -200 ||
                hhea.lineGap() != 100 ||
                hhea.advanceWidthMax() != 1200 ||
                hhea.numberOfHMetrics() != 3)
            {
                return fail("case 1 hhea geometry");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Explicit longHorMetric records.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeHorizontalMetricTestHmtx();
            const OpenTypeHmtxView hmtx(ByteSpan(data.data(), data.size()), 6, 3);

            if (!hmtx)
                return fail("case 2 hmtx validity");

            const uint16_t expectedAdvance[] = { 500, 600, 700 };
            const int16_t expectedLsb[] = { 10, -20, 30 };

            for (uint32_t glyphId = 0; glyphId < 3; ++glyphId)
            {
                OpenTypeHorizontalMetric metric;

                if (!hmtx.metric(glyphId, metric) ||
                    metric.advanceWidth != expectedAdvance[glyphId] ||
                    metric.leftSideBearing != expectedLsb[glyphId])
                {
                    return fail("case 2 explicit metric");
                }
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Trailing glyphs reuse final advance.
        // ================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeHorizontalMetricTestHmtx();
            const OpenTypeHmtxView hmtx(ByteSpan(data.data(), data.size()), 6, 3);

            const int16_t expectedLsb[] = { 40, -50, 60 };

            for (uint32_t glyphId = 3; glyphId < 6; ++glyphId)
            {
                OpenTypeHorizontalMetric metric;

                if (!hmtx.metric(glyphId, metric) ||
                    metric.advanceWidth != 700 ||
                    metric.leftSideBearing != expectedLsb[glyphId - 3])
                {
                    return fail("case 3 trailing metric");
                }
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - numberOfHMetrics == glyphCount.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendHorizontalMetricTestU16(data, 400);
            appendHorizontalMetricTestS16(data, 10);

            appendHorizontalMetricTestU16(data, 500);
            appendHorizontalMetricTestS16(data, 20);

            const OpenTypeHmtxView hmtx(ByteSpan(data.data(), data.size()), 2, 2);

            OpenTypeHorizontalMetric metric;

            if (!hmtx ||
                !hmtx.metric(1, metric) ||
                metric.advanceWidth != 500 ||
                metric.leftSideBearing != 20)
            {
                return fail("case 4 full metric array");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Monospaced-style single long metric.
        // ================================================================

        {
            ++cases;

            std::vector<uint8_t> data;

            appendHorizontalMetricTestU16(data, 900);
            appendHorizontalMetricTestS16(data, 10);

            appendHorizontalMetricTestS16(data, 20);
            appendHorizontalMetricTestS16(data, -30);
            appendHorizontalMetricTestS16(data, 40);

            const OpenTypeHmtxView hmtx(ByteSpan(data.data(), data.size()), 4, 1);

            const int16_t expectedLsb[] = { 10, 20, -30, 40 };

            for (uint32_t glyphId = 0; glyphId < 4; ++glyphId)
            {
                OpenTypeHorizontalMetric metric;

                if (!hmtx.metric(glyphId, metric) ||
                    metric.advanceWidth != 900 ||
                    metric.leftSideBearing != expectedLsb[glyphId])
                {
                    return fail("case 5 monospaced metrics");
                }
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Failure paths.
        // ================================================================

        {
            ++cases;

            // Truncated hhea.

            {
                std::vector<uint8_t> data = makeHorizontalMetricTestHhea(3);
                data.resize(35);

                if (OpenTypeHheaView(ByteSpan(data.data(), data.size())))
                    return fail("case 6 truncated hhea");
            }


            // Zero numberOfHMetrics.

            {
                const std::vector<uint8_t> data = makeHorizontalMetricTestHhea(0);

                if (OpenTypeHheaView(ByteSpan(data.data(), data.size())))
                    return fail("case 6 zero numberOfHMetrics");
            }


            // numberOfHMetrics exceeds glyph count.

            {
                const std::vector<uint8_t> data = makeHorizontalMetricTestHmtx();

                if (OpenTypeHmtxView(ByteSpan(data.data(), data.size()), 2, 3))
                    return fail("case 6 excessive numberOfHMetrics");
            }


            // Truncated trailing LSB array.

            {
                std::vector<uint8_t> data = makeHorizontalMetricTestHmtx();
                data.pop_back();

                if (OpenTypeHmtxView(ByteSpan(data.data(), data.size()), 6, 3))
                    return fail("case 6 truncated hmtx");
            }


            // Glyph outside maxp glyph count.

            {
                const std::vector<uint8_t> data = makeHorizontalMetricTestHmtx();
                const OpenTypeHmtxView hmtx(ByteSpan(data.data(), data.size()), 6, 3);

                OpenTypeHorizontalMetric metric;

                if (hmtx.metric(6, metric))
                    return fail("case 6 out-of-range glyph");
            }

            ++passed;
        }


        std::printf(
            "OpenType horizontal metrics view: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  hhea geometry:             PASS\n"
            "  Explicit hMetrics:         PASS\n"
            "  Shared final advance:      PASS\n"
            "  Full metric array:         PASS\n"
            "  Single long metric:        PASS\n"
            "  Failure paths:             PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs