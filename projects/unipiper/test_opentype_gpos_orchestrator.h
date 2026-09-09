// test_opentype_gpos_orchestrator.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "opentype_gpos_orchestrator.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGposOrchestratorU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposOrchestratorS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposOrchestratorU16(data, static_cast<uint16_t>(value));
    }

    static void appendGposOrchestratorU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGposOrchestratorU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    // ====================================================================
    // Coverage Format 1.
    // ====================================================================

    static void appendGposOrchestratorCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposOrchestratorU16(data, 1);
        appendGposOrchestratorU16(data, 1);
        appendGposOrchestratorU16(data, glyphId);
    }


    // ====================================================================
    // Anchor Format 1.
    // ====================================================================

    static void appendGposOrchestratorAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposOrchestratorU16(data, 1);
        appendGposOrchestratorS16(data, x);
        appendGposOrchestratorS16(data, y);
    }


    // ====================================================================
    // Type 1 SinglePos Format 1.
    //
    // valueFormat is either:
    //
    //   0x0001 xPlacement
    //   0x0004 xAdvance
    // ====================================================================

    static std::vector<uint8_t> makeGposOrchestratorSingle(
        uint16_t glyphId, uint16_t valueFormat, int16_t value)
    {
        std::vector<uint8_t> data;

        appendGposOrchestratorU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposOrchestratorU16(data, 0);

        appendGposOrchestratorU16(data, valueFormat);
        appendGposOrchestratorS16(data, value);

        patchGposOrchestratorU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposOrchestratorCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Type 4 Mark-to-Base.
    //
    // mark 100 anchor = (10,20)
    // base 10 anchor  = (300,400)
    //
    // Local attachment:
    //
    //   (290,380)
    // ====================================================================

    static std::vector<uint8_t> makeGposOrchestratorMarkBase()
    {
        std::vector<uint8_t> data;

        appendGposOrchestratorU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposOrchestratorU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposOrchestratorU16(data, 0);

        appendGposOrchestratorU16(data, 1);

        const size_t markArrayPatch = data.size();
        appendGposOrchestratorU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposOrchestratorU16(data, 0);


        patchGposOrchestratorU16(data, markCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposOrchestratorCoverage(data, 100);

        patchGposOrchestratorU16(data, baseCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposOrchestratorCoverage(data, 10);


        patchGposOrchestratorU16(data, markArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposOrchestratorU16(data, 1);
            appendGposOrchestratorU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposOrchestratorU16(data, 0);

            patchGposOrchestratorU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposOrchestratorAnchor(data, 10, 20);
        }


        patchGposOrchestratorU16(data, baseArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposOrchestratorU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposOrchestratorU16(data, 0);

            patchGposOrchestratorU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposOrchestratorAnchor(data, 300, 400);
        }

        return data;
    }


    // ====================================================================
    // Type 6 Mark-to-Mark.
    //
    // mark1 glyph 101 anchor = (5,10)
    // mark2 glyph 100 anchor = (20,30)
    //
    // Local mark1 -> mark2 attachment:
    //
    //   (15,20)
    // ====================================================================

    static std::vector<uint8_t> makeGposOrchestratorMarkMark()
    {
        std::vector<uint8_t> data;

        appendGposOrchestratorU16(data, 1);

        const size_t mark1CoveragePatch = data.size();
        appendGposOrchestratorU16(data, 0);

        const size_t mark2CoveragePatch = data.size();
        appendGposOrchestratorU16(data, 0);

        appendGposOrchestratorU16(data, 1);

        const size_t mark1ArrayPatch = data.size();
        appendGposOrchestratorU16(data, 0);

        const size_t mark2ArrayPatch = data.size();
        appendGposOrchestratorU16(data, 0);


        patchGposOrchestratorU16(data, mark1CoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposOrchestratorCoverage(data, 101);

        patchGposOrchestratorU16(data, mark2CoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposOrchestratorCoverage(data, 100);


        patchGposOrchestratorU16(data, mark1ArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposOrchestratorU16(data, 1);
            appendGposOrchestratorU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposOrchestratorU16(data, 0);

            patchGposOrchestratorU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposOrchestratorAnchor(data, 5, 10);
        }


        patchGposOrchestratorU16(data, mark2ArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposOrchestratorU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposOrchestratorU16(data, 0);

            patchGposOrchestratorU16(
                data, anchorPatch,
                static_cast<uint16_t>(data.size() - arrayBegin));

            appendGposOrchestratorAnchor(data, 20, 30);
        }

        return data;
    }


    // ====================================================================
    // Malformed Type 1 subtable.
    // ====================================================================

    static std::vector<uint8_t> makeGposOrchestratorMalformedSingle()
    {
        std::vector<uint8_t> data;

        appendGposOrchestratorU16(data, 1);

        return data;
    }


    // ====================================================================
    // Type 9 ExtensionPos.
    // ====================================================================

    static std::vector<uint8_t> makeGposOrchestratorExtension(
        uint16_t extensionLookupType, const std::vector<uint8_t>& subtable)
    {
        std::vector<uint8_t> data;

        appendGposOrchestratorU16(data, 1);
        appendGposOrchestratorU16(data, extensionLookupType);
        appendGposOrchestratorU32(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());

        return data;
    }


    // ====================================================================
    // LookupList builder.
    // ====================================================================

    struct GposOrchestratorLookupSpec
    {
        uint16_t lookupType{ 0 };
        uint16_t lookupFlag{ 0 };
        std::vector<uint8_t> subtable{};
    };


    static std::vector<uint8_t> makeGposOrchestratorLookupList(
        const std::vector<GposOrchestratorLookupSpec>& specs)
    {
        std::vector<uint8_t> data;

        appendGposOrchestratorU16(data, static_cast<uint16_t>(specs.size()));

        std::vector<size_t> lookupPatches;
        lookupPatches.reserve(specs.size());

        for (size_t i = 0; i < specs.size(); ++i)
        {
            lookupPatches.push_back(data.size());
            appendGposOrchestratorU16(data, 0);
        }

        for (size_t i = 0; i < specs.size(); ++i)
        {
            patchGposOrchestratorU16(data, lookupPatches[i], static_cast<uint16_t>(data.size()));

            const size_t lookupBegin = data.size();

            appendGposOrchestratorU16(data, specs[i].lookupType);
            appendGposOrchestratorU16(data, specs[i].lookupFlag);
            appendGposOrchestratorU16(data, 1);

            const size_t subtablePatch = data.size();
            appendGposOrchestratorU16(data, 0);

            patchGposOrchestratorU16(
                data, subtablePatch,
                static_cast<uint16_t>(data.size() - lookupBegin));

            data.insert(data.end(), specs[i].subtable.begin(), specs[i].subtable.end());
        }

        return data;
    }


    // ====================================================================
    // Standard synthetic LookupList.
    //
    // Lookup 0:
    //   Type 1
    //   glyph 10 xAdvance += 100
    //
    // Lookup 1:
    //   Type 4
    //   mark 100 -> base 10
    //
    // Lookup 2:
    //   Type 6
    //   mark 101 -> mark 100
    //
    // Lookup 3:
    //   malformed Type 1
    //
    // Lookup 4:
    //   Type 9 -> Type 1
    //   glyph 30 xPlacement += 17
    // ====================================================================

    static std::vector<uint8_t> makeGposOrchestratorStandardLookupList()
    {
        return makeGposOrchestratorLookupList(
            {
                {
                    1, 0,
                    makeGposOrchestratorSingle(10, 0x0004u, 100)
                },
                {
                    4, 0,
                    makeGposOrchestratorMarkBase()
                },
                {
                    6, 0,
                    makeGposOrchestratorMarkMark()
                },
                {
                    1, 0,
                    makeGposOrchestratorMalformedSingle()
                },
                {
                    9, 0,
                    makeGposOrchestratorExtension(
                        1,
                        makeGposOrchestratorSingle(30, 0x0001u, 17))
                }
            });
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static ShapedGlyph makeGposOrchestratorGlyph(
        uint32_t glyphId, uint32_t scalarOffset, int32_t advanceX)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = scalarOffset;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;

        return glyph;
    }


    static ShapedGlyphBuffer makeGposOrchestratorBaseMarkBuffer()
    {
        ShapedGlyphBuffer buffer;

        buffer.pushBack(makeGposOrchestratorGlyph(10, 0, 500));
        buffer.pushBack(makeGposOrchestratorGlyph(100, 1, 0));

        return buffer;
    }


    static ShapedGlyphBuffer makeGposOrchestratorStackBuffer()
    {
        ShapedGlyphBuffer buffer;

        buffer.pushBack(makeGposOrchestratorGlyph(10, 0, 500));
        buffer.pushBack(makeGposOrchestratorGlyph(100, 1, 0));
        buffer.pushBack(makeGposOrchestratorGlyph(101, 2, 0));

        return buffer;
    }


    static ShapedGlyphBuffer makeGposOrchestratorSimpleBuffer()
    {
        ShapedGlyphBuffer buffer;

        buffer.pushBack(makeGposOrchestratorGlyph(10, 0, 500));
        buffer.pushBack(makeGposOrchestratorGlyph(30, 1, 400));

        return buffer;
    }


    static OpenTypeLayoutLookupPlan makeGposOrchestratorPlan(
        const std::vector<uint8_t>& lookupList,
        std::initializer_list<uint16_t> indices)
    {
        OpenTypeLayoutLookupPlan plan;

        plan.lookupListData =
            ByteSpan(lookupList.data(), lookupList.size());

        plan.lookupIndices.assign(
            indices.begin(), indices.end());

        return plan;
    }


    static bool gposOrchestratorIdentityEqual(
        const ShapedGlyphBuffer& buffer,
        const uint32_t* glyphs,
        const uint32_t* offsets,
        size_t count) noexcept
    {
        if (buffer.size() != count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (buffer[i].shaping.glyphId != glyphs[i] ||
                buffer[i].shaping.scalarOffset != offsets[i] ||
                buffer[i].shaping.scalarCount != 1)
            {
                return false;
            }
        }

        return true;
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposOrchestrator()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS orchestrator: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> lookupListData =
            makeGposOrchestratorStandardLookupList();

        const OpenTypeLayoutLookupListView lookups(
            ByteSpan(
                lookupListData.data(),
                lookupListData.size()));

        if (!lookups || lookups.size() != 5)
            return fail("synthetic LookupList invalid");


        const OpenTypeGdefView gdef{};


        // ================================================================
        // Case 1 - Advance adjustment before final attachment resolution.
        //
        // Initial base advance:
        //
        //   500
        //
        // Lookup 0:
        //
        //   +100 xAdvance
        //
        // Final base advance:
        //
        //   600
        //
        // Lookup 1 records local mark attachment:
        //
        //   (300,400) - (10,20) = (290,380)
        //
        // Final LTR attachment:
        //
        //   x = 290 - 600 = -310
        //   y = 380
        //
        // If attachment resolution happened before Lookup 0 had accumulated,
        // this would incorrectly be -210.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorBaseMarkBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    { 0, 1 });

            if (!applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, false))
            {
                return fail("case 1 apply");
            }

            if (buffer[0].placement.advanceX != 600)
                return fail("case 1 adjusted advance");

            if (buffer[1].placement.offsetX != -310 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 1 delayed attachment resolution");
            }

            const uint32_t glyphs[] = { 10, 100 };
            const uint32_t offsets[] = { 0, 1 };

            if (!gposOrchestratorIdentityEqual(
                buffer, glyphs, offsets, 2))
            {
                return fail("case 1 shaping identity");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Shared attachment graph across top-level lookups.
        //
        // Lookup 1:
        //
        //   mark 100 -> base 10
        //
        // Lookup 2:
        //
        //   mark 101 -> mark 100
        //
        // After final resolution:
        //
        //   mark100 = (-310,380)
        //
        // mark101 local attachment:
        //
        //   (20,30) - (5,10) = (15,20)
        //
        // Therefore:
        //
        //   mark101 = mark100 + (15,20)
        //           = (-295,400)
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorStackBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    { 0, 1, 2 });

            if (!applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, false))
            {
                return fail("case 2 apply");
            }

            if (buffer[0].placement.advanceX != 600)
                return fail("case 2 advance");

            if (buffer[1].placement.offsetX != -310 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 2 first attachment");
            }

            if (buffer[2].placement.offsetX != -295 ||
                buffer[2].placement.offsetY != 400)
            {
                return fail("case 2 stacked attachment");
            }

            const uint32_t glyphs[] = { 10, 100, 101 };
            const uint32_t offsets[] = { 0, 1, 2 };

            if (!gposOrchestratorIdentityEqual(
                buffer, glyphs, offsets, 3))
            {
                return fail("case 2 shaping identity");
            }

            ++passed;
        }


        // ================================================================
        // Case 3 - Whole-plan rollback.
        //
        // Lookup 0 changes the base advance.
        // Lookup 1 creates an attachment.
        // Lookup 3 is malformed.
        //
        // No placement change may escape.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorBaseMarkBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    { 0, 1, 3 });

            if (applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, false))
            {
                return fail("case 3 malformed lookup accepted");
            }

            if (buffer[0].placement.advanceX != 500 ||
                buffer[0].placement.offsetX != 0 ||
                buffer[0].placement.offsetY != 0)
            {
                return fail("case 3 base rollback");
            }

            if (buffer[1].placement.advanceX != 0 ||
                buffer[1].placement.offsetX != 0 ||
                buffer[1].placement.offsetY != 0)
            {
                return fail("case 3 mark rollback");
            }

            const uint32_t glyphs[] = { 10, 100 };
            const uint32_t offsets[] = { 0, 1 };

            if (!gposOrchestratorIdentityEqual(
                buffer, glyphs, offsets, 2))
            {
                return fail("case 3 identity rollback");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - RTL final attachment resolution.
        //
        // Glyph order remains logical and unchanged.
        //
        // RTL attachment geometry uses:
        //
        //   x = localX + parentOffsetX + advance
        //
        // therefore:
        //
        //   290 + 600 = 890
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorBaseMarkBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    { 0, 1 });

            if (!applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, true))
            {
                return fail("case 4 apply");
            }

            if (buffer[0].placement.advanceX != 600)
                return fail("case 4 advance");

            if (buffer[1].placement.offsetX != 890 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 4 RTL attachment");
            }

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[1].shaping.glyphId != 100)
            {
                return fail("case 4 logical order");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Empty plan is a successful no-op.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorSimpleBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    {});

            if (!applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, false))
            {
                return fail("case 5 empty plan rejected");
            }

            if (buffer[0].placement.advanceX != 500 ||
                buffer[1].placement.advanceX != 400 ||
                buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 5 empty-plan mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 6 - Plan ordering and duplicate validation.
        // ================================================================

        {
            ++cases;

            {
                ShapedGlyphBuffer buffer =
                    makeGposOrchestratorBaseMarkBuffer();

                const OpenTypeLayoutLookupPlan plan =
                    makeGposOrchestratorPlan(
                        lookupListData,
                        { 1, 0 });

                if (applyOpenTypeGposLookupPlan(
                    plan, gdef, buffer, false))
                {
                    return fail("case 6 reversed plan accepted");
                }
            }

            {
                ShapedGlyphBuffer buffer =
                    makeGposOrchestratorBaseMarkBuffer();

                const OpenTypeLayoutLookupPlan plan =
                    makeGposOrchestratorPlan(
                        lookupListData,
                        { 0, 0 });

                if (applyOpenTypeGposLookupPlan(
                    plan, gdef, buffer, false))
                {
                    return fail("case 6 duplicate plan accepted");
                }
            }

            ++passed;
        }


        // ================================================================
        // Case 7 - Out-of-range LookupList index.
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorSimpleBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    { 0, 5 });

            if (applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, false))
            {
                return fail("case 7 out-of-range lookup accepted");
            }

            if (buffer[0].placement.advanceX != 500 ||
                buffer[1].placement.advanceX != 400)
            {
                return fail("case 7 mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Top-level Type 9 orchestration.
        //
        // Lookup 4:
        //
        //   Type 9 -> effective Type 1
        //   glyph 30 xPlacement += 17
        // ================================================================

        {
            ++cases;

            ShapedGlyphBuffer buffer =
                makeGposOrchestratorSimpleBuffer();

            const OpenTypeLayoutLookupPlan plan =
                makeGposOrchestratorPlan(
                    lookupListData,
                    { 4 });

            if (!applyOpenTypeGposLookupPlan(
                plan, gdef, buffer, false))
            {
                return fail("case 8 Type 9 apply");
            }

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 17)
            {
                return fail("case 8 Type 9 effective execution");
            }

            const uint32_t glyphs[] = { 10, 30 };
            const uint32_t offsets[] = { 0, 1 };

            if (!gposOrchestratorIdentityEqual(
                buffer, glyphs, offsets, 2))
            {
                return fail("case 8 shaping identity");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS orchestrator: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Delayed attachment resolve: PASS\n"
            "  Advance interaction:        PASS\n"
            "  Shared attachment graph:    PASS\n"
            "  Mark stack propagation:     PASS\n"
            "  Whole-plan rollback:        PASS\n"
            "  RTL attachment resolve:     PASS\n"
            "  Logical order preserved:    PASS\n"
            "  Empty plan:                 PASS\n"
            "  Plan validation:            PASS\n"
            "  Type 9 orchestration:       PASS\n"
            "  Identity/provenance:        PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs