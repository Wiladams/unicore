// test_opentype_lookup_glyph_filter.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_lookup_glyph_filter.h"

namespace waavs
{
    static void appendLookupFilterTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendLookupFilterTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchLookupFilterTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void patchLookupFilterTestU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Lookup test data
    //
    // The actual subtable contents are irrelevant to filtering. We provide
    // one small child solely to make a structurally normal Lookup table.
    // ====================================================================

    static std::vector<uint8_t> makeLookupFilterTestLookup(uint16_t lookupFlag, uint16_t markFilteringSet = 0)
    {
        std::vector<uint8_t> data;

        appendLookupFilterTestU16(data, 1);          // LookupType
        appendLookupFilterTestU16(data, lookupFlag);
        appendLookupFilterTestU16(data, 1);          // SubTableCount

        const size_t subtableOffsetPatch = data.size();
        appendLookupFilterTestU16(data, 0);

        if ((lookupFlag & kOpenTypeLookupFlagUseMarkFilteringSet) != 0)
            appendLookupFilterTestU16(data, markFilteringSet);

        const size_t subtableOffset = data.size();
        patchLookupFilterTestU16(data, subtableOffsetPatch, static_cast<uint16_t>(subtableOffset));

        appendLookupFilterTestU16(data, 0x1234);

        return data;
    }


    // ====================================================================
    // GDEF test data
    //
    // Glyph classes:
    //
    //   glyph 10       base       class 1
    //   glyph 20       ligature   class 2
    //   glyph 30..32   marks      class 3
    //   glyph 40       component  class 4
    //   glyph 50       unclassified / implicit class 0
    //
    // Mark attachment classes:
    //
    //   glyph 30       class 1
    //   glyph 31       class 2
    //   glyph 32       class 1
    //
    // Mark filtering sets:
    //
    //   set 0          { 30, 32 }
    //   set 1          { 31 }
    // ====================================================================

    static std::vector<uint8_t> makeLookupFilterTestGdef()
    {
        std::vector<uint8_t> data;

        // GDEF 1.2 header.

        appendLookupFilterTestU16(data, 1);
        appendLookupFilterTestU16(data, 2);

        const size_t glyphClassPatch = data.size();
        appendLookupFilterTestU16(data, 0);

        appendLookupFilterTestU16(data, 0);          // AttachList
        appendLookupFilterTestU16(data, 0);          // LigCaretList

        const size_t markAttachPatch = data.size();
        appendLookupFilterTestU16(data, 0);

        const size_t markGlyphSetsPatch = data.size();
        appendLookupFilterTestU16(data, 0);


        // ================================================================
        // GlyphClassDef - Format 2
        // ================================================================

        const size_t glyphClassOffset = data.size();
        patchLookupFilterTestU16(data, glyphClassPatch, static_cast<uint16_t>(glyphClassOffset));

        appendLookupFilterTestU16(data, 2);          // ClassDef Format 2
        appendLookupFilterTestU16(data, 4);          // range count

        appendLookupFilterTestU16(data, 10);
        appendLookupFilterTestU16(data, 10);
        appendLookupFilterTestU16(data, 1);

        appendLookupFilterTestU16(data, 20);
        appendLookupFilterTestU16(data, 20);
        appendLookupFilterTestU16(data, 2);

        appendLookupFilterTestU16(data, 30);
        appendLookupFilterTestU16(data, 32);
        appendLookupFilterTestU16(data, 3);

        appendLookupFilterTestU16(data, 40);
        appendLookupFilterTestU16(data, 40);
        appendLookupFilterTestU16(data, 4);


        // ================================================================
        // MarkAttachClassDef - Format 2
        // ================================================================

        const size_t markAttachOffset = data.size();
        patchLookupFilterTestU16(data, markAttachPatch, static_cast<uint16_t>(markAttachOffset));

        appendLookupFilterTestU16(data, 2);
        appendLookupFilterTestU16(data, 3);

        appendLookupFilterTestU16(data, 30);
        appendLookupFilterTestU16(data, 30);
        appendLookupFilterTestU16(data, 1);

        appendLookupFilterTestU16(data, 31);
        appendLookupFilterTestU16(data, 31);
        appendLookupFilterTestU16(data, 2);

        appendLookupFilterTestU16(data, 32);
        appendLookupFilterTestU16(data, 32);
        appendLookupFilterTestU16(data, 1);


        // ================================================================
        // MarkGlyphSetsDef
        //
        // Format 1, two Coverage tables.
        // ================================================================

        const size_t markGlyphSetsOffset = data.size();
        patchLookupFilterTestU16(data, markGlyphSetsPatch, static_cast<uint16_t>(markGlyphSetsOffset));

        appendLookupFilterTestU16(data, 1);
        appendLookupFilterTestU16(data, 2);

        const size_t set0Patch = data.size();
        appendLookupFilterTestU32(data, 0);

        const size_t set1Patch = data.size();
        appendLookupFilterTestU32(data, 0);


        // Set 0: glyphs 30, 32.

        const size_t set0Offset = data.size() - markGlyphSetsOffset;
        patchLookupFilterTestU32(data, set0Patch, static_cast<uint32_t>(set0Offset));

        appendLookupFilterTestU16(data, 1);
        appendLookupFilterTestU16(data, 2);
        appendLookupFilterTestU16(data, 30);
        appendLookupFilterTestU16(data, 32);


        // Set 1: glyph 31.

        const size_t set1Offset = data.size() - markGlyphSetsOffset;
        patchLookupFilterTestU32(data, set1Patch, static_cast<uint32_t>(set1Offset));

        appendLookupFilterTestU16(data, 1);
        appendLookupFilterTestU16(data, 1);
        appendLookupFilterTestU16(data, 31);

        return data;
    }


    static OpenTypeShapingBuffer makeLookupFilterTestBuffer()
    {
        OpenTypeShapingBuffer buffer;

        buffer.pushBack(OpenTypeShapingGlyph{ 10, 0, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 30, 1, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 31, 2, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 32, 3, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 20, 4, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 40, 5, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 50, 6, 1 });
        buffer.pushBack(OpenTypeShapingGlyph{ 10, 7, 1 });

        return buffer;
    }


    static bool testOpenTypeLookupGlyphFilter()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType lookup glyph filter: FAIL\n  %s\n", message);
                return false;
            };

        const std::vector<uint8_t> gdefData = makeLookupFilterTestGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

        if (!gdef)
            return fail("test GDEF invalid");


        // ====================================================================
        // Case 1
        //
        // No filtering flags.
        //
        // GDEF is not required and every glyph participates.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(0);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeGdefView noGdef{};
            const OpenTypeLookupGlyphFilter filter(lookup, noGdef);

            if (!filter)
                return fail("case 1 filter invalid");

            bool skip = true;

            if (!filter.shouldSkip(10, skip) || skip)
                return fail("case 1 base unexpectedly skipped");

            if (!filter.shouldSkip(30, skip) || skip)
                return fail("case 1 mark unexpectedly skipped");

            if (!filter.shouldSkip(50, skip) || skip)
                return fail("case 1 unclassified unexpectedly skipped");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // IgnoreBaseGlyphs, IgnoreLigatures, IgnoreMarks.
        // ====================================================================

        {
            ++cases;

            const uint16_t flags =
                kOpenTypeLookupFlagIgnoreBaseGlyphs |
                kOpenTypeLookupFlagIgnoreLigatures |
                kOpenTypeLookupFlagIgnoreMarks;

            const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(flags);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            if (!filter)
                return fail("case 2 filter invalid");

            bool skip = false;

            if (!filter.shouldSkip(10, skip) || !skip)
                return fail("case 2 base not skipped");

            if (!filter.shouldSkip(20, skip) || !skip)
                return fail("case 2 ligature not skipped");

            if (!filter.shouldSkip(30, skip) || !skip)
                return fail("case 2 mark not skipped");

            if (!filter.shouldSkip(40, skip) || skip)
                return fail("case 2 component skipped");

            if (!filter.shouldSkip(50, skip) || skip)
                return fail("case 2 unclassified skipped");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // MarkAttachmentType = 1.
        //
        // Marks in attachment class 1 participate.
        // Other marks are ignored.
        // ====================================================================

        {
            ++cases;

            const uint16_t flags = 0x0100u;

            const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(flags);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            if (!filter)
                return fail("case 3 filter invalid");

            if (filter.markAttachmentType() != 1)
                return fail("case 3 attachment type");

            bool skip = false;

            if (!filter.shouldSkip(30, skip) || skip)
                return fail("case 3 attachment class 1 glyph 30 skipped");

            if (!filter.shouldSkip(31, skip) || !skip)
                return fail("case 3 attachment class 2 glyph 31 accepted");

            if (!filter.shouldSkip(32, skip) || skip)
                return fail("case 3 attachment class 1 glyph 32 skipped");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // UseMarkFilteringSet, set 0 = { 30, 32 }.
        // ====================================================================

        {
            ++cases;

            const uint16_t flags = kOpenTypeLookupFlagUseMarkFilteringSet;

            const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(flags, 0);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            if (!filter)
                return fail("case 4 filter invalid");

            if (filter.markFilteringSet() != 0)
                return fail("case 4 filtering set index");

            bool skip = false;

            if (!filter.shouldSkip(30, skip) || skip)
                return fail("case 4 set member 30 skipped");

            if (!filter.shouldSkip(31, skip) || !skip)
                return fail("case 4 non-member 31 accepted");

            if (!filter.shouldSkip(32, skip) || skip)
                return fail("case 4 set member 32 skipped");

            if (!filter.shouldSkip(10, skip) || skip)
                return fail("case 4 base affected by mark set");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Mark filtering set takes precedence over MarkAttachmentType.
        //
        // Set 0 is {30,32}.
        // Attachment type 2 would normally accept 31 and reject 30/32.
        //
        // With both specified, the filtering set wins.
        // ====================================================================

        {
            ++cases;

            const uint16_t flags =
                kOpenTypeLookupFlagUseMarkFilteringSet |
                0x0200u;

            const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(flags, 0);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            if (!filter)
                return fail("case 5 filter invalid");

            bool skip = false;

            if (!filter.shouldSkip(30, skip) || skip)
                return fail("case 5 filtering set did not override attachment type");

            if (!filter.shouldSkip(31, skip) || !skip)
                return fail("case 5 attachment type incorrectly overrode filtering set");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // IgnoreMarks takes precedence over both mark filtering mechanisms.
        // ====================================================================

        {
            ++cases;

            const uint16_t flags =
                kOpenTypeLookupFlagIgnoreMarks |
                kOpenTypeLookupFlagUseMarkFilteringSet |
                0x0100u;

            const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(flags, 0);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);

            if (!filter)
                return fail("case 6 filter invalid");

            bool skip = false;

            if (!filter.shouldSkip(30, skip) || !skip)
                return fail("case 6 mark 30 not skipped");

            if (!filter.shouldSkip(31, skip) || !skip)
                return fail("case 6 mark 31 not skipped");

            if (!filter.shouldSkip(32, skip) || !skip)
                return fail("case 6 mark 32 not skipped");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Forward and backward traversal.
        //
        // Buffer:
        //
        //   10  30  31  32  20  40  50  10
        //
        // IgnoreMarks:
        //
        //   10  ----------- 20  40  50  10
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeLookupFilterTestLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeShapingBuffer buffer = makeLookupFilterTestBuffer();

            if (!filter)
                return fail("case 7 filter invalid");

            size_t index = 0;

            if (filter.next(buffer, 0, index) != OpenTypeLookupGlyphSearchResult::Found || index != 4)
                return fail("case 7 next did not skip marks");

            if (filter.next(buffer, 4, index) != OpenTypeLookupGlyphSearchResult::Found || index != 5)
                return fail("case 7 next adjacent participant");

            if (filter.previous(buffer, 4, index) != OpenTypeLookupGlyphSearchResult::Found || index != 0)
                return fail("case 7 previous did not skip marks");

            if (filter.previous(buffer, 0, index) != OpenTypeLookupGlyphSearchResult::End)
                return fail("case 7 previous beginning");

            if (filter.next(buffer, 7, index) != OpenTypeLookupGlyphSearchResult::End)
                return fail("case 7 next end");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Required GDEF data and invalid filtering-set index.
        // ====================================================================

        {
            ++cases;

            const OpenTypeGdefView noGdef{};

            {
                const std::vector<uint8_t> lookupData =
                    makeLookupFilterTestLookup(kOpenTypeLookupFlagIgnoreMarks);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeLookupGlyphFilter filter(lookup, noGdef);

                if (filter)
                    return fail("case 8 filtering accepted without GDEF");
            }

            {
                const std::vector<uint8_t> lookupData =
                    makeLookupFilterTestLookup(kOpenTypeLookupFlagUseMarkFilteringSet, 2);

                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeLookupGlyphFilter filter(lookup, gdef);

                if (filter)
                    return fail("case 8 out-of-range filtering set accepted");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Reserved LookupFlag bits and invalid calls.
        // ====================================================================

        {
            ++cases;

            {
                const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(0x0020u);
                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeLookupGlyphFilter filter(lookup, gdef);

                if (filter)
                    return fail("case 9 reserved LookupFlag accepted");
            }

            {
                const std::vector<uint8_t> lookupData = makeLookupFilterTestLookup(0);
                const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
                const OpenTypeGdefView noGdef{};
                const OpenTypeLookupGlyphFilter filter(lookup, noGdef);
                const OpenTypeShapingBuffer buffer = makeLookupFilterTestBuffer();

                bool skip = false;
                size_t index = 0;

                if (filter.shouldSkip(0x10000u, skip))
                    return fail("case 9 oversized glyph accepted");

                if (filter.next(buffer, buffer.size(), index) != OpenTypeLookupGlyphSearchResult::Invalid)
                    return fail("case 9 invalid next index accepted");

                if (filter.previous(buffer, buffer.size(), index) != OpenTypeLookupGlyphSearchResult::Invalid)
                    return fail("case 9 invalid previous index accepted");
            }

            ++passed;
        }


        std::printf(
            "OpenType lookup glyph filter: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  No filtering:             PASS\n"
            "  Ignore classes:           PASS\n"
            "  MarkAttachmentType:       PASS\n"
            "  MarkFilteringSet:         PASS\n"
            "  Filtering-set precedence: PASS\n"
            "  IgnoreMarks precedence:   PASS\n"
            "  Next/previous traversal:  PASS\n"
            "  Required GDEF data:       PASS\n"
            "  Failure paths:            PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs