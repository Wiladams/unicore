// test_opentype_gpos_extension_apply.h
#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    // ====================================================================
    // Binary helpers.
    // ====================================================================

    static void appendGposExtensionApplyU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void appendGposExtensionApplyS16(std::vector<uint8_t>& data, int16_t value)
    {
        appendGposExtensionApplyU16(data, static_cast<uint16_t>(value));
    }

    static void appendGposExtensionApplyU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }

    static void patchGposExtensionApplyU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }

    static void appendGposExtensionApplyCoverage(std::vector<uint8_t>& data, uint16_t glyphId)
    {
        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyU16(data, glyphId);
    }

    static void appendGposExtensionApplyAnchor(std::vector<uint8_t>& data, int16_t x, int16_t y)
    {
        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyS16(data, x);
        appendGposExtensionApplyS16(data, y);
    }


    // ====================================================================
    // Synthetic GDEF.
    //
    // Glyphs 100..102 are marks.
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplyGdef()
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyU16(data, 0);

        appendGposExtensionApplyU16(data, 12);
        appendGposExtensionApplyU16(data, 0);
        appendGposExtensionApplyU16(data, 0);
        appendGposExtensionApplyU16(data, 0);

        appendGposExtensionApplyU16(data, 2);
        appendGposExtensionApplyU16(data, 1);

        appendGposExtensionApplyU16(data, 100);
        appendGposExtensionApplyU16(data, 102);
        appendGposExtensionApplyU16(data, 3);

        return data;
    }


    // ====================================================================
    // Type 1 SinglePos Format 1.
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplySingle(uint16_t glyphId, int16_t xPlacement)
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposExtensionApplyU16(data, 0);

        appendGposExtensionApplyU16(data, 0x0001u);
        appendGposExtensionApplyS16(data, xPlacement);

        patchGposExtensionApplyU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposExtensionApplyCoverage(data, glyphId);

        return data;
    }


    // ====================================================================
    // Type 2 PairPos Format 1.
    //
    // Adjust first glyph xAdvance.
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplyPair(
        uint16_t firstGlyph, uint16_t secondGlyph, int16_t xAdvance)
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 1);

        const size_t coveragePatch = data.size();
        appendGposExtensionApplyU16(data, 0);

        appendGposExtensionApplyU16(data, 0x0004u);
        appendGposExtensionApplyU16(data, 0);

        appendGposExtensionApplyU16(data, 1);

        const size_t pairSetPatch = data.size();
        appendGposExtensionApplyU16(data, 0);

        patchGposExtensionApplyU16(data, pairSetPatch, static_cast<uint16_t>(data.size()));

        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyU16(data, secondGlyph);
        appendGposExtensionApplyS16(data, xAdvance);

        patchGposExtensionApplyU16(data, coveragePatch, static_cast<uint16_t>(data.size()));
        appendGposExtensionApplyCoverage(data, firstGlyph);

        return data;
    }


    // ====================================================================
    // Type 4 Mark-to-Base.
    //
    // mark 100 anchor = (10,20)
    // base 10 anchor  = (300,400)
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplyMarkBase()
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 1);

        const size_t markCoveragePatch = data.size();
        appendGposExtensionApplyU16(data, 0);

        const size_t baseCoveragePatch = data.size();
        appendGposExtensionApplyU16(data, 0);

        appendGposExtensionApplyU16(data, 1);

        const size_t markArrayPatch = data.size();
        appendGposExtensionApplyU16(data, 0);

        const size_t baseArrayPatch = data.size();
        appendGposExtensionApplyU16(data, 0);


        patchGposExtensionApplyU16(data, markCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposExtensionApplyCoverage(data, 100);

        patchGposExtensionApplyU16(data, baseCoveragePatch, static_cast<uint16_t>(data.size()));
        appendGposExtensionApplyCoverage(data, 10);


        patchGposExtensionApplyU16(data, markArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposExtensionApplyU16(data, 1);
            appendGposExtensionApplyU16(data, 0);

            const size_t anchorPatch = data.size();
            appendGposExtensionApplyU16(data, 0);

            patchGposExtensionApplyU16(data, anchorPatch, static_cast<uint16_t>(data.size() - arrayBegin));
            appendGposExtensionApplyAnchor(data, 10, 20);
        }


        patchGposExtensionApplyU16(data, baseArrayPatch, static_cast<uint16_t>(data.size()));

        {
            const size_t arrayBegin = data.size();

            appendGposExtensionApplyU16(data, 1);

            const size_t anchorPatch = data.size();
            appendGposExtensionApplyU16(data, 0);

            patchGposExtensionApplyU16(data, anchorPatch, static_cast<uint16_t>(data.size() - arrayBegin));
            appendGposExtensionApplyAnchor(data, 300, 400);
        }

        return data;
    }


    // ====================================================================
    // Type 7 ContextPos Format 3.
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplyContext3(
        const uint16_t* input, uint16_t inputCount,
        const OpenTypeSequenceLookup* actions, uint16_t actionCount)
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 3);
        appendGposExtensionApplyU16(data, inputCount);
        appendGposExtensionApplyU16(data, actionCount);

        std::vector<size_t> coveragePatches;
        coveragePatches.reserve(inputCount);

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            coveragePatches.push_back(data.size());
            appendGposExtensionApplyU16(data, 0);
        }

        for (uint16_t i = 0; i < actionCount; ++i)
        {
            appendGposExtensionApplyU16(data, actions[i].sequenceIndex);
            appendGposExtensionApplyU16(data, actions[i].lookupListIndex);
        }

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            patchGposExtensionApplyU16(data, coveragePatches[i], static_cast<uint16_t>(data.size()));
            appendGposExtensionApplyCoverage(data, input[i]);
        }

        return data;
    }


    // ====================================================================
    // Type 8 ChainContextPos Format 3.
    //
    // backtrack[] is stored nearest-first.
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplyChain3(
        const uint16_t* backtrack, uint16_t backtrackCount,
        const uint16_t* input, uint16_t inputCount,
        const uint16_t* lookahead, uint16_t lookaheadCount,
        const OpenTypeSequenceLookup* actions, uint16_t actionCount)
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 3);

        appendGposExtensionApplyU16(data, backtrackCount);

        std::vector<size_t> backtrackPatches;
        backtrackPatches.reserve(backtrackCount);

        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            backtrackPatches.push_back(data.size());
            appendGposExtensionApplyU16(data, 0);
        }


        appendGposExtensionApplyU16(data, inputCount);

        std::vector<size_t> inputPatches;
        inputPatches.reserve(inputCount);

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            inputPatches.push_back(data.size());
            appendGposExtensionApplyU16(data, 0);
        }


        appendGposExtensionApplyU16(data, lookaheadCount);

        std::vector<size_t> lookaheadPatches;
        lookaheadPatches.reserve(lookaheadCount);

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            lookaheadPatches.push_back(data.size());
            appendGposExtensionApplyU16(data, 0);
        }


        appendGposExtensionApplyU16(data, actionCount);

        for (uint16_t i = 0; i < actionCount; ++i)
        {
            appendGposExtensionApplyU16(data, actions[i].sequenceIndex);
            appendGposExtensionApplyU16(data, actions[i].lookupListIndex);
        }


        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            patchGposExtensionApplyU16(data, backtrackPatches[i], static_cast<uint16_t>(data.size()));
            appendGposExtensionApplyCoverage(data, backtrack[i]);
        }

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            patchGposExtensionApplyU16(data, inputPatches[i], static_cast<uint16_t>(data.size()));
            appendGposExtensionApplyCoverage(data, input[i]);
        }

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            patchGposExtensionApplyU16(data, lookaheadPatches[i], static_cast<uint16_t>(data.size()));
            appendGposExtensionApplyCoverage(data, lookahead[i]);
        }

        return data;
    }


    // ====================================================================
    // ExtensionPos Format 1.
    // ====================================================================

    static std::vector<uint8_t> makeGposExtensionApplyExtension(
        uint16_t extensionLookupType, const std::vector<uint8_t>& subtable)
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyU16(data, extensionLookupType);
        appendGposExtensionApplyU32(data, 8);

        data.insert(data.end(), subtable.begin(), subtable.end());
        return data;
    }


    static std::vector<uint8_t> makeGposExtensionApplyMalformedExtension()
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, 1);
        appendGposExtensionApplyU16(data, 1);

        // Invalid ExtensionOffset: points into the ExtensionPos header.

        appendGposExtensionApplyU32(data, 6);

        appendGposExtensionApplyU16(data, 0x1234);
        return data;
    }


    // ====================================================================
    // LookupList builder.
    // ====================================================================

    struct GposExtensionApplyLookupSpec
    {
        uint16_t lookupType{ 0 };
        uint16_t lookupFlag{ 0 };
        std::vector<std::vector<uint8_t>> subtables{};
    };


    static std::vector<uint8_t> makeGposExtensionApplyLookupList(
        const std::vector<GposExtensionApplyLookupSpec>& specs)
    {
        std::vector<uint8_t> data;

        appendGposExtensionApplyU16(data, static_cast<uint16_t>(specs.size()));

        std::vector<size_t> lookupPatches;
        lookupPatches.reserve(specs.size());

        for (size_t i = 0; i < specs.size(); ++i)
        {
            lookupPatches.push_back(data.size());
            appendGposExtensionApplyU16(data, 0);
        }


        for (size_t lookupIndex = 0; lookupIndex < specs.size(); ++lookupIndex)
        {
            const GposExtensionApplyLookupSpec& spec = specs[lookupIndex];

            patchGposExtensionApplyU16(
                data, lookupPatches[lookupIndex],
                static_cast<uint16_t>(data.size()));

            const size_t lookupBegin = data.size();

            appendGposExtensionApplyU16(data, spec.lookupType);
            appendGposExtensionApplyU16(data, spec.lookupFlag);
            appendGposExtensionApplyU16(data, static_cast<uint16_t>(spec.subtables.size()));

            std::vector<size_t> subtablePatches;
            subtablePatches.reserve(spec.subtables.size());

            for (size_t i = 0; i < spec.subtables.size(); ++i)
            {
                subtablePatches.push_back(data.size());
                appendGposExtensionApplyU16(data, 0);
            }

            for (size_t i = 0; i < spec.subtables.size(); ++i)
            {
                patchGposExtensionApplyU16(
                    data, subtablePatches[i],
                    static_cast<uint16_t>(data.size() - lookupBegin));

                data.insert(
                    data.end(),
                    spec.subtables[i].begin(),
                    spec.subtables[i].end());
            }
        }

        return data;
    }


    // ====================================================================
    // Buffer helpers.
    // ====================================================================

    static ShapedGlyph makeGposExtensionApplyGlyph(
        uint32_t glyphId, uint32_t scalarOffset, int32_t advanceX = 0)
    {
        ShapedGlyph glyph{};

        glyph.shaping.glyphId = glyphId;
        glyph.shaping.scalarOffset = scalarOffset;
        glyph.shaping.scalarCount = 1;

        glyph.placement.advanceX = advanceX;

        return glyph;
    }


    static ShapedGlyphBuffer makeGposExtensionApplyBuffer(
        const uint32_t* glyphIds, const int32_t* advances, size_t count)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < count; ++i)
        {
            buffer.pushBack(
                makeGposExtensionApplyGlyph(
                    glyphIds[i],
                    static_cast<uint32_t>(i),
                    advances ? advances[i] : 0));
        }

        return buffer;
    }


    // ====================================================================
    // Exact-position driver.
    //
    // This is deliberately close to the lower-level operation that the
    // eventual GPOS orchestration layer will call after Script/LangSys and
    // FeatureList processing have selected a LookupList entry.
    // ====================================================================

    static OpenTypeGposApplyAtResult applyGposExtensionTestLookupAt(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer,
        size_t glyphIndex, OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state, bool runRightToLeft = false)
    {
        return applyOpenTypeGposLookupAt(
            lookups, lookupIndex, gdef, buffer, glyphIndex,
            attachments, state, runRightToLeft,
            OpenTypeGposApplyRange::whole(buffer.size()));
    }


    // ====================================================================
    // Test.
    // ====================================================================

    static bool testOpenTypeGposExtensionApply()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "OpenType GPOS Extension apply: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> gdefData = makeGposExtensionApplyGdef();
        const OpenTypeGdefView gdef(ByteSpan(gdefData.data(), gdefData.size()));
        const OpenTypeGdefView emptyGdef{};

        if (!gdef)
            return fail("synthetic GDEF invalid");


        // ================================================================
        // Case 1 - Effective type and effective subtable.
        //
        // Lookup Type 9 -> effective Type 1.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(10, 21))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            if (!lookups || lookups.size() != 1)
                return fail("case 1 LookupList");

            const OpenTypeLayoutLookupView lookup =
                lookups.lookup(0);

            uint16_t effectiveType = 0;

            if (!openTypeGposEffectiveLookupType(
                lookup, effectiveType) ||
                effectiveType != 1)
            {
                return fail("case 1 effective type");
            }

            if (!openTypeGposHasEffectiveLookupType(lookup, 1) ||
                openTypeGposHasEffectiveLookupType(lookup, 2))
            {
                return fail("case 1 effective predicate");
            }

            const ByteSpan subtable =
                openTypeGposEffectiveSubtable(lookup, 1, 0);

            const OpenTypeGposSinglePosView single(subtable);

            if (!single || single.format() != 1)
                return fail("case 1 effective subtable");

            const OpenTypeCoverageView coverage = single.coverage();
            uint16_t coverageIndex = 0;

            if (!coverage ||
                !coverage.find(10, coverageIndex) ||
                coverageIndex != 0)
            {
                return fail("case 1 effective Coverage");
            }

            ++passed;
        }


        // ================================================================
        // Case 2 - Type 9 -> Type 1.
        //
        // This exercises the central exact-position dispatcher.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(10, 31))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, emptyGdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 2 result");

            if (buffer[0].placement.offsetX != 31)
                return fail("case 2 SinglePos");

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[0].shaping.scalarOffset != 0 ||
                buffer[0].shaping.scalarCount != 1)
            {
                return fail("case 2 shaping identity");
            }

            if (state.nestingDepth != 0)
                return fail("case 2 nesting state");

            ++passed;
        }


        // ================================================================
        // Case 3 - Type 9 -> Type 2 and outer LookupFlag.
        //
        // Physical:
        //
        //   10  mark100  20
        //
        // Type 9 has IgnoreMarks. The effective PairPos must therefore see
        // the pair 10,20 and add 25 to glyph 10's advance.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0x0008u,
                    {
                        makeGposExtensionApplyExtension(
                            2, makeGposExtensionApplyPair(10, 20, 25))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 100, 20 };
            const int32_t advances[] = { 500, 0, 400 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, advances, 3);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, gdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 3 result");

            if (buffer[0].placement.advanceX != 525)
                return fail("case 3 outer LookupFlag");

            if (buffer[1].placement.advanceX != 0 ||
                buffer[2].placement.advanceX != 400)
            {
                return fail("case 3 unrelated advances");
            }

            ++passed;
        }


        // ================================================================
        // Case 4 - Type 9 -> Type 4 attachment.
        //
        // base10 advance = 500
        //
        // local mark attachment:
        //
        //   (300,400) - (10,20) = (290,380)
        //
        // final LTR mark placement:
        //
        //   x = 290 - 500 = -210
        //   y = 380
        //
        // This proves attachment state survives the ExtensionPos wrapper.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            4, makeGposExtensionApplyMarkBase())
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 100 };
            const int32_t advances[] = { 500, 0 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, advances, 2);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, gdef,
                    buffer, 1, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 4 result");

            if (!resolveOpenTypeGposAttachments(
                buffer, attachments, false))
            {
                return fail("case 4 attachment resolution");
            }

            if (buffer[1].placement.offsetX != -210 ||
                buffer[1].placement.offsetY != 380)
            {
                return fail("case 4 attachment placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 5 - Multiple ExtensionPos subtables.
        //
        // Both resolve to effective Type 1.
        //
        // First covers glyph 99.
        // Second covers glyph 10.
        //
        // Stored subtable order and first-match semantics must survive
        // ExtensionPos unwrapping.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(99, 7)),
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(10, 43))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, emptyGdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 5 result");

            if (buffer[0].placement.offsetX != 43)
                return fail("case 5 second effective subtable");

            ++passed;
        }


        // ================================================================
        // Case 6 - Type 9 -> Type 7 ContextPos.
        //
        // Lookup 0:
        //
        //   Type 9 -> Type 7
        //   input 10
        //   SequenceLookup { 0, 1 }
        //
        // Lookup 1:
        //
        //   Type 1 SinglePos 10 -> +47
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            7,
                            makeGposExtensionApplyContext3(
                                input, 1, actions, 1))
                    }
                },
                {
                    1, 0,
                    {
                        makeGposExtensionApplySingle(10, 47)
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, emptyGdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 6 result");

            if (buffer[0].placement.offsetX != 47)
                return fail("case 6 Type 9 -> Type 7");

            if (state.nestingDepth != 0)
                return fail("case 6 nesting state");

            ++passed;
        }


        // ================================================================
        // Case 7 - Type 9 -> Type 8 ChainContextPos.
        //
        // Physical:
        //
        //   10  mark100  20
        //
        // Outer Type 9 has IgnoreMarks.
        //
        // Effective Type 8 therefore matches input {10,20}.
        // sequenceIndex 1 -> Lookup 1 -> SinglePos 20 +53.
        //
        // This proves the Type 9 LookupFlag is also used by contextual
        // matching after extension unwrapping.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10, 20 };
            const OpenTypeSequenceLookup actions[] = { { 1, 1 } };

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0x0008u,
                    {
                        makeGposExtensionApplyExtension(
                            8,
                            makeGposExtensionApplyChain3(
                                nullptr, 0,
                                input, 2,
                                nullptr, 0,
                                actions, 1))
                    }
                },
                {
                    1, 0,
                    {
                        makeGposExtensionApplySingle(20, 53)
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10, 100, 20 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 3);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, gdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Match)
                return fail("case 7 result");

            if (buffer[2].placement.offsetX != 53)
                return fail("case 7 Type 9 -> Type 8");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 7 unrelated placement");
            }

            ++passed;
        }


        // ================================================================
        // Case 8 - Native Type 7 -> Type 9 -> Type 1.
        //
        // This is closer to real LookupList orchestration:
        //
        //   contextual lookup
        //        |
        //        v
        //   SequenceLookupRecord
        //        |
        //        v
        //   LookupList entry Type 9
        //        |
        //        v
        //   effective Type 1
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    7, 0,
                    {
                        makeGposExtensionApplyContext3(
                            input, 1, actions, 1)
                    }
                },
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(10, 61))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            if (!applyOpenTypeGposContextLookup(
                lookups, 0, emptyGdef,
                buffer, false))
            {
                return fail("case 8 application");
            }

            if (buffer[0].placement.offsetX != 61)
                return fail("case 8 Type 7 -> Type 9");

            ++passed;
        }


        // ================================================================
        // Case 9 - Native Type 8 -> Type 9 -> Type 1.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };
            const OpenTypeSequenceLookup actions[] = { { 0, 1 } };

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    8, 0,
                    {
                        makeGposExtensionApplyChain3(
                            nullptr, 0,
                            input, 1,
                            nullptr, 0,
                            actions, 1)
                    }
                },
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(10, 67))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            if (!applyOpenTypeGposChainContextLookup(
                lookups, 0, emptyGdef,
                buffer, false))
            {
                return fail("case 9 application");
            }

            if (buffer[0].placement.offsetX != 67)
                return fail("case 9 Type 8 -> Type 9");

            ++passed;
        }


        // ================================================================
        // Case 10 - Mismatched effective subtable types.
        //
        // One Type 9 Lookup cannot contain:
        //
        //   ExtensionPos -> Type 1
        //   ExtensionPos -> Type 2
        //
        // Effective lookup type must be consistent across the Lookup.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyExtension(
                            1, makeGposExtensionApplySingle(10, 71)),
                        makeGposExtensionApplyExtension(
                            2, makeGposExtensionApplyPair(10, 20, 25))
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutLookupView lookup =
                lookups.lookup(0);

            uint16_t effectiveType = 0;

            if (openTypeGposEffectiveLookupType(
                lookup, effectiveType))
            {
                return fail("case 10 mixed types accepted");
            }

            const uint32_t glyphs[] = { 10, 20 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 2);

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, emptyGdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Invalid)
                return fail("case 10 dispatcher result");

            if (buffer[0].placement.offsetX != 0 ||
                buffer[1].placement.offsetX != 0)
            {
                return fail("case 10 mutation");
            }

            ++passed;
        }


        // ================================================================
        // Case 11 - Malformed ExtensionPos.
        //
        // Invalid ExtensionOffset must fail before any effective subtable is
        // executed and must leave the destination unchanged.
        // ================================================================

        {
            ++cases;

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    9, 0,
                    {
                        makeGposExtensionApplyMalformedExtension()
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            buffer[0].placement.advanceX = 500;

            OpenTypeGposAttachmentState attachments;
            attachments.reset(buffer.size());

            OpenTypeGposApplyState state;

            const OpenTypeGposApplyAtResult result =
                applyGposExtensionTestLookupAt(
                    lookups, 0, emptyGdef,
                    buffer, 0, attachments, state);

            if (result != OpenTypeGposApplyAtResult::Invalid)
                return fail("case 11 malformed extension accepted");

            if (buffer[0].placement.advanceX != 500 ||
                buffer[0].placement.offsetX != 0 ||
                buffer[0].placement.offsetY != 0)
            {
                return fail("case 11 transactional mutation");
            }

            if (state.nestingDepth != 0)
                return fail("case 11 nesting state");

            ++passed;
        }


        // ================================================================
        // Case 12 - Nested malformed Type 9 rolls back outer Type 7.
        //
        // Outer Type 7 actions:
        //
        //   action 0 -> Lookup 1 -> successful SinglePos +77
        //   action 1 -> Lookup 2 -> malformed Type 9
        //
        // The complete outer Type 7 lookup is transactional, so +77 must
        // disappear when the later nested Type 9 fails.
        //
        // This is probably the most orchestration-like case in the test.
        // ================================================================

        {
            ++cases;

            const uint16_t input[] = { 10 };

            const OpenTypeSequenceLookup actions[] =
            {
                { 0, 1 },
                { 0, 2 }
            };

            const std::vector<GposExtensionApplyLookupSpec> specs =
            {
                {
                    7, 0,
                    {
                        makeGposExtensionApplyContext3(
                            input, 1, actions, 2)
                    }
                },
                {
                    1, 0,
                    {
                        makeGposExtensionApplySingle(10, 77)
                    }
                },
                {
                    9, 0,
                    {
                        makeGposExtensionApplyMalformedExtension()
                    }
                }
            };

            const std::vector<uint8_t> data =
                makeGposExtensionApplyLookupList(specs);

            const OpenTypeLayoutLookupListView lookups(
                ByteSpan(data.data(), data.size()));

            const uint32_t glyphs[] = { 10 };

            ShapedGlyphBuffer buffer =
                makeGposExtensionApplyBuffer(glyphs, nullptr, 1);

            buffer[0].placement.advanceX = 500;

            if (applyOpenTypeGposContextLookup(
                lookups, 0, emptyGdef,
                buffer, false))
            {
                return fail("case 12 malformed nested Type 9 accepted");
            }

            if (buffer[0].placement.advanceX != 500 ||
                buffer[0].placement.offsetX != 0 ||
                buffer[0].placement.offsetY != 0)
            {
                return fail("case 12 outer rollback");
            }

            if (buffer[0].shaping.glyphId != 10 ||
                buffer[0].shaping.scalarOffset != 0 ||
                buffer[0].shaping.scalarCount != 1)
            {
                return fail("case 12 shaping identity");
            }

            ++passed;
        }


        std::printf(
            "OpenType GPOS Extension apply: PASS\n"
            "  Cases:                    %u\n"
            "  Passed:                   %u\n"
            "  Effective type/subtable:    PASS\n"
            "  Type 9 -> Type 1:           PASS\n"
            "  Type 9 -> Type 2 flags:     PASS\n"
            "  Type 9 -> attachment:       PASS\n"
            "  Multiple subtables:         PASS\n"
            "  Type 9 -> Type 7:           PASS\n"
            "  Type 9 -> Type 8:           PASS\n"
            "  Type 7 -> Type 9:           PASS\n"
            "  Type 8 -> Type 9:           PASS\n"
            "  Mixed type rejection:       PASS\n"
            "  Malformed extension:        PASS\n"
            "  Nested rollback:            PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs