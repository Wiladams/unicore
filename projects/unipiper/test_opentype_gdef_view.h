// test_opentype_gdef_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gdef_view.h"

namespace waavs
{
    static void appendGdefTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGdefTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGdefTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void patchGdefTestU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // ClassDef Format 1
    //
    // Returns offset relative to the beginning of the containing GDEF.
    // ====================================================================

    static size_t appendGdefTestClassDef1(std::vector<uint8_t>& data,
        uint16_t startGlyph, const std::vector<uint16_t>& classes)
    {
        const size_t offset = data.size();

        appendGdefTestU16(data, 1);
        appendGdefTestU16(data, startGlyph);
        appendGdefTestU16(data, static_cast<uint16_t>(classes.size()));

        for (uint16_t value : classes)
            appendGdefTestU16(data, value);

        return offset;
    }


    // ====================================================================
    // ClassDef Format 2
    // ====================================================================

    struct GdefTestClassRange
    {
        uint16_t startGlyph{ 0 };
        uint16_t endGlyph{ 0 };
        uint16_t classValue{ 0 };
    };

    static size_t appendGdefTestClassDef2(std::vector<uint8_t>& data,
        const std::vector<GdefTestClassRange>& ranges)
    {
        const size_t offset = data.size();

        appendGdefTestU16(data, 2);
        appendGdefTestU16(data, static_cast<uint16_t>(ranges.size()));

        for (const GdefTestClassRange& range : ranges)
        {
            appendGdefTestU16(data, range.startGlyph);
            appendGdefTestU16(data, range.endGlyph);
            appendGdefTestU16(data, range.classValue);
        }

        return offset;
    }


    // ====================================================================
    // GDEF 1.0
    //
    // GlyphClassDef Format 1:
    //
    //   10 -> base       class 1
    //   11 -> ligature   class 2
    //   12 -> mark       class 3
    //   13 -> component  class 4
    //
    // MarkAttachClassDef Format 2:
    //
    //   12 -> attachment class 5
    //   20..21 -> attachment class 7
    // ====================================================================

    static std::vector<uint8_t> makeGdef10TestData()
    {
        std::vector<uint8_t> data;

        appendGdefTestU16(data, 1);
        appendGdefTestU16(data, 0);

        const size_t glyphClassPatch = data.size();
        appendGdefTestU16(data, 0);

        const size_t attachListPatch = data.size();
        appendGdefTestU16(data, 0);

        const size_t ligCaretPatch = data.size();
        appendGdefTestU16(data, 0);

        const size_t markAttachPatch = data.size();
        appendGdefTestU16(data, 0);


        const size_t glyphClassOffset =
            appendGdefTestClassDef1(data, 10, { 1, 2, 3, 4 });

        patchGdefTestU16(data, glyphClassPatch,
            static_cast<uint16_t>(glyphClassOffset));


        const size_t markAttachOffset =
            appendGdefTestClassDef2(data, {
                { 12, 12, 5 },
                { 20, 21, 7 }
                });

        patchGdefTestU16(data, markAttachPatch,
            static_cast<uint16_t>(markAttachOffset));


        // Raw AttachList placeholder.

        const size_t attachListOffset = data.size();
        patchGdefTestU16(data, attachListPatch,
            static_cast<uint16_t>(attachListOffset));

        appendGdefTestU16(data, 0xA101);
        appendGdefTestU16(data, 0xA102);


        // Raw LigCaretList placeholder.

        const size_t ligCaretOffset = data.size();
        patchGdefTestU16(data, ligCaretPatch,
            static_cast<uint16_t>(ligCaretOffset));

        appendGdefTestU16(data, 0xB201);
        appendGdefTestU16(data, 0xB202);

        return data;
    }


    // ====================================================================
    // GDEF 1.2 with MarkGlyphSetsDef raw child.
    // ====================================================================

    static std::vector<uint8_t> makeGdef12TestData()
    {
        std::vector<uint8_t> data;

        appendGdefTestU16(data, 1);
        appendGdefTestU16(data, 2);

        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);

        const size_t markGlyphSetsPatch = data.size();
        appendGdefTestU16(data, 0);

        const size_t markGlyphSetsOffset = data.size();

        patchGdefTestU16(data, markGlyphSetsPatch,
            static_cast<uint16_t>(markGlyphSetsOffset));

        // MarkGlyphSetsDef Format 1, zero mark sets.

        appendGdefTestU16(data, 1);
        appendGdefTestU16(data, 0);

        return data;
    }


    // ====================================================================
    // GDEF 1.3 with ItemVariationStore beyond 64K.
    // ====================================================================

    static std::vector<uint8_t> makeGdef13TestData()
    {
        static constexpr uint32_t kItemVarOffset = 0x00010020u;

        std::vector<uint8_t> data;

        appendGdefTestU16(data, 1);
        appendGdefTestU16(data, 3);

        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);
        appendGdefTestU16(data, 0);

        appendGdefTestU32(data, kItemVarOffset);

        data.resize(kItemVarOffset, 0);

        appendGdefTestU16(data, 0xDD01);
        appendGdefTestU16(data, 0xDD02);

        return data;
    }


    static bool testOpenTypeGdefView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GDEF view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // GDEF 1.0 header and child offsets.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGdef10TestData();
            const OpenTypeGdefView gdef(ByteSpan(data.data(), data.size()));

            if (!gdef)
                return fail("case 1 GDEF 1.0 invalid");

            if (gdef.majorVersion() != 1 || gdef.minorVersion() != 0)
                return fail("case 1 version");

            if (gdef.glyphClassDefOffset() == 0 ||
                gdef.attachListOffset() == 0 ||
                gdef.ligCaretListOffset() == 0 ||
                gdef.markAttachClassDefOffset() == 0)
            {
                return fail("case 1 child offsets");
            }

            if (gdef.markGlyphSetsDefOffset() != 0 ||
                gdef.itemVarStoreOffset() != 0)
            {
                return fail("case 1 later-version offsets");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // GlyphClassDef Format 1.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGdef10TestData();
            const OpenTypeGdefView gdef(ByteSpan(data.data(), data.size()));

            const OpenTypeClassDefView classes = gdef.glyphClassDef();

            if (!classes || classes.format() != 1)
                return fail("case 2 GlyphClassDef");

            uint16_t value = 0;

            if (!gdef.glyphClass(10, value) || value != 1)
                return fail("case 2 base class");

            if (!gdef.glyphClass(11, value) || value != 2)
                return fail("case 2 ligature class");

            if (!gdef.glyphClass(12, value) || value != 3)
                return fail("case 2 mark class");

            if (!gdef.glyphClass(13, value) || value != 4)
                return fail("case 2 component class");

            if (!gdef.glyphClass(99, value) || value != 0)
                return fail("case 2 implicit class zero");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // MarkAttachClassDef Format 2.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGdef10TestData();
            const OpenTypeGdefView gdef(ByteSpan(data.data(), data.size()));

            const OpenTypeClassDefView classes = gdef.markAttachClassDef();

            if (!classes || classes.format() != 2)
                return fail("case 3 MarkAttachClassDef");

            uint16_t value = 0;

            if (!gdef.markAttachClass(12, value) || value != 5)
                return fail("case 3 attachment class 5");

            if (!gdef.markAttachClass(20, value) || value != 7)
                return fail("case 3 attachment class 7 start");

            if (!gdef.markAttachClass(21, value) || value != 7)
                return fail("case 3 attachment class 7 end");

            if (!gdef.markAttachClass(22, value) || value != 0)
                return fail("case 3 implicit attachment class zero");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Nullable ClassDef children.
        //
        // An absent ClassDef means class zero, not failure.
        // ====================================================================

        {
            ++cases;

            const uint8_t data[] = {
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00
            };

            const OpenTypeGdefView gdef(
                ByteSpan(data, sizeof(data)));

            if (!gdef)
                return fail("case 4 nullable GDEF invalid");

            if (gdef.glyphClassDef() || gdef.markAttachClassDef())
                return fail("case 4 absent ClassDef exposed");

            uint16_t value = 99;

            if (!gdef.glyphClass(10, value) || value != 0)
                return fail("case 4 absent GlyphClassDef");

            value = 99;

            if (!gdef.markAttachClass(10, value) || value != 0)
                return fail("case 4 absent MarkAttachClassDef");

            if (gdef.glyphClass(0x10000u, value))
                return fail("case 4 oversized glyph accepted");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // GDEF 1.2 and typed MarkGlyphSetsDef access.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGdef12TestData();
            const OpenTypeGdefView gdef(ByteSpan(data.data(), data.size()));

            if (!gdef)
                return fail("case 5 GDEF 1.2 invalid");

            if (gdef.majorVersion() != 1 || gdef.minorVersion() != 2)
                return fail("case 5 version");

            if (gdef.markGlyphSetsDefOffset() == 0)
                return fail("case 5 MarkGlyphSetsDef offset");

            if (gdef.itemVarStoreOffset() != 0)
                return fail("case 5 unexpected ItemVariationStore");

            const OpenTypeMarkGlyphSetsView sets = gdef.markGlyphSetsDef();

            if (!sets)
                return fail("case 5 MarkGlyphSetsDef");

            if (sets.format() != 1 || sets.size() != 0)
                return fail("case 5 MarkGlyphSetsDef contents");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // GDEF 1.3 and real Offset32 ItemVariationStore.
        // ====================================================================

        {
            ++cases;

            static constexpr uint32_t kItemVarOffset = 0x00010020u;

            const std::vector<uint8_t> data = makeGdef13TestData();
            const OpenTypeGdefView gdef(ByteSpan(data.data(), data.size()));

            if (!gdef)
                return fail("case 6 GDEF 1.3 invalid");

            if (gdef.majorVersion() != 1 || gdef.minorVersion() != 3)
                return fail("case 6 version");

            if (gdef.itemVarStoreOffset() != kItemVarOffset)
                return fail("case 6 Offset32 truncated");

            const ByteSpan child = gdef.itemVarStore();

            if (!child || child.size() < 4)
                return fail("case 6 ItemVariationStore span");

            if (child.begin()[0] != 0xDD || child.begin()[1] != 0x01)
                return fail("case 6 ItemVariationStore data");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Lazy child validation.
        //
        // Corrupt GlyphClassDef after the GDEF header has been built.
        // GDEF itself remains valid and MarkAttachClassDef remains usable.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeGdef10TestData();

            const OpenTypeGdefView original(
                ByteSpan(data.data(), data.size()));

            const uint16_t glyphClassOffset =
                original.glyphClassDefOffset();

            // Unsupported ClassDef format.

            patchGdefTestU16(data, glyphClassOffset, 99);

            const OpenTypeGdefView gdef(
                ByteSpan(data.data(), data.size()));

            if (!gdef)
                return fail("case 7 parent rejected lazy child corruption");

            if (gdef.glyphClassDef())
                return fail("case 7 malformed GlyphClassDef accepted");

            uint16_t value = 0;

            if (gdef.glyphClass(10, value))
                return fail("case 7 malformed GlyphClassDef lookup succeeded");

            if (!gdef.markAttachClass(12, value) || value != 5)
                return fail("case 7 unrelated MarkAttachClassDef damaged");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Raw AttachList and LigCaretList access.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeGdef10TestData();
            const OpenTypeGdefView gdef(ByteSpan(data.data(), data.size()));

            const ByteSpan attach = gdef.attachList();
            const ByteSpan caret = gdef.ligCaretList();

            if (!attach || attach.size() < 4)
                return fail("case 8 AttachList");

            if (!caret || caret.size() < 4)
                return fail("case 8 LigCaretList");

            if (attach.begin()[0] != 0xA1 || attach.begin()[1] != 0x01)
                return fail("case 8 AttachList data");

            if (caret.begin()[0] != 0xB2 || caret.begin()[1] != 0x01)
                return fail("case 8 LigCaretList data");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Header/version/offset failure paths.
        // ====================================================================

        {
            ++cases;


            // Unsupported major version.

            const uint8_t badMajor[] = {
                0x00, 0x02,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00
            };

            if (OpenTypeGdefView(ByteSpan(badMajor, sizeof(badMajor))))
                return fail("case 9 unsupported major version accepted");


            // Unsupported minor version.

            const uint8_t badMinor[] = {
                0x00, 0x01,
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00
            };

            if (OpenTypeGdefView(ByteSpan(badMinor, sizeof(badMinor))))
                return fail("case 9 unsupported minor version accepted");


            // Truncated 1.2 header.

            const uint8_t truncated12[] = {
                0x00, 0x01,
                0x00, 0x02,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00
            };

            if (OpenTypeGdefView(ByteSpan(truncated12, sizeof(truncated12))))
                return fail("case 9 truncated 1.2 header accepted");


            // Child offset points inside the GDEF header.

            const uint8_t insideHeader[] = {
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x08,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00
            };

            if (OpenTypeGdefView(ByteSpan(insideHeader, sizeof(insideHeader))))
                return fail("case 9 header-relative child accepted");


            // Child offset outside available data.

            const uint8_t outsideData[] = {
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x20,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00
            };

            if (OpenTypeGdefView(ByteSpan(outsideData, sizeof(outsideData))))
                return fail("case 9 outside child accepted");

            ++passed;
        }


        std::printf(
            "OpenType GDEF view: PASS\n"
            "  Cases:                  %u\n"
            "  Passed:                 %u\n"
            "  GDEF 1.0:               PASS\n"
            "  GlyphClassDef:          PASS\n"
            "  MarkAttachClassDef:     PASS\n"
            "  Nullable classes:       PASS\n"
            "  GDEF 1.2:               PASS\n"
            "  GDEF 1.3 Offset32:      PASS\n"
            "  Lazy child validation:  PASS\n"
            "  Raw child access:       PASS\n"
            "  Failure paths:          PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs