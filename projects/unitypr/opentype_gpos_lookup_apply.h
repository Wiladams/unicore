// opentype_gpos_lookup_apply.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <limits>

#include "shaped_glyph_buffer.h"
#include "opentype_gpos_single_view.h"
#include "opentype_gpos_pair_view.h"
#include "opentype_gpos_attachment_state.h"
#include "opentype_gpos_cursive_view.h"
#include "opentype_gpos_mark_base_view.h"
#include "opentype_gpos_mark_ligature_view.h"
#include "opentype_gpos_mark_mark_view.h"
#include "opentype_gpos_apply_state.h"
#include "opentype_gpos_context_match.h"
#include "opentype_gpos_chain_context_match.h"
#include "opentype_gpos_extension_view.h"
#include "opentype_layout_view.h"
#include "opentype_gdef_view.h"
#include "opentype_lookup_glyph_filter.h"



namespace waavs
{
    // ====================================================================
    // OpenTypeGposResolveResult
    //
    // Common result state for GPOS lookup resolution.
    // ====================================================================

    enum class OpenTypeGposResolveResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // openTypeGposEffectiveLookupType
    //
    // Native lookup:
    //
    //   Lookup Type N -> N
    //
    // Extension lookup:
    //
    //   Lookup Type 9 -> ExtensionPos -> actual Type 1..8
    //
    // All ExtensionPos subtables in one lookup must resolve to the same
    // effective lookup type.
    // ====================================================================

    static inline bool openTypeGposEffectiveLookupType(
        const OpenTypeLayoutLookupView& lookup, uint16_t& result) noexcept
    {
        result = 0;

        if (!lookup)
            return false;

        const uint16_t lookupType = lookup.lookupType();

        if (lookupType != 9)
        {
            if (lookupType < 1 || lookupType > 8)
                return false;

            result = lookupType;
            return true;
        }

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        uint16_t extensionLookupType = 0;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data = lookup.subtable(i);

            if (!data)
                return false;

            const OpenTypeGposExtensionPosView extension(data);

            if (!extension)
                return false;

            const uint16_t type = extension.extensionLookupType();

            if (i == 0)
                extensionLookupType = type;
            else if (type != extensionLookupType)
                return false;
        }

        result = extensionLookupType;
        return true;
    }


    static inline bool openTypeGposHasEffectiveLookupType(
        const OpenTypeLayoutLookupView& lookup, uint16_t expectedType) noexcept
    {
        uint16_t lookupType = 0;
        return openTypeGposEffectiveLookupType(lookup, lookupType) &&
            lookupType == expectedType;
    }


    // ====================================================================
    // openTypeGposEffectiveSubtable
    //
    // Native:
    //
    //   Lookup -> actual subtable
    //
    // Extension:
    //
    //   Lookup -> ExtensionPos -> actual subtable
    // ====================================================================

    static inline ByteSpan openTypeGposEffectiveSubtable(
        const OpenTypeLayoutLookupView& lookup, uint16_t effectiveType,
        uint16_t subtableIndex) noexcept
    {
        if (!lookup || subtableIndex >= lookup.subtableCount())
            return {};

        const ByteSpan data = lookup.subtable(subtableIndex);

        if (!data)
            return {};

        if (lookup.lookupType() != 9)
        {
            if (lookup.lookupType() != effectiveType)
                return {};

            return data;
        }

        const OpenTypeGposExtensionPosView extension(data);

        if (!extension || extension.extensionLookupType() != effectiveType)
            return {};

        return extension.extensionSubtable();
    }


    // ====================================================================
    // OpenTypeGposMarkLigatureMatch
    // ====================================================================

    struct OpenTypeGposMarkLigatureMatch
    {
        size_t ligatureIndex{ 0 };
        size_t markIndex{ 0 };

        uint16_t componentIndex{ 0 };

        int32_t ligatureX{ 0 };
        int32_t ligatureY{ 0 };
        int32_t markX{ 0 };
        int32_t markY{ 0 };

        void clear() noexcept
        {
            ligatureIndex = 0;
            markIndex = 0;
            componentIndex = 0;

            ligatureX = 0;
            ligatureY = 0;
            markX = 0;
            markY = 0;
        }
    };


    // ====================================================================
// findOpenTypeGposPreviousNonMark
//
// Mark-to-Base and Mark-to-Ligature both search backward past marks
// and stop at the first non-mark glyph.
//
// Coverage is tested only after that candidate has been found.
// ====================================================================

    static inline OpenTypeGposResolveResult findOpenTypeGposPreviousNonMark(
        const OpenTypeGdefView& gdef,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex, size_t& result) noexcept
    {
        result = 0;

        if (glyphIndex >= buffer.size())
            return OpenTypeGposResolveResult::Invalid;

        for (size_t i = glyphIndex; i != 0; --i)
        {
            const size_t candidate = i - 1;
            const uint32_t glyphId = buffer[candidate].shaping.glyphId;

            if (glyphId > 0xFFFFu)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t glyphClass = 0;

            if (gdef && !gdef.glyphClass(glyphId, glyphClass))
                return OpenTypeGposResolveResult::Invalid;

            if (glyphClass == 3)
                continue;

            result = candidate;
            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // openTypeGposMarkLigatureComponent
    //
    // OpenType itself does not encode the mark-to-component association.
    // GSUB provenance provides it.
    //
    // Matching non-zero ligature IDs:
    //
    //   mark component 1 -> ComponentRecord 0
    //   mark component 2 -> ComponentRecord 1
    //   ...
    //
    // Clamp oversized component numbers to the final component.
    //
    // If no matching association exists, use the final component.
    // ====================================================================

    static inline bool openTypeGposMarkLigatureComponent(
        const OpenTypeShapingGlyph& ligature,
        const OpenTypeShapingGlyph& mark,
        uint16_t componentCount,
        uint16_t& componentIndex) noexcept
    {
        componentIndex = 0;

        if (componentCount == 0)
            return false;

        componentIndex =
            static_cast<uint16_t>(componentCount - 1);

        const uint32_t ligatureId =
            ligature.ligature.id;

        const uint32_t markId =
            mark.ligature.id;

        const uint16_t markComponent =
            mark.ligature.component;

        if (ligatureId != 0 &&
            ligatureId == markId &&
            markComponent != 0)
        {
            uint16_t component = markComponent;

            if (component > componentCount)
                component = componentCount;

            componentIndex =
                static_cast<uint16_t>(component - 1);
        }

        return true;
    }


    // ====================================================================
    // resolveOpenTypeGposMarkLigatureLookup
    //
    // Resolve one exact mark against one complete LookupType 5.
    //
    // The first preceding non-mark glyph is the only ligature candidate.
    // A candidate not present in LigatureCoverage does not cause us to
    // search farther backward.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposMarkLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposMarkLigatureMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposHasEffectiveLookupType(lookup, 5) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        const uint32_t markGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (markGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposResolveResult::Invalid;

        bool haveLigature = false;
        size_t ligatureIndex = 0;
        uint32_t ligatureGlyphId = 0;

        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGposEffectiveSubtable(lookup, 5, subtableIndex);

            if (!data)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposMarkLigaturePosView markLig(data);

            if (!markLig)
                return OpenTypeGposResolveResult::Invalid;


            // ------------------------------------------------------------
            // Current exact target must be covered as a mark.
            // ------------------------------------------------------------

            const OpenTypeCoverageView markCoverage =
                markLig.markCoverage();

            if (!markCoverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t markCoverageIndex = 0;

            if (!markCoverage.find(
                markGlyphId, markCoverageIndex))
            {
                continue;
            }


            // ------------------------------------------------------------
            // Find the preceding non-mark once.
            // ------------------------------------------------------------

            if (!haveLigature)
            {
                const OpenTypeGposResolveResult parentResult =
                    findOpenTypeGposPreviousNonMark(
                        gdef, buffer,
                        glyphIndex, ligatureIndex);

                if (parentResult != OpenTypeGposResolveResult::Match)
                    return parentResult;

                ligatureGlyphId =
                    buffer[ligatureIndex].shaping.glyphId;

                if (ligatureGlyphId > 0xFFFFu)
                    return OpenTypeGposResolveResult::Invalid;

                haveLigature = true;
            }


            // ------------------------------------------------------------
            // The first non-mark must be covered by this subtable.
            // ------------------------------------------------------------

            const OpenTypeCoverageView ligatureCoverage =
                markLig.ligatureCoverage();

            if (!ligatureCoverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t ligatureCoverageIndex = 0;

            if (!ligatureCoverage.find(
                ligatureGlyphId,
                ligatureCoverageIndex))
            {
                continue;
            }


            // ------------------------------------------------------------
            // Resolve arrays.
            // ------------------------------------------------------------

            const OpenTypeGposMarkArrayView marks =
                markLig.markArray();

            const OpenTypeGposLigatureArrayView ligatures =
                markLig.ligatureArray();

            if (!marks || !ligatures)
                return OpenTypeGposResolveResult::Invalid;

            if (markCoverageIndex >= marks.markCount() ||
                ligatureCoverageIndex >= ligatures.ligatureCount())
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            uint16_t markClass = 0;

            if (!marks.markClass(
                markCoverageIndex, markClass))
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            if (markClass >= markLig.markClassCount())
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposLigatureAttachView attach =
                ligatures.ligatureAttach(
                    ligatureCoverageIndex);

            if (!attach)
                return OpenTypeGposResolveResult::Invalid;


            // ------------------------------------------------------------
            // Select ligature component using GSUB provenance.
            // ------------------------------------------------------------

            uint16_t componentIndex = 0;

            if (!openTypeGposMarkLigatureComponent(
                buffer[ligatureIndex].shaping,
                buffer[glyphIndex].shaping,
                attach.componentCount(),
                componentIndex))
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            if (componentIndex >= attach.componentCount())
                return OpenTypeGposResolveResult::Invalid;


            // ------------------------------------------------------------
            // NULL component/class anchor gives later subtables a chance.
            // ------------------------------------------------------------

            if (!attach.hasAnchor(
                componentIndex, markClass))
            {
                continue;
            }

            const OpenTypeGposAnchorView markAnchor =
                marks.markAnchor(markCoverageIndex);

            const OpenTypeGposAnchorView ligatureAnchor =
                attach.anchor(componentIndex, markClass);

            if (!markAnchor || !ligatureAnchor)
                return OpenTypeGposResolveResult::Invalid;

            match.ligatureIndex = ligatureIndex;
            match.markIndex = glyphIndex;
            match.componentIndex = componentIndex;

            match.ligatureX =
                ligatureAnchor.xCoordinate();

            match.ligatureY =
                ligatureAnchor.yCoordinate();

            match.markX =
                markAnchor.xCoordinate();

            match.markY =
                markAnchor.yCoordinate();

            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposMarkLigatureMatch
    //
    // Same attachment arithmetic as Mark-to-Base.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkLigatureMatch(
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposMarkLigatureMatch& match)
    {
        if (!attachments.matches(buffer.size()) ||
            match.ligatureIndex >= buffer.size() ||
            match.markIndex >= buffer.size() ||
            match.ligatureIndex >= match.markIndex)
        {
            return false;
        }

        const int64_t localX =
            int64_t(match.ligatureX) -
            int64_t(match.markX);

        const int64_t localY =
            int64_t(match.ligatureY) -
            int64_t(match.markY);

        if (localX < std::numeric_limits<int32_t>::min() ||
            localX > std::numeric_limits<int32_t>::max() ||
            localY < std::numeric_limits<int32_t>::min() ||
            localY > std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposMark(
            attachments, buffer,
            match.markIndex,
            match.ligatureIndex,
            static_cast<int32_t>(localX),
            static_cast<int32_t>(localY));
    }


    // ====================================================================
    // applyOpenTypeGposMarkLigatureLookupAt
    // ====================================================================

    static inline OpenTypeGposResolveResult applyOpenTypeGposMarkLigatureLookupAt(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposResolveResult::Invalid;

        OpenTypeGposMarkLigatureMatch match;

        const OpenTypeGposResolveResult result =
            resolveOpenTypeGposMarkLigatureLookup(
                lookup, gdef, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposResolveResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposMarkLigatureMatch(
            working, workingAttachments, match))
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposResolveResult::Match;
    }


    // ====================================================================
    // Shared-state whole lookup.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 5) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposMarkLigatureMatch match;

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposMarkLigatureLookup(
                    lookup, gdef, working,
                    glyphIndex, match);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;

            if (result == OpenTypeGposResolveResult::NoMatch)
                continue;

            if (!applyOpenTypeGposMarkLigatureMatch(
                working, workingAttachments,
                match))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    // ====================================================================
    // Standalone convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposMarkLigatureLookup(
            lookup, gdef, working,
            attachments))
        {
            return false;
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

    // ====================================================================
    // OpenTypeGposMarkBaseMatch
    // ====================================================================

    struct OpenTypeGposMarkBaseMatch
    {
        size_t baseIndex{ 0 };
        size_t markIndex{ 0 };

        int32_t baseX{ 0 };
        int32_t baseY{ 0 };
        int32_t markX{ 0 };
        int32_t markY{ 0 };

        void clear() noexcept
        {
            baseIndex = 0;
            markIndex = 0;
            baseX = 0;
            baseY = 0;
            markX = 0;
            markY = 0;
        }
    };





    // ====================================================================
    // findOpenTypeGposMarkBaseParent
    //
    // Search backward to the first non-mark glyph.
    //
    // Mark-to-Base has special traversal semantics: preceding marks are
    // skipped regardless of the lookup's ordinary IgnoreMarks flag.
    // ====================================================================

    static inline OpenTypeGposResolveResult findOpenTypeGposMarkBaseParent(
        const OpenTypeGdefView& gdef,
        const ShapedGlyphBuffer& buffer,
        size_t markIndex, size_t& baseIndex) noexcept
    {
        return findOpenTypeGposPreviousNonMark(
            gdef, buffer, markIndex, baseIndex);
    }





    // ====================================================================
    // resolveOpenTypeGposMarkBaseLookup
    //
    // Resolve one exact mark position against one complete Type 4 lookup.
    //
    // No placement mutation occurs here.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposMarkBaseLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposMarkBaseMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposHasEffectiveLookupType(lookup, 4) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        const uint32_t markGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (markGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposResolveResult::Invalid;

        bool haveBase = false;
        size_t baseIndex = 0;
        uint32_t baseGlyphId = 0;

        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGposEffectiveSubtable(lookup, 4, subtableIndex);

            if (!data)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposMarkBasePosView markBase(data);

            if (!markBase)
                return OpenTypeGposResolveResult::Invalid;


            // ------------------------------------------------------------
            // Current lookup position must be a covered mark.
            // ------------------------------------------------------------

            const OpenTypeCoverageView markCoverage =
                markBase.markCoverage();

            if (!markCoverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t markCoverageIndex = 0;

            if (!markCoverage.find(
                markGlyphId, markCoverageIndex))
            {
                continue;
            }


            // ------------------------------------------------------------
            // Find preceding non-mark only after we know this subtable
            // actually covers the current mark.
            // ------------------------------------------------------------

            if (!haveBase)
            {
                const OpenTypeGposResolveResult baseResult =
                    findOpenTypeGposMarkBaseParent(
                        gdef, buffer, glyphIndex, baseIndex);

                if (baseResult != OpenTypeGposResolveResult::Match)
                    return baseResult;

                baseGlyphId =
                    buffer[baseIndex].shaping.glyphId;

                if (baseGlyphId > 0xFFFFu)
                    return OpenTypeGposResolveResult::Invalid;

                haveBase = true;
            }


            // ------------------------------------------------------------
            // That first non-mark glyph must be covered by this subtable.
            //
            // Do not continue searching farther backward.
            // ------------------------------------------------------------

            const OpenTypeCoverageView baseCoverage =
                markBase.baseCoverage();

            if (!baseCoverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t baseCoverageIndex = 0;

            if (!baseCoverage.find(
                baseGlyphId, baseCoverageIndex))
            {
                continue;
            }


            // ------------------------------------------------------------
            // Resolve MarkRecord -> class -> BaseAnchor.
            // ------------------------------------------------------------

            const OpenTypeGposMarkArrayView marks =
                markBase.markArray();

            const OpenTypeGposBaseArrayView bases =
                markBase.baseArray();

            if (!marks || !bases)
                return OpenTypeGposResolveResult::Invalid;

            if (markCoverageIndex >= marks.markCount() ||
                baseCoverageIndex >= bases.baseCount())
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            uint16_t markClass = 0;

            if (!marks.markClass(
                markCoverageIndex, markClass))
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            if (markClass >= markBase.markClassCount())
                return OpenTypeGposResolveResult::Invalid;


            // NULL BaseAnchor means this subtable does not position this
            // particular base/class pair. A later subtable gets a chance.

            if (!bases.hasBaseAnchor(
                baseCoverageIndex, markClass))
            {
                continue;
            }

            const OpenTypeGposAnchorView markAnchor =
                marks.markAnchor(markCoverageIndex);

            const OpenTypeGposAnchorView baseAnchor =
                bases.baseAnchor(
                    baseCoverageIndex, markClass);

            if (!markAnchor || !baseAnchor)
                return OpenTypeGposResolveResult::Invalid;

            match.baseIndex = baseIndex;
            match.markIndex = glyphIndex;

            match.baseX = baseAnchor.xCoordinate();
            match.baseY = baseAnchor.yCoordinate();
            match.markX = markAnchor.xCoordinate();
            match.markY = markAnchor.yCoordinate();

            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposMarkBaseMatch
    //
    // Store the local anchor difference and the parent relation.
    //
    // The base placement and both advances are untouched.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkBaseMatch(
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposMarkBaseMatch& match)
    {
        if (!attachments.matches(buffer.size()) ||
            match.baseIndex >= buffer.size() ||
            match.markIndex >= buffer.size() ||
            match.baseIndex >= match.markIndex)
        {
            return false;
        }

        const int64_t localX =
            int64_t(match.baseX) - int64_t(match.markX);

        const int64_t localY =
            int64_t(match.baseY) - int64_t(match.markY);

        if (localX < std::numeric_limits<int32_t>::min() ||
            localX > std::numeric_limits<int32_t>::max() ||
            localY < std::numeric_limits<int32_t>::min() ||
            localY > std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposMark(
            attachments, buffer,
            match.markIndex, match.baseIndex,
            static_cast<int32_t>(localX),
            static_cast<int32_t>(localY));
    }


    // ====================================================================
    // applyOpenTypeGposMarkBaseLookupAt
    //
    // Exact-target Type 4 application.
    //
    // Attachments remain unresolved for later GPOS lookups.
    // ====================================================================

    static inline OpenTypeGposResolveResult applyOpenTypeGposMarkBaseLookupAt(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposResolveResult::Invalid;

        OpenTypeGposMarkBaseMatch match;

        const OpenTypeGposResolveResult result =
            resolveOpenTypeGposMarkBaseLookup(
                lookup, gdef, buffer, glyphIndex, match);

        if (result != OpenTypeGposResolveResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposMarkBaseMatch(
            working, workingAttachments, match))
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposResolveResult::Match;
    }


    // ====================================================================
    // Shared-state whole lookup.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkBaseLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 4) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposMarkBaseMatch match;

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposMarkBaseLookup(
                    lookup, gdef, working,
                    glyphIndex, match);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;

            if (result == OpenTypeGposResolveResult::NoMatch)
                continue;

            if (!applyOpenTypeGposMarkBaseMatch(
                working, workingAttachments, match))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    // ====================================================================
    // Standalone convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkBaseLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposMarkBaseLookup(
            lookup, gdef, working, attachments))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments, runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }



    // ====================================================================
// OpenTypeGposCursiveMatch
//
// firstIndex:
//   Glyph providing the ExitAnchor.
//
// secondIndex:
//   Following eligible glyph providing the EntryAnchor.
// ====================================================================

    struct OpenTypeGposCursiveMatch
    {
        size_t firstIndex{ 0 };
        size_t secondIndex{ 0 };

        int32_t exitX{ 0 };
        int32_t exitY{ 0 };
        int32_t entryX{ 0 };
        int32_t entryY{ 0 };

        void clear() noexcept
        {
            firstIndex = 0;
            secondIndex = 0;
            exitX = 0;
            exitY = 0;
            entryX = 0;
            entryY = 0;
        }
    };


    // ====================================================================
    // resolveOpenTypeGposCursiveLookup
    //
    // The current glyph is not filtered.
    // LookupFlag filtering is used to find the following participating glyph.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposCursiveLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposCursiveMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposHasEffectiveLookupType(lookup, 3) ||
            !filter || glyphIndex >= buffer.size())
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        const uint32_t firstGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        size_t secondIndex = 0;

        const OpenTypeLookupGlyphSearchResult search =
            filter.next(buffer, glyphIndex, secondIndex);

        if (search == OpenTypeLookupGlyphSearchResult::Invalid)
            return OpenTypeGposResolveResult::Invalid;

        if (search == OpenTypeLookupGlyphSearchResult::End)
            return OpenTypeGposResolveResult::NoMatch;

        if (secondIndex <= glyphIndex || secondIndex >= buffer.size())
            return OpenTypeGposResolveResult::Invalid;

        const uint32_t secondGlyphId =
            buffer[secondIndex].shaping.glyphId;

        if (secondGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposResolveResult::Invalid;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data = openTypeGposEffectiveSubtable(lookup, 3, i);

            if (!data)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposCursivePosView cursive(data);

            if (!cursive)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeCoverageView coverage =
                cursive.coverage();

            if (!coverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t firstCoverageIndex = 0;

            if (!coverage.find(firstGlyphId, firstCoverageIndex))
                continue;

            if (firstCoverageIndex >= cursive.entryExitCount())
                return OpenTypeGposResolveResult::Invalid;

            if (!cursive.hasExitAnchor(firstCoverageIndex))
                continue;

            uint16_t secondCoverageIndex = 0;

            if (!coverage.find(secondGlyphId, secondCoverageIndex))
                continue;

            if (secondCoverageIndex >= cursive.entryExitCount())
                return OpenTypeGposResolveResult::Invalid;

            if (!cursive.hasEntryAnchor(secondCoverageIndex))
                continue;

            const OpenTypeGposAnchorView exitAnchor =
                cursive.exitAnchor(firstCoverageIndex);

            const OpenTypeGposAnchorView entryAnchor =
                cursive.entryAnchor(secondCoverageIndex);

            if (!exitAnchor || !entryAnchor)
                return OpenTypeGposResolveResult::Invalid;

            match.firstIndex = glyphIndex;
            match.secondIndex = secondIndex;

            match.exitX = exitAnchor.xCoordinate();
            match.exitY = exitAnchor.yCoordinate();
            match.entryX = entryAnchor.xCoordinate();
            match.entryY = entryAnchor.yCoordinate();

            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposCursiveMatch
    //
    // Horizontal shaping only for now.
    //
    // Our shaped buffer remains in logical order and horizontal advances are
    // stored as positive logical advance magnitudes. Therefore:
    //
    // LTR:
    //
    //   nextOrigin = firstOrigin + advanceX
    //
    // RTL:
    //
    //   nextOrigin = firstOrigin - advanceX
    //
    // This is deliberately independent of LookupFlag RIGHT_TO_LEFT.
    // ====================================================================

    static inline bool applyOpenTypeGposCursiveMatch(
        const OpenTypeLayoutLookupView& lookup,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposCursiveMatch& match,
        bool runRightToLeft)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 3) ||
            !attachments.matches(buffer.size()) ||
            match.firstIndex >= buffer.size() ||
            match.secondIndex >= buffer.size() ||
            match.secondIndex <= match.firstIndex)
        {
            return false;
        }

        ShapedGlyph& first = buffer[match.firstIndex];
        ShapedGlyph& second = buffer[match.secondIndex];


        // ------------------------------------------------------------
        // Main-line attachment.
        //
        // Align:
        //
        //   first ExitAnchor == second EntryAnchor
        //
        // by changing the first glyph's logical horizontal advance.
        // ------------------------------------------------------------

        int64_t advance = 0;

        if (!runRightToLeft)
        {
            advance =
                int64_t(first.placement.offsetX) +
                int64_t(match.exitX) -
                int64_t(second.placement.offsetX) -
                int64_t(match.entryX);
        }
        else
        {
            advance =
                int64_t(second.placement.offsetX) +
                int64_t(match.entryX) -
                int64_t(first.placement.offsetX) -
                int64_t(match.exitX);
        }

        if (advance < INT32_MIN || advance > INT32_MAX)
            return false;

        first.placement.advanceX =
            static_cast<int32_t>(advance);


        // ------------------------------------------------------------
        // Cross-stream attachment.
        //
        // RIGHT_TO_LEFT does NOT reverse traversal.
        //
        // Clear:
        //   second attaches to first.
        //
        // Set:
        //   first attaches to second.
        // ------------------------------------------------------------

        const bool lookupRightToLeft =
            (lookup.lookupFlag() & 0x0001u) != 0;

        size_t child = 0;
        size_t parent = 0;
        int64_t minorOffset = 0;

        if (!lookupRightToLeft)
        {
            child = match.secondIndex;
            parent = match.firstIndex;

            minorOffset =
                int64_t(match.exitY) -
                int64_t(match.entryY);
        }
        else
        {
            child = match.firstIndex;
            parent = match.secondIndex;

            minorOffset =
                int64_t(match.entryY) -
                int64_t(match.exitY);
        }

        if (minorOffset < INT32_MIN || minorOffset > INT32_MAX)
            return false;

        return attachOpenTypeGposCursive(
            attachments, buffer,
            child, parent,
            static_cast<int32_t>(minorOffset));
    }


    // ====================================================================
    // applyOpenTypeGposCursiveLookupAt
    //
    // Exact-position Type 3 execution.
    //
    // Attachments are deliberately not finalized here. Nested/contextual
    // GPOS must retain them until all positioning lookups have executed.
    // ====================================================================

    static inline OpenTypeGposResolveResult applyOpenTypeGposCursiveLookupAt(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposResolveResult::Invalid;

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGposResolveResult::Invalid;

        OpenTypeGposCursiveMatch match;

        const OpenTypeGposResolveResult result =
            resolveOpenTypeGposCursiveLookup(
                lookup, filter, buffer, glyphIndex, match);

        if (result != OpenTypeGposResolveResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments = attachments;

        if (!applyOpenTypeGposCursiveMatch(
            lookup, working, workingAttachments,
            match, runRightToLeft))
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposResolveResult::Match;
    }


    // ====================================================================
    // applyOpenTypeGposCursiveLookup
    //
    // Shared-state version.
    //
    // Does NOT resolve attachment offsets. This is the version the eventual
    // complete GPOS pipeline should use.
    // ====================================================================

    static inline bool applyOpenTypeGposCursiveLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 3) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return false;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments = attachments;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            OpenTypeGposCursiveMatch match;

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposCursiveLookup(
                    lookup, filter, working,
                    glyphIndex, match);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;

            if (result == OpenTypeGposResolveResult::NoMatch)
                continue;

            if (!applyOpenTypeGposCursiveMatch(
                lookup, working, workingAttachments,
                match, runRightToLeft))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    // ====================================================================
    // Standalone convenience version.
    //
    // Useful for focused tests and isolated Type 3 execution.
    // ====================================================================

    static inline bool applyOpenTypeGposCursiveLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposCursiveLookup(
            lookup, gdef, working,
            runRightToLeft, attachments))
        {
            return false;
        }

        if (!resolveOpenTypeGposCursiveAttachments(
            working, attachments))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }




    // ====================================================================
    // OpenTypeGposPairMatch
    // ====================================================================

    struct OpenTypeGposPairMatch
    {
        size_t firstIndex{ 0 };
        size_t secondIndex{ 0 };

        OpenTypeGposValueRecord firstValue{};
        OpenTypeGposValueRecord secondValue{};

        uint16_t valueFormat2{ 0 };

        void clear() noexcept
        {
            firstIndex = 0;
            secondIndex = 0;
            firstValue = {};
            secondValue = {};
            valueFormat2 = 0;
        }
    };


    // ====================================================================
    // resolveOpenTypeGposPairSubtable
    //
    // Resolve a known pair of glyph IDs against one PairPos subtable.
    //
    // No placement mutation occurs here.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposPairSubtable(
        const OpenTypeGposPairPosView& pair,
        uint32_t firstGlyphId, uint32_t secondGlyphId,
        OpenTypeGposValueRecord& value1,
        OpenTypeGposValueRecord& value2) noexcept
    {
        value1 = {};
        value2 = {};

        if (!pair || firstGlyphId > 0xFFFFu || secondGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const OpenTypeCoverageView coverage = pair.coverage();

        if (!coverage)
            return OpenTypeGposResolveResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(firstGlyphId, coverageIndex))
            return OpenTypeGposResolveResult::NoMatch;


        // ------------------------------------------------------------
        // Format 1 - explicit second-glyph PairSet.
        // ------------------------------------------------------------

        if (pair.format() == 1)
        {
            if (coverageIndex >= pair.pairSetCount())
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposPairSetView set =
                pair.pairSet(coverageIndex);

            if (!set)
                return OpenTypeGposResolveResult::Invalid;

            if (!set.find(
                static_cast<uint16_t>(secondGlyphId),
                value1, value2))
            {
                return OpenTypeGposResolveResult::NoMatch;
            }

            return OpenTypeGposResolveResult::Match;
        }


        // ------------------------------------------------------------
        // Format 2 - class pair.
        // ------------------------------------------------------------

        if (pair.format() == 2)
        {
            const OpenTypeClassDefView classDef1 = pair.classDef1();
            const OpenTypeClassDefView classDef2 = pair.classDef2();

            if (!classDef1 || !classDef2)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t class1 = 0;
            uint16_t class2 = 0;

            if (!classDef1.classValue(firstGlyphId, class1) ||
                !classDef2.classValue(secondGlyphId, class2))
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            if (class1 >= pair.class1Count() ||
                class2 >= pair.class2Count())
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            if (!pair.classValueRecords(
                class1, class2, value1, value2))
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::Invalid;
    }


    // ====================================================================
    // resolveOpenTypeGposPairLookup
    //
    // Resolve one complete LookupType 2 at one exact starting position.
    //
    // The first glyph is the exact current glyph and is not filtered.
    // LookupFlag filtering is used only to find the second participating
    // glyph.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposPairLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposPairMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposHasEffectiveLookupType(lookup, 2) ||
            !filter || glyphIndex >= buffer.size())
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        const uint32_t firstGlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;


        // ------------------------------------------------------------
        // Locate the second eligible glyph.
        // ------------------------------------------------------------

        size_t secondIndex = 0;

        const OpenTypeLookupGlyphSearchResult search =
            filter.next(buffer, glyphIndex, secondIndex);

        if (search == OpenTypeLookupGlyphSearchResult::Invalid)
            return OpenTypeGposResolveResult::Invalid;

        if (search == OpenTypeLookupGlyphSearchResult::End)
            return OpenTypeGposResolveResult::NoMatch;

        if (secondIndex <= glyphIndex || secondIndex >= buffer.size())
            return OpenTypeGposResolveResult::Invalid;

        const uint32_t secondGlyphId =
            buffer[secondIndex].shaping.glyphId;

        if (secondGlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;


        // ------------------------------------------------------------
        // Try subtables in stored order.
        // ------------------------------------------------------------

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposResolveResult::Invalid;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data = openTypeGposEffectiveSubtable(lookup, 2, i);

            if (!data)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposPairPosView pair(data);

            if (!pair)
                return OpenTypeGposResolveResult::Invalid;

            OpenTypeGposValueRecord value1{};
            OpenTypeGposValueRecord value2{};

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposPairSubtable(
                    pair, firstGlyphId, secondGlyphId,
                    value1, value2);

            if (result == OpenTypeGposResolveResult::Invalid)
                return result;

            if (result == OpenTypeGposResolveResult::NoMatch)
                continue;

            match.firstIndex = glyphIndex;
            match.secondIndex = secondIndex;
            match.firstValue = value1;
            match.secondValue = value2;
            match.valueFormat2 = pair.valueFormat2();

            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposPairLookupAt
    //
    // Apply one PairPos lookup at one exact physical starting position.
    //
    // resumeIndex:
    //
    //   valueFormat2 == 0 -> second glyph is next lookup position
    //
    //   valueFormat2 != 0 -> continue after the second glyph
    // ====================================================================

    static inline OpenTypeGposResolveResult applyOpenTypeGposPairLookupAt(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        size_t* resumeIndex = nullptr) noexcept
    {
        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGposResolveResult::Invalid;

        OpenTypeGposPairMatch match;

        const OpenTypeGposResolveResult result =
            resolveOpenTypeGposPairLookup(
                lookup, filter, buffer, glyphIndex, match);

        if (result != OpenTypeGposResolveResult::Match)
            return result;

        applyOpenTypeGposValueRecord(
            match.firstValue,
            buffer[match.firstIndex].placement);

        applyOpenTypeGposValueRecord(
            match.secondValue,
            buffer[match.secondIndex].placement);

        if (resumeIndex)
        {
            *resumeIndex =
                match.valueFormat2 == 0
                ? match.secondIndex
                : match.secondIndex + 1;
        }

        return OpenTypeGposResolveResult::Match;
    }


    // ====================================================================
    // applyOpenTypeGposPairLookup
    //
    // Apply one complete LookupType 2 to the shaped glyph sequence.
    //
    // Work against a copy so malformed data cannot leave partial placement
    // changes in the destination.
    // ====================================================================

    static inline bool applyOpenTypeGposPairLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 2))
            return false;

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return false;

        ShapedGlyphBuffer working = buffer;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGposPairMatch match;

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposPairLookup(
                    lookup, filter, working, glyphIndex, match);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;

            if (result == OpenTypeGposResolveResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            applyOpenTypeGposValueRecord(
                match.firstValue,
                working[match.firstIndex].placement);

            applyOpenTypeGposValueRecord(
                match.secondValue,
                working[match.secondIndex].placement);

            glyphIndex =
                match.valueFormat2 == 0
                ? match.secondIndex
                : match.secondIndex + 1;
        }

        buffer = std::move(working);
        return true;
    }


    static inline bool applyOpenTypeGposPairLookup(
        const OpenTypeLayoutLookupView& lookup,
        ShapedGlyphBuffer& buffer)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGposPairLookup(
            lookup, gdef, buffer);
    }

    // ====================================================================
    // resolveOpenTypeGposSingleSubtable
    //
    // Resolve one glyph against one SinglePos subtable.
    //
    // No placement is modified here.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposSingleSubtable(
        const OpenTypeGposSinglePosView& single, uint32_t glyphId,
        OpenTypeGposValueRecord& value) noexcept
    {
        value = {};

        if (!single || glyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const OpenTypeCoverageView coverage = single.coverage();

        if (!coverage)
            return OpenTypeGposResolveResult::Invalid;

        uint16_t coverageIndex = 0;

        if (!coverage.find(glyphId, coverageIndex))
            return OpenTypeGposResolveResult::NoMatch;

        if (!single.valueRecordForCoverageIndex(coverageIndex, value))
            return OpenTypeGposResolveResult::Invalid;

        return OpenTypeGposResolveResult::Match;
    }


    // ====================================================================
    // resolveOpenTypeGposSingleLookup
    //
    // Resolve one glyph against one complete GPOS LookupType 1 Lookup.
    //
    // Subtables are tried in stored order. First match wins.
    //
    // This is an exact-target operation. LookupFlags are therefore not used
    // to filter the current glyph; traversal filtering belongs to the caller
    // that selects lookup positions.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposSingleLookup(
        const OpenTypeLayoutLookupView& lookup, uint32_t glyphId,
        OpenTypeGposValueRecord& value) noexcept
    {
        value = {};

        if (!openTypeGposHasEffectiveLookupType(lookup, 1) || glyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposResolveResult::Invalid;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data = openTypeGposEffectiveSubtable(lookup, 1, i);

            if (!data)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposSinglePosView single(data);

            if (!single)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposSingleSubtable(single, glyphId, value);

            if (result == OpenTypeGposResolveResult::Invalid)
                return result;

            if (result == OpenTypeGposResolveResult::Match)
                return result;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposSingleLookupAt
    //
    // Apply one Type 1 lookup at one exact physical glyph position.
    //
    // Glyph identity and provenance are untouched.
    // Only GlyphPlacement changes.
    // ====================================================================

    static inline OpenTypeGposResolveResult applyOpenTypeGposSingleLookupAt(
        const OpenTypeLayoutLookupView& lookup,
        ShapedGlyphBuffer& buffer, size_t glyphIndex) noexcept
    {
        if (glyphIndex >= buffer.size())
            return OpenTypeGposResolveResult::Invalid;

        OpenTypeGposValueRecord value{};

        const OpenTypeGposResolveResult result =
            resolveOpenTypeGposSingleLookup(
                lookup,
                buffer[glyphIndex].shaping.glyphId,
                value);

        if (result != OpenTypeGposResolveResult::Match)
            return result;

        applyOpenTypeGposValueRecord(
            value,
            buffer[glyphIndex].placement);

        return OpenTypeGposResolveResult::Match;
    }



    // ====================================================================
// OpenTypeGposMarkMarkMatch
// ====================================================================

    struct OpenTypeGposMarkMarkMatch
    {
        size_t mark1Index{ 0 };
        size_t mark2Index{ 0 };

        int32_t mark1X{ 0 };
        int32_t mark1Y{ 0 };
        int32_t mark2X{ 0 };
        int32_t mark2Y{ 0 };

        void clear() noexcept
        {
            mark1Index = 0;
            mark2Index = 0;

            mark1X = 0;
            mark1Y = 0;
            mark2X = 0;
            mark2Y = 0;
        }
    };


    // ====================================================================
    // openTypeGposMarkMarkCompatible
    //
    // Keep Mark-to-Mark attachment inside one logical ligature component.
    //
    // No ligature association:
    //   marks belong to the same ordinary base.
    //
    // Same non-zero ligature ID:
    //   component numbers must agree.
    //
    // Different IDs:
    //   allow attachment if one glyph is itself represented as a ligature
    //   base. This follows the same exception used by HarfBuzz.
    // ====================================================================

    static inline bool openTypeGposMarkMarkCompatible(
        const OpenTypeShapingGlyph& mark1,
        const OpenTypeShapingGlyph& mark2) noexcept
    {
        const uint32_t id1 = mark1.ligature.id;
        const uint32_t id2 = mark2.ligature.id;

        const uint16_t component1 =
            mark1.ligature.component;

        const uint16_t component2 =
            mark2.ligature.component;

        if (id1 == id2)
        {
            if (id1 == 0)
                return true;

            return component1 == component2;
        }

        if ((id1 != 0 && component1 == 0) ||
            (id2 != 0 && component2 == 0))
        {
            return true;
        }

        return false;
    }


    // ====================================================================
    // resolveOpenTypeGposMarkMarkLookup
    //
    // Resolve one exact Mark1 position.
    //
    // Lookup filtering is applied only while walking backward to Mark2.
    // The current Mark1 is matched directly against Mark1Coverage.
    // ====================================================================

    static inline OpenTypeGposResolveResult resolveOpenTypeGposMarkMarkLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeLookupGlyphFilter& filter,
        const ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposMarkMarkMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGposHasEffectiveLookupType(lookup, 6) ||
            !filter || glyphIndex >= buffer.size())
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        const uint32_t mark1GlyphId =
            buffer[glyphIndex].shaping.glyphId;

        if (mark1GlyphId > 0xFFFFu)
            return OpenTypeGposResolveResult::Invalid;

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposResolveResult::Invalid;

        bool haveMark2 = false;
        size_t mark2Index = 0;
        uint32_t mark2GlyphId = 0;

        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGposEffectiveSubtable(lookup, 6, subtableIndex);

            if (!data)
                return OpenTypeGposResolveResult::Invalid;

            const OpenTypeGposMarkMarkPosView markMark(data);

            if (!markMark)
                return OpenTypeGposResolveResult::Invalid;


            // ------------------------------------------------------------
            // Current exact target must be a covered Mark1.
            // ------------------------------------------------------------

            const OpenTypeCoverageView mark1Coverage =
                markMark.mark1Coverage();

            if (!mark1Coverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t mark1CoverageIndex = 0;

            if (!mark1Coverage.find(
                mark1GlyphId, mark1CoverageIndex))
            {
                continue;
            }


            // ------------------------------------------------------------
            // Find preceding eligible glyph once.
            // ------------------------------------------------------------

            if (!haveMark2)
            {
                const OpenTypeLookupGlyphSearchResult previousResult =
                    filter.previous(
                        buffer, glyphIndex, mark2Index);

                switch (previousResult)
                {
                case OpenTypeLookupGlyphSearchResult::Found:
                    break;

                case OpenTypeLookupGlyphSearchResult::End:
                    return OpenTypeGposResolveResult::NoMatch;

                default:
                    return OpenTypeGposResolveResult::Invalid;
                }

                if (mark2Index >= glyphIndex ||
                    mark2Index >= buffer.size())
                {
                    return OpenTypeGposResolveResult::Invalid;
                }

                mark2GlyphId =
                    buffer[mark2Index].shaping.glyphId;

                if (mark2GlyphId > 0xFFFFu)
                    return OpenTypeGposResolveResult::Invalid;

                haveMark2 = true;


                // --------------------------------------------------------
                // Do not stack marks belonging to different components
                // of the same ligature.
                // --------------------------------------------------------

                if (!openTypeGposMarkMarkCompatible(
                    buffer[glyphIndex].shaping,
                    buffer[mark2Index].shaping))
                {
                    return OpenTypeGposResolveResult::NoMatch;
                }
            }


            // ------------------------------------------------------------
            // The immediately preceding eligible glyph must be covered.
            //
            // Do not continue searching backward if it is not.
            // ------------------------------------------------------------

            const OpenTypeCoverageView mark2Coverage =
                markMark.mark2Coverage();

            if (!mark2Coverage)
                return OpenTypeGposResolveResult::Invalid;

            uint16_t mark2CoverageIndex = 0;

            if (!mark2Coverage.find(
                mark2GlyphId, mark2CoverageIndex))
            {
                continue;
            }


            // ------------------------------------------------------------
            // Resolve arrays and Mark1 class.
            // ------------------------------------------------------------

            const OpenTypeGposMarkArrayView mark1Array =
                markMark.mark1Array();

            const OpenTypeGposMark2ArrayView mark2Array =
                markMark.mark2Array();

            if (!mark1Array || !mark2Array)
                return OpenTypeGposResolveResult::Invalid;

            if (mark1CoverageIndex >=
                mark1Array.markCount() ||
                mark2CoverageIndex >=
                mark2Array.mark2Count())
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            uint16_t markClass = 0;

            if (!mark1Array.markClass(
                mark1CoverageIndex, markClass))
            {
                return OpenTypeGposResolveResult::Invalid;
            }

            if (markClass >=
                markMark.markClassCount())
            {
                return OpenTypeGposResolveResult::Invalid;
            }


            // ------------------------------------------------------------
            // NULL Mark2 anchor gives a later subtable a chance.
            // ------------------------------------------------------------

            if (!mark2Array.hasAnchor(
                mark2CoverageIndex, markClass))
            {
                continue;
            }

            const OpenTypeGposAnchorView mark1Anchor =
                mark1Array.markAnchor(
                    mark1CoverageIndex);

            const OpenTypeGposAnchorView mark2Anchor =
                mark2Array.anchor(
                    mark2CoverageIndex, markClass);

            if (!mark1Anchor || !mark2Anchor)
                return OpenTypeGposResolveResult::Invalid;

            match.mark1Index = glyphIndex;
            match.mark2Index = mark2Index;

            match.mark1X = mark1Anchor.xCoordinate();
            match.mark1Y = mark1Anchor.yCoordinate();

            match.mark2X = mark2Anchor.xCoordinate();
            match.mark2Y = mark2Anchor.yCoordinate();

            return OpenTypeGposResolveResult::Match;
        }

        return OpenTypeGposResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposMarkMarkMatch
    //
    // Same attachment primitive as Types 4 and 5.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkMarkMatch(
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        const OpenTypeGposMarkMarkMatch& match)
    {
        if (!attachments.matches(buffer.size()) ||
            match.mark1Index >= buffer.size() ||
            match.mark2Index >= buffer.size() ||
            match.mark2Index >= match.mark1Index)
        {
            return false;
        }

        const int64_t localX =
            int64_t(match.mark2X) -
            int64_t(match.mark1X);

        const int64_t localY =
            int64_t(match.mark2Y) -
            int64_t(match.mark1Y);

        if (localX <
            std::numeric_limits<int32_t>::min() ||
            localX >
            std::numeric_limits<int32_t>::max() ||
            localY <
            std::numeric_limits<int32_t>::min() ||
            localY >
            std::numeric_limits<int32_t>::max())
        {
            return false;
        }

        return attachOpenTypeGposMark(
            attachments, buffer,
            match.mark1Index,
            match.mark2Index,
            static_cast<int32_t>(localX),
            static_cast<int32_t>(localY));
    }


    // ====================================================================
    // applyOpenTypeGposMarkMarkLookupAt
    // ====================================================================

    static inline OpenTypeGposResolveResult applyOpenTypeGposMarkMarkLookupAt(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!attachments.matches(buffer.size()))
            return OpenTypeGposResolveResult::Invalid;

        const OpenTypeLookupGlyphFilter filter(
            lookup, gdef);

        if (!filter)
            return OpenTypeGposResolveResult::Invalid;

        OpenTypeGposMarkMarkMatch match;

        const OpenTypeGposResolveResult result =
            resolveOpenTypeGposMarkMarkLookup(
                lookup, filter, buffer,
                glyphIndex, match);

        if (result != OpenTypeGposResolveResult::Match)
            return result;

        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        if (!applyOpenTypeGposMarkMarkMatch(
            working, workingAttachments, match))
        {
            return OpenTypeGposResolveResult::Invalid;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return OpenTypeGposResolveResult::Match;
    }


    // ====================================================================
    // Shared-state whole lookup.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkMarkLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments)
    {
        if (!openTypeGposHasEffectiveLookupType(lookup, 6) ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        const OpenTypeLookupGlyphFilter filter(
            lookup, gdef);

        if (!filter)
            return false;

        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposMarkMarkMatch match;

            const OpenTypeGposResolveResult result =
                resolveOpenTypeGposMarkMarkLookup(
                    lookup, filter, working,
                    glyphIndex, match);

            if (result == OpenTypeGposResolveResult::Invalid)
                return false;

            if (result == OpenTypeGposResolveResult::NoMatch)
                continue;

            if (!applyOpenTypeGposMarkMarkMatch(
                working, workingAttachments,
                match))
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);

        return true;
    }


    // ====================================================================
    // Standalone convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGposMarkMarkLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        if (!applyOpenTypeGposMarkMarkLookup(
            lookup, gdef, working,
            attachments))
        {
            return false;
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


    // ====================================================================
// copyOpenTypeGposRange
//
// Build a local placement workspace corresponding exactly to range.
//
// Existing attachment relations whose parents are also inside range are
// translated to local coordinates. Relations reaching outside range are
// intentionally not imported; nested contextual execution may not use
// glyphs outside its input range.
// ====================================================================

    static inline bool copyOpenTypeGposRange(
        const ShapedGlyphBuffer& source,
        const OpenTypeGposAttachmentState& sourceAttachments,
        const OpenTypeGposApplyRange& range,
        ShapedGlyphBuffer& target,
        OpenTypeGposAttachmentState& targetAttachments)
    {
        if (!range.validFor(source.size()) ||
            !sourceAttachments.matches(source.size()))
        {
            return false;
        }

        target.clear();

        for (size_t i = range.begin; i < range.end; ++i)
            target.pushBack(source[i]);

        targetAttachments.reset(target.size());

        for (size_t i = range.begin; i < range.end; ++i)
        {
            const size_t localIndex = i - range.begin;
            const auto& attachment = sourceAttachments[i];

            if (attachment.type == OpenTypeGposAttachmentType::None)
                continue;

            if (attachment.parent < range.begin ||
                attachment.parent >= range.end)
            {
                continue;
            }

            targetAttachments[localIndex] = attachment;
            targetAttachments[localIndex].parent =
                attachment.parent - range.begin;
        }

        return true;
    }


    // ====================================================================
    // mergeOpenTypeGposRange
    //
    // GPOS must never alter shaping identity/provenance here.
    //
    // Copy placement results back to the original physical glyphs. New or
    // updated attachment relations are translated back to global indices.
    //
    // A local None relation does not erase an existing global relation. The
    // implemented Types 3-6 create/replace attachment relations; they do not
    // use contextual execution to explicitly detach glyphs.
    // ====================================================================

    static inline bool mergeOpenTypeGposRange(
        ShapedGlyphBuffer& target,
        OpenTypeGposAttachmentState& targetAttachments,
        const OpenTypeGposApplyRange& range,
        const ShapedGlyphBuffer& source,
        const OpenTypeGposAttachmentState& sourceAttachments)
    {
        if (!range.validFor(target.size()) ||
            source.size() != range.size() ||
            !targetAttachments.matches(target.size()) ||
            !sourceAttachments.matches(source.size()))
        {
            return false;
        }

        for (size_t i = 0; i < source.size(); ++i)
        {
            const size_t globalIndex = range.begin + i;

            // GPOS positioning must never mutate shaping identity.

            if (target[globalIndex].shaping.glyphId !=
                source[i].shaping.glyphId ||
                target[globalIndex].shaping.scalarOffset !=
                source[i].shaping.scalarOffset ||
                target[globalIndex].shaping.scalarCount !=
                source[i].shaping.scalarCount)
            {
                return false;
            }

            target[globalIndex].placement =
                source[i].placement;

            const auto& attachment =
                sourceAttachments[i];

            if (attachment.type ==
                OpenTypeGposAttachmentType::None)
            {
                continue;
            }

            if (attachment.parent >= source.size())
                return false;

            targetAttachments[globalIndex] =
                attachment;

            targetAttachments[globalIndex].parent =
                range.begin + attachment.parent;
        }

        return true;
    }


    // ====================================================================
    // applyOpenTypeGposLookupAt
    //
    // Central exact-position GPOS dispatcher.
    //
    // Contextual Types 7 and 8 reference LookupList entries using
    // SequenceLookupRecord. This dispatcher executes exactly one referenced
    // lookup at exactly one physical glyph position.
    //
    // range is the physical input span available to the nested lookup.
    // ====================================================================

    static inline OpenTypeGposApplyAtResult applyOpenTypeGposContextAt(
        const OpenTypeLayoutLookupListView& lookups, const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments, OpenTypeGposApplyState& state,
        bool runRightToLeft);

    static inline OpenTypeGposApplyAtResult applyOpenTypeGposChainContextAt(
        const OpenTypeLayoutLookupListView& lookups, const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments, OpenTypeGposApplyState& state,
        bool runRightToLeft);

    static inline OpenTypeGposApplyAtResult applyOpenTypeGposLookupAt(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer,
        size_t glyphIndex, OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state, bool runRightToLeft,
        const OpenTypeGposApplyRange& range)
    {
        if (!lookups || lookupIndex >= lookups.size() ||
            glyphIndex >= buffer.size() ||
            !attachments.matches(buffer.size()) ||
            !range.validFor(buffer.size()))
        {
            return OpenTypeGposApplyAtResult::Invalid;
        }

        if (!range.contains(glyphIndex))
            return OpenTypeGposApplyAtResult::NoMatch;

        const OpenTypeLayoutLookupView lookup =
            lookups.lookup(lookupIndex);

        if (!lookup)
            return OpenTypeGposApplyAtResult::Invalid;

        uint16_t effectiveType = 0;

        if (!openTypeGposEffectiveLookupType(lookup, effectiveType))
            return OpenTypeGposApplyAtResult::Invalid;

        OpenTypeGposApplyScope scope(state);

        if (!scope || !state.consumeOperation())
            return OpenTypeGposApplyAtResult::Invalid;


        // ------------------------------------------------------------
        // Execute against only the contextual input span.
        //
        // This makes all existing Type 1-6 traversal automatically obey
        // the outer contextual boundary.
        // ------------------------------------------------------------

        ShapedGlyphBuffer localBuffer;
        OpenTypeGposAttachmentState localAttachments;

        if (!copyOpenTypeGposRange(
            buffer, attachments, range,
            localBuffer, localAttachments))
        {
            return OpenTypeGposApplyAtResult::Invalid;
        }

        const size_t localIndex = glyphIndex - range.begin;

        if (effectiveType == 7 || effectiveType == 8)
        {
            OpenTypeGposApplyAtResult nestedResult =
                OpenTypeGposApplyAtResult::Invalid;

            if (effectiveType == 7)
            {
                nestedResult = applyOpenTypeGposContextAt(
                    lookups, lookup, gdef, localBuffer, localIndex,
                    localAttachments, state, runRightToLeft);
            }
            else
            {
                nestedResult = applyOpenTypeGposChainContextAt(
                    lookups, lookup, gdef, localBuffer, localIndex,
                    localAttachments, state, runRightToLeft);
            }

            if (nestedResult == OpenTypeGposApplyAtResult::Invalid)
                return OpenTypeGposApplyAtResult::Invalid;

            if (nestedResult == OpenTypeGposApplyAtResult::NoMatch)
                return OpenTypeGposApplyAtResult::NoMatch;

            if (!mergeOpenTypeGposRange(
                buffer, attachments, range,
                localBuffer, localAttachments))
            {
                return OpenTypeGposApplyAtResult::Invalid;
            }

            return OpenTypeGposApplyAtResult::Match;
        }

        OpenTypeGposResolveResult result = OpenTypeGposResolveResult::Invalid;


        switch (effectiveType)
        {
        case 1:
            result = applyOpenTypeGposSingleLookupAt(
                lookup, localBuffer, localIndex);
            break;

        case 2:
            result = applyOpenTypeGposPairLookupAt(
                lookup, gdef, localBuffer, localIndex);
            break;

        case 3:
            result = applyOpenTypeGposCursiveLookupAt(
                lookup, gdef, localBuffer, localIndex,
                runRightToLeft, localAttachments);
            break;

        case 4:
            result = applyOpenTypeGposMarkBaseLookupAt(
                lookup, gdef, localBuffer, localIndex,
                localAttachments);
            break;

        case 5:
            result = applyOpenTypeGposMarkLigatureLookupAt(
                lookup, gdef, localBuffer, localIndex,
                localAttachments);
            break;

        case 6:
            result = applyOpenTypeGposMarkMarkLookupAt(
                lookup, gdef, localBuffer, localIndex,
                localAttachments);
            break;

        default:
            return OpenTypeGposApplyAtResult::Invalid;
        }

        if (result == OpenTypeGposResolveResult::Invalid)
            return OpenTypeGposApplyAtResult::Invalid;

        if (result == OpenTypeGposResolveResult::NoMatch)
            return OpenTypeGposApplyAtResult::NoMatch;

        if (!mergeOpenTypeGposRange(
            buffer, attachments, range,
            localBuffer, localAttachments))
        {
            return OpenTypeGposApplyAtResult::Invalid;
        }

        return OpenTypeGposApplyAtResult::Match;
    }


    // ====================================================================
// applyOpenTypeGposContextAt
//
// Match and execute one native LookupType 7 at one physical position.
//
// GPOS never changes glyph-buffer topology, therefore the match.positions
// array remains valid for the entire execution.
// ====================================================================

    static inline OpenTypeGposApplyAtResult applyOpenTypeGposContextAt(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state,
        bool runRightToLeft)
    {
        if (!lookups ||
            !openTypeGposHasEffectiveLookupType(lookup, 7) ||
            glyphIndex >= buffer.size() ||
            !attachments.matches(buffer.size()))
        {
            return OpenTypeGposApplyAtResult::Invalid;
        }

        const OpenTypeLookupGlyphFilter filter(
            lookup, gdef);

        if (!filter)
            return OpenTypeGposApplyAtResult::Invalid;

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposApplyAtResult::Invalid;


        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan subtableData =
                openTypeGposEffectiveSubtable(lookup, 7, subtableIndex);

            if (!subtableData)
                return OpenTypeGposApplyAtResult::Invalid;

            const OpenTypeGposContextPosView pos(
                subtableData);

            if (!pos)
                return OpenTypeGposApplyAtResult::Invalid;

            OpenTypeGposContextMatch match;

            const OpenTypeGposContextMatchResult matchResult =
                matchOpenTypeGposContextPos(
                    pos, filter, buffer,
                    glyphIndex, match);

            if (matchResult ==
                OpenTypeGposContextMatchResult::Invalid)
            {
                return OpenTypeGposApplyAtResult::Invalid;
            }

            if (matchResult ==
                OpenTypeGposContextMatchResult::NoMatch)
            {
                continue;
            }

            if (match.positions.empty() ||
                match.positions[0] != glyphIndex)
            {
                return OpenTypeGposApplyAtResult::Invalid;
            }


            // ------------------------------------------------------------
            // Stable physical input range.
            //
            // Ignored glyphs between matched members are deliberately
            // included in this physical range.
            // ------------------------------------------------------------

            const size_t rangeEnd =
                match.positions.back() + 1;

            if (rangeEnd <= match.positions.back() ||
                rangeEnd > buffer.size())
            {
                return OpenTypeGposApplyAtResult::Invalid;
            }

            const OpenTypeGposApplyRange range{
                match.positions.front(),
                rangeEnd
            };


            // ------------------------------------------------------------
            // Execute SequenceLookupRecords in design order.
            //
            // Unlike GSUB, sequenceIndex is stable because GPOS does not
            // insert or delete glyphs.
            //
            // An out-of-range sequenceIndex is ignored.
            // ------------------------------------------------------------

            for (const OpenTypeSequenceLookup& action :
                match.lookups)
            {
                if (action.sequenceIndex >=
                    match.positions.size())
                {
                    continue;
                }

                if (action.lookupListIndex >=
                    lookups.size())
                {
                    return OpenTypeGposApplyAtResult::Invalid;
                }

                const size_t targetPosition =
                    match.positions[
                        action.sequenceIndex];

                const OpenTypeGposApplyAtResult nestedResult =
                    applyOpenTypeGposLookupAt(
                        lookups,
                        action.lookupListIndex,
                        gdef,
                        buffer,
                        targetPosition,
                        attachments,
                        state,
                        runRightToLeft,
                        range);

                if (nestedResult ==
                    OpenTypeGposApplyAtResult::Invalid)
                {
                    return OpenTypeGposApplyAtResult::Invalid;
                }

                // Nested NoMatch is legal.
            }


            // ------------------------------------------------------------
            // The context rule itself matched even if all referenced
            // positioning lookups returned NoMatch.
            // ------------------------------------------------------------

            return OpenTypeGposApplyAtResult::Match;
        }

        return OpenTypeGposApplyAtResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposContextLookup
    //
    // Apply one complete LookupType 7 transactionally.
    //
    // Since GPOS has stable topology, scan one physical glyph at a time.
    // ====================================================================

    static inline bool applyOpenTypeGposContextLookup(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        OpenTypeGposAttachmentState& attachments,
        OpenTypeGposApplyState& state,
        bool runRightToLeft)
    {
        if (!lookups ||
            lookupIndex >= lookups.size() ||
            !attachments.matches(buffer.size()))
        {
            return false;
        }

        const OpenTypeLayoutLookupView lookup =
            lookups.lookup(lookupIndex);

        if (!openTypeGposHasEffectiveLookupType(lookup, 7))
        {
            return false;
        }

        ShapedGlyphBuffer working =
            buffer;

        OpenTypeGposAttachmentState workingAttachments =
            attachments;

        for (size_t glyphIndex = 0;
            glyphIndex < working.size();
            ++glyphIndex)
        {
            OpenTypeGposApplyScope scope(state);

            if (!scope ||
                !state.consumeOperation())
            {
                return false;
            }

            const OpenTypeGposApplyAtResult result =
                applyOpenTypeGposContextAt(
                    lookups, lookup, gdef,
                    working, glyphIndex,
                    workingAttachments,
                    state, runRightToLeft);

            if (result ==
                OpenTypeGposApplyAtResult::Invalid)
            {
                return false;
            }
        }

        buffer = std::move(working);
        attachments =
            std::move(workingAttachments);

        return true;
    }


    // ====================================================================
    // Standalone convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGposContextLookup(
        const OpenTypeLayoutLookupListView& lookups,
        uint16_t lookupIndex,
        const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working =
            buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        OpenTypeGposApplyState state;

        if (!applyOpenTypeGposContextLookup(
            lookups, lookupIndex, gdef,
            working, attachments,
            state, runRightToLeft))
        {
            return false;
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


    // ====================================================================
// applyOpenTypeGposChainContextAt
//
// Match and execute one native LookupType 8 at one physical start
// position.
//
// Backtrack and lookahead are match-only constraints. Nested positioning
// is restricted to the physical span occupied by the matched input
// sequence.
//
// GPOS never changes glyph-buffer topology, so inputPositions remain
// stable while SequenceLookup records execute.
// ====================================================================

    static inline OpenTypeGposApplyAtResult applyOpenTypeGposChainContextAt(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        ShapedGlyphBuffer& buffer, size_t glyphIndex,
        OpenTypeGposAttachmentState& attachments, OpenTypeGposApplyState& state,
        bool runRightToLeft)
    {
        if (!lookups || !openTypeGposHasEffectiveLookupType(lookup, 8) ||
            glyphIndex >= buffer.size() || !attachments.matches(buffer.size()))
        {
            return OpenTypeGposApplyAtResult::Invalid;
        }

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGposApplyAtResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return OpenTypeGposApplyAtResult::Invalid;


        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan subtableData = openTypeGposEffectiveSubtable(lookup, 8, subtableIndex);

            if (!subtableData)
                return OpenTypeGposApplyAtResult::Invalid;

            const OpenTypeGposChainContextPosView pos(subtableData);

            if (!pos)
                return OpenTypeGposApplyAtResult::Invalid;

            OpenTypeGposChainContextMatch match;

            const OpenTypeGposChainContextMatchResult matchResult =
                matchOpenTypeGposChainContextPos(pos, filter, buffer, glyphIndex, match);

            if (matchResult == OpenTypeGposChainContextMatchResult::Invalid)
                return OpenTypeGposApplyAtResult::Invalid;

            if (matchResult == OpenTypeGposChainContextMatchResult::NoMatch)
                continue;

            if (match.inputPositions.empty() || match.inputPositions[0] != glyphIndex)
                return OpenTypeGposApplyAtResult::Invalid;

            if (match.inputPositions.back() == std::numeric_limits<size_t>::max())
                return OpenTypeGposApplyAtResult::Invalid;


            // ------------------------------------------------------------
            // Nested positioning is constrained to the INPUT sequence only.
            //
            // Backtrack and lookahead matched successfully, but they are not
            // part of the positioning workspace.
            //
            // Ignored glyphs physically between input members remain inside
            // the range and are subject to the nested lookup's own flags.
            // ------------------------------------------------------------

            const size_t rangeEnd = match.inputPositions.back() + 1;

            if (rangeEnd > buffer.size())
                return OpenTypeGposApplyAtResult::Invalid;

            const OpenTypeGposApplyRange range{
                match.inputPositions.front(),
                rangeEnd
            };


            // ------------------------------------------------------------
            // SequenceLookup records execute in stored/design order.
            //
            // GPOS topology is stable. sequenceIndex therefore indexes the
            // original inputPositions array directly.
            //
            // Oversized sequenceIndex is ignored.
            // ------------------------------------------------------------

            for (const OpenTypeSequenceLookup& action : match.lookups)
            {
                if (action.sequenceIndex >= match.inputPositions.size())
                    continue;

                if (action.lookupListIndex >= lookups.size())
                    return OpenTypeGposApplyAtResult::Invalid;

                const size_t targetPosition =
                    match.inputPositions[action.sequenceIndex];

                const OpenTypeGposApplyAtResult nestedResult =
                    applyOpenTypeGposLookupAt(
                        lookups, action.lookupListIndex, gdef,
                        buffer, targetPosition, attachments,
                        state, runRightToLeft, range);

                if (nestedResult == OpenTypeGposApplyAtResult::Invalid)
                    return OpenTypeGposApplyAtResult::Invalid;

                // Nested NoMatch is legal. Continue with later actions.
            }

            return OpenTypeGposApplyAtResult::Match;
        }

        return OpenTypeGposApplyAtResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGposChainContextLookup
    //
    // Apply one complete native LookupType 8 transactionally.
    // ====================================================================

    static inline bool applyOpenTypeGposChainContextLookup(
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

        if (!openTypeGposHasEffectiveLookupType(lookup, 8))
            return false;

        ShapedGlyphBuffer working = buffer;
        OpenTypeGposAttachmentState workingAttachments = attachments;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            OpenTypeGposApplyScope scope(state);

            if (!scope || !state.consumeOperation())
                return false;

            const OpenTypeGposApplyAtResult result =
                applyOpenTypeGposChainContextAt(
                    lookups, lookup, gdef, working, glyphIndex,
                    workingAttachments, state, runRightToLeft);

            if (result == OpenTypeGposApplyAtResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        attachments = std::move(workingAttachments);
        return true;
    }


    // ====================================================================
    // Standalone convenience overload.
    // ====================================================================

    static inline bool applyOpenTypeGposChainContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, ShapedGlyphBuffer& buffer,
        bool runRightToLeft)
    {
        ShapedGlyphBuffer working = buffer;

        OpenTypeGposAttachmentState attachments;
        attachments.reset(working.size());

        OpenTypeGposApplyState state;

        if (!applyOpenTypeGposChainContextLookup(
            lookups, lookupIndex, gdef, working,
            attachments, state, runRightToLeft))
        {
            return false;
        }

        if (!resolveOpenTypeGposAttachments(
            working, attachments, runRightToLeft))
        {
            return false;
        }

        buffer = std::move(working);
        return true;
    }

} // namespace waavs