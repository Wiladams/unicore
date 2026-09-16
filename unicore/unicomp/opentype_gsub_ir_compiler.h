// opentype_gsub_ir_compiler.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "opentype_gsub_extension_view.h"
#include "opentype_gsub_single_view.h"
#include "opentype_gsub_multiple_view.h"
#include "opentype_gsub_ligature_view.h"
#include "opentype_gsub_context_view.h"
#include "opentype_gsub_chain_context_view.h"
#include "opentype_gdef_view.h"
#include "opentype_lookup_glyph_filter.h"
#include "opentype_layout_view.h"
#include "opentype_shaping_ir.h"


namespace waavs
{
    // ========================================================================
    // openTypeGsubIREffectiveType
    //
    // Resolve native and ExtensionSubst lookup types during compilation.
    //
    // ExtensionSubst does not survive into the IR.
    // ========================================================================

    [[nodiscard]] static inline bool openTypeGsubIREffectiveType(
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

        uint16_t effectiveType = 0;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data = lookup.subtable(i);

            if (!data)
                return false;

            const OpenTypeGsubExtensionSubstView extension(data);

            if (!extension)
                return false;

            const uint16_t type = extension.extensionLookupType();

            if (type == 0 || type == 7)
                return false;

            if (i == 0)
                effectiveType = type;
            else if (type != effectiveType)
                return false;
        }

        result = effectiveType;
        return true;
    }


    // ========================================================================
    // openTypeGsubIREffectiveSubtable
    //
    // Return the semantic substitution subtable.
    //
    // Native lookup:
    //
    //     Lookup -> Subtable
    //
    // Extension lookup:
    //
    //     Lookup -> ExtensionSubst -> Subtable
    //
    // ExtensionSubst is therefore a compiler concern only.
    // ========================================================================

    [[nodiscard]] static inline ByteSpan openTypeGsubIREffectiveSubtable(
        const OpenTypeLayoutLookupView& lookup, uint16_t effectiveType,
        uint16_t subtableIndex) noexcept
    {
        if (!lookup || subtableIndex >= lookup.subtableCount())
            return {};

        const ByteSpan data = lookup.subtable(subtableIndex);

        if (!data)
            return {};

        if (lookup.lookupType() != 7)
            return lookup.lookupType() == effectiveType ? data : ByteSpan{};

        const OpenTypeGsubExtensionSubstView extension(data);

        if (!extension || extension.extensionLookupType() != effectiveType)
            return {};

        return extension.extensionSubtable();
    }


    // ========================================================================
    // Compiled GDEF / LookupFlag support
    // ========================================================================



    // ========================================================================
    // compileOpenTypeShapingIRGdefGlyphClasses
    //
    // Compile GDEF GlyphClassDef into direct glyph-ID indexed semantic data.
    //
    // requireClassDef:
    //
    //     true
    //         The caller requires an actual GlyphClassDef table. This matches
    //         OpenTypeLookupGlyphFilter validation for filtering lookups.
    //
    //     false
    //         A valid GDEF without GlyphClassDef is allowed; all glyphs then
    //         have class zero. This is useful for Type 4 provenance behavior.
    // ========================================================================

    static inline bool compileOpenTypeShapingIRGdefGlyphClasses(
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        bool requireClassDef)
    {
        if (ir.gdefGlyphClasses.size() == kOpenTypeShapingIRGlyphDomainSize)
            return true;

        if (!ir.gdefGlyphClasses.empty())
            return false;

        if (!gdef)
            return !requireClassDef;

        const OpenTypeClassDefView classes = gdef.glyphClassDef();

        if (requireClassDef && !classes)
            return false;

        std::vector<uint16_t> values(kOpenTypeShapingIRGlyphDomainSize, 0);

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t value = 0;

            if (!gdef.glyphClass(glyphId, value))
                return false;

            values[glyphId] = value;
        }

        ir.gdefGlyphClasses = std::move(values);
        return true;
    }


    // ========================================================================
    // compileOpenTypeShapingIRGdefMarkAttachClasses
    // ========================================================================

    static inline bool compileOpenTypeShapingIRGdefMarkAttachClasses(
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir)
    {
        if (ir.gdefMarkAttachClasses.size() == kOpenTypeShapingIRGlyphDomainSize)
            return true;

        if (!ir.gdefMarkAttachClasses.empty() || !gdef)
            return false;

        const OpenTypeClassDefView classes = gdef.markAttachClassDef();

        if (!classes)
            return false;

        std::vector<uint16_t> values(kOpenTypeShapingIRGlyphDomainSize, 0);

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t value = 0;

            if (!gdef.markAttachClass(glyphId, value))
                return false;

            values[glyphId] = value;
        }

        ir.gdefMarkAttachClasses = std::move(values);
        return true;
    }


    // ========================================================================
    // compileOpenTypeShapingIRMarkFilteringSet
    //
    // Compile one GDEF MarkGlyphSetsDef entry into the existing canonical
    // OpenTypeShapingIRGlyphSet representation.
    //
    // We deliberately enumerate the complete uint16 glyph domain for this
    // first compiler, exactly as we currently do for Coverage compilation.
    // ========================================================================

    static inline bool compileOpenTypeShapingIRMarkFilteringSet(
        const OpenTypeGdefView& gdef, uint16_t setIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!gdef)
            return false;

        const OpenTypeMarkGlyphSetsView sets = gdef.markGlyphSetsDef();

        if (!sets || setIndex >= sets.size())
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            bool member = false;

            if (!sets.contains(setIndex, glyphId, member))
                return false;

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        if (ir.glyphRanges.size() > std::numeric_limits<uint32_t>::max() ||
            ranges.size() > std::numeric_limits<uint32_t>::max() ||
            ir.glyphSets.size() > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        const uint32_t rangeOffset =
            static_cast<uint32_t>(ir.glyphRanges.size());

        const uint32_t rangeCount =
            static_cast<uint32_t>(ranges.size());

        if (uint64_t(rangeOffset) + uint64_t(rangeCount) >
            std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        result =
            static_cast<OpenTypeShapingIRGlyphSetId>(
                ir.glyphSets.size());

        ir.glyphRanges.insert(
            ir.glyphRanges.end(),
            ranges.begin(),
            ranges.end());

        ir.glyphSets.push_back({
            rangeOffset,
            rangeCount
            });

        return true;
    }


    // ========================================================================
    // compileOpenTypeShapingIRLookupFilter
    //
    // Normalize one raw LookupFlag and any GDEF data required by it.
    //
    // Raw OpenType flag encoding does not survive compilation.
    // ========================================================================

    static inline bool compileOpenTypeShapingIRLookupFilter(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupFilter& result)
    {
        result = {};

        if (!lookup)
            return false;

        const uint16_t lookupFlag = lookup.lookupFlag();

        if ((lookupFlag & kOpenTypeLookupFlagReservedMask) != 0)
            return false;

        if ((lookupFlag & kOpenTypeLookupFlagRightToLeft) != 0)
            result.flags |= OpenTypeShapingIRRightToLeft;

        if ((lookupFlag & kOpenTypeLookupFlagIgnoreBaseGlyphs) != 0)
            result.flags |= OpenTypeShapingIRIgnoreBaseGlyphs;

        if ((lookupFlag & kOpenTypeLookupFlagIgnoreLigatures) != 0)
            result.flags |= OpenTypeShapingIRIgnoreLigatures;

        if ((lookupFlag & kOpenTypeLookupFlagIgnoreMarks) != 0)
            result.flags |= OpenTypeShapingIRIgnoreMarks;

        const bool ignoreBase =
            (lookupFlag & kOpenTypeLookupFlagIgnoreBaseGlyphs) != 0;

        const bool ignoreLigatures =
            (lookupFlag & kOpenTypeLookupFlagIgnoreLigatures) != 0;

        const bool ignoreMarks =
            (lookupFlag & kOpenTypeLookupFlagIgnoreMarks) != 0;

        const bool useMarkFilteringSet =
            (lookupFlag & kOpenTypeLookupFlagUseMarkFilteringSet) != 0;

        const uint16_t markAttachmentType =
            static_cast<uint16_t>(
                (lookupFlag & kOpenTypeLookupFlagMarkAttachmentTypeMask) >> 8);

        result.markAttachmentType =
            static_cast<uint8_t>(markAttachmentType);

        const bool needsGlyphClass =
            ignoreBase ||
            ignoreLigatures ||
            ignoreMarks ||
            useMarkFilteringSet ||
            markAttachmentType != 0;

        if (needsGlyphClass)
        {
            if (!gdef ||
                !compileOpenTypeShapingIRGdefGlyphClasses(
                    gdef, ir, true))
            {
                return false;
            }
        }

        if (useMarkFilteringSet)
        {
            uint16_t setIndex = 0;

            if (!lookup.markFilteringSet(setIndex))
                return false;

            if (!compileOpenTypeShapingIRMarkFilteringSet(
                gdef, setIndex, ir, result.markFilteringSet))
            {
                return false;
            }
        }

        // IgnoreMarks supersedes both of these.
        // MarkFilteringSet supersedes MarkAttachmentType.
        if (!ignoreMarks &&
            !useMarkFilteringSet &&
            markAttachmentType != 0)
        {
            if (!compileOpenTypeShapingIRGdefMarkAttachClasses(
                gdef, ir))
            {
                return false;
            }
        }

        return true;
    }
    // ========================================================================
    // GSUB Single
    // ========================================================================

    // ========================================================================
    // compileOpenTypeGsubSingleSubtable
    //
    // Compile either SingleSubst Format 1 or Format 2 into one canonical
    // semantic representation:
    //
    //     input glyph -> output glyph
    //
    // OpenType serialization format does not survive compilation.
    //
    // The resulting pairs are ordered by input glyph.
    //
    // This first compiler deliberately enumerates the complete uint16 glyph
    // domain through Coverage::find(). That keeps the initial IR compiler
    // independent of Coverage Format 1/2 and gives us a simple semantic
    // reference implementation. A direct coverage enumerator can replace
    // this later without changing the IR.
    // ========================================================================

    static inline bool compileOpenTypeGsubSingleSubtable(
        const ByteSpan& data,
        std::vector<OpenTypeShapingIRGsubSinglePair>& pairs)
    {
        pairs.clear();

        const OpenTypeGsubSingleSubstView single(data);

        if (!single)
            return false;

        const uint16_t format = single.format();

        if (format != 1 && format != 2)
            return false;

        const OpenTypeCoverageView coverage = single.coverage();

        if (!coverage)
            return false;

        int32_t delta = 0;

        if (format == 1 && !single.deltaGlyphId(delta))
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            uint16_t replacement = 0;

            if (format == 1)
            {
                replacement = static_cast<uint16_t>(
                    glyphId +
                    static_cast<uint32_t>(
                        static_cast<uint16_t>(delta)));
            }
            else
            {
                if (!single.substituteGlyphId(coverageIndex, replacement))
                    return false;
            }

            pairs.push_back({
                static_cast<uint16_t>(glyphId),
                replacement
                });
        }

        return true;
    }


    // ========================================================================
    // appendOpenTypeGsubSingleSubtable
    // ========================================================================

    static inline bool appendOpenTypeGsubSingleSubtable(
        const std::vector<OpenTypeShapingIRGsubSinglePair>& pairs,
        OpenTypeShapingIR& ir)
    {
        if (ir.gsubSinglePairs.size() > std::numeric_limits<uint32_t>::max() ||
            pairs.size() > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        const uint32_t pairOffset =
            static_cast<uint32_t>(ir.gsubSinglePairs.size());

        const uint32_t pairCount =
            static_cast<uint32_t>(pairs.size());

        if (uint64_t(pairOffset) + uint64_t(pairCount) >
            std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        ir.gsubSinglePairs.insert(
            ir.gsubSinglePairs.end(),
            pairs.begin(),
            pairs.end());

        ir.gsubSingleSubtables.push_back({
            pairOffset,
            pairCount
            });

        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubSingleLookup
    // ========================================================================

    static inline bool compileOpenTypeGsubSingleLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(lookup, effectiveType) ||
            effectiveType != 1)
        {
            return false;
        }



        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const size_t oldLookupCount = ir.lookups.size();
        const size_t oldSubtableCount = ir.gsubSingleSubtables.size();
        const size_t oldPairCount = ir.gsubSinglePairs.size();

        const size_t oldGlyphRangeCount = ir.glyphRanges.size();
        const size_t oldGlyphSetCount = ir.glyphSets.size();
        const size_t oldGdefGlyphClassCount = ir.gdefGlyphClasses.size();
        const size_t oldGdefMarkAttachClassCount = ir.gdefMarkAttachClasses.size();


        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(oldLookupCount);
                ir.gsubSingleSubtables.resize(oldSubtableCount);
                ir.gsubSinglePairs.resize(oldPairCount);

                ir.glyphRanges.resize(oldGlyphRangeCount);
                ir.glyphSets.resize(oldGlyphSetCount);
                ir.gdefGlyphClasses.resize(oldGdefGlyphClassCount);
                ir.gdefMarkAttachClasses.resize(oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount > std::numeric_limits<uint32_t>::max())
            return false;

        const uint32_t payloadOffset =
            static_cast<uint32_t>(oldSubtableCount);

        std::vector<OpenTypeShapingIRGsubSinglePair> pairs;

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data =
                openTypeGsubIREffectiveSubtable(
                    lookup,
                    effectiveType,
                    i);

            if (!data)
            {
                rollback();
                return false;
            }

            if (!compileOpenTypeGsubSingleSubtable(data, pairs))
            {
                rollback();
                return false;
            }

            if (!appendOpenTypeGsubSingleSubtable(pairs, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() > std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op = OpenTypeShapingIROp::GsubSingle;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = subtableCount;
        compiled.filter = filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);

        return true;
    }

    static inline bool compileOpenTypeGsubSingleLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubSingleLookup(lookup, gdef, ir, result);
    }


    // ========================================================================
    // GSUB Multiple
    // ========================================================================

    // ========================================================================
    // compileOpenTypeGsubMultipleSubtable
    //
    // Compile one MultipleSubst subtable into:
    //
    //     input glyph
    //         -> sequence
    //             -> output glyphs
    //
    // Coverage indexing and source Sequence table offsets do not survive.
    //
    // pair.sequenceIndex is local to sequences.
    // sequence.glyphOffset is local to glyphs.
    //
    // appendOpenTypeGsubMultipleSubtable() rebases both when committing the
    // temporary subtable into the global IR pools.
    // ========================================================================

    static inline bool compileOpenTypeGsubMultipleSubtable(
        const ByteSpan& data,
        std::vector<OpenTypeShapingIRGsubMultiplePair>& pairs,
        std::vector<OpenTypeShapingIRGlyphSequence>& sequences,
        std::vector<uint16_t>& glyphs)
    {
        pairs.clear();
        sequences.clear();
        glyphs.clear();

        const OpenTypeGsubMultipleSubstView multiple(data);

        if (!multiple)
            return false;

        const OpenTypeCoverageView coverage = multiple.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            const OpenTypeGsubMultipleSequenceView sequence =
                multiple.sequence(coverageIndex);

            if (!sequence)
                return false;

            const uint16_t glyphCount = sequence.glyphCount();

            if (glyphCount == 0)
                return false;

            if (glyphs.size() > std::numeric_limits<uint32_t>::max() ||
                sequences.size() > std::numeric_limits<uint32_t>::max())
            {
                return false;
            }

            const uint32_t glyphOffset =
                static_cast<uint32_t>(glyphs.size());

            if (uint64_t(glyphOffset) + uint64_t(glyphCount) >
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }

            for (uint16_t i = 0; i < glyphCount; ++i)
            {
                uint16_t outputGlyph = 0;

                if (!sequence.glyphId(i, outputGlyph))
                    return false;

                glyphs.push_back(outputGlyph);
            }

            const uint32_t sequenceIndex =
                static_cast<uint32_t>(sequences.size());

            sequences.push_back({
                glyphOffset,
                glyphCount
                });

            pairs.push_back({
                static_cast<uint16_t>(glyphId),
                0,
                sequenceIndex
                });
        }

        return true;
    }


    // ========================================================================
    // appendOpenTypeGsubMultipleSubtable
    //
    // Commit one temporary compiled MultipleSubst subtable to the IR.
    //
    // Temporary indices are local:
    //
    //     pair.sequenceIndex -> sequences[]
    //     sequence.glyphOffset -> glyphs[]
    //
    // They are rebased here to address the global IR pools.
    // ========================================================================

    static inline bool appendOpenTypeGsubMultipleSubtable(
        const std::vector<OpenTypeShapingIRGsubMultiplePair>& pairs,
        const std::vector<OpenTypeShapingIRGlyphSequence>& sequences,
        const std::vector<uint16_t>& glyphs,
        OpenTypeShapingIR& ir)
    {
        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gsubMultipleGlyphs.size() > max32 ||
            ir.gsubMultipleSequences.size() > max32 ||
            ir.gsubMultiplePairs.size() > max32 ||
            pairs.size() > max32 ||
            sequences.size() > max32 ||
            glyphs.size() > max32)
        {
            return false;
        }

        const uint32_t glyphBase =
            static_cast<uint32_t>(ir.gsubMultipleGlyphs.size());

        const uint32_t sequenceBase =
            static_cast<uint32_t>(ir.gsubMultipleSequences.size());

        const uint32_t pairOffset =
            static_cast<uint32_t>(ir.gsubMultiplePairs.size());

        if (uint64_t(glyphBase) + glyphs.size() > max32 ||
            uint64_t(sequenceBase) + sequences.size() > max32 ||
            uint64_t(pairOffset) + pairs.size() > max32)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Preflight temporary references before mutating the IR.
        // ------------------------------------------------------------

        for (const OpenTypeShapingIRGlyphSequence& sequence : sequences)
        {
            if (sequence.glyphOffset > glyphs.size() ||
                sequence.glyphCount > glyphs.size() - sequence.glyphOffset)
            {
                return false;
            }

            if (uint64_t(glyphBase) + sequence.glyphOffset > max32)
                return false;
        }

        for (const OpenTypeShapingIRGsubMultiplePair& pair : pairs)
        {
            if (pair.sequenceIndex >= sequences.size())
                return false;

            if (uint64_t(sequenceBase) + pair.sequenceIndex > max32)
                return false;
        }


        // ------------------------------------------------------------
        // Glyph pool.
        // ------------------------------------------------------------

        ir.gsubMultipleGlyphs.insert(
            ir.gsubMultipleGlyphs.end(),
            glyphs.begin(),
            glyphs.end());


        // ------------------------------------------------------------
        // Sequence pool, rebasing glyph offsets.
        // ------------------------------------------------------------

        ir.gsubMultipleSequences.reserve(
            ir.gsubMultipleSequences.size() +
            sequences.size());

        for (const OpenTypeShapingIRGlyphSequence& sequence : sequences)
        {
            ir.gsubMultipleSequences.push_back({
                glyphBase + sequence.glyphOffset,
                sequence.glyphCount
                });
        }


        // ------------------------------------------------------------
        // Pair pool, rebasing sequence indices.
        // ------------------------------------------------------------

        ir.gsubMultiplePairs.reserve(
            ir.gsubMultiplePairs.size() +
            pairs.size());

        for (const OpenTypeShapingIRGsubMultiplePair& pair : pairs)
        {
            ir.gsubMultiplePairs.push_back({
                pair.input,
                0,
                sequenceBase + pair.sequenceIndex
                });
        }

        ir.gsubMultipleSubtables.push_back({
            pairOffset,
            static_cast<uint32_t>(pairs.size())
            });

        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubMultipleLookup
    //
    // Compile one complete effective GSUB LookupType 2.
    //
    // Native Type 2 and ExtensionSubst -> Type 2 compile identically.
    //
    // Subtable order is preserved. The first matching subtable wins for each
    // input glyph.
    //
    // This initial compiler accepts only lookupFlag == 0. Lookup filtering
    // will be compiled into OpenTypeShapingIRLookupFilter later.
    //
    // Compilation is transactional with respect to the supplied IR.
    // ========================================================================

    static inline bool compileOpenTypeGsubMultipleLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(lookup, effectiveType) ||
            effectiveType != 2)
        {
            return false;
        }



        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return false;


        // ------------------------------------------------------------
        // Transaction checkpoint.
        // ------------------------------------------------------------

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldSubtableCount =
            ir.gsubMultipleSubtables.size();

        const size_t oldPairCount =
            ir.gsubMultiplePairs.size();

        const size_t oldSequenceCount =
            ir.gsubMultipleSequences.size();

        const size_t oldGlyphCount =
            ir.gsubMultipleGlyphs.size();

        const size_t oldGlyphRangeCount = ir.glyphRanges.size();
        const size_t oldGlyphSetCount = ir.glyphSets.size();
        const size_t oldGdefGlyphClassCount = ir.gdefGlyphClasses.size();
        const size_t oldGdefMarkAttachClassCount = ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(oldLookupCount);
                ir.gsubMultipleSubtables.resize(oldSubtableCount);
                ir.gsubMultiplePairs.resize(oldPairCount);
                ir.gsubMultipleSequences.resize(oldSequenceCount);
                ir.gsubMultipleGlyphs.resize(oldGlyphCount);
                ir.glyphRanges.resize(oldGlyphRangeCount);
                ir.glyphSets.resize(oldGlyphSetCount);
                ir.gdefGlyphClasses.resize(oldGdefGlyphClassCount);
                ir.gdefMarkAttachClasses.resize(oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        const uint32_t payloadOffset = static_cast<uint32_t>(oldSubtableCount);


        // ------------------------------------------------------------
        // Temporary per-subtable pools.
        // ------------------------------------------------------------

        std::vector<OpenTypeShapingIRGsubMultiplePair> pairs;
        std::vector<OpenTypeShapingIRGlyphSequence> sequences;
        std::vector<uint16_t> glyphs;


        // ------------------------------------------------------------
        // Compile each source subtable in stored order.
        // ------------------------------------------------------------

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data =
                openTypeGsubIREffectiveSubtable(
                    lookup,
                    effectiveType,
                    i);

            if (!data)
            {
                rollback();
                return false;
            }

            if (!compileOpenTypeGsubMultipleSubtable(
                data,
                pairs,
                sequences,
                glyphs))
            {
                rollback();
                return false;
            }

            if (!appendOpenTypeGsubMultipleSubtable(
                pairs,
                sequences,
                glyphs,
                ir))
            {
                rollback();
                return false;
            }
        }


        // ------------------------------------------------------------
        // Append semantic lookup.
        // ------------------------------------------------------------

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op = OpenTypeShapingIROp::GsubMultiple;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = subtableCount;
        compiled.filter = filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);

        return true;
    }

    static inline bool compileOpenTypeGsubMultipleLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubMultipleLookup(lookup, gdef, ir, result);
    }



    // ========================================================================
    // GSUB Ligature
    // ========================================================================

    // ========================================================================
    // compileOpenTypeGsubLigatureSubtable
    //
    // Compile one LigatureSubst subtable into:
    //
    //     first input glyph
    //         -> ordered ligature candidates
    //
    // Each candidate stores:
    //
    //     resulting ligature glyph
    //     total component count
    //     trailing component glyph sequence
    //
    // The first component is represented by pair.input and is therefore not
    // repeated in gsubLigatureComponents.
    //
    // pair.ligatureOffset and ligature.componentOffset are local to the
    // temporary vectors. appendOpenTypeGsubLigatureSubtable() rebases them
    // when committing this subtable to the global IR.
    // ========================================================================

    static inline bool compileOpenTypeGsubLigatureSubtable(
        const ByteSpan& data,
        std::vector<OpenTypeShapingIRGsubLigaturePair>& pairs,
        std::vector<OpenTypeShapingIRGsubLigature>& ligatures,
        std::vector<uint16_t>& components)
    {
        pairs.clear();
        ligatures.clear();
        components.clear();

        const OpenTypeGsubLigatureSubstView subst(data);

        if (!subst)
            return false;

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            const OpenTypeGsubLigatureSetView set =
                subst.ligatureSet(coverageIndex);

            if (!set)
                return false;

            if (ligatures.size() >
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }

            const uint32_t ligatureOffset =
                static_cast<uint32_t>(ligatures.size());

            const uint16_t ligatureCount = set.size();

            for (uint16_t ligatureIndex = 0;
                ligatureIndex < ligatureCount;
                ++ligatureIndex)
            {
                const OpenTypeGsubLigatureView ligature =
                    set.ligature(ligatureIndex);

                if (!ligature)
                    return false;

                const uint16_t componentCount =
                    ligature.componentCount();

                if (componentCount < 2)
                    return false;

                if (components.size() >
                    std::numeric_limits<uint32_t>::max())
                {
                    return false;
                }

                const uint32_t componentOffset =
                    static_cast<uint32_t>(components.size());

                const uint32_t trailingCount =
                    uint32_t(componentCount) - 1u;

                if (uint64_t(componentOffset) +
                    uint64_t(trailingCount) >
                    std::numeric_limits<uint32_t>::max())
                {
                    return false;
                }

                for (uint16_t componentIndex = 1;
                    componentIndex < componentCount;
                    ++componentIndex)
                {
                    uint16_t componentGlyph = 0;

                    if (!ligature.componentGlyphId(
                        componentIndex - 1,
                        componentGlyph))
                    {
                        return false;
                    }

                    components.push_back(componentGlyph);
                }

                ligatures.push_back({
                    ligature.ligatureGlyph(),
                    componentCount,
                    componentOffset
                    });
            }

            if (uint64_t(ligatureOffset) +
                uint64_t(ligatureCount) >
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }

            pairs.push_back({
                static_cast<uint16_t>(glyphId),
                0,
                ligatureOffset,
                ligatureCount
                });
        }

        return true;
    }


    // ========================================================================
    // appendOpenTypeGsubLigatureSubtable
    //
    // Commit one temporary LigatureSubst subtable to the IR.
    //
    // Rebase:
    //
    //     ligature.componentOffset
    //         local components[] -> global gsubLigatureComponents[]
    //
    //     pair.ligatureOffset
    //         local ligatures[] -> global gsubLigatures[]
    // ========================================================================

    static inline bool appendOpenTypeGsubLigatureSubtable(
        const std::vector<OpenTypeShapingIRGsubLigaturePair>& pairs,
        const std::vector<OpenTypeShapingIRGsubLigature>& ligatures,
        const std::vector<uint16_t>& components,
        OpenTypeShapingIR& ir)
    {
        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gsubLigatureComponents.size() > max32 ||
            ir.gsubLigatures.size() > max32 ||
            ir.gsubLigaturePairs.size() > max32 ||
            components.size() > max32 ||
            ligatures.size() > max32 ||
            pairs.size() > max32)
        {
            return false;
        }

        const uint32_t componentBase =
            static_cast<uint32_t>(
                ir.gsubLigatureComponents.size());

        const uint32_t ligatureBase =
            static_cast<uint32_t>(
                ir.gsubLigatures.size());

        const uint32_t pairOffset =
            static_cast<uint32_t>(
                ir.gsubLigaturePairs.size());

        if (uint64_t(componentBase) + components.size() > max32 ||
            uint64_t(ligatureBase) + ligatures.size() > max32 ||
            uint64_t(pairOffset) + pairs.size() > max32)
        {
            return false;
        }


        // ------------------------------------------------------------
        // Validate all temporary references before changing the IR.
        // ------------------------------------------------------------

        for (const OpenTypeShapingIRGsubLigature& ligature :
            ligatures)
        {
            if (ligature.componentCount < 2)
                return false;

            const size_t trailingCount =
                size_t(ligature.componentCount) - 1u;

            if (ligature.componentOffset > components.size() ||
                trailingCount >
                components.size() - ligature.componentOffset)
            {
                return false;
            }

            if (uint64_t(componentBase) +
                ligature.componentOffset > max32)
            {
                return false;
            }
        }

        for (const OpenTypeShapingIRGsubLigaturePair& pair :
            pairs)
        {
            if (pair.ligatureOffset > ligatures.size() ||
                pair.ligatureCount >
                ligatures.size() - pair.ligatureOffset)
            {
                return false;
            }

            if (uint64_t(ligatureBase) +
                pair.ligatureOffset > max32)
            {
                return false;
            }
        }


        // ------------------------------------------------------------
        // Component pool.
        // ------------------------------------------------------------

        ir.gsubLigatureComponents.insert(
            ir.gsubLigatureComponents.end(),
            components.begin(),
            components.end());


        // ------------------------------------------------------------
        // Ligature candidates.
        // ------------------------------------------------------------

        ir.gsubLigatures.reserve(
            ir.gsubLigatures.size() +
            ligatures.size());

        for (const OpenTypeShapingIRGsubLigature& ligature :
            ligatures)
        {
            ir.gsubLigatures.push_back({
                ligature.output,
                ligature.componentCount,
                componentBase + ligature.componentOffset
                });
        }


        // ------------------------------------------------------------
        // First-glyph pairs.
        // ------------------------------------------------------------

        ir.gsubLigaturePairs.reserve(
            ir.gsubLigaturePairs.size() +
            pairs.size());

        for (const OpenTypeShapingIRGsubLigaturePair& pair :
            pairs)
        {
            ir.gsubLigaturePairs.push_back({
                pair.input,
                0,
                ligatureBase + pair.ligatureOffset,
                pair.ligatureCount
                });
        }


        ir.gsubLigatureSubtables.push_back({
            pairOffset,
            static_cast<uint32_t>(pairs.size())
            });

        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubLigatureLookup
    //
    // Compile one complete effective GSUB LookupType 4.
    //
    // Native Type 4 and ExtensionSubst -> Type 4 compile identically.
    //
    // Subtable order and LigatureSet candidate order are both preserved.
    //
    // This initial implementation accepts only lookupFlag == 0. GDEF/LookupFlag
    // filtering will be compiled into OpenTypeShapingIRLookupFilter separately.
    //
    // Compilation is transactional with respect to the supplied IR.
    // ========================================================================

    static inline bool compileOpenTypeGsubLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 4)
        {
            return false;
        }



        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;


        // ------------------------------------------------------------
        // Transaction checkpoint.
        // ------------------------------------------------------------

        const size_t oldLookupCount =
            ir.lookups.size();

        const size_t oldSubtableCount =
            ir.gsubLigatureSubtables.size();

        const size_t oldPairCount =
            ir.gsubLigaturePairs.size();

        const size_t oldLigatureCount =
            ir.gsubLigatures.size();

        const size_t oldComponentCount =
            ir.gsubLigatureComponents.size();

        const size_t oldGlyphRangeCount = ir.glyphRanges.size();
        const size_t oldGlyphSetCount = ir.glyphSets.size();
        const size_t oldGdefGlyphClassCount = ir.gdefGlyphClasses.size();
        const size_t oldGdefMarkAttachClassCount = ir.gdefMarkAttachClasses.size();



        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(oldLookupCount);
                ir.gsubLigatureSubtables.resize(oldSubtableCount);
                ir.gsubLigaturePairs.resize(oldPairCount);
                ir.gsubLigatures.resize(oldLigatureCount);
                ir.gsubLigatureComponents.resize(oldComponentCount);
                ir.glyphRanges.resize(oldGlyphRangeCount);
                ir.glyphSets.resize(oldGlyphSetCount);
                ir.gdefGlyphClasses.resize(oldGdefGlyphClassCount);
                ir.gdefMarkAttachClasses.resize(oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        // Type 4 provenance uses GDEF glyph classes even when LookupFlag does
        // not require filtering. In particular this distinguishes mark-only
        // ligatures from normal base ligatures.
        if (gdef &&
            !compileOpenTypeShapingIRGdefGlyphClasses(
                gdef, ir, false))
        {
            rollback();
            return false;
        }


        if (oldSubtableCount >
            std::numeric_limits<uint32_t>::max())
        {
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(
                oldSubtableCount);


        // ------------------------------------------------------------
        // Temporary per-subtable pools.
        // ------------------------------------------------------------

        std::vector<OpenTypeShapingIRGsubLigaturePair> pairs;
        std::vector<OpenTypeShapingIRGsubLigature> ligatures;
        std::vector<uint16_t> components;


        // ------------------------------------------------------------
        // Compile source subtables in stored order.
        // ------------------------------------------------------------

        for (uint16_t i = 0; i < subtableCount; ++i)
        {
            const ByteSpan data =
                openTypeGsubIREffectiveSubtable(
                    lookup,
                    effectiveType,
                    i);

            if (!data)
            {
                rollback();
                return false;
            }

            if (!compileOpenTypeGsubLigatureSubtable(
                data,
                pairs,
                ligatures,
                components))
            {
                rollback();
                return false;
            }

            if (!appendOpenTypeGsubLigatureSubtable(
                pairs,
                ligatures,
                components,
                ir))
            {
                rollback();
                return false;
            }
        }


        // ------------------------------------------------------------
        // Append semantic lookup.
        // ------------------------------------------------------------

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};

        compiled.op =
            OpenTypeShapingIROp::GsubLigature;

        compiled.payloadOffset =
            payloadOffset;

        compiled.payloadCount =
            subtableCount;

        compiled.filter = filter;

        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back(compiled);

        return true;
    }

    static inline bool compileOpenTypeGsubLigatureLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubLigatureLookup(lookup, gdef, ir, result);
    }

    // ========================================================================
    // GSUB Context
    //
    // LookupType 5 ContextSubst Formats 1, 2, and 3 compile to one semantic
    // representation:
    //
    //     ordered rules
    //         ordered input glyph sets
    //         ordered contextual lookup actions
    //
    // Coverage, ClassDef, source format numbers, and LookupList indices do not
    // survive in the runtime rule representation.
    // ========================================================================



    // ========================================================================
    // Compact raw readers used by GSUB Types 3 and 8.
    // ========================================================================

    static inline bool openTypeGsubIRReadUInt16(
        const ByteSpan& data, size_t offset, uint16_t& result) noexcept
    {
        result = 0;

        if (offset > data.size() || data.size() - offset < 2)
            return false;

        const uint8_t* p = data.begin() + offset;
        result = static_cast<uint16_t>((uint16_t(p[0]) << 8) | uint16_t(p[1]));
        return true;
    }


    // ========================================================================
    // GSUB Alternate
    //
    // AlternateSubst has one source format. The complete ordered AlternateSet
    // is retained in the IR even though the current execution policy selects
    // alternate zero.
    // ========================================================================

    static inline bool compileOpenTypeGsubAlternateSubtable(
        const ByteSpan& data,
        std::vector<OpenTypeShapingIRGsubAlternatePair>& pairs,
        std::vector<OpenTypeShapingIRGsubAlternateSet>& sets,
        std::vector<uint16_t>& glyphs)
    {
        pairs.clear();
        sets.clear();
        glyphs.clear();

        uint16_t format = 0;
        uint16_t coverageOffset = 0;
        uint16_t alternateSetCount = 0;

        if (!openTypeGsubIRReadUInt16(data, 0, format) || format != 1 ||
            !openTypeGsubIRReadUInt16(data, 2, coverageOffset) || coverageOffset == 0 ||
            !openTypeGsubIRReadUInt16(data, 4, alternateSetCount))
        {
            return false;
        }

        const size_t offsetsOffset = 6;

        if (size_t(alternateSetCount) > (data.size() - std::min(data.size(), offsetsOffset)) / 2)
            return false;

        if (coverageOffset >= data.size())
            return false;

        const OpenTypeCoverageView coverage(data.subSpan(coverageOffset));

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= alternateSetCount)
                return false;

            uint16_t setOffset = 0;

            if (!openTypeGsubIRReadUInt16(
                data, offsetsOffset + size_t(coverageIndex) * 2, setOffset) ||
                setOffset == 0 || setOffset >= data.size())
            {
                return false;
            }

            uint16_t glyphCount = 0;

            if (!openTypeGsubIRReadUInt16(data, setOffset, glyphCount))
                return false;

            if (size_t(glyphCount) > (data.size() - setOffset - 2) / 2)
                return false;

            // A legal empty AlternateSet cannot produce a substitution.
            // Omit it so execution naturally continues to later subtables.
            if (glyphCount == 0)
                continue;

            if (glyphs.size() > std::numeric_limits<uint32_t>::max() ||
                sets.size() > std::numeric_limits<uint32_t>::max())
            {
                return false;
            }

            const uint32_t glyphOffset = static_cast<uint32_t>(glyphs.size());
            const uint32_t setIndex = static_cast<uint32_t>(sets.size());

            if (uint64_t(glyphOffset) + glyphCount >
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }

            for (uint16_t i = 0; i < glyphCount; ++i)
            {
                uint16_t alternate = 0;

                if (!openTypeGsubIRReadUInt16(
                    data, size_t(setOffset) + 2 + size_t(i) * 2, alternate))
                {
                    return false;
                }

                glyphs.push_back(alternate);
            }

            sets.push_back({ glyphOffset, glyphCount });
            pairs.push_back({ static_cast<uint16_t>(glyphId), 0, setIndex });
        }

        return true;
    }


    static inline bool appendOpenTypeGsubAlternateSubtable(
        const std::vector<OpenTypeShapingIRGsubAlternatePair>& pairs,
        const std::vector<OpenTypeShapingIRGsubAlternateSet>& sets,
        const std::vector<uint16_t>& glyphs,
        OpenTypeShapingIR& ir)
    {
        const uint64_t max32 = std::numeric_limits<uint32_t>::max();

        if (ir.gsubAlternateGlyphs.size() > max32 ||
            ir.gsubAlternateSets.size() > max32 ||
            ir.gsubAlternatePairs.size() > max32 ||
            ir.gsubAlternateSubtables.size() > max32 ||
            glyphs.size() > max32 || sets.size() > max32 || pairs.size() > max32)
        {
            return false;
        }

        const uint32_t glyphBase = static_cast<uint32_t>(ir.gsubAlternateGlyphs.size());
        const uint32_t setBase = static_cast<uint32_t>(ir.gsubAlternateSets.size());
        const uint32_t pairBase = static_cast<uint32_t>(ir.gsubAlternatePairs.size());

        if (uint64_t(glyphBase) + glyphs.size() > max32 ||
            uint64_t(setBase) + sets.size() > max32 ||
            uint64_t(pairBase) + pairs.size() > max32)
        {
            return false;
        }

        ir.gsubAlternateGlyphs.insert(
            ir.gsubAlternateGlyphs.end(), glyphs.begin(), glyphs.end());

        for (const OpenTypeShapingIRGsubAlternateSet& set : sets)
        {
            ir.gsubAlternateSets.push_back({
                glyphBase + set.glyphOffset,
                set.glyphCount
                });
        }

        for (const OpenTypeShapingIRGsubAlternatePair& pair : pairs)
        {
            ir.gsubAlternatePairs.push_back({
                pair.input,
                0,
                setBase + pair.alternateSetIndex
                });
        }

        ir.gsubAlternateSubtables.push_back({
            pairBase,
            static_cast<uint32_t>(pairs.size())
            });

        return true;
    }


    static inline bool compileOpenTypeGsubAlternateLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        uint16_t effectiveType = 0;

        if (!lookup ||
            !openTypeGsubIREffectiveType(lookup, effectiveType) ||
            effectiveType != 3 || lookup.subtableCount() == 0)
        {
            return false;
        }

        const size_t oldLookupCount = ir.lookups.size();
        const size_t oldGlyphCount = ir.gsubAlternateGlyphs.size();
        const size_t oldSetCount = ir.gsubAlternateSets.size();
        const size_t oldPairCount = ir.gsubAlternatePairs.size();
        const size_t oldSubtableCount = ir.gsubAlternateSubtables.size();
        const size_t oldGlyphRangeCount = ir.glyphRanges.size();
        const size_t oldGlyphSetCount = ir.glyphSets.size();
        const size_t oldGdefGlyphClassCount = ir.gdefGlyphClasses.size();
        const size_t oldGdefMarkAttachClassCount = ir.gdefMarkAttachClasses.size();

        auto rollback = [&]() noexcept
            {
                ir.lookups.resize(oldLookupCount);
                ir.gsubAlternateGlyphs.resize(oldGlyphCount);
                ir.gsubAlternateSets.resize(oldSetCount);
                ir.gsubAlternatePairs.resize(oldPairCount);
                ir.gsubAlternateSubtables.resize(oldSubtableCount);
                ir.glyphRanges.resize(oldGlyphRangeCount);
                ir.glyphSets.resize(oldGlyphSetCount);
                ir.gdefGlyphClasses.resize(oldGdefGlyphClassCount);
                ir.gdefMarkAttachClasses.resize(oldGdefMarkAttachClassCount);
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (oldSubtableCount > std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset = static_cast<uint32_t>(oldSubtableCount);
        std::vector<OpenTypeShapingIRGsubAlternatePair> pairs;
        std::vector<OpenTypeShapingIRGsubAlternateSet> sets;
        std::vector<uint16_t> glyphs;

        for (uint16_t i = 0; i < lookup.subtableCount(); ++i)
        {
            const ByteSpan data = openTypeGsubIREffectiveSubtable(lookup, 3, i);

            if (!data ||
                !compileOpenTypeGsubAlternateSubtable(data, pairs, sets, glyphs) ||
                !appendOpenTypeGsubAlternateSubtable(pairs, sets, glyphs, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() > std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};
        compiled.op = OpenTypeShapingIROp::GsubAlternate;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = lookup.subtableCount();
        compiled.filter = filter;

        result = static_cast<OpenTypeShapingIRLookupId>(ir.lookups.size());
        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGsubAlternateLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubAlternateLookup(lookup, gdef, ir, result);
    }


    // ========================================================================
    // OpenTypeGsubIRCompileCheckpoint
    //
    // Type 5 compilation can recursively compile lookup dependencies. A failed
    // root compilation must therefore roll back every pool a nested lookup may
    // have changed, not only the Type 5 pools.
    // ========================================================================

    struct OpenTypeGsubIRCompileCheckpoint
    {
        size_t glyphRangeCount{ 0 };
        size_t glyphSetCount{ 0 };
        size_t gdefGlyphClassCount{ 0 };
        size_t gdefMarkAttachClassCount{ 0 };

        size_t lookupCount{ 0 };

        size_t singleSubtableCount{ 0 };
        size_t singlePairCount{ 0 };

        size_t multipleGlyphCount{ 0 };
        size_t multipleSequenceCount{ 0 };
        size_t multiplePairCount{ 0 };
        size_t multipleSubtableCount{ 0 };

        size_t alternateGlyphCount{ 0 };
        size_t alternateSetCount{ 0 };
        size_t alternatePairCount{ 0 };
        size_t alternateSubtableCount{ 0 };

        size_t ligatureComponentCount{ 0 };
        size_t ligatureCount{ 0 };
        size_t ligaturePairCount{ 0 };
        size_t ligatureSubtableCount{ 0 };

        size_t contextInputSetCount{ 0 };
        size_t contextLookupCount{ 0 };
        size_t contextRuleCount{ 0 };
        size_t contextSubtableCount{ 0 };

        size_t chainContextSetCount{ 0 };
        size_t chainContextLookupCount{ 0 };
        size_t chainContextRuleCount{ 0 };
        size_t chainContextSubtableCount{ 0 };

        size_t reverseChainSetCount{ 0 };
        size_t reverseChainPairCount{ 0 };
        size_t reverseChainSubtableCount{ 0 };

        explicit OpenTypeGsubIRCompileCheckpoint(
            const OpenTypeShapingIR& ir) noexcept
            : glyphRangeCount(ir.glyphRanges.size()),
            glyphSetCount(ir.glyphSets.size()),
            gdefGlyphClassCount(ir.gdefGlyphClasses.size()),
            gdefMarkAttachClassCount(ir.gdefMarkAttachClasses.size()),
            lookupCount(ir.lookups.size()),
            singleSubtableCount(ir.gsubSingleSubtables.size()),
            singlePairCount(ir.gsubSinglePairs.size()),
            multipleGlyphCount(ir.gsubMultipleGlyphs.size()),
            multipleSequenceCount(ir.gsubMultipleSequences.size()),
            multiplePairCount(ir.gsubMultiplePairs.size()),
            multipleSubtableCount(ir.gsubMultipleSubtables.size()),
            alternateGlyphCount(ir.gsubAlternateGlyphs.size()),
            alternateSetCount(ir.gsubAlternateSets.size()),
            alternatePairCount(ir.gsubAlternatePairs.size()),
            alternateSubtableCount(ir.gsubAlternateSubtables.size()),
            ligatureComponentCount(ir.gsubLigatureComponents.size()),
            ligatureCount(ir.gsubLigatures.size()),
            ligaturePairCount(ir.gsubLigaturePairs.size()),
            ligatureSubtableCount(ir.gsubLigatureSubtables.size()),
            contextInputSetCount(ir.gsubContextInputSets.size()),
            contextLookupCount(ir.gsubContextLookups.size()),
            contextRuleCount(ir.gsubContextRules.size()),
            contextSubtableCount(ir.gsubContextSubtables.size()),
            chainContextSetCount(ir.gsubChainContextSets.size()),
            chainContextLookupCount(ir.gsubChainContextLookups.size()),
            chainContextRuleCount(ir.gsubChainContextRules.size()),
            chainContextSubtableCount(ir.gsubChainContextSubtables.size()),
            reverseChainSetCount(ir.gsubReverseChainSingleSets.size()),
            reverseChainPairCount(ir.gsubReverseChainSinglePairs.size()),
            reverseChainSubtableCount(ir.gsubReverseChainSingleSubtables.size())
        {}

        void rollback(OpenTypeShapingIR& ir) const noexcept
        {
            ir.glyphRanges.resize(glyphRangeCount);
            ir.glyphSets.resize(glyphSetCount);
            ir.gdefGlyphClasses.resize(gdefGlyphClassCount);
            ir.gdefMarkAttachClasses.resize(gdefMarkAttachClassCount);

            ir.lookups.resize(lookupCount);

            ir.gsubSingleSubtables.resize(singleSubtableCount);
            ir.gsubSinglePairs.resize(singlePairCount);

            ir.gsubMultipleGlyphs.resize(multipleGlyphCount);
            ir.gsubMultipleSequences.resize(multipleSequenceCount);
            ir.gsubMultiplePairs.resize(multiplePairCount);
            ir.gsubMultipleSubtables.resize(multipleSubtableCount);

            ir.gsubAlternateGlyphs.resize(alternateGlyphCount);
            ir.gsubAlternateSets.resize(alternateSetCount);
            ir.gsubAlternatePairs.resize(alternatePairCount);
            ir.gsubAlternateSubtables.resize(alternateSubtableCount);

            ir.gsubLigatureComponents.resize(ligatureComponentCount);
            ir.gsubLigatures.resize(ligatureCount);
            ir.gsubLigaturePairs.resize(ligaturePairCount);
            ir.gsubLigatureSubtables.resize(ligatureSubtableCount);

            ir.gsubContextInputSets.resize(contextInputSetCount);
            ir.gsubContextLookups.resize(contextLookupCount);
            ir.gsubContextRules.resize(contextRuleCount);
            ir.gsubContextSubtables.resize(contextSubtableCount);

            ir.gsubChainContextSets.resize(chainContextSetCount);
            ir.gsubChainContextLookups.resize(chainContextLookupCount);
            ir.gsubChainContextRules.resize(chainContextRuleCount);
            ir.gsubChainContextSubtables.resize(chainContextSubtableCount);

            ir.gsubReverseChainSingleSets.resize(reverseChainSetCount);
            ir.gsubReverseChainSinglePairs.resize(reverseChainPairCount);
            ir.gsubReverseChainSingleSubtables.resize(reverseChainSubtableCount);
        }
    };


    // ========================================================================
    // Glyph-set construction helpers
    // ========================================================================

    static inline bool appendOpenTypeShapingIRGlyphSetRanges(
        const std::vector<OpenTypeShapingIRGlyphRange>& ranges,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        const uint64_t max32 = std::numeric_limits<uint32_t>::max();

        if (ir.glyphRanges.size() > max32 ||
            ir.glyphSets.size() > max32 ||
            ranges.size() > max32)
        {
            return false;
        }

        const uint32_t rangeOffset =
            static_cast<uint32_t>(ir.glyphRanges.size());

        const uint32_t rangeCount =
            static_cast<uint32_t>(ranges.size());

        if (uint64_t(rangeOffset) + uint64_t(rangeCount) > max32)
            return false;

        for (size_t i = 0; i < ranges.size(); ++i)
        {
            if (ranges[i].first > ranges[i].last)
                return false;

            if (i != 0 &&
                uint32_t(ranges[i - 1].last) + 1u >=
                uint32_t(ranges[i].first))
            {
                return false;
            }
        }

        result =
            static_cast<OpenTypeShapingIRGlyphSetId>(
                ir.glyphSets.size());

        ir.glyphRanges.insert(
            ir.glyphRanges.end(),
            ranges.begin(),
            ranges.end());

        ir.glyphSets.push_back({
            rangeOffset,
            rangeCount
            });

        return true;
    }


    static inline bool compileOpenTypeShapingIRSingletonGlyphSet(
        uint16_t glyphId, OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId& result)
    {
        const std::vector<OpenTypeShapingIRGlyphRange> ranges{
            { glyphId, glyphId }
        };

        return appendOpenTypeShapingIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeShapingIRCoverageGlyphSet(
        const OpenTypeCoverageView& coverage, OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!coverage)
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;
            const bool member = coverage.find(glyphId, coverageIndex);

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        return appendOpenTypeShapingIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeShapingIRClassGlyphSet(
        const OpenTypeClassDefView& classDef, uint16_t classValue,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!classDef)
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t actualClass = 0;

            if (!classDef.classValue(glyphId, actualClass))
                return false;

            const bool member = actualClass == classValue;

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        return appendOpenTypeShapingIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool compileOpenTypeShapingIRCoverageClassGlyphSet(
        const OpenTypeCoverageView& coverage,
        const OpenTypeClassDefView& classDef, uint16_t classValue,
        OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!coverage || !classDef)
            return false;

        std::vector<OpenTypeShapingIRGlyphRange> ranges;

        bool inRange = false;
        uint16_t rangeFirst = 0;
        uint16_t rangeLast = 0;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;
            uint16_t actualClass = 0;

            if (!classDef.classValue(glyphId, actualClass))
                return false;

            const bool member =
                actualClass == classValue &&
                coverage.find(glyphId, coverageIndex);

            if (member)
            {
                if (!inRange)
                {
                    rangeFirst = static_cast<uint16_t>(glyphId);
                    rangeLast = rangeFirst;
                    inRange = true;
                }
                else
                {
                    rangeLast = static_cast<uint16_t>(glyphId);
                }
            }
            else if (inRange)
            {
                ranges.push_back({ rangeFirst, rangeLast });
                inRange = false;
            }
        }

        if (inRange)
            ranges.push_back({ rangeFirst, rangeLast });

        return appendOpenTypeShapingIRGlyphSetRanges(
            ranges, ir, result);
    }


    static inline bool openTypeShapingIRGlyphSetEmpty(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingIRGlyphSetId id) noexcept
    {
        const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(id);
        return !set || set->rangeCount == 0;
    }


    // ========================================================================
    // GSUB Reverse Chain Single
    // ========================================================================

    struct OpenTypeGsubIRReverseChainTemp
    {
        std::vector<OpenTypeShapingIRGlyphSetId> backtrackSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> lookaheadSets{};
        std::vector<OpenTypeShapingIRGsubReverseChainSinglePair> pairs{};
    };


    static inline bool compileOpenTypeGsubReverseChainSingleSubtable(
        const ByteSpan& data, OpenTypeShapingIR& ir,
        OpenTypeGsubIRReverseChainTemp& result)
    {
        result = {};

        uint16_t format = 0;
        uint16_t coverageOffset = 0;
        uint16_t backtrackCount = 0;

        if (!openTypeGsubIRReadUInt16(data, 0, format) || format != 1 ||
            !openTypeGsubIRReadUInt16(data, 2, coverageOffset) || coverageOffset == 0 ||
            !openTypeGsubIRReadUInt16(data, 4, backtrackCount))
        {
            return false;
        }

        size_t offset = 6;

        if (size_t(backtrackCount) > (data.size() - std::min(data.size(), offset)) / 2)
            return false;

        result.backtrackSets.reserve(backtrackCount);

        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            uint16_t coverage = 0;

            if (!openTypeGsubIRReadUInt16(data, offset + size_t(i) * 2, coverage) ||
                coverage == 0 || coverage >= data.size())
            {
                return false;
            }

            OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageGlyphSet(
                OpenTypeCoverageView(data.subSpan(coverage)), ir, setId))
            {
                return false;
            }

            result.backtrackSets.push_back(setId);
        }

        offset += size_t(backtrackCount) * 2;

        uint16_t lookaheadCount = 0;

        if (!openTypeGsubIRReadUInt16(data, offset, lookaheadCount))
            return false;

        offset += 2;

        if (size_t(lookaheadCount) > (data.size() - std::min(data.size(), offset)) / 2)
            return false;

        result.lookaheadSets.reserve(lookaheadCount);

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            uint16_t coverage = 0;

            if (!openTypeGsubIRReadUInt16(data, offset + size_t(i) * 2, coverage) ||
                coverage == 0 || coverage >= data.size())
            {
                return false;
            }

            OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageGlyphSet(
                OpenTypeCoverageView(data.subSpan(coverage)), ir, setId))
            {
                return false;
            }

            result.lookaheadSets.push_back(setId);
        }

        offset += size_t(lookaheadCount) * 2;

        uint16_t glyphCount = 0;

        if (!openTypeGsubIRReadUInt16(data, offset, glyphCount))
            return false;

        offset += 2;

        if (size_t(glyphCount) > (data.size() - std::min(data.size(), offset)) / 2 ||
            coverageOffset >= data.size())
        {
            return false;
        }

        const OpenTypeCoverageView inputCoverage(data.subSpan(coverageOffset));

        if (!inputCoverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!inputCoverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= glyphCount)
                return false;

            uint16_t output = 0;

            if (!openTypeGsubIRReadUInt16(
                data, offset + size_t(coverageIndex) * 2, output))
            {
                return false;
            }

            result.pairs.push_back({ static_cast<uint16_t>(glyphId), output });
        }

        return true;
    }


    static inline bool appendOpenTypeGsubReverseChainSingleSubtable(
        const OpenTypeGsubIRReverseChainTemp& source,
        OpenTypeShapingIR& ir)
    {
        const uint64_t max32 = std::numeric_limits<uint32_t>::max();

        if (ir.gsubReverseChainSingleSets.size() > max32 ||
            ir.gsubReverseChainSinglePairs.size() > max32 ||
            ir.gsubReverseChainSingleSubtables.size() > max32 ||
            source.backtrackSets.size() > max32 ||
            source.lookaheadSets.size() > max32 ||
            source.pairs.size() > max32)
        {
            return false;
        }

        const uint32_t backtrackOffset =
            static_cast<uint32_t>(ir.gsubReverseChainSingleSets.size());

        if (uint64_t(backtrackOffset) + source.backtrackSets.size() > max32)
            return false;

        ir.gsubReverseChainSingleSets.insert(
            ir.gsubReverseChainSingleSets.end(),
            source.backtrackSets.begin(), source.backtrackSets.end());

        const uint32_t lookaheadOffset =
            static_cast<uint32_t>(ir.gsubReverseChainSingleSets.size());

        if (uint64_t(lookaheadOffset) + source.lookaheadSets.size() > max32)
            return false;

        ir.gsubReverseChainSingleSets.insert(
            ir.gsubReverseChainSingleSets.end(),
            source.lookaheadSets.begin(), source.lookaheadSets.end());

        const uint32_t pairOffset =
            static_cast<uint32_t>(ir.gsubReverseChainSinglePairs.size());

        if (uint64_t(pairOffset) + source.pairs.size() > max32)
            return false;

        ir.gsubReverseChainSinglePairs.insert(
            ir.gsubReverseChainSinglePairs.end(),
            source.pairs.begin(), source.pairs.end());

        ir.gsubReverseChainSingleSubtables.push_back({
            backtrackOffset,
            static_cast<uint32_t>(source.backtrackSets.size()),
            lookaheadOffset,
            static_cast<uint32_t>(source.lookaheadSets.size()),
            pairOffset,
            static_cast<uint32_t>(source.pairs.size())
            });

        return true;
    }


    static inline bool compileOpenTypeGsubReverseChainSingleLookup(
        const OpenTypeLayoutLookupView& lookup, const OpenTypeGdefView& gdef,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        uint16_t effectiveType = 0;

        if (!lookup ||
            !openTypeGsubIREffectiveType(lookup, effectiveType) ||
            effectiveType != 8 || lookup.subtableCount() == 0)
        {
            return false;
        }

        const OpenTypeGsubIRCompileCheckpoint checkpoint(ir);

        auto rollback = [&]() noexcept
            {
                checkpoint.rollback(ir);
                result = kOpenTypeShapingIRInvalid;
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        if (ir.gsubReverseChainSingleSubtables.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        const uint32_t payloadOffset =
            static_cast<uint32_t>(ir.gsubReverseChainSingleSubtables.size());

        OpenTypeGsubIRReverseChainTemp subtable;

        for (uint16_t i = 0; i < lookup.subtableCount(); ++i)
        {
            const ByteSpan data = openTypeGsubIREffectiveSubtable(lookup, 8, i);

            if (!data ||
                !compileOpenTypeGsubReverseChainSingleSubtable(data, ir, subtable) ||
                !appendOpenTypeGsubReverseChainSingleSubtable(subtable, ir))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() > std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};
        compiled.op = OpenTypeShapingIROp::GsubReverseChainSingle;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = lookup.subtableCount();
        compiled.filter = filter;

        result = static_cast<OpenTypeShapingIRLookupId>(ir.lookups.size());
        ir.lookups.push_back(compiled);
        return true;
    }


    static inline bool compileOpenTypeGsubReverseChainSingleLookup(
        const OpenTypeLayoutLookupView& lookup,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubReverseChainSingleLookup(lookup, gdef, ir, result);
    }


    // ========================================================================
    // Temporary Type 5 compiler representation
    //
    // Context rules are first decoded without resolving nested lookup
    // references. That is important: resolving a nested Type 5 lookup may
    // append its own context pools, so the root lookup's subtables are committed
    // only after all dependencies have been compiled.
    // ========================================================================

    struct OpenTypeGsubIRContextRuleTemp
    {
        std::vector<OpenTypeShapingIRGlyphSetId> inputSets{};
        std::vector<OpenTypeSequenceLookup> sourceLookups{};
        std::vector<OpenTypeShapingIRSequenceLookup> lookups{};
    };


    struct OpenTypeGsubIRContextSubtableTemp
    {
        std::vector<OpenTypeGsubIRContextRuleTemp> rules{};
    };


    // ========================================================================
    // Recursive lookup compiler state
    //
    // Contextual lookups refer back into the source LookupList. These arrays
    // map source LookupList indices to stable compiled lookup IDs.
    //
    // A contextual lookup reserves its IR lookup slot before resolving actions.
    // Therefore self-reference and mutual Type 5/6 recursion can retain stable
    // lookup IDs. Runtime recursion limits remain the executor's responsibility.
    // ========================================================================

    enum class OpenTypeGsubIRLookupCompileState : uint8_t
    {
        Unseen = 0,
        Compiling,
        Done
    };


    struct OpenTypeGsubIRCompilerContext
    {
        std::vector<OpenTypeGsubIRLookupCompileState> states{};
        std::vector<OpenTypeShapingIRLookupId> lookupIds{};

        bool reset(size_t lookupCount)
        {
            states.assign(
                lookupCount,
                OpenTypeGsubIRLookupCompileState::Unseen);

            lookupIds.assign(
                lookupCount,
                kOpenTypeShapingIRInvalid);

            return true;
        }
    };


    static inline bool compileOpenTypeGsubIRLookupByIndex(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGsubIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result);


    // ========================================================================
    // Type 5 Format 1
    // ========================================================================

    static inline bool compileOpenTypeGsubContextFormat1(
        const OpenTypeGsubContextSubstView& subst,
        OpenTypeShapingIR& ir,
        OpenTypeGsubIRContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!subst || subst.format() != 1)
            return false;

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= subst.ruleSetCount())
                continue;

            uint16_t ruleSetOffset = 0;

            if (!subst.ruleSetOffset(coverageIndex, ruleSetOffset))
                return false;

            if (ruleSetOffset == 0)
                continue;

            const OpenTypeGsubContextRuleSetView set =
                subst.ruleSet(coverageIndex);

            if (!set)
                return false;

            OpenTypeShapingIRGlyphSetId firstSet =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRSingletonGlyphSet(
                static_cast<uint16_t>(glyphId), ir, firstSet))
            {
                return false;
            }

            for (uint16_t ruleIndex = 0;
                ruleIndex < set.size();
                ++ruleIndex)
            {
                const OpenTypeGsubContextRuleView rule =
                    set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t glyphCount = rule.glyphCount();

                if (glyphCount == 0)
                    return false;

                OpenTypeGsubIRContextRuleTemp compiled;
                compiled.inputSets.reserve(glyphCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t sequenceIndex = 1;
                    sequenceIndex < glyphCount;
                    ++sequenceIndex)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.inputGlyphId(
                        sequenceIndex - 1, expectedGlyph))
                    {
                        return false;
                    }

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeShapingIRSingletonGlyphSet(
                        expectedGlyph, ir, setId))
                    {
                        return false;
                    }

                    compiled.inputSets.push_back(setId);
                }

                const uint16_t lookupCount =
                    rule.sequenceLookupCount();

                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t actionIndex = 0;
                    actionIndex < lookupCount;
                    ++actionIndex)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(
                        actionIndex, action))
                    {
                        return false;
                    }

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    // ========================================================================
    // Type 5 Format 2
    // ========================================================================

    static inline bool compileOpenTypeGsubContextFormat2(
        const OpenTypeGsubContextSubstView& subst,
        OpenTypeShapingIR& ir,
        OpenTypeGsubIRContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!subst || subst.format() != 2)
            return false;

        const OpenTypeCoverageView coverage = subst.coverage();
        const OpenTypeClassDefView classDef = subst.classDef();

        if (!coverage || !classDef)
            return false;

        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>
            classSetCache;

        auto classGlyphSet =
            [&](uint16_t classValue,
                OpenTypeShapingIRGlyphSetId& setId) -> bool
            {
                for (const auto& entry : classSetCache)
                {
                    if (entry.first == classValue)
                    {
                        setId = entry.second;
                        return true;
                    }
                }

                if (!compileOpenTypeShapingIRClassGlyphSet(
                    classDef, classValue, ir, setId))
                {
                    return false;
                }

                classSetCache.push_back({
                    classValue,
                    setId
                    });

                return true;
            };

        const uint16_t classSetCount = subst.classSetCount();

        for (uint16_t firstClass = 0;
            firstClass < classSetCount;
            ++firstClass)
        {
            uint16_t classSetOffset = 0;

            if (!subst.classSetOffset(
                firstClass, classSetOffset))
            {
                return false;
            }

            if (classSetOffset == 0)
                continue;

            OpenTypeShapingIRGlyphSetId firstSet =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageClassGlyphSet(
                coverage, classDef, firstClass, ir, firstSet))
            {
                return false;
            }

            // No covered glyph can select this ClassSet. It is unreachable.
            if (openTypeShapingIRGlyphSetEmpty(ir, firstSet))
                continue;

            const OpenTypeGsubContextClassSetView set =
                subst.classSet(firstClass);

            if (!set)
                return false;

            for (uint16_t ruleIndex = 0;
                ruleIndex < set.size();
                ++ruleIndex)
            {
                const OpenTypeGsubContextClassRuleView rule =
                    set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t glyphCount = rule.glyphCount();

                if (glyphCount == 0)
                    return false;

                OpenTypeGsubIRContextRuleTemp compiled;
                compiled.inputSets.reserve(glyphCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t sequenceIndex = 1;
                    sequenceIndex < glyphCount;
                    ++sequenceIndex)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.inputClass(
                        sequenceIndex - 1,
                        expectedClass))
                    {
                        return false;
                    }

                    OpenTypeShapingIRGlyphSetId setId =
                        kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(
                        expectedClass, setId))
                    {
                        return false;
                    }

                    compiled.inputSets.push_back(setId);
                }

                const uint16_t lookupCount =
                    rule.sequenceLookupCount();

                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t actionIndex = 0;
                    actionIndex < lookupCount;
                    ++actionIndex)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(
                        actionIndex, action))
                    {
                        return false;
                    }

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    // ========================================================================
    // Type 5 Format 3
    // ========================================================================

    static inline bool compileOpenTypeGsubContextFormat3(
        const OpenTypeGsubContextSubstView& subst,
        OpenTypeShapingIR& ir,
        OpenTypeGsubIRContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!subst || subst.format() != 3)
            return false;

        const uint16_t glyphCount = subst.glyphCount();

        if (glyphCount == 0)
            return false;

        OpenTypeGsubIRContextRuleTemp compiled;
        compiled.inputSets.reserve(glyphCount);

        for (uint16_t sequenceIndex = 0;
            sequenceIndex < glyphCount;
            ++sequenceIndex)
        {
            const OpenTypeCoverageView coverage =
                subst.inputCoverage(sequenceIndex);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId =
                kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageGlyphSet(
                coverage, ir, setId))
            {
                return false;
            }

            compiled.inputSets.push_back(setId);
        }

        const uint16_t lookupCount =
            subst.sequenceLookupCount();

        compiled.sourceLookups.reserve(lookupCount);

        for (uint16_t actionIndex = 0;
            actionIndex < lookupCount;
            ++actionIndex)
        {
            OpenTypeSequenceLookup action{};

            if (!subst.sequenceLookup(
                actionIndex, action))
            {
                return false;
            }

            compiled.sourceLookups.push_back(action);
        }

        result.rules.push_back(std::move(compiled));
        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubContextSubtable
    // ========================================================================

    static inline bool compileOpenTypeGsubContextSubtable(
        const ByteSpan& data, OpenTypeShapingIR& ir,
        OpenTypeGsubIRContextSubtableTemp& result)
    {
        result.rules.clear();

        const OpenTypeGsubContextSubstView subst(data);

        if (!subst)
            return false;

        switch (subst.format())
        {
        case 1:
            return compileOpenTypeGsubContextFormat1(
                subst, ir, result);

        case 2:
            return compileOpenTypeGsubContextFormat2(
                subst, ir, result);

        case 3:
            return compileOpenTypeGsubContextFormat3(
                subst, ir, result);

        default:
            return false;
        }
    }


    // ========================================================================
    // resolveOpenTypeGsubIRContextActions
    //
    // Convert source LookupList indices to stable compiled lookup IDs.
    // ========================================================================

    static inline bool resolveOpenTypeGsubIRContextActions(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGsubIRCompilerContext& context,
        std::vector<OpenTypeGsubIRContextSubtableTemp>& subtables)
    {
        for (OpenTypeGsubIRContextSubtableTemp& subtable : subtables)
        {
            for (OpenTypeGsubIRContextRuleTemp& rule : subtable.rules)
            {
                rule.lookups.clear();
                rule.lookups.reserve(rule.sourceLookups.size());

                for (const OpenTypeSequenceLookup& action :
                    rule.sourceLookups)
                {
                    OpenTypeShapingIRLookupId lookupId =
                        kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGsubIRLookupByIndex(
                        lookups,
                        action.lookupListIndex,
                        gdef,
                        ir,
                        context,
                        lookupId))
                    {
                        return false;
                    }

                    rule.lookups.push_back({
                        action.sequenceIndex,
                        lookupId
                        });
                }
            }
        }

        return true;
    }


    // ========================================================================
    // appendOpenTypeGsubContextSubtables
    //
    // Commit all subtables belonging to one Type 5 lookup as one contiguous
    // payload. Nested dependencies have already been compiled at this point.
    // ========================================================================

    static inline bool appendOpenTypeGsubContextSubtables(
        const std::vector<OpenTypeGsubIRContextSubtableTemp>& subtables,
        OpenTypeShapingIR& ir, uint32_t& payloadOffset)
    {
        payloadOffset = 0;

        const uint64_t max32 =
            std::numeric_limits<uint32_t>::max();

        if (ir.gsubContextSubtables.size() > max32 ||
            ir.gsubContextRules.size() > max32 ||
            ir.gsubContextInputSets.size() > max32 ||
            ir.gsubContextLookups.size() > max32 ||
            subtables.size() > max32)
        {
            return false;
        }

        payloadOffset =
            static_cast<uint32_t>(
                ir.gsubContextSubtables.size());

        if (uint64_t(payloadOffset) + subtables.size() > max32)
            return false;

        for (const OpenTypeGsubIRContextSubtableTemp& subtable :
            subtables)
        {
            if (ir.gsubContextRules.size() > max32 ||
                subtable.rules.size() > max32)
            {
                return false;
            }

            const uint32_t ruleOffset =
                static_cast<uint32_t>(
                    ir.gsubContextRules.size());

            if (uint64_t(ruleOffset) + subtable.rules.size() > max32)
                return false;

            for (const OpenTypeGsubIRContextRuleTemp& rule :
                subtable.rules)
            {
                if (rule.inputSets.empty())
                    return false;

                if (ir.gsubContextInputSets.size() > max32 ||
                    ir.gsubContextLookups.size() > max32 ||
                    rule.inputSets.size() > max32 ||
                    rule.lookups.size() > max32)
                {
                    return false;
                }

                const uint32_t inputSetOffset =
                    static_cast<uint32_t>(
                        ir.gsubContextInputSets.size());

                const uint32_t inputCount =
                    static_cast<uint32_t>(
                        rule.inputSets.size());

                const uint32_t lookupOffset =
                    static_cast<uint32_t>(
                        ir.gsubContextLookups.size());

                const uint32_t lookupCount =
                    static_cast<uint32_t>(
                        rule.lookups.size());

                if (uint64_t(inputSetOffset) + inputCount > max32 ||
                    uint64_t(lookupOffset) + lookupCount > max32)
                {
                    return false;
                }

                for (OpenTypeShapingIRGlyphSetId setId :
                rule.inputSets)
                {
                    if (!ir.glyphSet(setId))
                        return false;
                }

                for (const OpenTypeShapingIRSequenceLookup& action :
                    rule.lookups)
                {
                    if (action.lookup == kOpenTypeShapingIRInvalid ||
                        action.lookup >= ir.lookups.size())
                    {
                        return false;
                    }
                }

                ir.gsubContextInputSets.insert(
                    ir.gsubContextInputSets.end(),
                    rule.inputSets.begin(),
                    rule.inputSets.end());

                ir.gsubContextLookups.insert(
                    ir.gsubContextLookups.end(),
                    rule.lookups.begin(),
                    rule.lookups.end());

                ir.gsubContextRules.push_back({
                    inputSetOffset,
                    inputCount,
                    lookupOffset,
                    lookupCount
                    });
            }

            ir.gsubContextSubtables.push_back({
                ruleOffset,
                static_cast<uint32_t>(
                    subtable.rules.size())
                });
        }

        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubContextLookupInternal
    //
    // Compile one source LookupList entry whose effective type is 5.
    //
    // A placeholder lookup is reserved before nested action resolution so
    // recursive Type 5 references can resolve to a stable IR lookup ID.
    // ========================================================================

    static inline bool compileOpenTypeGsubContextLookupInternal(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGsubIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || !lookup ||
            lookupIndex >= lookups.size())
        {
            return false;
        }

        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(
            lookup, effectiveType) ||
            effectiveType != 5)
        {
            return false;
        }

        const uint16_t subtableCount =
            lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const OpenTypeGsubIRCompileCheckpoint checkpoint(ir);

        auto rollback = [&]() noexcept
            {
                checkpoint.rollback(ir);
                result = kOpenTypeShapingIRInvalid;

                if (lookupIndex < context.lookupIds.size())
                    context.lookupIds[lookupIndex] =
                    kOpenTypeShapingIRInvalid;
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(
            lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        std::vector<OpenTypeGsubIRContextSubtableTemp>
            subtables;

        subtables.resize(subtableCount);

        for (uint16_t subtableIndex = 0;
            subtableIndex < subtableCount;
            ++subtableIndex)
        {
            const ByteSpan data =
                openTypeGsubIREffectiveSubtable(
                    lookup, effectiveType, subtableIndex);

            if (!data ||
                !compileOpenTypeGsubContextSubtable(
                    data, ir, subtables[subtableIndex]))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() >
            std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        // Reserve the lookup ID before resolving nested actions.
        result =
            static_cast<OpenTypeShapingIRLookupId>(
                ir.lookups.size());

        ir.lookups.push_back({});

        if (lookupIndex >= context.lookupIds.size())
        {
            rollback();
            return false;
        }

        context.lookupIds[lookupIndex] = result;

        if (!resolveOpenTypeGsubIRContextActions(
            lookups, gdef, ir, context, subtables))
        {
            rollback();
            return false;
        }

        uint32_t payloadOffset = 0;

        if (!appendOpenTypeGsubContextSubtables(
            subtables, ir, payloadOffset))
        {
            rollback();
            return false;
        }

        if (result >= ir.lookups.size())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};
        compiled.op = OpenTypeShapingIROp::GsubContext;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = subtableCount;
        compiled.filter = filter;

        ir.lookups[result] = compiled;
        return true;
    }


    // ========================================================================
    // GSUB Chain Context
    //
    // LookupType 6 ChainContextSubst Formats 1, 2, and 3 compile to one
    // semantic representation:
    //
    //     ordered rules
    //         backtrack glyph sets, nearest-first
    //         input glyph sets, current-first
    //         lookahead glyph sets, nearest-first
    //         ordered contextual lookup actions
    //
    // Backtrack and lookahead are match-only constraints. Only the input
    // sequence participates in SequenceLookup execution.
    // ========================================================================

    struct OpenTypeGsubIRChainContextRuleTemp
    {
        std::vector<OpenTypeShapingIRGlyphSetId> backtrackSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> inputSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> lookaheadSets{};
        std::vector<OpenTypeSequenceLookup> sourceLookups{};
        std::vector<OpenTypeShapingIRSequenceLookup> lookups{};
    };


    struct OpenTypeGsubIRChainContextSubtableTemp
    {
        std::vector<OpenTypeGsubIRChainContextRuleTemp> rules{};
    };


    // ========================================================================
    // Type 6 Format 1
    // ========================================================================

    static inline bool compileOpenTypeGsubChainContextFormat1(
        const OpenTypeGsubChainContextSubstView& subst,
        OpenTypeShapingIR& ir, OpenTypeGsubIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!subst || subst.format() != 1)
            return false;

        const OpenTypeCoverageView coverage = subst.coverage();

        if (!coverage)
            return false;

        for (uint32_t glyphId = 0; glyphId <= 0xFFFFu; ++glyphId)
        {
            uint16_t coverageIndex = 0;

            if (!coverage.find(glyphId, coverageIndex))
                continue;

            if (coverageIndex >= subst.ruleSetCount())
                continue;

            uint16_t ruleSetOffset = 0;

            if (!subst.ruleSetOffset(coverageIndex, ruleSetOffset))
                return false;

            if (ruleSetOffset == 0)
                continue;

            const OpenTypeGsubChainContextRuleSetView set = subst.ruleSet(coverageIndex);

            if (!set)
                return false;

            OpenTypeShapingIRGlyphSetId firstSet = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRSingletonGlyphSet(
                static_cast<uint16_t>(glyphId), ir, firstSet))
            {
                return false;
            }

            for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
            {
                const OpenTypeGsubChainContextRuleView rule = set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t backtrackCount = rule.backtrackGlyphCount();
                const uint16_t inputCount = rule.inputGlyphCount();
                const uint16_t lookaheadCount = rule.lookaheadGlyphCount();

                if (inputCount == 0)
                    return false;

                OpenTypeGsubIRChainContextRuleTemp compiled;
                compiled.backtrackSets.reserve(backtrackCount);
                compiled.inputSets.reserve(inputCount);
                compiled.lookaheadSets.reserve(lookaheadCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t i = 0; i < backtrackCount; ++i)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.backtrackGlyphId(i, expectedGlyph))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeShapingIRSingletonGlyphSet(expectedGlyph, ir, setId))
                        return false;

                    compiled.backtrackSets.push_back(setId);
                }

                for (uint16_t i = 1; i < inputCount; ++i)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.inputGlyphId(i - 1, expectedGlyph))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeShapingIRSingletonGlyphSet(expectedGlyph, ir, setId))
                        return false;

                    compiled.inputSets.push_back(setId);
                }

                for (uint16_t i = 0; i < lookaheadCount; ++i)
                {
                    uint16_t expectedGlyph = 0;

                    if (!rule.lookaheadGlyphId(i, expectedGlyph))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeShapingIRSingletonGlyphSet(expectedGlyph, ir, setId))
                        return false;

                    compiled.lookaheadSets.push_back(setId);
                }

                const uint16_t lookupCount = rule.sequenceLookupCount();
                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t i = 0; i < lookupCount; ++i)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(i, action))
                        return false;

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    // ========================================================================
    // Type 6 Format 2
    // ========================================================================

    static inline bool compileOpenTypeGsubChainContextFormat2(
        const OpenTypeGsubChainContextSubstView& subst,
        OpenTypeShapingIR& ir, OpenTypeGsubIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!subst || subst.format() != 2)
            return false;

        const OpenTypeCoverageView coverage = subst.coverage();
        const OpenTypeClassDefView backtrackClassDef = subst.backtrackClassDef();
        const OpenTypeClassDefView inputClassDef = subst.inputClassDef();
        const OpenTypeClassDefView lookaheadClassDef = subst.lookaheadClassDef();

        if (!coverage || !backtrackClassDef || !inputClassDef || !lookaheadClassDef)
            return false;

        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>> backtrackCache;
        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>> inputCache;
        std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>> lookaheadCache;

        auto classGlyphSet = [&](const OpenTypeClassDefView& classDef,
            std::vector<std::pair<uint16_t, OpenTypeShapingIRGlyphSetId>>& cache,
            uint16_t classValue, OpenTypeShapingIRGlyphSetId& setId) -> bool
            {
                for (const auto& entry : cache)
                {
                    if (entry.first == classValue)
                    {
                        setId = entry.second;
                        return true;
                    }
                }

                if (!compileOpenTypeShapingIRClassGlyphSet(classDef, classValue, ir, setId))
                    return false;

                cache.push_back({ classValue, setId });
                return true;
            };

        const uint16_t classSetCount = subst.classSetCount();

        for (uint16_t firstClass = 0; firstClass < classSetCount; ++firstClass)
        {
            uint16_t classSetOffset = 0;

            if (!subst.classSetOffset(firstClass, classSetOffset))
                return false;

            if (classSetOffset == 0)
                continue;

            OpenTypeShapingIRGlyphSetId firstSet = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageClassGlyphSet(
                coverage, inputClassDef, firstClass, ir, firstSet))
            {
                return false;
            }

            if (openTypeShapingIRGlyphSetEmpty(ir, firstSet))
                continue;

            const OpenTypeGsubChainContextClassSetView set = subst.classSet(firstClass);

            if (!set)
                return false;

            for (uint16_t ruleIndex = 0; ruleIndex < set.size(); ++ruleIndex)
            {
                const OpenTypeGsubChainContextClassRuleView rule = set.rule(ruleIndex);

                if (!rule)
                    return false;

                const uint16_t backtrackCount = rule.backtrackGlyphCount();
                const uint16_t inputCount = rule.inputGlyphCount();
                const uint16_t lookaheadCount = rule.lookaheadGlyphCount();

                if (inputCount == 0)
                    return false;

                OpenTypeGsubIRChainContextRuleTemp compiled;
                compiled.backtrackSets.reserve(backtrackCount);
                compiled.inputSets.reserve(inputCount);
                compiled.lookaheadSets.reserve(lookaheadCount);
                compiled.inputSets.push_back(firstSet);

                for (uint16_t i = 0; i < backtrackCount; ++i)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.backtrackClass(i, expectedClass))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(backtrackClassDef, backtrackCache, expectedClass, setId))
                        return false;

                    compiled.backtrackSets.push_back(setId);
                }

                for (uint16_t i = 1; i < inputCount; ++i)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.inputClass(i - 1, expectedClass))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(inputClassDef, inputCache, expectedClass, setId))
                        return false;

                    compiled.inputSets.push_back(setId);
                }

                for (uint16_t i = 0; i < lookaheadCount; ++i)
                {
                    uint16_t expectedClass = 0;

                    if (!rule.lookaheadClass(i, expectedClass))
                        return false;

                    OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

                    if (!classGlyphSet(lookaheadClassDef, lookaheadCache, expectedClass, setId))
                        return false;

                    compiled.lookaheadSets.push_back(setId);
                }

                const uint16_t lookupCount = rule.sequenceLookupCount();
                compiled.sourceLookups.reserve(lookupCount);

                for (uint16_t i = 0; i < lookupCount; ++i)
                {
                    OpenTypeSequenceLookup action{};

                    if (!rule.sequenceLookup(i, action))
                        return false;

                    compiled.sourceLookups.push_back(action);
                }

                result.rules.push_back(std::move(compiled));
            }
        }

        return true;
    }


    // ========================================================================
    // Type 6 Format 3
    // ========================================================================

    static inline bool compileOpenTypeGsubChainContextFormat3(
        const OpenTypeGsubChainContextSubstView& subst,
        OpenTypeShapingIR& ir, OpenTypeGsubIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        if (!subst || subst.format() != 3)
            return false;

        const uint16_t backtrackCount = subst.backtrackGlyphCount();
        const uint16_t inputCount = subst.inputGlyphCount();
        const uint16_t lookaheadCount = subst.lookaheadGlyphCount();

        if (inputCount == 0)
            return false;

        OpenTypeGsubIRChainContextRuleTemp compiled;
        compiled.backtrackSets.reserve(backtrackCount);
        compiled.inputSets.reserve(inputCount);
        compiled.lookaheadSets.reserve(lookaheadCount);

        for (uint16_t i = 0; i < backtrackCount; ++i)
        {
            const OpenTypeCoverageView coverage = subst.backtrackCoverage(i);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageGlyphSet(coverage, ir, setId))
                return false;

            compiled.backtrackSets.push_back(setId);
        }

        for (uint16_t i = 0; i < inputCount; ++i)
        {
            const OpenTypeCoverageView coverage = subst.inputCoverage(i);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageGlyphSet(coverage, ir, setId))
                return false;

            compiled.inputSets.push_back(setId);
        }

        for (uint16_t i = 0; i < lookaheadCount; ++i)
        {
            const OpenTypeCoverageView coverage = subst.lookaheadCoverage(i);

            if (!coverage)
                return false;

            OpenTypeShapingIRGlyphSetId setId = kOpenTypeShapingIRInvalid;

            if (!compileOpenTypeShapingIRCoverageGlyphSet(coverage, ir, setId))
                return false;

            compiled.lookaheadSets.push_back(setId);
        }

        const uint16_t lookupCount = subst.sequenceLookupCount();
        compiled.sourceLookups.reserve(lookupCount);

        for (uint16_t i = 0; i < lookupCount; ++i)
        {
            OpenTypeSequenceLookup action{};

            if (!subst.sequenceLookup(i, action))
                return false;

            compiled.sourceLookups.push_back(action);
        }

        result.rules.push_back(std::move(compiled));
        return true;
    }


    static inline bool compileOpenTypeGsubChainContextSubtable(
        const ByteSpan& data, OpenTypeShapingIR& ir,
        OpenTypeGsubIRChainContextSubtableTemp& result)
    {
        result.rules.clear();

        const OpenTypeGsubChainContextSubstView subst(data);

        if (!subst)
            return false;

        switch (subst.format())
        {
        case 1:
            return compileOpenTypeGsubChainContextFormat1(subst, ir, result);
        case 2:
            return compileOpenTypeGsubChainContextFormat2(subst, ir, result);
        case 3:
            return compileOpenTypeGsubChainContextFormat3(subst, ir, result);
        default:
            return false;
        }
    }


    static inline bool resolveOpenTypeGsubIRChainContextActions(
        const OpenTypeLayoutLookupListView& lookups,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGsubIRCompilerContext& context,
        std::vector<OpenTypeGsubIRChainContextSubtableTemp>& subtables)
    {
        for (OpenTypeGsubIRChainContextSubtableTemp& subtable : subtables)
        {
            for (OpenTypeGsubIRChainContextRuleTemp& rule : subtable.rules)
            {
                rule.lookups.clear();
                rule.lookups.reserve(rule.sourceLookups.size());

                for (const OpenTypeSequenceLookup& action : rule.sourceLookups)
                {
                    OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;

                    if (!compileOpenTypeGsubIRLookupByIndex(
                        lookups, action.lookupListIndex, gdef, ir, context, lookupId))
                    {
                        return false;
                    }

                    rule.lookups.push_back({ action.sequenceIndex, lookupId });
                }
            }
        }

        return true;
    }


    static inline bool appendOpenTypeGsubChainContextSubtables(
        const std::vector<OpenTypeGsubIRChainContextSubtableTemp>& subtables,
        OpenTypeShapingIR& ir, uint32_t& payloadOffset)
    {
        payloadOffset = 0;

        const uint64_t max32 = std::numeric_limits<uint32_t>::max();

        if (ir.gsubChainContextSubtables.size() > max32 ||
            ir.gsubChainContextRules.size() > max32 ||
            ir.gsubChainContextSets.size() > max32 ||
            ir.gsubChainContextLookups.size() > max32 ||
            subtables.size() > max32)
        {
            return false;
        }

        payloadOffset = static_cast<uint32_t>(ir.gsubChainContextSubtables.size());

        if (uint64_t(payloadOffset) + subtables.size() > max32)
            return false;

        for (const OpenTypeGsubIRChainContextSubtableTemp& subtable : subtables)
        {
            if (ir.gsubChainContextRules.size() > max32 || subtable.rules.size() > max32)
                return false;

            const uint32_t ruleOffset = static_cast<uint32_t>(ir.gsubChainContextRules.size());

            if (uint64_t(ruleOffset) + subtable.rules.size() > max32)
                return false;

            for (const OpenTypeGsubIRChainContextRuleTemp& rule : subtable.rules)
            {
                if (rule.inputSets.empty())
                    return false;

                for (OpenTypeShapingIRGlyphSetId setId : rule.backtrackSets)
                    if (!ir.glyphSet(setId)) return false;
                for (OpenTypeShapingIRGlyphSetId setId : rule.inputSets)
                    if (!ir.glyphSet(setId)) return false;
                for (OpenTypeShapingIRGlyphSetId setId : rule.lookaheadSets)
                    if (!ir.glyphSet(setId)) return false;

                if (ir.gsubChainContextSets.size() > max32 ||
                    ir.gsubChainContextLookups.size() > max32 ||
                    rule.backtrackSets.size() > max32 ||
                    rule.inputSets.size() > max32 ||
                    rule.lookaheadSets.size() > max32 ||
                    rule.lookups.size() > max32)
                {
                    return false;
                }

                const uint32_t backtrackSetOffset = static_cast<uint32_t>(ir.gsubChainContextSets.size());
                const uint32_t backtrackCount = static_cast<uint32_t>(rule.backtrackSets.size());
                if (uint64_t(backtrackSetOffset) + backtrackCount > max32) return false;
                ir.gsubChainContextSets.insert(ir.gsubChainContextSets.end(), rule.backtrackSets.begin(), rule.backtrackSets.end());

                const uint32_t inputSetOffset = static_cast<uint32_t>(ir.gsubChainContextSets.size());
                const uint32_t inputCount = static_cast<uint32_t>(rule.inputSets.size());
                if (uint64_t(inputSetOffset) + inputCount > max32) return false;
                ir.gsubChainContextSets.insert(ir.gsubChainContextSets.end(), rule.inputSets.begin(), rule.inputSets.end());

                const uint32_t lookaheadSetOffset = static_cast<uint32_t>(ir.gsubChainContextSets.size());
                const uint32_t lookaheadCount = static_cast<uint32_t>(rule.lookaheadSets.size());
                if (uint64_t(lookaheadSetOffset) + lookaheadCount > max32) return false;
                ir.gsubChainContextSets.insert(ir.gsubChainContextSets.end(), rule.lookaheadSets.begin(), rule.lookaheadSets.end());

                const uint32_t lookupOffset = static_cast<uint32_t>(ir.gsubChainContextLookups.size());
                const uint32_t lookupCount = static_cast<uint32_t>(rule.lookups.size());
                if (uint64_t(lookupOffset) + lookupCount > max32) return false;

                for (const OpenTypeShapingIRSequenceLookup& action : rule.lookups)
                {
                    if (action.lookup == kOpenTypeShapingIRInvalid || action.lookup >= ir.lookups.size())
                        return false;
                }

                ir.gsubChainContextLookups.insert(
                    ir.gsubChainContextLookups.end(), rule.lookups.begin(), rule.lookups.end());

                ir.gsubChainContextRules.push_back({
                    backtrackSetOffset, backtrackCount,
                    inputSetOffset, inputCount,
                    lookaheadSetOffset, lookaheadCount,
                    lookupOffset, lookupCount
                    });
            }

            ir.gsubChainContextSubtables.push_back({
                ruleOffset, static_cast<uint32_t>(subtable.rules.size())
                });
        }

        return true;
    }


    static inline bool compileOpenTypeGsubChainContextLookupInternal(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeLayoutLookupView& lookup,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGsubIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || !lookup || lookupIndex >= lookups.size())
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(lookup, effectiveType) || effectiveType != 6)
            return false;

        const uint16_t subtableCount = lookup.subtableCount();

        if (subtableCount == 0)
            return false;

        const OpenTypeGsubIRCompileCheckpoint checkpoint(ir);

        auto rollback = [&]() noexcept
            {
                checkpoint.rollback(ir);
                result = kOpenTypeShapingIRInvalid;

                if (lookupIndex < context.lookupIds.size())
                    context.lookupIds[lookupIndex] = kOpenTypeShapingIRInvalid;
            };

        OpenTypeShapingIRLookupFilter filter{};

        if (!compileOpenTypeShapingIRLookupFilter(lookup, gdef, ir, filter))
        {
            rollback();
            return false;
        }

        std::vector<OpenTypeGsubIRChainContextSubtableTemp> subtables(subtableCount);

        for (uint16_t subtableIndex = 0; subtableIndex < subtableCount; ++subtableIndex)
        {
            const ByteSpan data = openTypeGsubIREffectiveSubtable(lookup, effectiveType, subtableIndex);

            if (!data || !compileOpenTypeGsubChainContextSubtable(data, ir, subtables[subtableIndex]))
            {
                rollback();
                return false;
            }
        }

        if (ir.lookups.size() > std::numeric_limits<uint32_t>::max())
        {
            rollback();
            return false;
        }

        result = static_cast<OpenTypeShapingIRLookupId>(ir.lookups.size());
        ir.lookups.push_back({});

        if (lookupIndex >= context.lookupIds.size())
        {
            rollback();
            return false;
        }

        context.lookupIds[lookupIndex] = result;

        if (!resolveOpenTypeGsubIRChainContextActions(lookups, gdef, ir, context, subtables))
        {
            rollback();
            return false;
        }

        uint32_t payloadOffset = 0;

        if (!appendOpenTypeGsubChainContextSubtables(subtables, ir, payloadOffset))
        {
            rollback();
            return false;
        }

        if (result >= ir.lookups.size())
        {
            rollback();
            return false;
        }

        OpenTypeShapingIRLookup compiled{};
        compiled.op = OpenTypeShapingIROp::GsubChainContext;
        compiled.payloadOffset = payloadOffset;
        compiled.payloadCount = subtableCount;
        compiled.filter = filter;

        ir.lookups[result] = compiled;
        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubIRLookupByIndex
    //
    // Compile one source LookupList entry and any dependencies needed by a
    // contextual lookup.
    //
    // Current semantic-IR coverage:
    //
    //     Type 1  Single
    //     Type 2  Multiple
    //     Type 3  Alternate
    //     Type 4  Ligature
    //     Type 5  Context
    //     Type 6  Chain Context
    //     Type 8  Reverse Chain Single
    //
    // Type 7 is compiled away to its effective semantic type.
    // ========================================================================

    static inline bool compileOpenTypeGsubIRLookupByIndex(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeGsubIRCompilerContext& context,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size() ||
            context.states.size() != lookups.size() ||
            context.lookupIds.size() != lookups.size())
        {
            return false;
        }

        const OpenTypeGsubIRLookupCompileState state =
            context.states[lookupIndex];

        if (state == OpenTypeGsubIRLookupCompileState::Done)
        {
            result = context.lookupIds[lookupIndex];
            return ir.lookup(result) != nullptr;
        }

        if (state == OpenTypeGsubIRLookupCompileState::Compiling)
        {
            result = context.lookupIds[lookupIndex];

            // A recursive reference is valid only after the owning Type 5
            // compiler has reserved its placeholder lookup ID.
            return result != kOpenTypeShapingIRInvalid &&
                result < ir.lookups.size();
        }

        const OpenTypeLayoutLookupView lookup =
            lookups.lookup(lookupIndex);

        if (!lookup)
            return false;

        uint16_t effectiveType = 0;

        if (!openTypeGsubIREffectiveType(
            lookup, effectiveType))
        {
            return false;
        }

        context.states[lookupIndex] =
            OpenTypeGsubIRLookupCompileState::Compiling;

        bool success = false;

        switch (effectiveType)
        {
        case 1:
            success = compileOpenTypeGsubSingleLookup(
                lookup, gdef, ir, result);
            break;

        case 2:
            success = compileOpenTypeGsubMultipleLookup(
                lookup, gdef, ir, result);
            break;

        case 3:
            success = compileOpenTypeGsubAlternateLookup(
                lookup, gdef, ir, result);
            break;

        case 4:
            success = compileOpenTypeGsubLigatureLookup(
                lookup, gdef, ir, result);
            break;

        case 5:
            success = compileOpenTypeGsubContextLookupInternal(
                lookups, lookupIndex, lookup, gdef,
                ir, context, result);
            break;

        case 6:
            success = compileOpenTypeGsubChainContextLookupInternal(
                lookups, lookupIndex, lookup, gdef,
                ir, context, result);
            break;

        case 8:
            success = compileOpenTypeGsubReverseChainSingleLookup(
                lookup, gdef, ir, result);
            break;

        default:
            success = false;
            break;
        }

        if (!success)
        {
            context.states[lookupIndex] =
                OpenTypeGsubIRLookupCompileState::Unseen;

            context.lookupIds[lookupIndex] =
                kOpenTypeShapingIRInvalid;

            result = kOpenTypeShapingIRInvalid;
            return false;
        }

        if (result == kOpenTypeShapingIRInvalid ||
            result >= ir.lookups.size())
        {
            context.states[lookupIndex] =
                OpenTypeGsubIRLookupCompileState::Unseen;

            context.lookupIds[lookupIndex] =
                kOpenTypeShapingIRInvalid;

            result = kOpenTypeShapingIRInvalid;
            return false;
        }

        context.lookupIds[lookupIndex] = result;
        context.states[lookupIndex] =
            OpenTypeGsubIRLookupCompileState::Done;

        return true;
    }


    // ========================================================================
    // compileOpenTypeGsubContextLookup
    //
    // Public Type 5 entry point.
    //
    // The parent LookupList is required because SequenceLookupRecord actions
    // contain LookupList indices. Those references are recursively compiled to
    // semantic IR lookup IDs.
    // ========================================================================

    static inline bool compileOpenTypeGsubContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size())
            return false;

        OpenTypeGsubIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        return compileOpenTypeGsubIRLookupByIndex(
            lookups, lookupIndex, gdef,
            ir, context, result);
    }


    static inline bool compileOpenTypeGsubContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};

        return compileOpenTypeGsubContextLookup(
            lookups, lookupIndex, gdef, ir, result);
    }

    // ========================================================================
    // compileOpenTypeGsubChainContextLookup
    //
    // Public Type 6 entry point.
    // ========================================================================

    static inline bool compileOpenTypeGsubChainContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId& result)
    {
        result = kOpenTypeShapingIRInvalid;

        if (!lookups || lookupIndex >= lookups.size())
            return false;

        OpenTypeGsubIRCompilerContext context;

        if (!context.reset(lookups.size()))
            return false;

        return compileOpenTypeGsubIRLookupByIndex(
            lookups, lookupIndex, gdef, ir, context, result);
    }


    static inline bool compileOpenTypeGsubChainContextLookup(
        const OpenTypeLayoutLookupListView& lookups, uint16_t lookupIndex,
        OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId& result)
    {
        const OpenTypeGdefView gdef{};
        return compileOpenTypeGsubChainContextLookup(
            lookups, lookupIndex, gdef, ir, result);
    }

} // namespace waavs
