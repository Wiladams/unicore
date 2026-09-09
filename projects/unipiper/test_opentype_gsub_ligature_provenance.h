// test_opentype_gsub_ligature_provenance.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gsub_lookup_apply.h"

namespace waavs
{
    static void appendGsubLigProvU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static std::vector<uint8_t> makeGsubLigProvGdef()
    {
        std::vector<uint8_t> data;

        // GDEF 1.0.

        appendGsubLigProvU16(data, 1);
        appendGsubLigProvU16(data, 0);

        appendGsubLigProvU16(data, 12);
        appendGsubLigProvU16(data, 0);
        appendGsubLigProvU16(data, 0);
        appendGsubLigProvU16(data, 0);

        // GlyphClassDef Format 2.
        //
        // Glyphs 100..199 are Marks.

        appendGsubLigProvU16(data, 2);
        appendGsubLigProvU16(data, 1);

        appendGsubLigProvU16(data, 100);
        appendGsubLigProvU16(data, 199);
        appendGsubLigProvU16(data, 3);

        return data;
    }


    static OpenTypeShapingGlyph makeGsubLigProvGlyph(
        uint32_t glyphId, uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};

        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;

        return glyph;
    }


    static OpenTypeGsubLigatureMatch makeGsubLigProvMatch(
        uint16_t ligatureGlyph,
        std::initializer_list<size_t> positions)
    {
        OpenTypeGsubLigatureMatch match;

        match.ligatureGlyph = ligatureGlyph;
        match.positions.assign(
            positions.begin(), positions.end());

        return match;
    }


    static bool testOpenTypeGsubLigatureProvenance()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GSUB ligature provenance: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        const std::vector<uint8_t> gdefData =
            makeGsubLigProvGdef();

        const OpenTypeGdefView gdef(
            ByteSpan(gdefData.data(), gdefData.size()));

        if (!gdef)
            return fail("synthetic GDEF");


        // ================================================================
        // Case 1 - Basic three-component ligature.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeGsubLigProvGlyph(10, 0));
            buffer.pushBack(makeGsubLigProvGlyph(20, 1));
            buffer.pushBack(makeGsubLigProvGlyph(30, 2));

            const OpenTypeGsubLigatureMatch match =
                makeGsubLigProvMatch(
                    500, { 0, 1, 2 });

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer, match))
            {
                return fail("case 1 apply");
            }

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 500 ||
                buffer[0].scalarOffset != 0 ||
                buffer[0].scalarCount != 3)
            {
                return fail("case 1 output");
            }

            if (!buffer[0].ligature.ligatureBase() ||
                buffer[0].ligature.id == 0 ||
                buffer[0].ligature.component != 0 ||
                buffer[0].ligature.componentCount != 3)
            {
                return fail("case 1 ligature metadata");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Ignored marks inherit component association.
        //
        // Physical input:
        //
        //   10 100 20 101 30
        //
        // Participating components:
        //
        //   10     20     30
        //
        // Result:
        //
        //   500 100 101
        //
        // mark 100 -> component 1
        // mark 101 -> component 2
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeGsubLigProvGlyph(10, 0));
            buffer.pushBack(makeGsubLigProvGlyph(100, 1));
            buffer.pushBack(makeGsubLigProvGlyph(20, 2));
            buffer.pushBack(makeGsubLigProvGlyph(101, 3));
            buffer.pushBack(makeGsubLigProvGlyph(30, 4));

            const OpenTypeGsubLigatureMatch match =
                makeGsubLigProvMatch(
                    500, { 0, 2, 4 });

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer, match))
            {
                return fail("case 2 apply");
            }

            if (buffer.size() != 3 ||
                buffer[0].glyphId != 500 ||
                buffer[1].glyphId != 100 ||
                buffer[2].glyphId != 101)
            {
                return fail("case 2 surviving glyphs");
            }

            const uint32_t ligatureId =
                buffer[0].ligature.id;

            if (ligatureId == 0 ||
                buffer[0].ligature.componentCount != 3)
            {
                return fail("case 2 ligature");
            }

            if (buffer[1].ligature.id != ligatureId ||
                buffer[1].ligature.component != 1)
            {
                return fail("case 2 first mark");
            }

            if (buffer[2].ligature.id != ligatureId ||
                buffer[2].ligature.component != 2)
            {
                return fail("case 2 second mark");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Different ligatures receive different IDs.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeGsubLigProvGlyph(10, 0));
            buffer.pushBack(makeGsubLigProvGlyph(20, 1));
            buffer.pushBack(makeGsubLigProvGlyph(30, 2));
            buffer.pushBack(makeGsubLigProvGlyph(40, 3));

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(500, { 0, 1 })))
            {
                return fail("case 3 first ligature");
            }

            const uint32_t firstId =
                buffer[0].ligature.id;

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(600, { 1, 2 })))
            {
                return fail("case 3 second ligature");
            }

            const uint32_t secondId =
                buffer[1].ligature.id;

            if (firstId == 0 ||
                secondId == 0 ||
                firstId == secondId)
            {
                return fail("case 3 unique IDs");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Nested ligature component count.
        //
        // First:
        //
        //   10 + 20 -> 500, componentCount = 2
        //
        // Then:
        //
        //   500 + 30 -> 600, componentCount = 3
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeGsubLigProvGlyph(10, 0));
            buffer.pushBack(makeGsubLigProvGlyph(20, 1));
            buffer.pushBack(makeGsubLigProvGlyph(30, 2));

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(500, { 0, 1 })))
            {
                return fail("case 4 inner ligature");
            }

            if (buffer[0].ligature.componentCount != 2)
                return fail("case 4 inner component count");

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(600, { 0, 1 })))
            {
                return fail("case 4 outer ligature");
            }

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 600 ||
                buffer[0].ligature.componentCount != 3)
            {
                return fail("case 4 accumulated count");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Existing component association is remapped.
        //
        // First:
        //
        //   10 100 20
        //      ^
        //      component 1
        //
        // becomes:
        //
        //   500 100
        //
        // Then:
        //
        //   500 100 30
        //
        // becomes:
        //
        //   600 100
        //
        // The mark remains on component 1.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(makeGsubLigProvGlyph(10, 0));
            buffer.pushBack(makeGsubLigProvGlyph(100, 1));
            buffer.pushBack(makeGsubLigProvGlyph(20, 2));

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(500, { 0, 2 })))
            {
                return fail("case 5 inner ligature");
            }

            if (buffer.size() != 2 ||
                buffer[1].ligature.component != 1)
            {
                return fail("case 5 inner association");
            }

            buffer.pushBack(
                makeGsubLigProvGlyph(30, 3));

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(600, { 0, 2 })))
            {
                return fail("case 5 outer ligature");
            }

            if (buffer.size() != 2)
                return fail("case 5 output size");

            if (buffer[1].ligature.id !=
                buffer[0].ligature.id ||
                buffer[1].ligature.component != 1)
            {
                return fail("case 5 remapped association");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Mark ligature preserves parent association.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            OpenTypeShapingGlyph first =
                makeGsubLigProvGlyph(100, 0);

            OpenTypeShapingGlyph second =
                makeGsubLigProvGlyph(101, 1);

            first.ligature.id = 77;
            first.ligature.component = 2;

            second.ligature.id = 77;
            second.ligature.component = 2;

            buffer.pushBack(first);
            buffer.pushBack(second);

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(150, { 0, 1 })))
            {
                return fail("case 6 mark ligature");
            }

            if (buffer.size() != 1 ||
                buffer[0].glyphId != 150 ||
                buffer[0].ligature.id != 77 ||
                buffer[0].ligature.component != 2 ||
                buffer[0].ligature.componentCount != 0)
            {
                return fail("case 6 preserved association");
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Conflicting mark associations are cleared.
        // ================================================================

        {
            ++cases;

            OpenTypeShapingBuffer buffer;

            OpenTypeShapingGlyph first =
                makeGsubLigProvGlyph(100, 0);

            OpenTypeShapingGlyph second =
                makeGsubLigProvGlyph(101, 1);

            first.ligature.id = 77;
            first.ligature.component = 1;

            second.ligature.id = 77;
            second.ligature.component = 2;

            buffer.pushBack(first);
            buffer.pushBack(second);

            if (!applyOpenTypeGsubLigatureMatch(
                gdef, buffer,
                makeGsubLigProvMatch(150, { 0, 1 })))
            {
                return fail("case 7 mark ligature");
            }

            if (buffer[0].ligature.id != 0 ||
                buffer[0].ligature.component != 0 ||
                buffer[0].ligature.componentCount != 0)
            {
                return fail("case 7 conflicting association");
            }

            ++passed;
        }


        std::printf(
            "OpenType GSUB ligature provenance: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Ligature identity:         PASS\n"
            "  Interspersed marks:        PASS\n"
            "  Unique ligature IDs:       PASS\n"
            "  Nested component count:    PASS\n"
            "  Component remapping:       PASS\n"
            "  Mark ligature inheritance: PASS\n"
            "  Conflict handling:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs