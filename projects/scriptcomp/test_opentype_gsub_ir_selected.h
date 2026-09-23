// test_opentype_gsub_ir_selected.h
#pragma once

#include "test_core.h"

#include <cstdint>
#include <cstdio>

#include "opentype_gsub_ir_executor.h"

namespace waavs
{
    // ========================================================================
    // SingleSubst IR
    //
    //     10 -> 20
    // ========================================================================

    static bool makeOpenTypeGsubIRSelectedSingle(
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId)
    {
        ir.clear();

        ir.gsubSinglePairs.push_back({
            10,
            20
            });

        ir.gsubSingleSubtables.push_back({
            0,
            1
            });

        OpenTypeShapingIRLookup lookup{};
        lookup.op = OpenTypeShapingIROp::GsubSingle;
        lookup.payloadOffset = 0;
        lookup.payloadCount = 1;

        lookupId =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(lookup);
        return true;
    }


    // ========================================================================
    // LigatureSubst IR
    //
    //     10 20 -> 30
    //
    // This is important for selection semantics: only glyph position zero is
    // selected as a lookup start. Glyph position one is deliberately outside
    // the selection, but must remain visible as a ligature component.
    // ========================================================================

    static bool makeOpenTypeGsubIRSelectedLigature(
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& lookupId)
    {
        ir.clear();

        ir.gsubLigatureComponents.push_back(20);

        OpenTypeShapingIRGsubLigature ligature{};
        ligature.output = 30;
        ligature.componentCount = 2;
        ligature.componentOffset = 0;

        ir.gsubLigatures.push_back(ligature);

        OpenTypeShapingIRGsubLigaturePair pair{};
        pair.input = 10;
        pair.ligatureOffset = 0;
        pair.ligatureCount = 1;

        ir.gsubLigaturePairs.push_back(pair);

        ir.gsubLigatureSubtables.push_back({
            0,
            1
            });

        OpenTypeShapingIRLookup lookup{};
        lookup.op = OpenTypeShapingIROp::GsubLigature;
        lookup.payloadOffset = 0;
        lookup.payloadCount = 1;

        lookupId =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(lookup);
        return true;
    }


    static OpenTypeShapingGlyph makeOpenTypeGsubIRSelectedGlyph(
        uint32_t glyphId,
        uint32_t scalarOffset)
    {
        OpenTypeShapingGlyph glyph{};
        glyph.glyphId = glyphId;
        glyph.scalarOffset = scalarOffset;
        glyph.scalarCount = 1;
        return glyph;
    }


    // ========================================================================
    // testOpenTypeGsubIRSelected
    // ========================================================================

    static bool testOpenTypeGsubIRSelected()
    {
        bool singleSelection = false;
        bool emptySelection = false;
        bool fullContext = false;
        bool invalidSelection = false;


        // ------------------------------------------------------------
        // SingleSubst selection restriction.
        //
        // All three glyphs are eligible for 10 -> 20, but only physical
        // position 1 is selected.
        //
        //     before: 10 10 10
        //     select:     ^
        //     after:  10 20 10
        // ------------------------------------------------------------

        {
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            if (!makeOpenTypeGsubIRSelectedSingle(
                ir,
                lookupId))
            {
                return false;
            }

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(10, 0));

            buffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(10, 1));

            buffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(10, 2));

            const uint32_t selected[] =
            {
                1
            };

            if (!applyOpenTypeGsubIRLookupSelected(
                ir,
                lookupId,
                buffer,
                selected,
                1))
            {
                return false;
            }

            singleSelection =
                buffer.size() == 3 &&
                buffer[0].glyphId == 10 &&
                buffer[1].glyphId == 20 &&
                buffer[2].glyphId == 10;

            if (!singleSelection)
                return false;


            // --------------------------------------------------------
            // Empty selection is a legal no-op.
            // --------------------------------------------------------

            OpenTypeShapingBuffer emptyBuffer;

            emptyBuffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(10, 0));

            if (!applyOpenTypeGsubIRLookupSelected(
                ir,
                lookupId,
                emptyBuffer,
                nullptr,
                0))
            {
                return false;
            }

            emptySelection =
                emptyBuffer.size() == 1 &&
                emptyBuffer[0].glyphId == 10;

            if (!emptySelection)
                return false;


            // --------------------------------------------------------
            // Invalid physical selection must fail transactionally.
            // --------------------------------------------------------

            OpenTypeShapingBuffer invalidBuffer;

            invalidBuffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(10, 0));

            const uint32_t invalid[] =
            {
                4
            };

            if (applyOpenTypeGsubIRLookupSelected(
                ir,
                lookupId,
                invalidBuffer,
                invalid,
                1))
            {
                return false;
            }

            invalidSelection =
                invalidBuffer.size() == 1 &&
                invalidBuffer[0].glyphId == 10;

            if (!invalidSelection)
                return false;
        }


        // ------------------------------------------------------------
        // Full-buffer lookup context.
        //
        // Only position 0 is selected:
        //
        //     10 20
        //     ^
        //
        // Yet LigatureSubst must still see position 1 and produce:
        //
        //     30
        //
        // Selection restricts lookup STARTS, not lookup visibility.
        // ------------------------------------------------------------

        {
            OpenTypeShapingIR ir;
            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            if (!makeOpenTypeGsubIRSelectedLigature(
                ir,
                lookupId))
            {
                return false;
            }

            OpenTypeShapingBuffer buffer;

            buffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(10, 0));

            buffer.pushBack(
                makeOpenTypeGsubIRSelectedGlyph(20, 1));

            const uint32_t selected[] =
            {
                0
            };

            if (!applyOpenTypeGsubIRLookupSelected(
                ir,
                lookupId,
                buffer,
                selected,
                1))
            {
                return false;
            }

            fullContext =
                buffer.size() == 1 &&
                buffer[0].glyphId == 30 &&
                buffer[0].scalarOffset == 0 &&
                buffer[0].scalarCount == 2;

            if (!fullContext)
                return false;
        }


        std::printf(
            "GSUB IR selected execution: PASS\n"
            "  Selected start restriction: PASS\n"
            "  Empty selection no-op:      PASS\n"
            "  Full-buffer context:         PASS\n"
            "  Invalid selection:           PASS\n");

        return true;
    }

} // namespace waavs