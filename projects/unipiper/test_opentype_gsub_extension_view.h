// test_opentype_gsub_extension_view.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_extension_view.h"
#include "opentype_gsub_single_view.h"
#include "opentype_gsub_multiple_view.h"
#include "opentype_gsub_alternate_view.h"
#include "opentype_gsub_ligature_view.h"

namespace waavs
{
    static void appendGsubExtensionTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGsubExtensionTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubExtensionTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void patchGsubExtensionTestU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 24);
        data[offset + 1] = static_cast<uint8_t>(value >> 16);
        data[offset + 2] = static_cast<uint8_t>(value >> 8);
        data[offset + 3] = static_cast<uint8_t>(value);
    }

    static std::vector<uint8_t> wrapGsubExtensionTestSubtable(
        uint16_t lookupType, const std::vector<uint8_t>& target, uint32_t targetOffset = 8)
    {
        std::vector<uint8_t> data;

        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, lookupType);
        appendGsubExtensionTestU32(data, targetOffset);

        if (data.size() < targetOffset)
            data.resize(targetOffset, 0);

        data.insert(data.end(), target.begin(), target.end());

        return data;
    }


    // ====================================================================
    // Small valid Type 1 target:
    //
    //   glyph 10 -> 11
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionSingleTarget()
    {
        std::vector<uint8_t> data;

        // SingleSubst Format 1

        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 6);
        appendGsubExtensionTestU16(data, 1);

        // Coverage Format 1

        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 10);

        return data;
    }


    // ====================================================================
    // Small valid Type 2 target:
    //
    //   glyph 20 -> 200, 201
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionMultipleTarget()
    {
        std::vector<uint8_t> data;

        // MultipleSubst Format 1

        appendGsubExtensionTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubExtensionTestU16(data, 0);

        appendGsubExtensionTestU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubExtensionTestU16(data, 0);


        // Sequence

        const size_t sequenceOffset = data.size();
        patchGsubExtensionTestU16(data, sequencePatch, static_cast<uint16_t>(sequenceOffset));

        appendGsubExtensionTestU16(data, 2);
        appendGsubExtensionTestU16(data, 200);
        appendGsubExtensionTestU16(data, 201);


        // Coverage

        const size_t coverageOffset = data.size();
        patchGsubExtensionTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 20);

        return data;
    }


    // ====================================================================
    // Small valid Type 3 target:
    //
    //   glyph 30 -> {300, 301}
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionAlternateTarget()
    {
        std::vector<uint8_t> data;

        // AlternateSubst Format 1

        appendGsubExtensionTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubExtensionTestU16(data, 0);

        appendGsubExtensionTestU16(data, 1);

        const size_t setPatch = data.size();
        appendGsubExtensionTestU16(data, 0);


        // AlternateSet

        const size_t setOffset = data.size();
        patchGsubExtensionTestU16(data, setPatch, static_cast<uint16_t>(setOffset));

        appendGsubExtensionTestU16(data, 2);
        appendGsubExtensionTestU16(data, 300);
        appendGsubExtensionTestU16(data, 301);


        // Coverage

        const size_t coverageOffset = data.size();
        patchGsubExtensionTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 30);

        return data;
    }


    // ====================================================================
    // Small valid Type 4 target:
    //
    //   40, 41 -> 400
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionLigatureTarget()
    {
        std::vector<uint8_t> data;

        // LigatureSubst Format 1

        appendGsubExtensionTestU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubExtensionTestU16(data, 0);

        appendGsubExtensionTestU16(data, 1);

        const size_t setPatch = data.size();
        appendGsubExtensionTestU16(data, 0);


        // LigatureSet

        const size_t setOffset = data.size();
        patchGsubExtensionTestU16(data, setPatch, static_cast<uint16_t>(setOffset));

        appendGsubExtensionTestU16(data, 1);

        const size_t ligaturePatch = data.size();
        appendGsubExtensionTestU16(data, 0);


        // Ligature

        const size_t ligatureOffset = data.size() - setOffset;
        patchGsubExtensionTestU16(data, ligaturePatch, static_cast<uint16_t>(ligatureOffset));

        appendGsubExtensionTestU16(data, 400);
        appendGsubExtensionTestU16(data, 2);
        appendGsubExtensionTestU16(data, 41);


        // Coverage

        const size_t coverageOffset = data.size();
        patchGsubExtensionTestU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 1);
        appendGsubExtensionTestU16(data, 40);

        return data;
    }


    static bool testOpenTypeGsubExtensionView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB ExtensionSubst view: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Basic ExtensionSubst structure.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> target = makeGsubExtensionSingleTarget();
            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(1, target);

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            if (!extension)
                return fail("case 1 ExtensionSubst invalid");

            if (extension.format() != 1)
                return fail("case 1 format");

            if (extension.extensionLookupType() != 1)
                return fail("case 1 lookup type");

            if (extension.extensionOffset() != 8)
                return fail("case 1 extension offset");

            const ByteSpan targetData = extension.extensionSubtable();

            if (!targetData || targetData.size() != target.size())
                return fail("case 1 target span");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Real 32-bit offset.
        //
        // Put the target beyond 64K. This proves ExtensionSubst is not
        // accidentally truncating its Offset32 to 16 bits.
        // ====================================================================

        {
            ++cases;

            static constexpr uint32_t kLargeOffset = 0x00010020u;

            const std::vector<uint8_t> target = makeGsubExtensionSingleTarget();
            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(1, target, kLargeOffset);

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            if (!extension)
                return fail("case 2 large ExtensionSubst invalid");

            if (extension.extensionOffset() != kLargeOffset)
                return fail("case 2 Offset32 truncated");

            const ByteSpan targetData = extension.extensionSubtable();

            if (!targetData)
                return fail("case 2 large-offset target unavailable");

            const OpenTypeGsubSingleSubstView single(targetData);

            if (!single)
                return fail("case 2 large-offset SingleSubst invalid");

            uint16_t replacement = 0;

            if (!single.substitute(10, replacement) || replacement != 11)
                return fail("case 2 large-offset substitution");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Extension to LookupType 1.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(1, makeGsubExtensionSingleTarget());

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGsubSingleSubstView single(
                extension.extensionSubtable());

            if (!single)
                return fail("case 3 SingleSubst");

            uint16_t replacement = 0;

            if (!single.substitute(10, replacement) || replacement != 11)
                return fail("case 3 SingleSubst result");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Extension to LookupType 2.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(2, makeGsubExtensionMultipleTarget());

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGsubMultipleSubstView multiple(
                extension.extensionSubtable());

            if (!multiple)
                return fail("case 4 MultipleSubst");

            OpenTypeGsubMultipleSequenceView sequence;

            if (!multiple.sequenceForGlyph(20, sequence))
                return fail("case 4 sequence lookup");

            if (sequence.glyphCount() != 2)
                return fail("case 4 sequence count");

            uint16_t glyph0 = 0;
            uint16_t glyph1 = 0;

            if (!sequence.glyphId(0, glyph0) ||
                !sequence.glyphId(1, glyph1) ||
                glyph0 != 200 || glyph1 != 201)
            {
                return fail("case 4 sequence glyphs");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Extension to LookupType 3.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(3, makeGsubExtensionAlternateTarget());

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGsubAlternateSubstView alternate(
                extension.extensionSubtable());

            if (!alternate)
                return fail("case 5 AlternateSubst");

            OpenTypeGsubAlternateSetView set;

            if (!alternate.alternateSetForGlyph(30, set))
                return fail("case 5 AlternateSet lookup");

            uint16_t glyph0 = 0;
            uint16_t glyph1 = 0;

            if (set.glyphCount() != 2 ||
                !set.glyphId(0, glyph0) ||
                !set.glyphId(1, glyph1) ||
                glyph0 != 300 || glyph1 != 301)
            {
                return fail("case 5 alternates");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Extension to LookupType 4.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(4, makeGsubExtensionLigatureTarget());

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            const OpenTypeGsubLigatureSubstView ligatureSubst(
                extension.extensionSubtable());

            if (!ligatureSubst)
                return fail("case 6 LigatureSubst");

            OpenTypeGsubLigatureSetView set;

            if (!ligatureSubst.ligatureSetForGlyph(40, set))
                return fail("case 6 LigatureSet lookup");

            const OpenTypeGsubLigatureView ligature = set.ligature(0);

            if (!ligature)
                return fail("case 6 Ligature");

            uint16_t component = 0;

            if (ligature.ligatureGlyph() != 400 ||
                ligature.componentCount() != 2 ||
                !ligature.componentGlyphId(0, component) ||
                component != 41)
            {
                return fail("case 6 Ligature data");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Lazy target interpretation.
        //
        // ExtensionSubst itself only validates the wrapper and target offset.
        // Corrupting the target must not invalidate the wrapper.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(1, makeGsubExtensionSingleTarget());

            // Corrupt the underlying SingleSubst format.

            patchGsubExtensionTestU16(data, 8, 99);

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            if (!extension)
                return fail("case 7 wrapper rejected lazy target corruption");

            if (extension.extensionLookupType() != 1)
                return fail("case 7 lookup type damaged");

            if (!extension.extensionSubtable())
                return fail("case 7 raw target unavailable");

            const OpenTypeGsubSingleSubstView single(
                extension.extensionSubtable());

            if (single)
                return fail("case 7 malformed target accepted");

            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Recursive ExtensionSubst is forbidden.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data =
                wrapGsubExtensionTestSubtable(7, makeGsubExtensionSingleTarget());

            const OpenTypeGsubExtensionSubstView extension(
                ByteSpan(data.data(), data.size()));

            if (extension)
                return fail("case 8 recursive Type 7 accepted");

            ++passed;
        }


        // ====================================================================
        // Case 9
        //
        // Structural failure paths.
        // ====================================================================

        {
            ++cases;


            // Unsupported format.

            const uint8_t badFormatBytes[] = {
                0x00, 0x02,
                0x00, 0x01,
                0x00, 0x00, 0x00, 0x08,
                0x00
            };

            const OpenTypeGsubExtensionSubstView badFormat(
                ByteSpan(badFormatBytes, sizeof(badFormatBytes)));

            if (badFormat)
                return fail("case 9 unsupported format accepted");


            // Lookup type zero.

            const uint8_t zeroTypeBytes[] = {
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x00, 0x00, 0x08,
                0x00
            };

            const OpenTypeGsubExtensionSubstView zeroType(
                ByteSpan(zeroTypeBytes, sizeof(zeroTypeBytes)));

            if (zeroType)
                return fail("case 9 zero lookup type accepted");


            // Offset points into the ExtensionSubst header.

            const uint8_t headerOffsetBytes[] = {
                0x00, 0x01,
                0x00, 0x01,
                0x00, 0x00, 0x00, 0x06
            };

            const OpenTypeGsubExtensionSubstView headerOffset(
                ByteSpan(headerOffsetBytes, sizeof(headerOffsetBytes)));

            if (headerOffset)
                return fail("case 9 header-relative target accepted");


            // Offset beyond available data.

            const uint8_t outsideOffsetBytes[] = {
                0x00, 0x01,
                0x00, 0x01,
                0x00, 0x00, 0x00, 0x20,
                0x00
            };

            const OpenTypeGsubExtensionSubstView outsideOffset(
                ByteSpan(outsideOffsetBytes, sizeof(outsideOffsetBytes)));

            if (outsideOffset)
                return fail("case 9 outside target accepted");


            // Truncated Offset32.

            const uint8_t truncatedBytes[] = {
                0x00, 0x01,
                0x00, 0x01,
                0x00, 0x00, 0x00
            };

            const OpenTypeGsubExtensionSubstView truncated(
                ByteSpan(truncatedBytes, sizeof(truncatedBytes)));

            if (truncated)
                return fail("case 9 truncated header accepted");

            ++passed;
        }


        std::printf(
            "OpenType GSUB ExtensionSubst view: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  ExtensionSubst:        PASS\n"
            "  Offset32:              PASS\n"
            "  Type 1 target:         PASS\n"
            "  Type 2 target:         PASS\n"
            "  Type 3 target:         PASS\n"
            "  Type 4 target:         PASS\n"
            "  Lazy target access:    PASS\n"
            "  Recursive Type 7:      PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs