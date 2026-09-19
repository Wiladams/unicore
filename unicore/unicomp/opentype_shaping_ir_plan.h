// opentype_shaping_ir_plan.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_gdef_view.h"
#include "opentype_gpos_attachment_state.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_gpos_ir_executor.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gsub_ir_executor.h"
#include "opentype_layout_selection.h"
#include "opentype_layout_view.h"
#include "opentype_shaping_buffer.h"
#include "opentype_shaping_ir.h"
#include "shaped_glyph_buffer.h"

namespace waavs
{
    // ========================================================================
    // OpenTypeShapingIRPlan
    //
    // Temporary bridge between the existing OpenType feature/layout selector
    // and the semantic GSUB/GPOS IR executors.
    //
    // ir:
    //     Owns the semantic lookup graph compiled from one source LookupList.
    //
    // lookups:
    //     Ordered semantic root lookup IDs corresponding to the source lookup
    //     indices selected by OpenTypeLayoutLookupPlan.
    //
    // Contextual dependencies are compiled into ir but are not added to this
    // root list unless they were independently selected by the layout plan.
    //
    // For the first production cutover, GSUB and GPOS each use their own
    // OpenTypeShapingIRPlan instance. Later this can be replaced by a
    // per-face compiled representation without changing executor semantics.
    // ========================================================================

    struct OpenTypeShapingIRPlan
    {
        OpenTypeShapingIR ir{};
        std::vector<OpenTypeShapingIRLookupId> lookups{};

        void clear()
        {
            ir.clear();
            lookups.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return lookups.empty(); }
        [[nodiscard]] size_t size() const noexcept { return lookups.size(); }
    };


    // ========================================================================
    // validateOpenTypeShapingIRPlan
    //
    // This validates only the generic plan shell:
    //
    //     every root ID must resolve to a semantic lookup
    //
    // Operation-specific validation remains in the GSUB/GPOS executors.
    // ========================================================================

    [[nodiscard]] static inline bool validateOpenTypeShapingIRPlan(
        const OpenTypeShapingIRPlan& plan) noexcept
    {
        for (OpenTypeShapingIRLookupId lookupId : plan.lookups)
        {
            if (lookupId == kOpenTypeShapingIRInvalid ||
                !plan.ir.lookup(lookupId))
            {
                return false;
            }
        }

        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubIRPlan
    //
    // Compile every root lookup selected by an existing
    // OpenTypeLayoutLookupPlan.
    //
    // One shared OpenTypeGsubIRCompilerContext is used for the entire plan.
    // This is important:
    //
    //     - dependencies are compiled once
    //     - source LookupList indices retain stable semantic lookup IDs
    //     - self/mutual contextual recursion keeps stable graph references
    //
    // The source layout plan remains responsible for feature selection and
    // LookupList ordering. This function changes only the representation used
    // for execution.
    // ========================================================================

    static inline bool compileOpenTypeGsubIRPlan(
        const OpenTypeLayoutLookupPlan& source,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIRPlan& result)
    {
        result.clear();

        if (!source.lookupListData)
            return false;

        const OpenTypeLayoutLookupListView lookups(source.lookupListData);

        if (!lookups)
            return false;

        OpenTypeGsubIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        result.lookups.reserve(source.lookupIndices.size());

        for (uint16_t sourceLookupIndex : source.lookupIndices)
        {
            if (sourceLookupIndex >= lookups.size())
            {
                result.clear();
                return false;
            }

            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGsubIRLookupByIndex(
                lookups, sourceLookupIndex, gdef,
                result.ir, context, lookupId))
            {
                result.clear();
                return false;
            }

            if (lookupId == kOpenTypeShapingIRInvalid ||
                !result.ir.lookup(lookupId))
            {
                result.clear();
                return false;
            }

            result.lookups.push_back(lookupId);
        }

        if (!validateOpenTypeShapingIRPlan(result))
        {
            result.clear();
            return false;
        }

        return true;
    }


    static inline bool compileOpenTypeGsubIRPlan(
        const OpenTypeLayoutLookupPlan& source,
        OpenTypeShapingIRPlan& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubIRPlan(source, gdef, result);
    }


    // ========================================================================
    // applyOpenTypeGsubIRPlan
    //
    // Execute root lookups in the order established by the layout selector.
    //
    // The entire plan is transactional at this seam. Individual semantic
    // lookup executors are themselves transactional, but keeping a top-level
    // working copy means failure in a later selected lookup does not leave the
    // caller with a partially shaped buffer.
    // ========================================================================

    static inline bool applyOpenTypeGsubIRPlan(
        const OpenTypeShapingIRPlan& plan,
        OpenTypeShapingBuffer& buffer)
    {
        if (!validateOpenTypeShapingIRPlan(plan))
            return false;

        OpenTypeShapingBuffer working = buffer;

        for (OpenTypeShapingIRLookupId lookupId : plan.lookups)
        {
            if (!applyOpenTypeGsubIRLookup(
                plan.ir, lookupId, working))
            {
                return false;
            }
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // compileOpenTypeGposIRPlan
    //
    // GPOS equivalent of compileOpenTypeGsubIRPlan().
    //
    // One OpenTypeGposIRCompilerContext is shared by every selected root.
    // Types 7 and 8 may recursively reference any other supported GPOS lookup,
    // so the shared source-index -> semantic-ID mapping is required here too.
    // ========================================================================

    static inline bool compileOpenTypeGposIRPlan(
        const OpenTypeLayoutLookupPlan& source,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIRPlan& result)
    {
        result.clear();

        if (!source.lookupListData)
            return false;

        const OpenTypeLayoutLookupListView lookups(source.lookupListData);

        if (!lookups)
            return false;

        OpenTypeGposIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        result.lookups.reserve(source.lookupIndices.size());

        for (uint16_t sourceLookupIndex : source.lookupIndices)
        {
            if (sourceLookupIndex >= lookups.size())
            {
                result.clear();
                return false;
            }

            OpenTypeShapingIRLookupId lookupId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeGposIRLookupByIndex(
                lookups, sourceLookupIndex, gdef,
                result.ir, context, lookupId))
            {
                result.clear();
                return false;
            }

            if (lookupId == kOpenTypeShapingIRInvalid ||
                !result.ir.lookup(lookupId))
            {
                result.clear();
                return false;
            }

            result.lookups.push_back(lookupId);
        }

        if (!validateOpenTypeShapingIRPlan(result))
        {
            result.clear();
            return false;
        }

        return true;
    }


    static inline bool compileOpenTypeGposIRPlan(
        const OpenTypeLayoutLookupPlan& source,
        OpenTypeShapingIRPlan& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGposIRPlan(source, gdef, result);
    }


    // ========================================================================
    // applyOpenTypeGposIRPlan
    //
    // Execute the complete selected GPOS root sequence with one shared
    // attachment graph and resolve that graph exactly once after all lookups.
    //
    // This preserves the existing raw GPOS orchestrator invariant:
    //
    //     lookup 0
    //       |
    //     lookup 1
    //       |
    //     lookup N
    //       |
    //     shared attachment state
    //       |
    //     one final resolve
    //
    // Do not replace this with per-lookup convenience overloads that resolve
    // attachments individually.
    // ========================================================================

    static inline bool applyOpenTypeGposIRPlan(
        const OpenTypeShapingIRPlan& plan,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        if (!validateOpenTypeShapingIRPlan(plan))
            return false;

        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        for (OpenTypeShapingIRLookupId lookupId : plan.lookups)
        {
            if (!applyOpenTypeGposIRLookup(
                plan.ir, lookupId, working,
                runRightToLeft, attachments))
            {
                return false;
            }
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments,
            runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }


    // ========================================================================
    // Convenience compile + execute bridges
    //
    // These are useful at the first integration seam in
    // opentype_horizontal_shaper.h. They intentionally compile per call.
    //
    // Later, when semantic IR moves to a per-face lifetime, the horizontal
    // shaper can stop using these convenience functions and receive/reuse a
    // precompiled plan instead.
    // ========================================================================

    static inline bool compileAndApplyOpenTypeGsubIRPlan(
        const OpenTypeLayoutLookupPlan& source,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        OpenTypeShapingIRPlan plan;

        if (!compileOpenTypeGsubIRPlan(source, gdef, plan))
            return false;

        return applyOpenTypeGsubIRPlan(plan, buffer);
    }


    static inline bool compileAndApplyOpenTypeGposIRPlan(
        const OpenTypeLayoutLookupPlan& source,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        OpenTypeShapingIRPlan plan;

        if (!compileOpenTypeGposIRPlan(source, gdef, plan))
            return false;

        return applyOpenTypeGposIRPlan(
            plan, buffer, runRightToLeft);
    }

} // namespace waavs
