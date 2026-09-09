// test_opentype_gpos_cursive_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    struct GposCursiveTestAnchor
    {
        bool present{ false };
        int16_t x{ 0 };
        int16_t y{ 0 };
    };


    static void appendGposCursiveApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposCursiveApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposCursiveApplyU16(data, static_cast<uint16_t>(value));
    }

    static void patchGposCursiveApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposCursiveApplyAnchor(
        std::vector<uint8_t>& data, const GposCursiveTestAnchor& anchor)
    {
        appendGposCursiveApplyU16(data, 1);
        appendGposCursiveApplyS16(data, anchor.x);
        appendGposCursiveApplyS16(data, anchor.y);
    }


    static std::vector<uint8_t> makeGposCursiveApplySubtable(
        const uint16_t* glyphs,
        const GposCursiveTestAnchor* entries,
        const GposCursiveTestAnchor* exits,
        uint16_t count)
    {
        std::vector<uint8_t> data;

        appendGposCursiveApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposCursiveApplyU16(data, 0);

        appendGposCursiveApplyU16(data, count);

        std::vector<size_t> entryPatches(count);
        std::vector<size_t> exitPatches(count);

        for (uint16_t i = 0; i < count; ++i)
        {
            entryPatches[i] = data.size();
            appendGposCursiveApplyU16(data, 0);

            exitPatches[i] = data.size();
            appendGposCursiveApplyU16(data, 0);
        }


        // Coverage.

        patchGposCursiveApplyU16(
            data, coveragePatch,
            static_cast<uint16_t>(data.size()));

        appendGposCursiveApplyU16(data, 1);
        appendGposCursiveApplyU16(data, count);

        for (uint16_t i = 0; i < count; ++i)
            appendGposCursiveApplyU16(data, glyphs[i]);


        // Anchors.

        for (uint16_t i = 0; i < count; ++i)
        {
            if (entries[i].present)
            {
                patchGposCursiveApplyU16(
                    data, entryPatches[i],
                    static_cast<uint16_t>(data.size()));

                appendGposCursiveApplyAnchor(data, entries[i]);
            }

            if (exits[i].present)
            {
                patchGposCursiveApplyU16(
                    data, exitPatches[i],
                    static_cast<uint16_t>(data.size()));

                appendGposCursiveApplyAnchor(data, exits[i]);
            }
        }

        return data;
    }


    static std::vector<uint8_t> makeGposCursiveApplyLookup(
        const std::vector<uint8_t>& subtable,
        uint16_t lookupFlag = 0)
    {
        std::vector<uint8_t> data;

        appendGposCursiveApplyU16(data, 3);
        appendGposCursiveApplyU16(data, lookupFlag);
        appendGposCursiveApplyU16(data, 1);
        appendGposCursiveApplyU16(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());

        return data;
    }


    static std::vector<uint8_t> makeGposCursiveApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposCursiveApplyU16(data, 1);
        appendGposCursiveApplyU16(data, 0);

        appendGposCursiveApplyU16(data, 12);
        appendGposCursiveApplyU16(data, 0);
        appendGposCursiveApplyU16(data, 0);
        appendGposCursiveApplyU16(data, 0);

        appendGposCursiveApplyU16(data, 2);
        appendGposCursiveApplyU16(data, 1);

        appendGposCursiveApplyU16(data, 100);
        appendGposCursiveApplyU16(data, 100);
        appendGposCursiveApplyU16(data, 3);

        return data;
    }


    static ShapedGlyph makeGposCursiveApplyGlyph(
        uint32_t glyphId, int32_t advanceX,
        int32_t offsetX = 0, int32_t offsetY = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = glyphId;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;
        glyph.placement.offsetX = offsetX;
        glyph.placement.offsetY = offsetY;

        return glyph;
    }


    static bool testOpenTypeGposCursiveApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS CursivePos apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1 - LTR main-axis and cross-axis alignment.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 450, 20 },
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 2);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposCursiveApplyGlyph(10, 600, 10, 7));

            buffer.pushBack(
                makeGposCursiveApplyGlyph(20, 500, -5, 0));

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 1 application");
            }

            if (buffer[0].placement.advanceX != 415)
                return fail("case 1 advance");

            if (buffer[1].placement.offsetY != -73)
                return fail("case 1 cross-stream offset");


            // X anchor alignment:
            //
            // first:  10 + 450 = 460
            // second: 415 - 5 + 50 = 460

            const int32_t firstExitX =
                buffer[0].placement.offsetX + 450;

            const int32_t secondEntryX =
                buffer[0].placement.advanceX +
                buffer[1].placement.offsetX + 50;

            if (firstExitX != secondEntryX)
                return fail("case 1 X alignment");


            // Y anchor alignment.

            const int32_t firstExitY =
                buffer[0].placement.offsetY + 20;

            const int32_t secondEntryY =
                buffer[1].placement.offsetY + 100;

            if (firstExitY != secondEntryY)
                return fail("case 1 Y alignment");

            ++passed;
        }


        // ====================================================================
        // Case 2 - Logical RTL main-axis alignment.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 450, 100 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 50, 20 },
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 2);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(
                makeGposCursiveApplyGlyph(10, 600, 10, 7));

            buffer.pushBack(
                makeGposCursiveApplyGlyph(20, 500, -5, 0));

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, true))
            {
                return fail("case 2 application");
            }

            if (buffer[0].placement.advanceX != 385)
                return fail("case 2 RTL advance");


            // Logical RTL:
            //
            // second origin = -385
            //
            // first exit:
            //    10 + 50 = 60
            //
            // second entry:
            //   -385 - 5 + 450 = 60

            const int32_t firstExitX =
                buffer[0].placement.offsetX + 50;

            const int32_t secondEntryX =
                -buffer[0].placement.advanceX +
                buffer[1].placement.offsetX + 450;

            if (firstExitX != secondEntryX)
                return fail("case 2 RTL X alignment");

            ++passed;
        }


        // ====================================================================
        // Case 3 - RIGHT_TO_LEFT flag roots cross-stream chain at the end.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20, 30 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 },
                { true, 50, 70 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 450, 20 },
                { true, 450, 40 },
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 3);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(
                    subtable, 0x0001u);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposCursiveApplyGlyph(10, 500));
            buffer.pushBack(makeGposCursiveApplyGlyph(20, 500));
            buffer.pushBack(makeGposCursiveApplyGlyph(30, 500, 0, 5));

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 3 application");
            }

            if (buffer[2].placement.offsetY != 5 ||
                buffer[1].placement.offsetY != 35 ||
                buffer[0].placement.offsetY != 115)
            {
                return fail("case 3 chain propagation");
            }

            if (buffer[0].placement.offsetY + 20 !=
                buffer[1].placement.offsetY + 100)
            {
                return fail("case 3 first attachment");
            }

            if (buffer[1].placement.offsetY + 40 !=
                buffer[2].placement.offsetY + 70)
            {
                return fail("case 3 second attachment");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4 - Clear RIGHT_TO_LEFT roots chain at the beginning.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20, 30 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 },
                { true, 50, 70 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 450, 20 },
                { true, 450, 40 },
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 3);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposCursiveApplyGlyph(10, 500, 0, 5));
            buffer.pushBack(makeGposCursiveApplyGlyph(20, 500));
            buffer.pushBack(makeGposCursiveApplyGlyph(30, 500));

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 4 application");
            }

            if (buffer[0].placement.offsetY != 5 ||
                buffer[1].placement.offsetY != -75 ||
                buffer[2].placement.offsetY != -105)
            {
                return fail("case 4 chain propagation");
            }

            ++passed;
        }


        // ====================================================================
        // Case 5 - LookupFlag filtering.
        //
        // Physical:
        //
        //   10 mark 20
        //
        // IgnoreMarks:
        //
        //   10 -> 20
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 450, 20 },
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 2);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(
                    subtable, 0x0008u);

            const std::vector<uint8_t> gdefData =
                makeGposCursiveApplyGdef();

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            const OpenTypeGdefView gdef(
                ByteSpan(gdefData.data(), gdefData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposCursiveApplyGlyph(10, 600));
            buffer.pushBack(makeGposCursiveApplyGlyph(100, 0));
            buffer.pushBack(makeGposCursiveApplyGlyph(20, 500));

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 5 application");
            }

            if (buffer[0].placement.advanceX != 400 ||
                buffer[1].placement.advanceX != 0 ||
                buffer[1].placement.offsetY != 0 ||
                buffer[2].placement.offsetY != -80)
            {
                return fail("case 5 filtered placement");
            }

            ++passed;
        }


        // ====================================================================
        // Case 6 - NULL anchor produces NoMatch, not failure.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                {},
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 2);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposCursiveApplyGlyph(10, 600));
            buffer.pushBack(makeGposCursiveApplyGlyph(20, 500));

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 6 lookup failure");
            }

            if (buffer[0].placement.advanceX != 600 ||
                buffer[1].placement.advanceX != 500)
            {
                return fail("case 6 unexpected mutation");
            }

            ++passed;
        }


        // ====================================================================
        // Case 7 - Malformed child is transactional.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 450, 20 },
                {}
            };

            std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 2);

            const uint16_t exitOffset =
                static_cast<uint16_t>(
                    (uint16_t(subtable[8]) << 8) |
                    uint16_t(subtable[9]));

            if (exitOffset >= subtable.size())
                return fail("case 7 synthetic offset");

            subtable[exitOffset] = 0;
            subtable[exitOffset + 1] = 4;

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposCursiveApplyGlyph(10, 600, 10, 7));
            buffer.pushBack(makeGposCursiveApplyGlyph(20, 500, -5, 3));

            const OpenTypeGdefView gdef{};

            if (applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, false))
            {
                return fail("case 7 malformed child accepted");
            }

            if (buffer[0].placement.advanceX != 600 ||
                buffer[0].placement.offsetX != 10 ||
                buffer[0].placement.offsetY != 7 ||
                buffer[1].placement.advanceX != 500 ||
                buffer[1].placement.offsetX != -5 ||
                buffer[1].placement.offsetY != 3)
            {
                return fail("case 7 transactional failure");
            }

            ++passed;
        }


        // ====================================================================
        // Case 8 - Delayed attachment propagation.
        //
        // This proves why attachment state survives until GPOS completion.
        // ====================================================================

        {
            ++cases;

            const uint16_t glyphs[] = { 10, 20 };

            const GposCursiveTestAnchor entries[] =
            {
                {},
                { true, 50, 100 }
            };

            const GposCursiveTestAnchor exits[] =
            {
                { true, 450, 20 },
                {}
            };

            const std::vector<uint8_t> subtable =
                makeGposCursiveApplySubtable(
                    glyphs, entries, exits, 2);

            const std::vector<uint8_t> lookupData =
                makeGposCursiveApplyLookup(subtable);

            const OpenTypeLayoutLookupView lookup(
                ByteSpan(lookupData.data(), lookupData.size()));

            ShapedGlyphBuffer buffer;

            buffer.pushBack(makeGposCursiveApplyGlyph(10, 600, 0, 10));
            buffer.pushBack(makeGposCursiveApplyGlyph(20, 500));

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            const OpenTypeGdefView gdef{};

            if (!applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer,
                false, attachments))
            {
                return fail("case 8 application");
            }


            // Child currently contains only its local attachment offset.

            if (buffer[1].placement.offsetY != -80)
                return fail("case 8 local offset");


            // Simulate a later GPOS lookup moving the parent.

            buffer[0].placement.offsetY += 20;

            if (!resolveOpenTypeGposCursiveAttachments(
                buffer, attachments))
            {
                return fail("case 8 finalization");
            }


            // Parent is now at +30.
            // Child local attachment is -80.
            //
            // Final child offset = -50.

            if (buffer[0].placement.offsetY != 30 ||
                buffer[1].placement.offsetY != -50)
            {
                return fail("case 8 delayed propagation");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS CursivePos apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  LTR attachment:            PASS\n"
            "  Logical RTL attachment:    PASS\n"
            "  RTL lookup chain:          PASS\n"
            "  LTR lookup chain:          PASS\n"
            "  LookupFlag filtering:      PASS\n"
            "  NULL anchors:              PASS\n"
            "  Transactional failure:     PASS\n"
            "  Delayed propagation:       PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs