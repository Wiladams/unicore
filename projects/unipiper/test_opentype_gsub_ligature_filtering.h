// test_opentype_gsub_ligature_filtering.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    static void appendGsubLigFilterU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGsubLigFilterU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubLigFilterU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void patchGsubLigFilterU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Append one Type 4 LigatureSubst subtable.
    //
    // firstGlyph + secondGlyph -> ligatureGlyph
    // ====================================================================

    static void appendGsubLigFilterType4Subtable(
        std::vector<uint8_t>& data, uint16_t firstGlyph,
        uint16_t secondGlyph, uint16_t ligatureGlyph)
    {
        const size_t subtableBase = data.size();

        appendGsubLigFilterU16(data, 1);              // substFormat

        const size_t coveragePatch = data.size();
        appendGsubLigFilterU16(data, 0);

        appendGsubLigFilterU16(data, 1);              // ligatureSetCount

        const size_t ligatureSetPatch = data.size();
        appendGsubLigFilterU16(data, 0);


        // ================================================================
        // Coverage Format 1
        // ================================================================

        const size_t coverageOffset = data.size() - subtableBase;
        patchGsubLigFilterU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubLigFilterU16(data, 1);
        appendGsubLigFilterU16(data, 1);
        appendGsubLigFilterU16(data, firstGlyph);


        // ================================================================
        // LigatureSet
        // ================================================================

        const size_t ligatureSetBase = data.size();
        const size_t ligatureSetOffset = ligatureSetBase - subtableBase;

        patchGsubLigFilterU16(data, ligatureSetPatch, static_cast<uint16_t>(ligatureSetOffset));

        appendGsubLigFilterU16(data, 1);              // ligatureCount

        const size_t ligaturePatch = data.size();
        appendGsubLigFilterU16(data, 0);


        // ================================================================
        // Ligature
        // ================================================================

        const size_t ligatureOffset = data.size() - ligatureSetBase;
        patchGsubLigFilterU16(data, ligaturePatch, static_cast<uint16_t>(ligatureOffset));

        appendGsubLigFilterU16(data, ligatureGlyph);
        appendGsubLigFilterU16(data, 2);              // componentCount
        appendGsubLigFilterU16(data, secondGlyph);
    }


    // ====================================================================
    // Ordinary LookupType 4.
    // ====================================================================

    static std::vector<uint8_t> makeGsubLigFilterLookup(
        uint16_t lookupFlag, uint16_t firstGlyph = 10,
        uint16_t secondGlyph = 20, uint16_t ligatureGlyph = 100)
    {
        std::vector<uint8_t> data;

        appendGsubLigFilterU16(data, 4);              // LookupType
        appendGsubLigFilterU16(data, lookupFlag);
        appendGsubLigFilterU16(data, 1);              // SubTableCount

        const size_t subtablePatch = data.size();
        appendGsubLigFilterU16(data, 0);

        const size_t subtableOffset = data.size();
        patchGsubLigFilterU16(data, subtablePatch, static_cast<uint16_t>(subtableOffset));

        appendGsubLigFilterType4Subtable(data, firstGlyph, secondGlyph, ligatureGlyph);

        return data;
    }


    // ====================================================================
    // LookupType 7 wrapping a Type 4 LigatureSubst.
    // ====================================================================

    static std::vector<uint8_t> makeGsubLigFilterExtensionLookup(
        uint16_t lookupFlag, uint16_t firstGlyph = 10,
        uint16_t secondGlyph = 20, uint16_t ligatureGlyph = 100)
    {
        std::vector<uint8_t> data;

        appendGsubLigFilterU16(data, 7);              // LookupType
        appendGsubLigFilterU16(data, lookupFlag);
        appendGsubLigFilterU16(data, 1);              // SubTableCount

        const size_t extensionPatch = data.size();
        appendGsubLigFilterU16(data, 0);

        const size_t extensionBase = data.size();
        patchGsubLigFilterU16(data, extensionPatch, static_cast<uint16_t>(extensionBase));


        // ExtensionSubst Format 1.

        appendGsubLigFilterU16(data, 1);
        appendGsubLigFilterU16(data, 4);              // extensionLookupType

        const size_t targetPatch = data.size();
        appendGsubLigFilterU32(data, 0);

        const size_t targetOffset = data.size() - extensionBase;
        patchGsubLigFilterU32(data, targetPatch, static_cast<uint32_t>(targetOffset));

        appendGsubLigFilterType4Subtable(data, firstGlyph, secondGlyph, ligatureGlyph);

        return data;
    }


    // ====================================================================
    // GDEF 1.0
    //
    // Glyph classes:
    //
    //   10..20   base glyphs
    //   30..31   mark glyphs
    // ====================================================================

    static std::vector<uint8_t> makeGsubLigFilterGdef()
    {
        std::vector<uint8_t> data;

        appendGsubLigFilterU16(data, 1);
        appendGsubLigFilterU16(data, 0);

        const size_t glyphClassPatch = data.size();
        appendGsubLigFilterU16(data, 0);

        appendGsubLigFilterU16(data, 0);              // AttachList
        appendGsubLigFilterU16(data, 0);              // LigCaretList
        appendGsubLigFilterU16(data, 0);              // MarkAttachClassDef


        // ================================================================
        // GlyphClassDef Format 2
        // ================================================================

        const size_t glyphClassOffset = data.size();
        patchGsubLigFilterU16(data, glyphClassPatch, static_cast<uint16_t>(glyphClassOffset));

        appendGsubLigFilterU16(data, 2);
        appendGsubLigFilterU16(data, 2);

        // 10..20 -> base glyph class 1.

        appendGsubLigFilterU16(data, 10);
        appendGsubLigFilterU16(data, 20);
        appendGsubLigFilterU16(data, 1);

        // 30..31 -> mark glyph class 3.

        appendGsubLigFilterU16(data, 30);
        appendGsubLigFilterU16(data, 31);
        appendGsubLigFilterU16(data, 3);

        return data;
    }


    static OpenTypeShapingBuffer makeGsubLigFilterBuffer(std::initializer_list<uint32_t> glyphIds)
    {
        OpenTypeShapingBuffer buffer;
        uint32_t scalarOffset = 0;

        for (uint32_t glyphId : glyphIds)
        {
            buffer.pushBack(OpenTypeShapingGlyph{ glyphId, scalarOffset, 1 });
            ++scalarOffset;
        }

        return buffer;
    }


    static bool testOpenTypeGsubLigatureFiltering()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB Type 4 filtering: FAIL\n  %s\n", message);
                return false;
            };

        const std::vector<uint8_t> gdefData = makeGsubLigFilterGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));

        if (!gdef)
            return fail("test GDEF invalid");


        // ====================================================================
        // Case 1
        //
        // Resolver records exact participating positions.
        //
        //   A mark B
        //
        // becomes positions:
        //
        //   { 0, 2 }
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            const OpenTypeLookupGlyphFilter filter(lookup, gdef);
            const OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 20 });

            if (!lookup || !filter)
                return fail("case 1 setup");

            OpenTypeGsubLigatureMatch match;

            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubLigatureLookup(lookup, filter, buffer, 0, match);

            if (result != OpenTypeGsubResolveResult::Match)
                return fail("case 1 resolver did not match");

            if (match.ligatureGlyph != 100)
                return fail("case 1 ligature glyph");

            if (match.positions.size() != 2 ||
                match.positions[0] != 0 ||
                match.positions[1] != 2)
            {
                return fail("case 1 participating positions");
            }

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // IgnoreMarks permits a ligature across an intervening mark.
        //
        // Before:
        //
        //   index      0    1     2
        //              A   mark   B
        //   scalar     0    1     2
        //
        // After:
        //
        //              L   mark
        //
        // L provenance is the bounding logical extent 0..3.
        // The mark survives unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 20 });

            if (!applyOpenTypeGsubLigatureLookup(lookup, gdef, buffer))
                return fail("case 2 apply failed");

            if (buffer.size() != 2)
                return fail("case 2 output size");

            if (buffer[0].glyphId != 100 || buffer[1].glyphId != 30)
                return fail("case 2 output glyphs");

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 3)
                return fail("case 2 ligature provenance");

            if (buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1)
                return fail("case 2 ignored mark provenance");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Without IgnoreMarks, the intervening mark blocks the ligature.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData = makeGsubLigFilterLookup(0);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 20 });

            if (!applyOpenTypeGsubLigatureLookup(lookup, buffer))
                return fail("case 3 apply failed");

            if (buffer.size() != 3 ||
                buffer[0].glyphId != 10 ||
                buffer[1].glyphId != 30 ||
                buffer[2].glyphId != 20)
            {
                return fail("case 3 mark did not block ligature");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Existing adjacent Type 4 behavior remains unchanged.
        //
        //   A B -> L
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData = makeGsubLigFilterLookup(0);
            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 20 });

            if (!applyOpenTypeGsubLigatureLookup(lookup, buffer))
                return fail("case 4 apply failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 100)
                return fail("case 4 adjacent ligature");

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 2)
                return fail("case 4 provenance");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Multiple ignored marks survive and remain in stored order.
        //
        //   A m1 m2 B
        //
        // becomes:
        //
        //   L m1 m2
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 31, 20 });

            if (!applyOpenTypeGsubLigatureLookup(lookup, gdef, buffer))
                return fail("case 5 apply failed");

            if (buffer.size() != 3)
                return fail("case 5 output size");

            if (buffer[0].glyphId != 100 ||
                buffer[1].glyphId != 30 ||
                buffer[2].glyphId != 31)
            {
                return fail("case 5 surviving glyph order");
            }

            if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 4)
                return fail("case 5 ligature provenance");

            if (buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1 ||
                buffer[2].scalarOffset != 2 || buffer[2].scalarCount != 1)
            {
                return fail("case 5 mark provenance");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // LookupFlag filtering does not reject the current/start glyph.
        //
        // Glyph 30 is a mark, and IgnoreMarks is active, but the lookup may
        // still begin at glyph 30. Filtering applies while finding subsequent
        // components.
        //
        //   mark B -> L
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterLookup(kOpenTypeLookupFlagIgnoreMarks, 30, 20, 200);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 30, 20 });

            if (!applyOpenTypeGsubLigatureLookup(lookup, gdef, buffer))
                return fail("case 6 apply failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 200)
                return fail("case 6 current mark was incorrectly filtered");

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // GDEF-aware general dispatcher reaches filtered Type 4.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 20 });

            if (!applyOpenTypeGsubLookup(lookup, gdef, buffer))
                return fail("case 7 dispatcher failed");

            if (buffer.size() != 2 ||
                buffer[0].glyphId != 100 ||
                buffer[1].glyphId != 30)
            {
                return fail("case 7 dispatcher result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Extension LookupType 7 delegates GDEF filtering to effective
        // LookupType 4.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterExtensionLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 20 });

            if (!applyOpenTypeGsubLookup(lookup, gdef, buffer))
                return fail("case 8 extension dispatcher failed");

            if (buffer.size() != 2 ||
                buffer[0].glyphId != 100 ||
                buffer[1].glyphId != 30)
            {
                return fail("case 8 extension filtering result");
            }

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // A filtering lookup called through the legacy no-GDEF overload must
        // fail rather than silently shape incorrectly. The buffer must remain
        // unchanged.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubLigFilterLookup(kOpenTypeLookupFlagIgnoreMarks);

            const OpenTypeLayoutLookupView lookup(ByteSpan(lookupData.data(), lookupData.size()));
            OpenTypeShapingBuffer buffer = makeGsubLigFilterBuffer({ 10, 30, 20 });

            if (applyOpenTypeGsubLigatureLookup(lookup, buffer))
                return fail("case 9 filtering succeeded without GDEF");

            if (buffer.size() != 3 ||
                buffer[0].glyphId != 10 ||
                buffer[1].glyphId != 30 ||
                buffer[2].glyphId != 20)
            {
                return fail("case 9 failed lookup modified buffer");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB Type 4 filtering: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Exact match positions:    PASS\n"
            "  IgnoreMarks ligature:     PASS\n"
            "  Unfiltered blocking:      PASS\n"
            "  Adjacent compatibility:   PASS\n"
            "  Ignored mark survival:    PASS\n"
            "  Start glyph semantics:    PASS\n"
            "  GDEF-aware dispatcher:    PASS\n"
            "  Extension Type 7:         PASS\n"
            "  Missing GDEF failure:     PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs