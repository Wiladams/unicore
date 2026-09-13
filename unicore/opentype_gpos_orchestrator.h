// opentype_gpos_orchestrator.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "opentype_layout_selection.h"
#include "opentype_gpos_lookup_apply.h"

namespace waavs
{
    // ====================================================================
    // validateOpenTypeGposLookupPlan
    //
    // The layout selector emits unique LookupList indices in increasing
    // LookupList order. Validate that invariant again at the execution seam.
    // ====================================================================

    static inline bool validateOpenTypeGposLookupPlan(
        const OpenTypeLayoutLookupPlan& plan,
        const OpenTypeLayoutLookupListView& lookups) noexcept
    {
        if (!plan.lookupListData)
            return false;

        bool havePrevious = false;
        uint16_t previous = 0;

        for (uint16_t lookupIndex : plan.lookupIndices)
        {
            if (lookupIndex >= lookups.size())
                return false;

            if (havePrevious && lookupIndex <= previous)
                return false;

            previous = lookupIndex;
            havePrevious = true;
        }

        return true;
    }


    // ====================================================================
    // validateOpenTypeGposIdentity
    //
    // GPOS may change placement only. It must never change glyph topology,
    // glyph identity, or source provenance.
    // ====================================================================

    static inline bool validateOpenTypeGposIdentity(
        const ShapedGlyphBuffer& before,
        const ShapedGlyphBuffer& after) noexcept
    {
        if (before.size() != after.size())
            return false;

        for (size_t i = 0; i < before.size(); ++i)
        {
            if (before[i].shaping.glyphId != after[i].shaping.glyphId ||
                before[i].shaping.scalarOffset != after[i].shaping.scalarOffset ||
                before[i].shaping.scalarCount != after[i].shaping.scalarCount)
            {
                return false;
            }
        }

        return true;
    }


    // ====================================================================
    // applyOpenTypeGposSingleWholeLookup
    //
    // Type 1 has no pair-resume or attachment behavior, so whole-lookup
    // execution is simply exact application at each physical glyph.
    // ====================================================================

    static inline bool applyOpenTypeGposSingleWholeLookup(
        const OpenTypeLayoutLookupView& lookup,
        ShapedGlyphBuffer& buffer)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 1))
            return false;

        ShapedGlyphBuffer working = buffer;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            const OpenTypeGposResolveResult result =
                applyOpenTypeGposSingleLookupAt(lookup, working, glyphIndex);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ====================================================================
    // applyOpenTypeGposPlannedLookup
    //
    // Execute one complete selected LookupList entry without resolving the
    // attachment graph.
    //
    // Type 9 is already transparent here. openTypeGposEffectiveLookupType()
    // resolves it to effective Type 1..8, while the original outer Lookup is
    // passed to the executor so its LookupFlag remains authoritative.
    // ====================================================================

    static inline bool applyOpenTypeGposPlannedLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments, OpenTypeGposApplyState& state,
        bool runRightToLeft)
    {
        if (!lookups || lookupIndex >= lookups.size() ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGposEffectiveLookupType(lookup, effectiveType))
            return false;

        switch (effectiveType)
        {
        case 1:
            return applyOpenTypeGposSingleWholeLookup(lookup, buffer);

        case 2:
            return applyOpenTypeGposPairLookup(lookup, gdef, buffer);

        case 3:
            return applyOpenTypeGposCursiveLookup(
                lookup, gdef, buffer, runRightToLeft, attachments);

        case 4:
            return applyOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, attachments);

        case 5:
            return applyOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer, attachments);

        case 6:
            return applyOpenTypeGposMarkMarkLookup(
                lookup, gdef, buffer, attachments);

        case 7:
            return applyOpenTypeGposContextLookup(
                lookups, lookupIndex, gdef, buffer,
                attachments, state, runRightToLeft);

        case 8:
            return applyOpenTypeGposChainContextLookup(
                lookups, lookupIndex, gdef, buffer,
                attachments, state, runRightToLeft);

        default:
            return false;
        }
    }


    // ====================================================================
    // applyOpenTypeGposLookupPlan
    //
    // Apply the complete selected GPOS lookup sequence transactionally.
    //
    // One attachment graph is shared by every selected lookup. Attachment
    // offsets are resolved exactly once, after all lookups have succeeded.
    //
    // runRightToLeft describes the shaping run direction. It is separate
    // from the RIGHT_TO_LEFT bit in an individual OpenType LookupFlag.
    // ====================================================================

    static inline bool applyOpenTypeGposLookupPlan(
        const OpenTypeLayoutLookupPlan& plan,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        if (!plan.lookupListData)
            return false;

        const OpenTypeLayoutLookupListView lookups(plan.lookupListData);

        if (!lookups || !validateOpenTypeGposLookupPlan(plan, lookups))
            return false;


        // No selected positioning features is a successful no-op.

        if (plan.lookupIndices.empty())
            return true;


        const ShapedGlyphBuffer original = buffer;
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        OpenTypeGposApplyState state;


        // ------------------------------------------------------------
        // Execute selected LookupList entries in normalized LookupList order.
        //
        // Attachment relations deliberately survive between lookups.
        // ------------------------------------------------------------

        for (uint16_t lookupIndex : plan.lookupIndices)
        {
            if (!applyOpenTypeGposPlannedLookup(
                lookups, lookupIndex, gdef, working,
                attachments, state, runRightToLeft))
            {
                return false;
            }

            if (state.nestingDepth != 0)
                return false;

            if (!attachments.matches(working.size()))
                return false;
        }


        // ------------------------------------------------------------
        // Resolve mark/cursive attachment chains once, after every selected
        // GPOS lookup has had an opportunity to contribute.
        // ------------------------------------------------------------

        if (!resolveOpenTypeGposAttachments(
            working, attachments, runRightToLeft))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Hard execution-boundary invariant.
        //
        // GPOS may alter only GlyphPlacement.
        // ------------------------------------------------------------

        if (!validateOpenTypeGposIdentity(original, working))
            return false;


        buffer = std::move(working);
        return true;
    }


    // ====================================================================
    // Convenience overload without GDEF.
    // ====================================================================

    static inline bool applyOpenTypeGposLookupPlan(
        const OpenTypeLayoutLookupPlan& plan,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGposLookupPlan(plan, gdef, buffer, runRightToLeft);
    }

} // namespace waavs