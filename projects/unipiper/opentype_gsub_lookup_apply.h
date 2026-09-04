// opentype_gsub_lookup_apply.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_gsub_single_view.h"
#include "opentype_gsub_multiple_view.h"
#include "opentype_gsub_ligature_view.h"
#include "opentype_gsub_alternate_view.h"
#include "opentype_gsub_extension_view.h"

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
    // applyOpenTypeGsubSingleLookup
    //
    // Apply one complete LookupType 1 Lookup to the shaping buffer.
    //
    // LookupFlags are deliberately not implemented yet. A non-zero flag
    // is rejected rather than silently producing incorrect shaping.
    //
    // The first pass validates all substitutions that would affect this
    // buffer. The second pass performs the mutations.
    // ====================================================================

    static inline bool applyOpenTypeGsubSingleLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer) noexcept
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 1))
            return false;

        if (lookup.lookupFlag() != 0)
            return false;


        // ------------------------------------------------------------
        // Preflight.
        //
        // No glyph is changed unless every substitution encountered in
        // this buffer can be resolved successfully.
        // ------------------------------------------------------------

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            uint16_t replacement = 0;

            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubSingleLookup(lookup, buffer[i].glyphId, replacement);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;
        }


        // ------------------------------------------------------------
        // Apply.
        // ------------------------------------------------------------

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            OpenTypeShapingGlyph& glyph = buffer[i];

            uint16_t replacement = 0;

            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubSingleLookup(lookup, glyph.glyphId, replacement);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::Match)
                glyph.glyphId = replacement;
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
    // applyOpenTypeGsubMultipleLookup
    //
    // Apply one complete LookupType 2 Lookup to the shaping buffer.
    //
    // One input glyph becomes one or more output glyphs. Every output glyph
    // inherits the complete provenance span of the input glyph.
    //
    // Newly-created output glyphs are skipped for this lookup. Later lookups
    // may act on them.
    // ====================================================================

    static inline bool applyOpenTypeGsubMultipleLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 2))
            return false;

        if (lookup.lookupFlag() != 0)
            return false;


        // ------------------------------------------------------------
        // Preflight.
        //
        // MultipleSubst processes each original input glyph once. Newly
        // generated glyphs do not participate again in this lookup, so the
        // original glyph stream is sufficient for structural validation.
        // ------------------------------------------------------------

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            OpenTypeGsubMultipleSequenceView sequence;

            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubMultipleLookup(lookup, buffer[i].glyphId, sequence);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;
        }


        // ------------------------------------------------------------
        // Apply.
        //
        // i advances over the complete replacement sequence after a match.
        // ------------------------------------------------------------

        size_t i = 0;

        while (i < buffer.size())
        {
            OpenTypeGsubMultipleSequenceView sequence;

            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubMultipleLookup(lookup, buffer[i].glyphId, sequence);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::NoMatch)
            {
                ++i;
                continue;
            }


            const uint16_t replacementCount = sequence.glyphCount();

            if (replacementCount == 0)
                return false;


            // Preserve the complete provenance of the input glyph.

            const OpenTypeShapingGlyph source = buffer[i];


            // Grow the buffer before assigning replacement glyph IDs.
            //
            // Sequence tables are views into font bytes, so vector
            // reallocation here does not affect the Sequence view.

            if (replacementCount > 1)
            {
                std::vector<OpenTypeShapingGlyph>& glyphs = buffer.glyphs();

                glyphs.insert(
                    glyphs.begin() + i + 1,
                    size_t(replacementCount - 1),
                    source);
            }


            // Assign output glyph IDs in Sequence-table order.

            for (uint16_t j = 0; j < replacementCount; ++j)
            {
                uint16_t replacement = 0;

                if (!sequence.glyphId(j, replacement))
                    return false;

                OpenTypeShapingGlyph& glyph = buffer[i + j];

                glyph.glyphId = replacement;
                glyph.scalarOffset = source.scalarOffset;
                glyph.scalarCount = source.scalarCount;
            }


            // All output glyphs participated in this substitution. Skip them
            // before continuing this same lookup.

            i += replacementCount;
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
    // applyOpenTypeGsubLigatureLookup
    //
    // Apply one complete effective GSUB LookupType 4 Lookup to the shaping
    // buffer using LookupFlag/GDEF filtering.
    //
    // Ignored glyphs may occur between ligature components and remain in
    // the shaping buffer. They do not directly contribute to the provenance
    // calculation. The stored provenance remains the bounding logical extent
    // of the participating glyphs.
    // ====================================================================

    static inline bool applyOpenTypeGsubLigatureLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 4))
            return false;

        const OpenTypeLookupGlyphFilter filter(lookup, gdef);

        if (!filter)
            return false;

        // Work transactionally. Ligature matching later in the lookup depends
        // on substitutions already performed earlier in the same lookup, so a
        // simple preflight over the original buffer is not sufficient.

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

            if (match.size() < 2 || match.positions[0] != glyphIndex)
                return false;

            // Validate the recorded positions before mutating the buffer.

            for (size_t i = 0; i < match.positions.size(); ++i)
            {
                if (match.positions[i] >= working.size())
                    return false;

                if (i != 0 && match.positions[i] <= match.positions[i - 1])
                    return false;
            }


            // ------------------------------------------------------------
            // Merge the bounding provenance extent from participating glyphs.
            // ------------------------------------------------------------

            const OpenTypeShapingGlyph& first = working[match.positions[0]];
            uint32_t scalarBegin = first.scalarOffset;
            uint64_t scalarEnd = uint64_t(first.scalarOffset) + uint64_t(first.scalarCount);

            for (size_t i = 1; i < match.positions.size(); ++i)
            {
                const OpenTypeShapingGlyph& component = working[match.positions[i]];

                if (component.scalarOffset < scalarBegin)
                    scalarBegin = component.scalarOffset;

                const uint64_t componentEnd =
                    uint64_t(component.scalarOffset) + uint64_t(component.scalarCount);

                if (componentEnd > scalarEnd)
                    scalarEnd = componentEnd;
            }

            if (scalarEnd < scalarBegin || scalarEnd - scalarBegin > 0xFFFFFFFFull)
                return false;


            // ------------------------------------------------------------
            // Replace the first participating glyph with the ligature.
            // ------------------------------------------------------------

            OpenTypeShapingGlyph& output = working[match.positions[0]];
            output.glyphId = match.ligatureGlyph;
            output.scalarOffset = scalarBegin;
            output.scalarCount = static_cast<uint32_t>(scalarEnd - scalarBegin);


            // ------------------------------------------------------------
            // Remove only the remaining participating glyphs. Erase in
            // descending physical-position order so earlier indexes remain
            // valid and ignored glyphs survive unchanged.
            // ------------------------------------------------------------

            std::vector<OpenTypeShapingGlyph>& glyphs = working.glyphs();

            for (size_t i = match.positions.size(); i > 1; --i)
                glyphs.erase(glyphs.begin() + match.positions[i - 1]);


            // Do not feed the newly-created ligature back through this same
            // lookup. The next physical glyph may be one that was ignored
            // while matching the ligature, which is intentional.

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
//
// AlternateSubst is a 1 -> 1 substitution. The selected replacement
// inherits the input glyph's provenance unchanged.
//
// For now, resolveOpenTypeGsubAlternateLookup() supplies alternate 0
// as the default alternate-selection policy.
// ====================================================================

    static inline bool applyOpenTypeGsubAlternateLookup(
        const OpenTypeLayoutLookupView& lookup, OpenTypeShapingBuffer& buffer)
    {
        if (!openTypeGsubHasEffectiveLookupType(lookup, 3))
            return false;

        if (lookup.lookupFlag() != 0)
            return false;


        // Work transactionally so malformed data cannot leave the caller's
        // shaping buffer partially substituted.

        OpenTypeShapingBuffer working = buffer;

        for (size_t glyphIndex = 0; glyphIndex < working.size(); ++glyphIndex)
        {
            uint16_t replacementGlyph = 0;

            const OpenTypeGsubResolveResult result =
                resolveOpenTypeGsubAlternateLookup(
                    lookup,
                    working[glyphIndex].glyphId,
                    replacementGlyph);

            if (result == OpenTypeGsubResolveResult::Invalid)
                return false;

            if (result == OpenTypeGsubResolveResult::NoMatch)
                continue;


            // ------------------------------------------------------------
            // AlternateSubst changes only the glyph ID.
            //
            // scalarOffset and scalarCount remain unchanged.
            // ------------------------------------------------------------

            working[glyphIndex].glyphId = replacementGlyph;
        }


        buffer = std::move(working);

        return true;
    }














    // ====================================================================
    // applyOpenTypeGsubExtensionLookup
    //
    // GDEF-aware overload. ExtensionSubst introduces no substitution
    // behavior of its own; delegate to the effective lookup executor.
    //
    // Type 4 now consumes GDEF filtering. Types 1/2/3 retain their current
    // filtering behavior until their executors are updated separately.
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
    // GDEF-aware general GSUB Lookup dispatcher.
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
    // LookupList convenience overloads.
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
