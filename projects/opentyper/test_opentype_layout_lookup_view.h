// test_opentype_layout_lookup_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_layout_view.h"

namespace waavs
{
    static void appendLookupTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchLookupTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static std::vector<uint8_t> makeOpenTypeLookupListTestData()
    {
        std::vector<uint8_t> data;

        // ================================================================
        // LookupList
        //
        // lookup 0:
        //   type 1
        //   flag 0
        //   two subtables
        //
        // lookup 1:
        //   type 4
        //   UseMarkFilteringSet
        //   one subtable
        //   markFilteringSet = 7
        //
        // lookup 2:
        //   type 6
        //   flag 0x0008
        //   one subtable
        // ================================================================

        appendLookupTestU16(data, 3);

        const size_t lookup0Patch = data.size();
        appendLookupTestU16(data, 0);

        const size_t lookup1Patch = data.size();
        appendLookupTestU16(data, 0);

        const size_t lookup2Patch = data.size();
        appendLookupTestU16(data, 0);


        // ================================================================
        // Lookup 0
        // ================================================================

        const size_t lookup0Offset = data.size();
        patchLookupTestU16(data, lookup0Patch, static_cast<uint16_t>(lookup0Offset));

        appendLookupTestU16(data, 1);      // lookupType
        appendLookupTestU16(data, 0);      // lookupFlag
        appendLookupTestU16(data, 2);      // subTableCount

        const size_t lookup0Sub0Patch = data.size();
        appendLookupTestU16(data, 0);

        const size_t lookup0Sub1Patch = data.size();
        appendLookupTestU16(data, 0);

        const size_t lookup0Sub0Offset = data.size() - lookup0Offset;
        patchLookupTestU16(data, lookup0Sub0Patch, static_cast<uint16_t>(lookup0Sub0Offset));

        data.push_back(0x11);
        data.push_back(0x22);
        data.push_back(0x33);

        const size_t lookup0Sub1Offset = data.size() - lookup0Offset;
        patchLookupTestU16(data, lookup0Sub1Patch, static_cast<uint16_t>(lookup0Sub1Offset));

        data.push_back(0x44);
        data.push_back(0x55);
        data.push_back(0x66);
        data.push_back(0x77);


        // ================================================================
        // Lookup 1
        // ================================================================

        const size_t lookup1Offset = data.size();
        patchLookupTestU16(data, lookup1Patch, static_cast<uint16_t>(lookup1Offset));

        appendLookupTestU16(data, 4);      // lookupType
        appendLookupTestU16(data, kOpenTypeLookupFlagUseMarkFilteringSet);
        appendLookupTestU16(data, 1);      // subTableCount

        const size_t lookup1Sub0Patch = data.size();
        appendLookupTestU16(data, 0);

        appendLookupTestU16(data, 7);      // markFilteringSet

        const size_t lookup1Sub0Offset = data.size() - lookup1Offset;
        patchLookupTestU16(data, lookup1Sub0Patch, static_cast<uint16_t>(lookup1Sub0Offset));

        data.push_back(0x88);
        data.push_back(0x99);


        // ================================================================
        // Lookup 2
        // ================================================================

        const size_t lookup2Offset = data.size();
        patchLookupTestU16(data, lookup2Patch, static_cast<uint16_t>(lookup2Offset));

        appendLookupTestU16(data, 6);      // lookupType
        appendLookupTestU16(data, 0x0008); // arbitrary preserved lookup flag
        appendLookupTestU16(data, 1);      // subTableCount

        const size_t lookup2Sub0Patch = data.size();
        appendLookupTestU16(data, 0);

        const size_t lookup2Sub0Offset = data.size() - lookup2Offset;
        patchLookupTestU16(data, lookup2Sub0Patch, static_cast<uint16_t>(lookup2Sub0Offset));

        data.push_back(0xAA);
        data.push_back(0xBB);
        data.push_back(0xCC);

        return data;
    }


    static bool testOpenTypeLayoutLookupView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType layout lookup views: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // LookupList metadata and offsets.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLookupListTestData();
            OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 3)
                return fail("case 1 LookupList");

            uint16_t offset0 = 0;
            uint16_t offset1 = 0;
            uint16_t offset2 = 0;

            if (!lookups.lookupOffset(0, offset0) ||
                !lookups.lookupOffset(1, offset1) ||
                !lookups.lookupOffset(2, offset2))
            {
                return fail("case 1 lookup offsets");
            }

            if (offset0 == 0 || offset1 <= offset0 || offset2 <= offset1)
                return fail("case 1 unexpected lookup offset ordering");

            uint16_t dummy = 0;

            if (lookups.lookupOffset(3, dummy))
                return fail("case 1 out-of-range lookup accepted");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Lookup header fields.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLookupListTestData();
            OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutLookupView lookup0 = lookups.lookup(0);
            const OpenTypeLayoutLookupView lookup2 = lookups.lookup(2);

            if (!lookup0 || !lookup2)
                return fail("case 2 lookup access");

            if (lookup0.lookupType() != 1 || lookup0.lookupFlag() != 0 || lookup0.subtableCount() != 2)
                return fail("case 2 lookup 0 header");

            if (lookup2.lookupType() != 6 || lookup2.lookupFlag() != 0x0008 || lookup2.subtableCount() != 1)
                return fail("case 2 lookup 2 header");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Subtable offsets and raw subtable views.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLookupListTestData();
            OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutLookupView lookup = lookups.lookup(0);

            if (!lookup)
                return fail("case 3 lookup");

            uint16_t offset0 = 0;
            uint16_t offset1 = 0;

            if (!lookup.subtableOffset(0, offset0) || !lookup.subtableOffset(1, offset1))
                return fail("case 3 subtable offsets");

            if (offset0 != 10 || offset1 != 13)
                return fail("case 3 wrong subtable offsets");

            const ByteSpan subtable0 = lookup.subtable(0);
            const ByteSpan subtable1 = lookup.subtable(1);

            if (!subtable0 || !subtable1)
                return fail("case 3 subtable views");

            if (subtable0.size() < 3 || subtable0[0] != 0x11 || subtable0[1] != 0x22 || subtable0[2] != 0x33)
                return fail("case 3 subtable 0 data");

            if (subtable1.size() < 4 || subtable1[0] != 0x44 || subtable1[1] != 0x55 ||
                subtable1[2] != 0x66 || subtable1[3] != 0x77)
            {
                return fail("case 3 subtable 1 data");
            }

            if (lookup.subtable(2))
                return fail("case 3 out-of-range subtable accepted");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // UseMarkFilteringSet.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLookupListTestData();
            OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutLookupView lookup0 = lookups.lookup(0);
            const OpenTypeLayoutLookupView lookup1 = lookups.lookup(1);

            if (!lookup0 || !lookup1)
                return fail("case 4 lookup access");

            if (lookup0.usesMarkFilteringSet())
                return fail("case 4 unexpected mark filtering set");

            if (!lookup1.usesMarkFilteringSet())
                return fail("case 4 missing mark filtering set");

            uint16_t markFilteringSet = 0;

            if (!lookup1.markFilteringSet(markFilteringSet) || markFilteringSet != 7)
                return fail("case 4 mark filtering set value");

            if (lookup0.markFilteringSet(markFilteringSet))
                return fail("case 4 mark filtering set available without flag");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Lazy traversal.
        //
        // A malformed Lookup offset does not invalidate the LookupList or any
        // unrelated Lookup.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeOpenTypeLookupListTestData();

            // LookupList layout:
            //
            // uint16 count
            // Offset16 lookupOffsets[3]
            //
            // Corrupt lookup 1 only.
            patchLookupTestU16(data, 4, 0xFFFF);

            OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));

            if (!lookups)
                return fail("case 5 LookupList rejected lazy corruption");

            uint16_t offset = 0;

            if (!lookups.lookupOffset(1, offset) || offset != 0xFFFF)
                return fail("case 5 raw corrupted offset unavailable");

            if (lookups.lookup(1))
                return fail("case 5 malformed lookup accepted");

            if (!lookups.lookup(0) || !lookups.lookup(2))
                return fail("case 5 unrelated lookup damaged");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // A malformed subtable offset does not invalidate the Lookup itself.
        // The failure occurs only when that subtable is dereferenced.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeOpenTypeLookupListTestData();

            uint16_t lookup0Offset = 0;
            {
                OpenTypeLayoutLookupListView original(ByteSpan(data.data(), data.size()));

                if (!original.lookupOffset(0, lookup0Offset))
                    return fail("case 6 lookup offset setup");
            }

            // First subtable offset starts six bytes into Lookup 0.
            patchLookupTestU16(data, size_t(lookup0Offset) + 6, 0xFFFF);

            OpenTypeLayoutLookupListView lookups(ByteSpan(data.data(), data.size()));
            const OpenTypeLayoutLookupView lookup = lookups.lookup(0);

            if (!lookup)
                return fail("case 6 Lookup rejected lazy subtable corruption");

            uint16_t offset = 0;

            if (!lookup.subtableOffset(0, offset) || offset != 0xFFFF)
                return fail("case 6 raw corrupted subtable offset unavailable");

            if (lookup.subtable(0))
                return fail("case 6 malformed subtable accepted");

            if (!lookup.subtable(1))
                return fail("case 6 unrelated subtable damaged");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Structural failure paths.
        // ====================================================================

        {
            ++cases;

            const uint8_t truncatedListBytes[] = {
                0x00, 0x02,
                0x00, 0x06
            };

            OpenTypeLayoutLookupListView truncatedList(ByteSpan(truncatedListBytes, sizeof(truncatedListBytes)));

            if (truncatedList)
                return fail("case 7 truncated LookupList accepted");


            const uint8_t truncatedLookupBytes[] = {
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x02,
                0x00, 0x0A
            };

            OpenTypeLayoutLookupView truncatedLookup(ByteSpan(truncatedLookupBytes, sizeof(truncatedLookupBytes)));

            if (truncatedLookup)
                return fail("case 7 truncated Lookup accepted");


            const uint8_t missingMarkSetBytes[] = {
                0x00, 0x04,
                0x00, 0x10,
                0x00, 0x01,
                0x00, 0x08
            };

            OpenTypeLayoutLookupView missingMarkSet(ByteSpan(missingMarkSetBytes, sizeof(missingMarkSetBytes)));

            if (missingMarkSet)
                return fail("case 7 missing MarkFilteringSet accepted");

            ++passed;
        }


        std::printf(
            "OpenType layout lookup views: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  LookupList:            PASS\n"
            "  Lookup headers:        PASS\n"
            "  Subtable views:        PASS\n"
            "  Mark filtering set:    PASS\n"
            "  Lazy lookup access:    PASS\n"
            "  Lazy subtable access:  PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs