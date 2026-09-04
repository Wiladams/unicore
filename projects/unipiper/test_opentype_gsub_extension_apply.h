// test_opentype_gsub_extension_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    static void appendGsubExtensionApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGsubExtensionApplyU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGsubExtensionApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGsubExtensionApplyGlyph(OpenTypeShapingBuffer& buffer,
        uint32_t glyphId, uint32_t scalarOffset, uint32_t scalarCount)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = scalarCount;
        buffer.pushBack(glyph);
    }


    // ====================================================================
    // Underlying Type 1:
    //
    //   input -> replacement
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionApplySingle(
        uint16_t inputGlyph, uint16_t replacementGlyph)
    {
        std::vector<uint8_t> data;

        // SingleSubst Format 2

        appendGsubExtensionApplyU16(data, 2);

        const size_t coveragePatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, replacementGlyph);

        const size_t coverageOffset = data.size();
        patchGsubExtensionApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, inputGlyph);

        return data;
    }


    // ====================================================================
    // Underlying Type 2:
    //
    //   input -> replacement0, replacement1
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionApplyMultiple(
        uint16_t inputGlyph, uint16_t replacement0, uint16_t replacement1)
    {
        std::vector<uint8_t> data;

        appendGsubExtensionApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        appendGsubExtensionApplyU16(data, 1);

        const size_t sequencePatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        const size_t sequenceOffset = data.size();
        patchGsubExtensionApplyU16(data, sequencePatch, static_cast<uint16_t>(sequenceOffset));

        appendGsubExtensionApplyU16(data, 2);
        appendGsubExtensionApplyU16(data, replacement0);
        appendGsubExtensionApplyU16(data, replacement1);

        const size_t coverageOffset = data.size();
        patchGsubExtensionApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, inputGlyph);

        return data;
    }


    // ====================================================================
    // Underlying Type 3:
    //
    //   input -> { alternate0, alternate1 }
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionApplyAlternate(
        uint16_t inputGlyph, uint16_t alternate0, uint16_t alternate1)
    {
        std::vector<uint8_t> data;

        appendGsubExtensionApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        appendGsubExtensionApplyU16(data, 1);

        const size_t setPatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        const size_t setOffset = data.size();
        patchGsubExtensionApplyU16(data, setPatch, static_cast<uint16_t>(setOffset));

        appendGsubExtensionApplyU16(data, 2);
        appendGsubExtensionApplyU16(data, alternate0);
        appendGsubExtensionApplyU16(data, alternate1);

        const size_t coverageOffset = data.size();
        patchGsubExtensionApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, inputGlyph);

        return data;
    }


    // ====================================================================
    // Underlying Type 4:
    //
    //   firstGlyph, secondGlyph -> ligatureGlyph
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionApplyLigature(
        uint16_t firstGlyph, uint16_t secondGlyph, uint16_t ligatureGlyph)
    {
        std::vector<uint8_t> data;

        appendGsubExtensionApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        appendGsubExtensionApplyU16(data, 1);

        const size_t setPatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        const size_t setOffset = data.size();
        patchGsubExtensionApplyU16(data, setPatch, static_cast<uint16_t>(setOffset));

        appendGsubExtensionApplyU16(data, 1);

        const size_t ligaturePatch = data.size();
        appendGsubExtensionApplyU16(data, 0);

        const size_t ligatureOffset = data.size() - setOffset;
        patchGsubExtensionApplyU16(data, ligaturePatch, static_cast<uint16_t>(ligatureOffset));

        appendGsubExtensionApplyU16(data, ligatureGlyph);
        appendGsubExtensionApplyU16(data, 2);
        appendGsubExtensionApplyU16(data, secondGlyph);

        const size_t coverageOffset = data.size();
        patchGsubExtensionApplyU16(data, coveragePatch, static_cast<uint16_t>(coverageOffset));

        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, 1);
        appendGsubExtensionApplyU16(data, firstGlyph);

        return data;
    }


    struct GsubExtensionApplyTarget
    {
        uint16_t lookupType{ 0 };
        std::vector<uint8_t> subtable{};
    };


    // ====================================================================
    // Build one complete LookupType 7 Lookup.
    //
    // Each ExtensionSubst stores its target immediately after its 8-byte
    // wrapper, so extensionOffset is always 8.
    // ====================================================================

    static std::vector<uint8_t> makeGsubExtensionApplyLookup(
        const std::vector<GsubExtensionApplyTarget>& targets)
    {
        std::vector<uint8_t> data;

        appendGsubExtensionApplyU16(data, 7);
        appendGsubExtensionApplyU16(data, 0);
        appendGsubExtensionApplyU16(data, static_cast<uint16_t>(targets.size()));

        std::vector<size_t> subtablePatches;
        subtablePatches.reserve(targets.size());

        for (size_t i = 0; i < targets.size(); ++i)
        {
            subtablePatches.push_back(data.size());
            appendGsubExtensionApplyU16(data, 0);
        }

        for (size_t i = 0; i < targets.size(); ++i)
        {
            const size_t extensionOffset = data.size();

            patchGsubExtensionApplyU16(data, subtablePatches[i],
                static_cast<uint16_t>(extensionOffset));

            appendGsubExtensionApplyU16(data, 1);
            appendGsubExtensionApplyU16(data, targets[i].lookupType);
            appendGsubExtensionApplyU32(data, 8);

            data.insert(data.end(), targets[i].subtable.begin(), targets[i].subtable.end());
        }

        return data;
    }


    static bool testOpenTypeGsubExtensionApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType GSUB ExtensionSubst apply: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Type 7 -> Type 1 SingleSubst.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubExtensionApplyLookup({
                    { 1, makeGsubExtensionApplySingle(10, 100) }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubExtensionApplyGlyph(buffer, 10, 4, 2);
            appendGsubExtensionApplyGlyph(buffer, 20, 6, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 1 application failed");

            if (buffer.size() != 2 || buffer[0].glyphId != 100 || buffer[1].glyphId != 20)
                return fail("case 1 glyph sequence");

            if (buffer[0].scalarOffset != 4 || buffer[0].scalarCount != 2)
                return fail("case 1 provenance");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Type 7 -> Type 2 MultipleSubst.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubExtensionApplyLookup({
                    { 2, makeGsubExtensionApplyMultiple(20, 200, 201) }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubExtensionApplyGlyph(buffer, 20, 7, 3);
            appendGsubExtensionApplyGlyph(buffer, 30, 10, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 2 application failed");

            if (buffer.size() != 3)
                return fail("case 2 buffer size");

            if (buffer[0].glyphId != 200 || buffer[1].glyphId != 201 || buffer[2].glyphId != 30)
                return fail("case 2 glyph sequence");

            if (buffer[0].scalarOffset != 7 || buffer[0].scalarCount != 3 ||
                buffer[1].scalarOffset != 7 || buffer[1].scalarCount != 3)
            {
                return fail("case 2 provenance");
            }

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Type 7 -> Type 3 AlternateSubst.
        //
        // Default policy selects alternate index 0.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubExtensionApplyLookup({
                    { 3, makeGsubExtensionApplyAlternate(30, 300, 301) }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubExtensionApplyGlyph(buffer, 30, 11, 2);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 3 application failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 300)
                return fail("case 3 alternate");

            if (buffer[0].scalarOffset != 11 || buffer[0].scalarCount != 2)
                return fail("case 3 provenance");

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Type 7 -> Type 4 LigatureSubst.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubExtensionApplyLookup({
                    { 4, makeGsubExtensionApplyLigature(40, 41, 400) }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubExtensionApplyGlyph(buffer, 40, 2, 1);
            appendGsubExtensionApplyGlyph(buffer, 41, 3, 2);
            appendGsubExtensionApplyGlyph(buffer, 50, 5, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 4 application failed");

            if (buffer.size() != 2 || buffer[0].glyphId != 400 || buffer[1].glyphId != 50)
                return fail("case 4 glyph sequence");

            if (buffer[0].scalarOffset != 2 || buffer[0].scalarCount != 3)
                return fail("case 4 provenance union");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Type 7 preserves ordinary lookup subtable semantics.
        //
        // Extension subtable 0:
        //
        //   10 -> 100
        //
        // Extension subtable 1:
        //
        //   100 -> 900
        //
        // Input 10 must become 100, not 900. The glyph generated by the
        // first matching subtable is not fed into another subtable of the
        // same lookup.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubExtensionApplyLookup({
                    { 1, makeGsubExtensionApplySingle(10, 100) },
                    { 1, makeGsubExtensionApplySingle(100, 900) }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubExtensionApplyGlyph(buffer, 10, 0, 1);

            if (!applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 5 application failed");

            if (buffer.size() != 1 || buffer[0].glyphId != 100)
                return fail("case 5 same-lookup subtable semantics");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // All ExtensionSubst records in one Type 7 Lookup must specify the
        // same underlying lookup type.
        //
        // This lookup deliberately mixes Type 1 and Type 3.
        // Application must fail without modifying the caller's buffer.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> lookupData =
                makeGsubExtensionApplyLookup({
                    { 1, makeGsubExtensionApplySingle(10, 100) },
                    { 3, makeGsubExtensionApplyAlternate(20, 200, 201) }
                    });

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            OpenTypeShapingBuffer buffer;
            appendGsubExtensionApplyGlyph(buffer, 10, 5, 1);
            appendGsubExtensionApplyGlyph(buffer, 20, 6, 2);

            if (applyOpenTypeGsubLookup(lookup, buffer))
                return fail("case 6 mixed extension types accepted");

            if (buffer.size() != 2)
                return fail("case 6 caller buffer size changed");

            if (buffer[0].glyphId != 10 || buffer[1].glyphId != 20)
                return fail("case 6 caller glyphs changed");

            if (buffer[0].scalarOffset != 5 || buffer[0].scalarCount != 1 ||
                buffer[1].scalarOffset != 6 || buffer[1].scalarCount != 2)
            {
                return fail("case 6 caller provenance changed");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ExtensionSubst apply: PASS\n"
            "  Cases:                  %u\n"
            "  Passed:                 %u\n"
            "  Type 1 Single:          PASS\n"
            "  Type 2 Multiple:        PASS\n"
            "  Type 3 Alternate:       PASS\n"
            "  Type 4 Ligature:        PASS\n"
            "  Subtable semantics:     PASS\n"
            "  Mixed-type rejection:   PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs