// test_opentype_layout_feature_view.h

#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_layout_view.h"


namespace waavs
{
    static void appendFeatureTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendFeatureTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchFeatureTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeOpenTypeFeatureListTestData()
    {
        std::vector<uint8_t> data;

        appendFeatureTestU16(data, 5);

        appendFeatureTestU32(data, OTAG("calt"));
        const size_t caltPatch = data.size();
        appendFeatureTestU16(data, 0);

        appendFeatureTestU32(data, OTAG("liga"));
        const size_t ligaPatch = data.size();
        appendFeatureTestU16(data, 0);

        appendFeatureTestU32(data, OTAG("smcp"));
        const size_t smcpPatch = data.size();
        appendFeatureTestU16(data, 0);

        appendFeatureTestU32(data, OTAG("ccmp"));
        const size_t ccmpPatch = data.size();
        appendFeatureTestU16(data, 0);

        appendFeatureTestU32(data, OTAG("rlig"));
        const size_t rligPatch = data.size();
        appendFeatureTestU16(data, 0);


        // calt -> lookups 7, 2

        const size_t caltOffset = data.size();
        patchFeatureTestU16(data, caltPatch, static_cast<uint16_t>(caltOffset));

        appendFeatureTestU16(data, 0);
        appendFeatureTestU16(data, 2);
        appendFeatureTestU16(data, 7);
        appendFeatureTestU16(data, 2);


        // liga -> lookup 3

        const size_t ligaOffset = data.size();
        patchFeatureTestU16(data, ligaPatch, static_cast<uint16_t>(ligaOffset));

        appendFeatureTestU16(data, 0);
        appendFeatureTestU16(data, 1);
        appendFeatureTestU16(data, 3);


        // smcp -> lookup 11

        const size_t smcpOffset = data.size();
        patchFeatureTestU16(data, smcpPatch, static_cast<uint16_t>(smcpOffset));

        appendFeatureTestU16(data, 0);
        appendFeatureTestU16(data, 1);
        appendFeatureTestU16(data, 11);


        // ccmp -> lookups 0, 1, 5

        const size_t ccmpOffset = data.size();
        patchFeatureTestU16(data, ccmpPatch, static_cast<uint16_t>(ccmpOffset));

        appendFeatureTestU16(data, 0);
        appendFeatureTestU16(data, 3);
        appendFeatureTestU16(data, 0);
        appendFeatureTestU16(data, 1);
        appendFeatureTestU16(data, 5);


        // rlig -> no lookups

        const size_t rligOffset = data.size();
        patchFeatureTestU16(data, rligPatch, static_cast<uint16_t>(rligOffset));

        appendFeatureTestU16(data, 0);
        appendFeatureTestU16(data, 0);

        return data;
    }


    static bool testOpenTypeLayoutFeatureView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType layout feature views: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // FeatureList metadata and tags.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeFeatureListTestData();
           OpenTypeLayoutFeatureListView features(ByteSpan(data.data(), data.size()));

            if (!features || features.size() != 5)
                return fail("case 1 FeatureList");

            const Tag expected[] = {
                OTAG("calt"),
                OTAG("liga"),
                OTAG("smcp"),
                OTAG("ccmp"),
                OTAG("rlig")
            };

            for (size_t i = 0; i < 5; ++i)
            {
                Tag tag = 0;

                if (!features.featureTag(i, tag) || tag != expected[i])
                    return fail("case 1 feature tags");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Feature table access by index.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeFeatureListTestData();
           OpenTypeLayoutFeatureListView features(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutFeatureView feature = features.feature(0);

            if (!feature)
                return fail("case 2 calt Feature");

            if (feature.featureParamsOffset() != 0)
                return fail("case 2 unexpected FeatureParams");

            if (feature.lookupCount() != 2)
                return fail("case 2 lookup count");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Lookup indices preserve Feature table order.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeFeatureListTestData();
           OpenTypeLayoutFeatureListView features(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutFeatureView feature = features.find(OTAG("ccmp"));

            if (!feature || feature.lookupCount() != 3)
                return fail("case 3 ccmp Feature");

            const uint16_t expected[] = { 0, 1, 5 };

            for (size_t i = 0; i < 3; ++i)
            {
                uint16_t lookupIndex = 0;

                if (!feature.lookupIndex(i, lookupIndex) || lookupIndex != expected[i])
                    return fail("case 3 lookup indices");
            }

            uint16_t lookupIndex = 0;

            if (feature.lookupIndex(3, lookupIndex))
                return fail("case 3 out-of-range lookup accepted");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Tag lookup and containment.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeFeatureListTestData();
           OpenTypeLayoutFeatureListView features(ByteSpan(data.data(), data.size()));

            if (!features.contains(OTAG("liga")) || !features.contains(OTAG("rlig")))
                return fail("case 4 expected feature missing");

            if (features.contains(OTAG("kern")))
                return fail("case 4 unexpected feature");

            const OpenTypeLayoutFeatureView liga = features.find(OTAG("liga"));

            if (!liga || liga.lookupCount() != 1)
                return fail("case 4 liga lookup");

            uint16_t lookupIndex = 0;

            if (!liga.lookupIndex(0, lookupIndex) || lookupIndex != 3)
                return fail("case 4 liga lookup index");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Lazy traversal.
        //
        // Corrupt smcp's Feature offset. Tag queries remain usable, and
        // unrelated Feature tables remain readable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeOpenTypeFeatureListTestData();

            const size_t smcpOffsetField = 2 + 2 * 6 + 4;
            patchFeatureTestU16(data, smcpOffsetField, 0xFFFF);

           OpenTypeLayoutFeatureListView features(ByteSpan(data.data(), data.size()));

            if (!features)
                return fail("case 5 FeatureList rejected lazy corruption");

            if (!features.contains(OTAG("smcp")) || !features.contains(OTAG("liga")))
                return fail("case 5 tag discovery damaged");

            if (features.find(OTAG("smcp")))
                return fail("case 5 malformed smcp Feature accepted");

            if (!features.find(OTAG("liga")))
                return fail("case 5 unrelated liga Feature damaged");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Structural failure paths.
        // ====================================================================

        {
            ++cases;

            const uint8_t truncatedListBytes[] = {
                0x00, 0x02,
                'l', 'i', 'g', 'a', 0x00, 0x08
            };

           OpenTypeLayoutFeatureListView truncatedList(
                ByteSpan(truncatedListBytes, sizeof(truncatedListBytes)));

            if (truncatedList)
                return fail("case 6 truncated FeatureList accepted");


            const uint8_t truncatedFeatureBytes[] = {
                0x00, 0x00,
                0x00, 0x02,
                0x00, 0x01
            };

            OpenTypeLayoutFeatureView truncatedFeature(
                ByteSpan(truncatedFeatureBytes, sizeof(truncatedFeatureBytes)));

            if (truncatedFeature)
                return fail("case 6 truncated Feature accepted");

            ++passed;
        }


        std::printf(
            "OpenType layout feature views: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  FeatureList:           PASS\n"
            "  Feature access:        PASS\n"
            "  Lookup indices:        PASS\n"
            "  Tag lookup:            PASS\n"
            "  Lazy traversal:        PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs