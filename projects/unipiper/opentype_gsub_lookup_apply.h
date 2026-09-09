// opentype_gsub_lookup_apply.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>
#include <algorithm>


#include "opentype_gsub_single_view.h"
#include "opentype_gsub_multiple_view.h"
#include "opentype_gsub_ligature_view.h"
#include "opentype_gsub_alternate_view.h"
#include "opentype_gsub_extension_view.h"
#include "opentype_gsub_context_match.h"
#include "opentype_gsub_chain_context_match.h"
#include "opentype_gsub_reverse_chain_single_match.h"
#include "opentype_gsub_edit.h"
#include "opentype_gsub_sequence_state.h"
#include "opentype_gsub_apply_state.h"

#include "opentype_layout_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_shaping_buffer.h"


namespace waavs
{
    enum class OpenTypeGsubResolveResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGsubLigatureMatch
    //
    // Exact physical buffer positions that participated in one ligature
    // match. LookupFlag filtering can make these positions non-contiguous.
    // ====================================================================

    struct OpenTypeGsubLigatureMatch
    {
        uint16_t ligatureGlyph{ 0 };
        std::vector<size_t> positions{};

        void clear() noexcept
        {
            ligatureGlyph = 0;
            positions.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return positions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return positions.size(); }
    };


    // ====================================================================
// openTypeGsubEffectiveLookupType
//
// Normal GSUB lookup:
//
//   LookupType 1 -> effective type 1
//
// Extension lookup:
//
//   LookupType 7
//       -> ExtensionSubst
//              -> extensionLookupType 1
//
// All ExtensionSubst records in one Type 7 Lookup must specify the same
// underlying lookup type.
// ====================================================================

    static inline bool openTypeGsubEffectiveLookupType(
        const OpenTypeLayoutLookupView& lookup, uint16_t& result) noexcept
    {
        result = 0;

        if (!lookup)
            return false;

        const uint16_t lookupType = lookup.lookupType();

        if (lookupType != 7)
        {
            if (lookupType == 0)
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

            const OpenTypeGsubExtensionSubstView extension(data);

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


    static inline bool openTypeGsubHasEffectiveLookupType(
        const OpenTypeLayoutLookupView& lookup, uint16_t expectedType) noexcept
    {
        uint16_t lookupType = 0;
        return openTypeGsubEffectiveLookupType(lookup, lookupType) &&
            lookupType == expectedType;
    }


    // ====================================================================
    // openTypeGsubEffectiveSubtable
    //
    // Return the actual substitution subtable.
    //
    // Normal lookup:
    //
    //   Lookup -> subtable
    //
    // Extension lookup:
    //
    //   Lookup -> ExtensionSubst -> actual subtable
    // ====================================================================

    static inline ByteSpan openTypeGsubEffectiveSubtable(
        const OpenTypeLayoutLookupView& lookup, uint16_t effectiveType,
        uint16_t subtableIndex) noexcept
    {
        if (!lookup || subtableIndex >= lookup.subtableCount())
            return {};

        const ByteSpan data = lookup.subtable(subtableIndex);

        if (!data)
            return {};


        if (lookup.lookupType() != 7)
        {
            if (lookup.lookupType() != effectiveType)
                return {};

            return data;
        }


        const OpenTypeGsubExtensionSubstView extension(data);

        if (!extension || extension.extensionLookupType() != effectiveType)
            return {};

        return extension.extensionSubtable();
    }




    // ====================================================================
    // resolveOpenTypeGsubSingleLookup
    //
    // Resolve one glyph against one GSUB LookupType 1 Lookup.
    //
    // Subtables are tried in stored order. The first matching subtable
    // finishes this lookup for the glyph.
    //
    // The glyph is not modified here.
    // ====================================================================

    static inline OpenTypeGsubResolveResult resolveOpenTypeGsubSingleLookup(
        const OpenTypeLayoutLookupView& lookup, uint32_t glyphId, uint16_t& replacement) noexcept
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 1) || glyphId > 0xFFFFu)
            return OpenTypeGsubResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan subtableData = openTypeGsubEffectiveSubtable(lookup, 1, i);

            if (!subtableData)
                return OpenTypeGsubResolveResult::Invalid;

            const OpenTypeGsubSingleSubstView single(subtableData);

            if (!single)
                return OpenTypeGsubResolveResult::Invalid;

            const OpenTypeCoverageView coverage = single.coverage();

            if (!coverage)
                return OpenTypeGsubResolveResult::Invalid;

            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;


            // ------------------------------------------------------------
            // Format 1
            // ------------------------------------------------------------

            if (single.format() == 1)
            {
                int32_t delta = 0;

                if (!single.deltaGlyphId(delta))
                    return OpenTypeGsubResolveResult::Invalid;

                replacement = static_cast<uint16_t>(
                    static_cast<uint32_t>(glyphId) +
                    static_cast<uint32_t>(static_cast<uint16_t>(delta)));

                return OpenTypeGsubResolveResult::Match;
            }


            // ------------------------------------------------------------
            // Format 2
            // ------------------------------------------------------------

            if (single.format() == 2)
            {
                if (!single.substituteGlyphId(coverageIndex, replacement))
                    return OpenTypeGsubResolveResult::Invalid;

                return OpenTypeGsubResolveResult::Match;
            }

            return OpenTypeGsubResolveResult::Invalid;
        }

        return OpenTypeGsubResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGsubOneToOne
    //
    // Shared mutation primitive for SingleSubst and AlternateSubst.
    // Provenance remains unchanged.
    // ====================================================================

    static inline bool applyOpenTypeGsubOneToOne(
        OpenTypeShapingBuffer& buffer, size_t glyphIndex, uint16_t replacement) noexcept
    {
        if (glyphIndex >= buffer.size())
            return false;

        buffer[glyphIndex].glyphId = replacement;
        return true;
    }


    // ====================================================================
    // applyOpenTypeGsubSingleLookup
    //
    // Apply one complete LookupType 1 Lookup to the shaping buffer.
    //
    // LookupFlags are deliberately not implemented yet for whole-buffer
    // Type 1 scanning. A non-zero flag is rejected rather than silently
    // producing incorrect shaping.
    // ====================================================================

    static inline bool applyOpenTypeGsubSingleLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer) noexcept
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 1))
            return false;

        if (lookup.lookupFlag() != 0)
            return false;

        // Preflight so malformed data cannot leave a partially changed buffer.

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            uint16_t replacement = 0;
            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubSingleLookup(lookup, buffer[i].glyphId, replacement);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;
        }

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            uint16_t replacement = 0;
            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubSingleLookup(lookup, buffer[i].glyphId, replacement);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::Match &&
                !applyOpenTypeGsubOneToOne(buffer, i, replacement))
            {
                return false;
            }
        }

        return true;
    }


    // ====================================================================
// resolveOpenTypeGsubMultipleLookup
//
// Resolve one glyph against one GSUB LookupType 2 Lookup.
//
// Subtables are tried in stored order. The first matching subtable
// finishes this lookup for the glyph.
//
// The shaping buffer is not modified here.
// ====================================================================

    static inline OpenTypeGsubResolveResult resolveOpenTypeGsubMultipleLookup(
        const OpenTypeLayoutLookupView& lookup, uint32_t glyphId,
        OpenTypeGsubMultipleSequenceView& sequence) noexcept
    {
        sequence = {};

        if (!openTypeGsubHasEffectiveLookupType(lookup, 2) || glyphId > 0xFFFFu)
            return OpenTypeGsubResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan subtableData = openTypeGsubEffectiveSubtable(lookup, 2, i);

            if (!subtableData)
                return OpenTypeGsubResolveResult::Invalid;

            const OpenTypeGsubMultipleSubstView multiple(subtableData);

            if (!multiple)
                return OpenTypeGsubResolveResult::Invalid;

            const OpenTypeCoverageView coverage = multiple.coverage();

            if (!coverage)
                return OpenTypeGsubResolveResult::Invalid;

            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            sequence = multiple.sequence(coverageIndex);

            if (!sequence)
                return OpenTypeGsubResolveResult::Invalid;

            return OpenTypeGsubResolveResult::Match;
        }

        return OpenTypeGsubResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGsubMultipleSequence
    //
    // Apply one already-resolved MultipleSubst sequence at one physical
    // glyph position. Every output inherits the source glyph provenance.
    // ====================================================================

    static inline bool applyOpenTypeGsubMultipleSequence(
        OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        const OpenTypeGsubMultipleSequenceView& sequence)
    {
        if (!sequence || glyphIndex >= buffer.size())
            return false;

        const uint16_t replacementCount = sequence.glyphCount();

        if (replacementCount == 0)
            return false;

        // Decode all replacement glyph IDs before changing the buffer.

        std::vector<uint16_t> replacements(replacementCount);

        for (uint16_t i = 0; i < replacementCount; ++i)
        {
            if (!sequence.glyphId(i, replacements[i]))
                return false;
        }

        const OpenTypeShapingGlyph source = buffer[glyphIndex];

        if (replacementCount > 1)
        {
            std::vector<OpenTypeShapingGlyph>& glyphs = buffer.glyphs();
            glyphs.insert(glyphs.begin() + glyphIndex + 1,
                size_t(replacementCount - 1), source);
        }

        for (uint16_t i = 0; i < replacementCount; ++i)
        {
            OpenTypeShapingGlyph& glyph = buffer[glyphIndex + i];
            glyph.glyphId = replacements[i];
            glyph.scalarOffset = source.scalarOffset;
            glyph.scalarCount = source.scalarCount;
        }

        return true;
    }


    // ====================================================================
    // applyOpenTypeGsubMultipleLookup
    //
    // Apply one complete LookupType 2 Lookup to the shaping buffer.
    // Newly-created output glyphs are skipped for this lookup.
    // ====================================================================

    static inline bool applyOpenTypeGsubMultipleLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 2))
            return false;

        if (lookup.lookupFlag() != 0)
            return false;

        // Preflight the original stream. Newly emitted glyphs are not fed
        // back through this same lookup.

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            OpenTypeGsubMultipleSequenceView sequence;
            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubMultipleLookup(lookup, buffer[i].glyphId, sequence);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;
        }

        size_t glyphIndex = 0;

        while (glyphIndex < buffer.size())
        {
            OpenTypeGsubMultipleSequenceView sequence;
            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubMultipleLookup(lookup, buffer[glyphIndex].glyphId, sequence);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            const uint16_t replacementCount = sequence.glyphCount();

            if (!applyOpenTypeGsubMultipleSequence(buffer, glyphIndex, sequence))
                return false;

            glyphIndex += replacementCount;
        }

        return true;
    }


    // ====================================================================
    // resolveOpenTypeGsubLigatureLookup
    //
    // Resolve one glyph-buffer position against one effective GSUB
    // LookupType 4.
    //
    // The current glyph is matched normally against the first ligature
    // component through Coverage. LookupFlag filtering is used only while
    // walking the remaining components.
    //
    // Ligature records are tested in stored order. The first matching
    // Ligature wins.
    //
    // match.positions contains the exact physical buffer positions that
    // participated in the ligature. Ignored glyphs are not included.
    // ====================================================================

    static inline OpenTypeGsubResolveResult resolveOpenTypeGsubLigatureLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeLookupGlyphFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubLigatureMatch& match) noexcept
    {
        match.clear();

        if (!openTypeGsubHasEffectiveLookupType(lookup, 4) || !filter || glyphIndex >= buffer.size())
            return OpenTypeGsubResolveResult::Invalid;

        const uint32_t firstGlyphId = buffer[glyphIndex].glyphId;

        if (firstGlyphId > 0xFFFFu)
            return OpenTypeGsubResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan subtableData = openTypeGsubEffectiveSubtable(lookup, 4, subtableIndex);

            if (!subtableData)
                return OpenTypeGsubResolveResult::Invalid;

            const OpenTypeGsubLigatureSubstView subst(subtableData);

            if (!subst)
                return OpenTypeGsubResolveResult::Invalid;


            // ------------------------------------------------------------
            // First component selects the LigatureSet through Coverage.
            // ------------------------------------------------------------

            const OpenTypeCoverageView coverage = subst.coverage();

            if (!coverage)
                return OpenTypeGsubResolveResult::Invalid;

            uint16_t coverageIndex = 0;

            if (!coverage.find(firstGlyphId, coverageIndex))
                continue;

            const OpenTypeGsubLigatureSetView set = subst.ligatureSet(coverageIndex);

            if (!set)
                return OpenTypeGsubResolveResult::Invalid;


            // ------------------------------------------------------------
            // Ligature order is significant. Try candidates exactly in the
            // order stored by the font.
            // ------------------------------------------------------------

            for (uint16_t ligatureIndex = 0; ligatureIndex < set.size(); ++ligatureIndex)
            {
                const OpenTypeGsubLigatureView ligature = set.ligature(ligatureIndex);

                if (!ligature)
                    return OpenTypeGsubResolveResult::Invalid;

                const uint16_t componentCount = ligature.componentCount();

                if (componentCount < 2)
                    return OpenTypeGsubResolveResult::Invalid;

                // Even with filtering, every participating component needs a
                // distinct physical glyph position.

                if (size_t(componentCount) > buffer.size() - glyphIndex)
                    continue;

                match.clear();
                match.positions.reserve(componentCount);
                match.positions.push_back(glyphIndex);

                bool matches = true;
                size_t position = glyphIndex;

                // componentGlyphId(0) describes the second glyph. The first
                // glyph came from Coverage.

                for (uint16_t componentIndex = 1; componentIndex < componentCount; ++componentIndex)
                {
                    uint16_t expectedGlyphId = 0;

                    if (!ligature.componentGlyphId(componentIndex - 1, expectedGlyphId))
                        return OpenTypeGsubResolveResult::Invalid;

                    size_t nextPosition = 0;
                    const OpenTypeLookupGlyphSearchResult search = filter.next(buffer, position, nextPosition);

                    if (search == OpenTypeLookupGlyphSearchResult::Invalid)
                        return OpenTypeGsubResolveResult::Invalid;

                    if (search == OpenTypeLookupGlyphSearchResult::End)
                    {
                        matches = false;
                        break;
                    }

                    const uint32_t actualGlyphId = buffer[nextPosition].glyphId;

                    if (actualGlyphId > 0xFFFFu)
                        return OpenTypeGsubResolveResult::Invalid;

                    if (actualGlyphId != expectedGlyphId)
                    {
                        matches = false;
                        break;
                    }

                    match.positions.push_back(nextPosition);
                    position = nextPosition;
                }

                if (!matches)
                    continue;

                if (match.positions.size() != componentCount)
                    return OpenTypeGsubResolveResult::Invalid;

                match.ligatureGlyph = ligature.ligatureGlyph();
                return OpenTypeGsubResolveResult::Match;
            }
        }

        match.clear();
        return OpenTypeGsubResolveResult::NoMatch;
    }


    // ====================================================================
    // Compatibility resolver for existing zero-filter callers/tests.
    //
    // matchedGlyphCount remains meaningful here because without GDEF-based
    // filtering the participating positions are contiguous.
    // ====================================================================

    static inline OpenTypeGsubResolveResult resolveOpenTypeGsubLigatureLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, uint16_t& ligatureGlyph, uint16_t& matchedGlyphCount) noexcept
    {
        ligatureGlyph = 0;
        matchedGlyphCount = 0;

        const OpenTypeGdefView gdef{};
        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubResolveResult::Invalid;

        OpenTypeGsubLigatureMatch match;
        const OpenTypeGsubResolveResult result =
            resolveOpenTypeGsubLigatureLookup(lookup, filter, buffer, glyphIndex, match);

        if (result != OpenTypeGsubResolveResult::Match)
            return result;

        if (match.size() > 0xFFFFu)
            return OpenTypeGsubResolveResult::Invalid;

        ligatureGlyph = match.ligatureGlyph;
        matchedGlyphCount = static_cast<uint16_t>(match.size());
        return OpenTypeGsubResolveResult::Match;
    }


    // ====================================================================
// openTypeGsubNextLigatureId
//
// Zero is reserved for "no ligature association".
//
// Scanning keeps allocation state out of OpenTypeShapingBuffer and makes
// copies of the buffer naturally preserve the allocation namespace.
// ====================================================================

    static inline uint32_t openTypeGsubNextLigatureId(
        const OpenTypeShapingBuffer& buffer) noexcept
    {
        uint32_t maximum = 0;

        for (const OpenTypeShapingGlyph& glyph : buffer)
        {
            if (glyph.ligature.id > maximum)
                maximum = glyph.ligature.id;
        }

        if (maximum == std::numeric_limits<uint32_t>::max())
            return 0;

        return maximum + 1;
    }


    static inline bool openTypeGsubGlyphIsMark(
        const OpenTypeGdefView& gdef, uint32_t glyphId,
        bool& result) noexcept
    {
        result = false;

        if (glyphId > 0xFFFFu)
            return false;

        // Without GDEF we cannot reliably identify ignored marks here.
        //
        // Type 5 still has its spec-defined fallback to the final ligature
        // component for marks without a matching ligature association.

        if (!gdef)
            return true;

        uint16_t glyphClass = 0;

        if (!gdef.glyphClass(glyphId, glyphClass))
            return false;

        result = glyphClass == 3;
        return true;
    }


    static inline uint16_t openTypeGsubLigatureComponentCount(
        const OpenTypeShapingGlyph& glyph) noexcept
    {
        return glyph.ligature.effectiveComponentCount();
    }





    // ====================================================================
    // applyOpenTypeGsubLigatureMatch
    //
    // Besides producing the ligature glyph, record the component association
    // required later by GPOS Type 5 Mark-to-Ligature.
    //
    // Ignored marks between participating glyphs survive physically and are
    // assigned to the preceding logical ligature component.
    // ====================================================================

    static inline bool applyOpenTypeGsubLigatureMatch(
        const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer,
        const OpenTypeGsubLigatureMatch& match)
    {
        if (match.size() < 2 || match.positions[0] >= buffer.size())
            return false;

        for (size_t i = 0; i < match.positions.size(); ++i)
        {
            if (match.positions[i] >= buffer.size())
                return false;

            if (i != 0 && match.positions[i] <= match.positions[i - 1])
                return false;
        }


        // ------------------------------------------------------------
        // Preserve ordinary scalar provenance exactly as before.
        // ------------------------------------------------------------

        const OpenTypeShapingGlyph& first =
            buffer[match.positions[0]];

        uint32_t scalarBegin = first.scalarOffset;
        uint64_t scalarEnd =
            uint64_t(first.scalarOffset) +
            uint64_t(first.scalarCount);

        for (size_t i = 1; i < match.positions.size(); ++i)
        {
            const OpenTypeShapingGlyph& component =
                buffer[match.positions[i]];

            if (component.scalarOffset < scalarBegin)
                scalarBegin = component.scalarOffset;

            const uint64_t componentEnd =
                uint64_t(component.scalarOffset) +
                uint64_t(component.scalarCount);

            if (componentEnd > scalarEnd)
                scalarEnd = componentEnd;
        }

        if (scalarEnd < scalarBegin ||
            scalarEnd - scalarBegin > 0xFFFFFFFFull)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Determine whether this is a ligature made entirely from marks.
        //
        // A mark ligature should retain an existing parent-ligature
        // association instead of becoming a new base ligature.
        // ------------------------------------------------------------

        bool allMarks = bool(gdef);

        if (allMarks)
        {
            for (size_t position : match.positions)
            {
                bool isMark = false;

                if (!openTypeGsubGlyphIsMark(
                    gdef, buffer[position].glyphId, isMark))
                {
                    return false;
                }

                if (!isMark)
                {
                    allMarks = false;
                    break;
                }
            }
        }


        OpenTypeLigatureProvenance outputLigature{};


        // ------------------------------------------------------------
        // Mark ligature.
        //
        // Preserve the common parent-ligature association if all components
        // already belong to the same component of the same ligature.
        // ------------------------------------------------------------

        if (allMarks)
        {
            outputLigature =
                buffer[match.positions[0]].ligature;

            outputLigature.componentCount = 0;

            for (size_t i = 1; i < match.positions.size(); ++i)
            {
                const OpenTypeLigatureProvenance& candidate =
                    buffer[match.positions[i]].ligature;

                if (candidate.id != outputLigature.id ||
                    candidate.component != outputLigature.component)
                {
                    outputLigature.clear();
                    break;
                }
            }
        }
        else
        {
            // ------------------------------------------------------------
            // Normal base ligature.
            // ------------------------------------------------------------

            const uint32_t ligatureId =
                openTypeGsubNextLigatureId(buffer);

            if (ligatureId == 0)
                return false;

            uint32_t totalComponentCount = 0;

            for (size_t position : match.positions)
            {
                totalComponentCount +=
                    openTypeGsubLigatureComponentCount(
                        buffer[position]);

                if (totalComponentCount >
                    std::numeric_limits<uint16_t>::max())
                {
                    return false;
                }
            }

            if (totalComponentCount < 2)
                return false;

            outputLigature.id = ligatureId;
            outputLigature.component = 0;
            outputLigature.componentCount =
                static_cast<uint16_t>(totalComponentCount);


            // ------------------------------------------------------------
            // Remap surviving marks between participating components.
            //
            // Example:
            //
            //   A mark B mark C
            //   ^      ^      ^
            //   1      2      3
            //
            // After A+B+C ligate:
            //
            //   ligature mark mark
            //
            // The surviving marks retain component 1 and 2 respectively.
            // ------------------------------------------------------------

            uint32_t componentsBefore = 0;

            for (size_t i = 0; i + 1 < match.positions.size(); ++i)
            {
                const size_t componentPosition =
                    match.positions[i];

                const size_t nextComponentPosition =
                    match.positions[i + 1];

                const OpenTypeShapingGlyph componentGlyph =
                    buffer[componentPosition];

                const uint16_t componentCount =
                    openTypeGsubLigatureComponentCount(
                        componentGlyph);

                for (size_t position = componentPosition + 1;
                    position < nextComponentPosition;
                    ++position)
                {
                    bool isMark = false;

                    if (!openTypeGsubGlyphIsMark(
                        gdef,
                        buffer[position].glyphId,
                        isMark))
                    {
                        return false;
                    }

                    if (!isMark)
                        continue;

                    uint16_t localComponent = componentCount;

                    const OpenTypeLigatureProvenance oldAssociation =
                        buffer[position].ligature;

                    if (componentGlyph.ligature.id != 0 &&
                        oldAssociation.id ==
                        componentGlyph.ligature.id &&
                        oldAssociation.component != 0)
                    {
                        localComponent =
                            std::min(
                                oldAssociation.component,
                                componentCount);
                    }

                    const uint32_t newComponent =
                        componentsBefore +
                        uint32_t(localComponent);

                    if (newComponent == 0 ||
                        newComponent >
                        std::numeric_limits<uint16_t>::max())
                    {
                        return false;
                    }

                    buffer[position].ligature.id =
                        ligatureId;

                    buffer[position].ligature.component =
                        static_cast<uint16_t>(
                            newComponent);

                    buffer[position].ligature.componentCount = 0;
                }

                componentsBefore += componentCount;
            }


            // ------------------------------------------------------------
            // If the final participating glyph was itself a ligature,
            // marks immediately following it may already be associated with
            // one of its internal components. Remap those associations into
            // the new ligature's component space.
            // ------------------------------------------------------------

            const size_t lastPosition =
                match.positions.back();

            const OpenTypeShapingGlyph lastComponent =
                buffer[lastPosition];

            const uint16_t lastComponentCount =
                openTypeGsubLigatureComponentCount(
                    lastComponent);

            if (lastComponent.ligature.id != 0)
            {
                const uint32_t componentBase =
                    totalComponentCount -
                    lastComponentCount;

                for (size_t position = lastPosition + 1;
                    position < buffer.size();
                    ++position)
                {
                    OpenTypeLigatureProvenance& association =
                        buffer[position].ligature;

                    if (association.id !=
                        lastComponent.ligature.id ||
                        association.component == 0)
                    {
                        break;
                    }

                    const uint16_t localComponent =
                        std::min(
                            association.component,
                            lastComponentCount);

                    const uint32_t newComponent =
                        componentBase +
                        uint32_t(localComponent);

                    if (newComponent == 0 ||
                        newComponent >
                        std::numeric_limits<uint16_t>::max())
                    {
                        return false;
                    }

                    association.id = ligatureId;
                    association.component =
                        static_cast<uint16_t>(
                            newComponent);

                    association.componentCount = 0;
                }
            }
        }


        // ------------------------------------------------------------
        // Produce the ligature glyph.
        // ------------------------------------------------------------

        OpenTypeShapingGlyph& output =
            buffer[match.positions[0]];

        output.glyphId = match.ligatureGlyph;
        output.scalarOffset = scalarBegin;
        output.scalarCount =
            static_cast<uint32_t>(
                scalarEnd - scalarBegin);

        output.ligature = outputLigature;


        // ------------------------------------------------------------
        // Remove only participating components.
        //
        // Ignored marks remain in the buffer.
        // ------------------------------------------------------------

        std::vector<OpenTypeShapingGlyph>& glyphs =
            buffer.glyphs();

        for (size_t i = match.positions.size();
            i > 1;
            --i)
        {
            glyphs.erase(
                glyphs.begin() +
                match.positions[i - 1]);
        }

        return true;
    }


    // ====================================================================
    // Compatibility overload.
    // ====================================================================

    static inline bool applyOpenTypeGsubLigatureMatch( OpenTypeShapingBuffer& buffer, const OpenTypeGsubLigatureMatch& match)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGsubLigatureMatch(
            gdef, buffer, match);
    }


    // ====================================================================
    // applyOpenTypeGsubLigatureLookup
    //
    // Apply one complete effective GSUB LookupType 4 using LookupFlag/GDEF
    // filtering. The complete lookup remains transactional.
    // ====================================================================

    static inline bool applyOpenTypeGsubLigatureLookup( const OpenTypeLayoutLookupView& lookup, 
        const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 4))
            return false;

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return false;

        OpenTypeShapingBuffer working = buffer;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGsubLigatureMatch match;
            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubLigatureLookup(lookup, filter, working, glyphIndex, match);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            if (match.positions.empty() || match.positions[0] != glyphIndex)
                return false;

            if (!applyOpenTypeGsubLigatureMatch(gdef, working, match))
                return false;

            // Do not feed the newly-created ligature back through this same
            // lookup. An ignored glyph immediately after it remains eligible
            // as the next physical start position.

            ++glyphIndex;
        }

        buffer = std::move(working);
        return true;
    }


    // ====================================================================
    // Convenience overload for existing callers.
    //
    // Zero filtering flags require no GDEF data. Lookups that require GDEF
    // filtering fail cleanly when called through this overload.
    // ====================================================================

    static inline bool applyOpenTypeGsubLigatureLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGsubLigatureLookup(lookup, gdef, buffer);
    }


    // ====================================================================
// resolveOpenTypeGsubAlternateLookup
//
// Resolve one glyph against one complete GSUB LookupType 3.
//
// Subtables are tried in stored order. For now, when an AlternateSet
// contains multiple choices, alternate index 0 is selected.
//
// The view remains policy-neutral; choosing alternate 0 here is only
// the default shaping policy and can later be replaced by an explicit
// alternate-selection policy.
    // ====================================================================

    static inline OpenTypeGsubResolveResult resolveOpenTypeGsubAlternateLookup(
        const OpenTypeLayoutLookupView& lookup, uint32_t glyphId,
        uint16_t& replacementGlyph) noexcept
    {
        replacementGlyph = 0;

        if (!openTypeGsubHasEffectiveLookupType(lookup, 3) || glyphId > 0xFFFFu)
            return OpenTypeGsubResolveResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan subtableData = openTypeGsubEffectiveSubtable(lookup, 3, subtableIndex);

            if (!subtableData)
                return OpenTypeGsubResolveResult::Invalid;

            const OpenTypeGsubAlternateSubstView subst(subtableData);

            if (!subst)
                return OpenTypeGsubResolveResult::Invalid;


            // ------------------------------------------------------------
            // Coverage maps the input glyph to its AlternateSet.
            // ------------------------------------------------------------

            const OpenTypeCoverageView coverage = subst.coverage();

            if (!coverage)
                return OpenTypeGsubResolveResult::Invalid;

            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;


            const OpenTypeGsubAlternateSetView set =
                subst.alternateSet(coverageIndex);

            if (!set)
                return OpenTypeGsubResolveResult::Invalid;


            // ------------------------------------------------------------
            // An empty AlternateSet is structurally readable, but cannot
            // produce a substitution. Continue trying later subtables.
            // ------------------------------------------------------------

            if (set.glyphCount() == 0)
                continue;


            // ------------------------------------------------------------
            // Default alternate-selection policy:
            //
            // choose the first alternate.
            //
            // A later policy layer can replace this with an explicitly
            // selected alternate index.
            // ------------------------------------------------------------

            if (!set.glyphId(0, replacementGlyph))
                return OpenTypeGsubResolveResult::Invalid;

            return OpenTypeGsubResolveResult::Match;
        }

        return OpenTypeGsubResolveResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGsubAlternateLookup
    //
    // Apply one complete GSUB LookupType 3 Lookup to the shaping buffer.
    // Alternate 0 remains the default selection policy.
    // ====================================================================

    static inline bool applyOpenTypeGsubAlternateLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 3))
            return false;

        if (lookup.lookupFlag() != 0)
            return false;

        OpenTypeShapingBuffer working = buffer;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            uint16_t replacementGlyph = 0;
            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubAlternateLookup(
                    lookup, working[glyphIndex].glyphId, replacementGlyph);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::Match &&
                !applyOpenTypeGsubOneToOne(working, glyphIndex, replacementGlyph))
            {
                return false;
            }
        }

        buffer = std::move(working);
        return true;
    }


    // ====================================================================
// applyOpenTypeGsubReverseChainSingleAt
//
// Match and execute one native LookupType 8 at one physical position.
//
// This exact-position primitive has no reverse scanning responsibility.
// Reverse traversal belongs to the complete LookupType 8 executor.
//
// Type 8 is always one-to-one, so provenance remains unchanged and the
// edit records one consumed position and one output glyph.
// ====================================================================

    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubReverseChainSingleAt(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex, OpenTypeGsubEditLog& edits)
    {
        if (!lookup || !openTypeGsubHasEffectiveLookupType(lookup, 8) || glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubApplyAtResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan subtableData =
                openTypeGsubEffectiveSubtable(lookup, 8, subtableIndex);

            if (!subtableData)
                return OpenTypeGsubApplyAtResult::Invalid;

            const OpenTypeGsubReverseChainSingleSubstView subst(subtableData);

            if (!subst)
                return OpenTypeGsubApplyAtResult::Invalid;

            OpenTypeGsubReverseChainSingleMatch match;

            const OpenTypeGsubReverseChainSingleMatchResult matchResult =
                matchOpenTypeGsubReverseChainSingleSubst(
                    subst, filter, buffer, glyphIndex, match);

            if (matchResult == OpenTypeGsubReverseChainSingleMatchResult::Invalid)
                return OpenTypeGsubApplyAtResult::Invalid;

            if (matchResult == OpenTypeGsubReverseChainSingleMatchResult::NoMatch)
                continue;

            if (!applyOpenTypeGsubOneToOne(
                buffer, glyphIndex, match.substituteGlyph))
            {
                return OpenTypeGsubApplyAtResult::Invalid;
            }

            OpenTypeGsubEdit edit;
            edit.inputPositions.push_back(glyphIndex);
            edit.outputCount = 1;
            edits.push_back(std::move(edit));

            return OpenTypeGsubApplyAtResult::Match;
        }

        return OpenTypeGsubApplyAtResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGsubReverseChainSingleLookup
    //
    // Apply one complete effective LookupType 8 transactionally.    
    // //
    // Type 8 is unique among GSUB lookups: candidate glyph positions are
    // visited from the logical end of the shaping buffer toward the start.
    //
    // Every substitution is one-to-one, so physical indices remain stable
    // throughout the reverse scan.
    // ====================================================================

    static inline bool applyOpenTypeGsubReverseChainSingleLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        if (!lookup || !openTypeGsubHasEffectiveLookupType(lookup, 8))
            return false;

        OpenTypeShapingBuffer working = buffer;

        for (size_t position = working.size(); position > 0; --position)
        {
            const size_t glyphIndex = position - 1;
            OpenTypeGsubEditLog edits;

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubReverseChainSingleAt(
                    lookup, gdef, working, glyphIndex, edits);

            if (result == OpenTypeGsubApplyAtResult::Invalid)
                return false;
        }

        buffer = std::move(working);
        return true;
    }




    // ====================================================================
    // Nested / at-position GSUB execution
    //
    // Contextual substitutions reference LookupList entries by index. The
    // at-position API therefore always receives the parent LookupList.
    //
    // OpenTypeGsubEditLog is append-only here. Every edit uses physical
    // coordinates immediately before that edit was performed.
    // ====================================================================

    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubLookupAt(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubApplyState& state,
        OpenTypeGsubEditLog& edits);


    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubSingleAt(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubEditLog& edits)
    {
        if (glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        uint16_t replacement = 0;
        const OpenTypeGsubResolveResult result =
            resolveOpenTypeGsubSingleLookup(lookup, buffer[glyphIndex].glyphId, replacement);

        if (result == OpenTypeGsubResolveResult::Invalid)
            return OpenTypeGsubApplyAtResult::Invalid;

        if (result == OpenTypeGsubResolveResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (!applyOpenTypeGsubOneToOne(buffer, glyphIndex, replacement))
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = 1;
        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }


    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubMultipleAt(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubEditLog& edits)
    {
        if (glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubMultipleSequenceView sequence;
        const OpenTypeGsubResolveResult result =
            resolveOpenTypeGsubMultipleLookup(lookup, buffer[glyphIndex].glyphId, sequence);

        if (result == OpenTypeGsubResolveResult::Invalid)
            return OpenTypeGsubApplyAtResult::Invalid;

        if (result == OpenTypeGsubResolveResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        const size_t outputCount = sequence.glyphCount();

        if (outputCount == 0 || !applyOpenTypeGsubMultipleSequence(buffer, glyphIndex, sequence))
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = outputCount;
        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }


    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubAlternateAt(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubEditLog& edits)
    {
        if (glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        uint16_t replacement = 0;
        const OpenTypeGsubResolveResult result =
            resolveOpenTypeGsubAlternateLookup(lookup, buffer[glyphIndex].glyphId, replacement);

        if (result == OpenTypeGsubResolveResult::Invalid)
            return OpenTypeGsubApplyAtResult::Invalid;

        if (result == OpenTypeGsubResolveResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (!applyOpenTypeGsubOneToOne(buffer, glyphIndex, replacement))
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubEdit edit;
        edit.inputPositions.push_back(glyphIndex);
        edit.outputCount = 1;
        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }


    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubLigatureAt( const OpenTypeLayoutLookupView& lookup, 
        const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubEditLog& edits)
    {
        if (glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubLigatureMatch match;
        const OpenTypeGsubResolveResult result =
            resolveOpenTypeGsubLigatureLookup(lookup, filter, buffer, glyphIndex, match);

        if (result == OpenTypeGsubResolveResult::Invalid)
            return OpenTypeGsubApplyAtResult::Invalid;

        if (result == OpenTypeGsubResolveResult::NoMatch)
            return OpenTypeGsubApplyAtResult::NoMatch;

        if (match.positions.empty() || match.positions[0] != glyphIndex)
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubEdit edit;
        edit.inputPositions = match.positions;
        edit.outputCount = 1;

        if (!applyOpenTypeGsubLigatureMatch(gdef, buffer, match))
            return OpenTypeGsubApplyAtResult::Invalid;

        edits.push_back(std::move(edit));
        return OpenTypeGsubApplyAtResult::Match;
    }


    // ====================================================================
    // openTypeGsubAdjustBoundaryForEdit
    //
    // Map a one-past physical boundary through one atomic edit. This is used
    // to resume contextual scanning after the matched actionable input sequence.
    // It does not restrict what a nested lookup may inspect or consume.
    // ====================================================================

    static inline bool openTypeGsubAdjustBoundaryForEdit(
        size_t& boundary, const OpenTypeGsubEdit& edit) noexcept
    {
        if (!edit)
            return false;

        const size_t oldBoundary = boundary;
        const size_t insertedCount = edit.outputCount - 1;
        size_t removedBeforeBoundary = 0;

        for (size_t i = 1; i < edit.inputPositions.size(); ++i)
        {
            if (edit.inputPositions[i] < oldBoundary)
                ++removedBeforeBoundary;
        }

        size_t newBoundary = oldBoundary;

        if (edit.anchor() < oldBoundary)
        {
            if (insertedCount > std::numeric_limits<size_t>::max() - newBoundary)
                return false;

            newBoundary += insertedCount;
        }

        if (removedBeforeBoundary > newBoundary)
            return false;

        newBoundary -= removedBeforeBoundary;
        boundary = newBoundary;
        return true;
    }


    // ====================================================================
    // applyOpenTypeGsubContextAt
    //
    // Match and execute one effective LookupType 5 at one physical start
    // position. Matching is completed before any SequenceLookup action runs.
    //
    // Later sequenceIndex values are resolved through OpenTypeGsubSequenceState
    // after all edits produced by preceding nested actions.
    // ====================================================================

    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubContextAt(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubApplyState& state, OpenTypeGsubEditLog& edits,
        size_t* resumeIndex = nullptr)
    {
        if (!lookups || !openTypeGsubHasEffectiveLookupType(lookup, 5) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubApplyAtResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan subtableData =
                openTypeGsubEffectiveSubtable(lookup, 5, subtableIndex);

            if (!subtableData)
                return OpenTypeGsubApplyAtResult::Invalid;

            const OpenTypeGsubContextSubstView subst(subtableData);

            if (!subst)
                return OpenTypeGsubApplyAtResult::Invalid;

            OpenTypeGsubContextMatch match;
            const OpenTypeGsubContextMatchResult matchResult =
                matchOpenTypeGsubContextSubst(subst, filter, buffer, glyphIndex, match);

            if (matchResult == OpenTypeGsubContextMatchResult::Invalid)
                return OpenTypeGsubApplyAtResult::Invalid;

            if (matchResult == OpenTypeGsubContextMatchResult::NoMatch)
                continue;

            if (match.positions.empty() || match.positions[0] != glyphIndex ||
                match.positions.back() == std::numeric_limits<size_t>::max())
            {
                return OpenTypeGsubApplyAtResult::Invalid;
            }

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(match.positions.data(), match.positions.size()))
                return OpenTypeGsubApplyAtResult::Invalid;

            size_t boundary = match.positions.back() + 1;

            for (const OpenTypeSequenceLookup& action : match.lookups)
            {
                size_t targetPosition = 0;

                // An earlier nested substitution may have removed enough
                // sequence entries that this action no longer has a target.

                if (!sequence.position(action.sequenceIndex, targetPosition))
                    continue;

                if (targetPosition >= buffer.size())
                    return OpenTypeGsubApplyAtResult::Invalid;

                const size_t firstNewEdit = edits.size();
                const OpenTypeGsubApplyAtResult nestedResult =
                    applyOpenTypeGsubLookupAt(
                        lookups, action.lookupListIndex, gdef, buffer,
                        targetPosition, state, edits);

                if (nestedResult == OpenTypeGsubApplyAtResult::Invalid)
                    return OpenTypeGsubApplyAtResult::Invalid;

                if (nestedResult == OpenTypeGsubApplyAtResult::NoMatch)
                {
                    if (edits.size() != firstNewEdit)
                        return OpenTypeGsubApplyAtResult::Invalid;

                    continue;
                }

                // Nested contextual/chaining lookups can emit more than one
                // atomic edit. Apply them to the outer sequence in exact order.

                for (size_t editIndex = firstNewEdit; editIndex < edits.size(); ++editIndex)
                {
                    if (!openTypeGsubAdjustBoundaryForEdit(boundary, edits[editIndex]))
                        return OpenTypeGsubApplyAtResult::Invalid;

                    if (!sequence.applyEdit(edits[editIndex]))
                        return OpenTypeGsubApplyAtResult::Invalid;
                }
            }

            if (boundary > buffer.size())
                return OpenTypeGsubApplyAtResult::Invalid;

            if (resumeIndex)
                *resumeIndex = boundary;

            // A contextual rule matching with zero effective substitutions is
            // still a Match. Do not continue to later rules/subtables.

            return OpenTypeGsubApplyAtResult::Match;
        }

        return OpenTypeGsubApplyAtResult::NoMatch;
    }


    // ====================================================================
    // applyOpenTypeGsubChainContextAt
    //
    // Match and execute one effective LookupType 6 at one physical start
    // position.
    //
    // Backtrack and lookahead are match-only constraints. Once the complete
    // chain has matched, only match.inputPositions participate in
    // OpenTypeGsubSequenceState.
    //
    // Later sequenceIndex values are resolved against that mutable current
// input sequence after all edits produced by preceding nested actions.
    // ====================================================================

    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubChainContextAt(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubApplyState& state, OpenTypeGsubEditLog& edits,
        size_t* resumeIndex = nullptr)
    {
        if (!lookups || !openTypeGsubHasEffectiveLookupType(lookup, 6) ||
            glyphIndex >= buffer.size())
        {
            return OpenTypeGsubApplyAtResult::Invalid;
        }

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return OpenTypeGsubApplyAtResult::Invalid;

        const uint16_t subtableCount = lookup.subtableCount();

        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan subtableData =
                openTypeGsubEffectiveSubtable(lookup, 6, subtableIndex);

            if (!subtableData)
                return OpenTypeGsubApplyAtResult::Invalid;

            const OpenTypeGsubChainContextSubstView subst(subtableData);

            if (!subst)
                return OpenTypeGsubApplyAtResult::Invalid;

            OpenTypeGsubChainContextMatch match;
            const OpenTypeGsubChainContextMatchResult matchResult =
                matchOpenTypeGsubChainContextSubst(subst, filter, buffer, glyphIndex, match);

            if (matchResult == OpenTypeGsubChainContextMatchResult::Invalid)
                return OpenTypeGsubApplyAtResult::Invalid;

            if (matchResult == OpenTypeGsubChainContextMatchResult::NoMatch)
                continue;

            if (match.inputPositions.empty() ||
                match.inputPositions[0] != glyphIndex ||
                match.inputPositions.back() == std::numeric_limits<size_t>::max())
            {
                return OpenTypeGsubApplyAtResult::Invalid;
            }

            OpenTypeGsubSequenceState sequence;

            if (!sequence.reset(match.inputPositions.data(), match.inputPositions.size()))
                return OpenTypeGsubApplyAtResult::Invalid;


            // ------------------------------------------------------------
            // This boundary is immediately after the matched input sequence,
            // not after the lookahead sequence.
            //
            // Therefore an unconsumed lookahead glyph remains eligible for
            // the next outer lookup attempt.
            // ------------------------------------------------------------

            size_t boundary = match.inputPositions.back() + 1;


            // ------------------------------------------------------------
            // Execute SequenceLookup records in stored/design order.
            // ------------------------------------------------------------

            for (const OpenTypeSequenceLookup& action : match.lookups)
            {
                size_t targetPosition = 0;

                // A preceding nested action may have changed the current input
                // sequence enough that this sequenceIndex no longer exists.

                if (!sequence.position(action.sequenceIndex, targetPosition))
                    continue;

                if (targetPosition >= buffer.size())
                    return OpenTypeGsubApplyAtResult::Invalid;

                const size_t firstNewEdit = edits.size();

                const OpenTypeGsubApplyAtResult nestedResult =
                    applyOpenTypeGsubLookupAt(
                        lookups, action.lookupListIndex, gdef, buffer,
                        targetPosition, state, edits);

                if (nestedResult == OpenTypeGsubApplyAtResult::Invalid)
                    return OpenTypeGsubApplyAtResult::Invalid;

                if (nestedResult == OpenTypeGsubApplyAtResult::NoMatch)
                {
                    if (edits.size() != firstNewEdit)
                        return OpenTypeGsubApplyAtResult::Invalid;

                    continue;
                }


                // Nested contextual/chaining substitutions may produce more
                // than one atomic edit. Replay every edit in exact order.

                for (size_t editIndex = firstNewEdit; editIndex < edits.size(); ++editIndex)
                {
                    if (!openTypeGsubAdjustBoundaryForEdit(boundary, edits[editIndex]))
                        return OpenTypeGsubApplyAtResult::Invalid;

                    if (!sequence.applyEdit(edits[editIndex]))
                        return OpenTypeGsubApplyAtResult::Invalid;
                }
            }

            if (boundary > buffer.size())
                return OpenTypeGsubApplyAtResult::Invalid;

            if (resumeIndex)
                *resumeIndex = boundary;

            // A chaining rule with zero effective substitutions is still a
            // successful match. Do not continue to later rules/subtables.

            return OpenTypeGsubApplyAtResult::Match;
        }

        return OpenTypeGsubApplyAtResult::NoMatch;
    }



    // ====================================================================
    // applyOpenTypeGsubLookupAt
    //
    // Apply one LookupList entry exactly at one physical glyph position.
    // Type 7 is handled through the existing effective-type machinery.
    // ====================================================================

    static inline OpenTypeGsubApplyAtResult applyOpenTypeGsubLookupAt(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubApplyState& state,
        OpenTypeGsubEditLog& edits)
    {
        if (!lookups || lookupIndex >= lookups.size() || glyphIndex >= buffer.size())
            return OpenTypeGsubApplyAtResult::Invalid;

        const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

        if (!lookup)
            return OpenTypeGsubApplyAtResult::Invalid;

        OpenTypeGsubApplyScope scope(state);

        if (!scope || !state.consumeOperation())
            return OpenTypeGsubApplyAtResult::Invalid;

        uint16_t effectiveType = 0;

        if (!openTypeGsubEffectiveLookupType(lookup, effectiveType))
            return OpenTypeGsubApplyAtResult::Invalid;

        switch (effectiveType)
        {
        case 1:
            return applyOpenTypeGsubSingleAt(lookup, buffer, glyphIndex, edits);

        case 2:
            return applyOpenTypeGsubMultipleAt(lookup, buffer, glyphIndex, edits);

        case 3:
            return applyOpenTypeGsubAlternateAt(lookup, buffer, glyphIndex, edits);

        case 4:
            return applyOpenTypeGsubLigatureAt(lookup, gdef, buffer, glyphIndex, edits);

        case 5:
            return applyOpenTypeGsubContextAt(
                lookups, lookup, gdef, buffer, glyphIndex, state, edits);

        case 6:
            return applyOpenTypeGsubChainContextAt(
                lookups, lookup, gdef, buffer, glyphIndex, state, edits);

        case 8:
            return applyOpenTypeGsubReverseChainSingleAt(
                lookup, gdef, buffer, glyphIndex, edits);

        default:
            return OpenTypeGsubApplyAtResult::Invalid;
        }
    }


    // ====================================================================
    // applyOpenTypeGsubContextLookup
    //
    // Apply one complete effective LookupType 5 Lookup transactionally.
    // After a context Match, scanning resumes at the mapped one-past boundary
    // of that matched input sequence; newly emitted glyphs inside it are not
    // fed back through the same outer lookup.
    // ====================================================================

    static inline bool applyOpenTypeGsubContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer)
    {
        if (!lookups || lookupIndex >= lookups.size())
            return false;

        const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

        if (!lookup || !openTypeGsubHasEffectiveLookupType(lookup, 5))
            return false;

        OpenTypeShapingBuffer working = buffer;
        OpenTypeGsubApplyState state;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGsubApplyScope scope(state);

            if (!scope || !state.consumeOperation())
                return false;

            OpenTypeGsubEditLog edits;
            size_t resumeIndex = glyphIndex + 1;
            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubContextAt(
                    lookups, lookup, gdef, working, glyphIndex,
                    state, edits, &resumeIndex);

            if (result == OpenTypeGsubApplyAtResult::Invalid)
                return false;

            if (result == OpenTypeGsubApplyAtResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            if (resumeIndex <= glyphIndex || resumeIndex > working.size())
                return false;

            glyphIndex = resumeIndex;
        }

        buffer = std::move(working);
        return true;
    }


    // ====================================================================
    // applyOpenTypeGsubChainContextLookup
    //
    // Apply one complete effective LookupType 6 Lookup transactionally.
    // After a successful match, resume immediately after the mapped current
    // input sequence. Backtrack is already behind us and lookahead remains
    // eligible unless a nested substitution actually consumed it.
    // ====================================================================

    static inline bool applyOpenTypeGsubChainContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer)
    {
        if (!lookups || lookupIndex >= lookups.size())
            return false;

        const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

        if (!lookup || !openTypeGsubHasEffectiveLookupType(lookup, 6))
            return false;

        OpenTypeShapingBuffer working = buffer;
        OpenTypeGsubApplyState state;
        size_t glyphIndex = 0;

        while (glyphIndex < working.size())
        {
            OpenTypeGsubApplyScope scope(state);

            if (!scope || !state.consumeOperation())
                return false;

            OpenTypeGsubEditLog edits;
            size_t resumeIndex = glyphIndex + 1;

            const OpenTypeGsubApplyAtResult result =
                applyOpenTypeGsubChainContextAt(
                    lookups, lookup, gdef, working, glyphIndex,
                    state, edits, &resumeIndex);

            if (result == OpenTypeGsubApplyAtResult::Invalid)
                return false;

            if (result == OpenTypeGsubApplyAtResult::NoMatch)
            {
                ++glyphIndex;
                continue;
            }

            if (resumeIndex <= glyphIndex || resumeIndex > working.size())
                return false;

            glyphIndex = resumeIndex;
        }

        buffer = std::move(working);
        return true;
    }



    // ====================================================================
    // applyOpenTypeGsubExtensionLookup
    //
    // LookupView-only Extension dispatcher. Effective Types 5 and 6 cannot
    // execute here because SequenceLookup records require the parent
    // LookupList. The LookupList dispatcher below handles Type 7 -> 5/6.
    // ====================================================================

    static inline bool applyOpenTypeGsubExtensionLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        if (!lookup || lookup.lookupType() != 7)
            return false;

        uint16_t extensionLookupType = 0;

        if (!openTypeGsubEffectiveLookupType(lookup, extensionLookupType))
            return false;

        switch (extensionLookupType)
        {
        case 1:
            return applyOpenTypeGsubSingleLookup(lookup, buffer);

        case 2:
            return applyOpenTypeGsubMultipleLookup(lookup, buffer);

        case 3:
            return applyOpenTypeGsubAlternateLookup(lookup, buffer);

        case 4:
            return applyOpenTypeGsubLigatureLookup(lookup, gdef, buffer);

        case 8:
            return applyOpenTypeGsubReverseChainSingleLookup(lookup, gdef, buffer);

        default:
            return false;
        }
    }


    static inline bool applyOpenTypeGsubExtensionLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGsubExtensionLookup(lookup, gdef, buffer);
    }


    // ====================================================================
    // applyOpenTypeGsubLookup
    //
    // LookupView-only dispatcher retained for existing callers. Types 5 and 6
    // are intentionally absent because this overload has no parent LookupList.
    // ====================================================================

    static inline bool applyOpenTypeGsubLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        if (!lookup)
            return false;

        switch (lookup.lookupType())
        {
        case 1:
            return applyOpenTypeGsubSingleLookup(lookup, buffer);

        case 2:
            return applyOpenTypeGsubMultipleLookup(lookup, buffer);

        case 3:
            return applyOpenTypeGsubAlternateLookup(lookup, buffer);

        case 4:
            return applyOpenTypeGsubLigatureLookup(lookup, gdef, buffer);

        case 7:
            return applyOpenTypeGsubExtensionLookup(lookup, gdef, buffer);

        case 8:
            return applyOpenTypeGsubReverseChainSingleLookup( lookup, gdef, buffer);

        default:
            return false;
        }
    }


    static inline bool applyOpenTypeGsubLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGsubLookup(lookup, gdef, buffer);
    }


    // ====================================================================
    // LookupList dispatchers.
    //
    // This is the complete path for implemented GSUB types. Contextual Types
    // 5 and 6 require the parent LookupList because their SequenceLookup records
    // reference other LookupList entries.
    //
    // Type 7 -> Types 5 and 6 are handled here through effective lookup type.
    // ====================================================================

    static inline bool applyOpenTypeGsubLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingBuffer& buffer) noexcept
    {
        if (!lookups || lookupIndex >= lookups.size())
            return false;

        const OpenTypeLayoutLookupView lookup = lookups.lookup(lookupIndex);

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGsubEffectiveLookupType(lookup, effectiveType))
            return false;

        if (effectiveType == 5)
            return applyOpenTypeGsubContextLookup(lookups, lookupIndex, gdef, buffer);

        if (effectiveType == 6)
            return applyOpenTypeGsubChainContextLookup(lookups, lookupIndex, gdef, buffer);

        return applyOpenTypeGsubLookup(lookup, gdef, buffer);
    }


    static inline bool applyOpenTypeGsubLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        OpenTypeShapingBuffer& buffer) noexcept
    {
        const OpenTypeGdefView gdef{};
        return applyOpenTypeGsubLookup(lookups, lookupIndex, gdef, buffer);
    }

} // namespace waavs
