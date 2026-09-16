// opentype_shaping_ir.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace waavs
{
    // ========================================================================
    // Stable IR identifiers
    //
    // These are indices into immutable compiled pools, not OpenType table
    // indices or byte offsets.
    // ========================================================================

    using OpenTypeShapingIRLookupId = uint32_t;
    using OpenTypeShapingIRFeatureId = uint32_t;
    using OpenTypeShapingIRGlyphSetId = uint32_t;

    static constexpr uint32_t kOpenTypeShapingIRInvalid = std::numeric_limits<uint32_t>::max();
    static constexpr size_t kOpenTypeShapingIRGlyphDomainSize = 0x10000u;

    // ========================================================================
    // OpenTypeShapingIROp
    //
    // Semantic shaping operations.
    //
    // OpenType serialization details deliberately do not appear here:
    //
    //   - no subtable format numbers
    //   - no ExtensionSubst / ExtensionPos
    //   - no Coverage format numbers
    //   - no ClassDef format numbers
    //
    // Those belong exclusively to the OpenType -> IR compiler.
    // ========================================================================

    enum class OpenTypeShapingIROp : uint8_t
    {
        Invalid = 0,

        // GSUB
        GsubSingle,
        GsubMultiple,
        GsubAlternate,
        GsubLigature,
        GsubContext,
        GsubChainContext,
        GsubReverseChainSingle,
        GsubLAST = GsubReverseChainSingle,

        // GPOS
        GposSingle,
        GposPair,
        GposCursive,
        GposMarkBase,
        GposMarkLigature,
        GposMarkMark,
        GposContext,
        GposChainContext,
        GposLAST = GposChainContext
    };


    [[nodiscard]] static constexpr bool isOpenTypeShapingIRGsub(OpenTypeShapingIROp op) noexcept
    {
        return op >= OpenTypeShapingIROp::GsubSingle &&
            op <= OpenTypeShapingIROp::GsubLAST;
    }


    [[nodiscard]] static constexpr bool isOpenTypeShapingIRGpos(OpenTypeShapingIROp op) noexcept
    {
        return op >= OpenTypeShapingIROp::GposSingle &&
            op <= OpenTypeShapingIROp::GposLAST;
    }


    // ========================================================================
    // Normalized glyph sets
    //
    // Canonical membership representation shared by:
    //
    //   - lookup filtering
    //   - future contextual matchers
    //   - future mark filtering sets
    //
    // This intentionally removes OpenType Coverage Format 1/2 from runtime.
    //
    // Ranges are inclusive and sorted by first glyph.
    // ========================================================================

    struct OpenTypeShapingIRGlyphRange
    {
        uint16_t first{ 0 };
        uint16_t last{ 0 };
    };


    struct OpenTypeShapingIRGlyphSet
    {
        uint32_t rangeOffset{ 0 };
        uint32_t rangeCount{ 0 };
    };


    // ========================================================================
    // Lookup filtering
    //
    // Semantic equivalent of the OpenType LookupFlag/GDEF behavior.
    //
    // markFilteringSet == invalid:
    //   no mark filtering set
    //
    // markAttachmentType == 0:
    //   no mark attachment type restriction
    //
    // Raw LookupFlag bits do not survive compilation.
    // ========================================================================

    enum OpenTypeShapingIRFilterFlags : uint8_t
    {
        OpenTypeShapingIRFilterNone = 0,
        OpenTypeShapingIRRightToLeft = 1u << 0,
        OpenTypeShapingIRIgnoreBaseGlyphs = 1u << 1,
        OpenTypeShapingIRIgnoreLigatures = 1u << 2,
        OpenTypeShapingIRIgnoreMarks = 1u << 3
    };


    struct OpenTypeShapingIRLookupFilter
    {
        uint8_t flags{ OpenTypeShapingIRFilterNone };
        uint8_t markAttachmentType{ 0 };
        uint16_t reserved{ 0 };

        OpenTypeShapingIRGlyphSetId markFilteringSet{
            kOpenTypeShapingIRInvalid
        };
    };


    // ========================================================================
    // GSUB Single
    //
    // Canonical semantic representation:
    //
    //     input glyph -> replacement glyph
    //
    // Both OpenType SingleSubst Format 1 and Format 2 compile here.
    //
    // Pairs within one subtable are sorted by input glyph.
    //
    // A lookup may contain multiple subtables. They remain distinct because
    // OpenType lookup semantics require subtables to be tried in stored order,
    // with the first matching subtable winning for a glyph.
    // ========================================================================

    struct OpenTypeShapingIRGsubSinglePair
    {
        uint16_t input{ 0 };
        uint16_t output{ 0 };
    };


    struct OpenTypeShapingIRGsubSingleSubtable
    {
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };


    // ========================================================================
    // GSUB Multiple
    //
    // Canonical semantic representation:
    //
    //     input glyph -> replacement glyph sequence
    //
    // OpenType Coverage indexing and Sequence table offsets are removed during
    // compilation.
    //
    // A pair references one entry in gsubMultipleSequences. That sequence
    // references a contiguous slice of gsubMultipleGlyphs.
    //
    // Pairs within one subtable are sorted by input glyph.
    //
    // Subtables remain ordered because the first matching subtable wins.
    // ========================================================================

    struct OpenTypeShapingIRGlyphSequence
    {
        uint32_t glyphOffset{ 0 };
        uint32_t glyphCount{ 0 };
    };


    struct OpenTypeShapingIRGsubMultiplePair
    {
        uint16_t input{ 0 };
        uint16_t reserved{ 0 };
        uint32_t sequenceIndex{ 0 };
    };


    struct OpenTypeShapingIRGsubMultipleSubtable
    {
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };


    // ========================================================================
    // GSUB Alternate
    //
    // Canonical semantic representation:
    //
    //     input glyph -> ordered alternate glyph set
    //
    // The complete AlternateSet is retained in the IR. The current executor
    // uses alternate index 0 as its default policy, matching the raw path.
    // A later shaping policy can select another index without recompiling the
    // font tables.
    // ========================================================================

    struct OpenTypeShapingIRGsubAlternateSet
    {
        uint32_t glyphOffset{ 0 };
        uint32_t glyphCount{ 0 };
    };


    struct OpenTypeShapingIRGsubAlternatePair
    {
        uint16_t input{ 0 };
        uint16_t reserved{ 0 };
        uint32_t alternateSetIndex{ 0 };
    };


    struct OpenTypeShapingIRGsubAlternateSubtable
    {
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };

    // ========================================================================
// GSUB Ligature
//
// Canonical semantic representation:
//
//     first glyph + trailing component glyphs -> ligature glyph
//
// The first glyph is stored in OpenTypeShapingIRGsubLigaturePair.
//
// OpenTypeShapingIRGsubLigature stores:
//
//     output             resulting ligature glyph
//     componentCount     total input components, including first glyph
//     componentOffset    trailing components in gsubLigatureComponents
//
// Ligature candidates belonging to one first glyph remain in source
// order because the first matching candidate wins.
//
// Pairs within one subtable are sorted by first input glyph.
// ========================================================================

    struct OpenTypeShapingIRGsubLigature
    {
        uint16_t output{ 0 };
        uint16_t componentCount{ 0 };
        uint32_t componentOffset{ 0 };
    };


    struct OpenTypeShapingIRGsubLigaturePair
    {
        uint16_t input{ 0 };
        uint16_t reserved{ 0 };

        uint32_t ligatureOffset{ 0 };
        uint32_t ligatureCount{ 0 };
    };


    struct OpenTypeShapingIRGsubLigatureSubtable
    {
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };


    // ========================================================================
    // Shared contextual action
    // ========================================================================

    struct OpenTypeShapingIRSequenceLookup
    {
        uint32_t sequenceIndex{ 0 };
        OpenTypeShapingIRLookupId lookup{ kOpenTypeShapingIRInvalid };
    };


    // ========================================================================
    // GSUB Context
    // ========================================================================

    struct OpenTypeShapingIRGsubContextRule
    {
        uint32_t inputSetOffset{ 0 };
        uint32_t inputCount{ 0 };
        uint32_t lookupOffset{ 0 };
        uint32_t lookupCount{ 0 };
    };


    struct OpenTypeShapingIRGsubContextSubtable
    {
        uint32_t ruleOffset{ 0 };
        uint32_t ruleCount{ 0 };
    };


    // ========================================================================
    // GSUB Chain Context
    //
    // Backtrack sets are stored nearest-first. Input set 0 is the current
    // glyph. Lookahead sets are stored nearest-first after the input sequence.
    // ========================================================================

    struct OpenTypeShapingIRGsubChainContextRule
    {
        uint32_t backtrackSetOffset{ 0 };
        uint32_t backtrackCount{ 0 };
        uint32_t inputSetOffset{ 0 };
        uint32_t inputCount{ 0 };
        uint32_t lookaheadSetOffset{ 0 };
        uint32_t lookaheadCount{ 0 };
        uint32_t lookupOffset{ 0 };
        uint32_t lookupCount{ 0 };
    };


    struct OpenTypeShapingIRGsubChainContextSubtable
    {
        uint32_t ruleOffset{ 0 };
        uint32_t ruleCount{ 0 };
    };


    // ========================================================================
    // GSUB Reverse Chain Single
    //
    // Type 8 is always one-to-one. Each subtable owns an ordered backtrack
    // glyph-set slice, an ordered lookahead glyph-set slice, and sorted input
    // glyph -> replacement pairs. Backtrack index 0 is nearest to the current
    // glyph. Runtime application scans candidate glyphs from end to start.
    // ========================================================================

    struct OpenTypeShapingIRGsubReverseChainSinglePair
    {
        uint16_t input{ 0 };
        uint16_t output{ 0 };
    };


    struct OpenTypeShapingIRGsubReverseChainSingleSubtable
    {
        uint32_t backtrackSetOffset{ 0 };
        uint32_t backtrackCount{ 0 };
        uint32_t lookaheadSetOffset{ 0 };
        uint32_t lookaheadCount{ 0 };
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };


    // ========================================================================
    // OpenTypeShapingIRLookup
    //
    // One compiled GSUB or GPOS lookup.
    //
    // payloadOffset/payloadCount address the pool selected by op.
    //
    // For example:
    //
    //   GsubSingle
    //       payloadOffset -> gsubSingleSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GsubMultiple
    //       payloadOffset -> gsubMultipleSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GsubAlternate
    //       payloadOffset -> gsubAlternateSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GsubLigature
    //       payloadOffset -> gsubLigatureSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GsubContext
    //       payloadOffset -> gsubContextSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GsubChainContext
    //       payloadOffset -> gsubChainContextSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GsubReverseChainSingle
    //       payloadOffset -> gsubReverseChainSingleSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    // Future semantic operations add their own typed payload pools without
    // changing this core lookup representation.
    // ========================================================================

    struct OpenTypeShapingIRLookup
    {
        OpenTypeShapingIROp op{ OpenTypeShapingIROp::Invalid };

        uint8_t reserved0{ 0 };
        uint16_t reserved1{ 0 };

        uint32_t payloadOffset{ 0 };
        uint32_t payloadCount{ 0 };

        OpenTypeShapingIRLookupFilter filter{};
    };


    // ========================================================================
    // Features
    //
    // A compiled feature owns an ordered slice of lookup IDs.
    //
    // The tag remains because it is useful for:
    //
    //   - shape-plan construction
    //   - diagnostics
    //   - feature lookup
    //
    // The runtime lookup executor itself does not need the tag.
    // ========================================================================

    struct OpenTypeShapingIRFeature
    {
        uint32_t tag{ 0 };

        uint32_t lookupOffset{ 0 };
        uint32_t lookupCount{ 0 };
    };


    // ========================================================================
    // OpenTypeShapingIR
    //
    // Initial owning compiled representation.
    //
    // The compiler mutates this object while building it. Once compilation
    // succeeds it should be treated as immutable by shaping execution.
    //
    // We deliberately use straightforward vectors initially. Compacting,
    // deduplicating, serializing, or mmap-friendly finalization can be added
    // after the semantic representation has been proven.
    // ========================================================================

    struct OpenTypeShapingIR
    {
        std::vector<OpenTypeShapingIRGlyphRange> glyphRanges{};
        std::vector<OpenTypeShapingIRGlyphSet> glyphSets{};

        // Compiled GDEF classification data.
        //
        // Empty means the corresponding data has not been compiled.
        // When present, each table contains exactly 65536 entries indexed
        // directly by uint16 glyph ID.
        std::vector<uint16_t> gdefGlyphClasses{};
        std::vector<uint16_t> gdefMarkAttachClasses{};

        std::vector<OpenTypeShapingIRLookup> lookups{};

        std::vector<OpenTypeShapingIRFeature> features{};
        std::vector<OpenTypeShapingIRLookupId> featureLookups{};

        // GSUB Single semantic pool.
        std::vector<OpenTypeShapingIRGsubSingleSubtable> gsubSingleSubtables{};
        std::vector<OpenTypeShapingIRGsubSinglePair> gsubSinglePairs{};

        // GSUB Multiple semantic pool.
        std::vector<uint16_t> gsubMultipleGlyphs{};
        std::vector<OpenTypeShapingIRGlyphSequence> gsubMultipleSequences{};
        std::vector<OpenTypeShapingIRGsubMultiplePair> gsubMultiplePairs{};
        std::vector<OpenTypeShapingIRGsubMultipleSubtable> gsubMultipleSubtables{};


        // GSUB Alternate semantic pool.
        std::vector<uint16_t> gsubAlternateGlyphs{};
        std::vector<OpenTypeShapingIRGsubAlternateSet> gsubAlternateSets{};
        std::vector<OpenTypeShapingIRGsubAlternatePair> gsubAlternatePairs{};
        std::vector<OpenTypeShapingIRGsubAlternateSubtable> gsubAlternateSubtables{};

        // GSUB Ligature semantic pool.
        std::vector<uint16_t> gsubLigatureComponents{};
        std::vector<OpenTypeShapingIRGsubLigature> gsubLigatures{};
        std::vector<OpenTypeShapingIRGsubLigaturePair> gsubLigaturePairs{};
        std::vector<OpenTypeShapingIRGsubLigatureSubtable> gsubLigatureSubtables{};


        // GSUB Context semantic pools.
        std::vector<OpenTypeShapingIRGlyphSetId> gsubContextInputSets{};
        std::vector<OpenTypeShapingIRSequenceLookup> gsubContextLookups{};
        std::vector<OpenTypeShapingIRGsubContextRule> gsubContextRules{};
        std::vector<OpenTypeShapingIRGsubContextSubtable> gsubContextSubtables{};

        // GSUB Chain Context semantic pools.
        std::vector<OpenTypeShapingIRGlyphSetId> gsubChainContextSets{};
        std::vector<OpenTypeShapingIRSequenceLookup> gsubChainContextLookups{};
        std::vector<OpenTypeShapingIRGsubChainContextRule> gsubChainContextRules{};
        std::vector<OpenTypeShapingIRGsubChainContextSubtable> gsubChainContextSubtables{};

        // GSUB Reverse Chain Single semantic pools.
        std::vector<OpenTypeShapingIRGlyphSetId> gsubReverseChainSingleSets{};
        std::vector<OpenTypeShapingIRGsubReverseChainSinglePair> gsubReverseChainSinglePairs{};
        std::vector<OpenTypeShapingIRGsubReverseChainSingleSubtable> gsubReverseChainSingleSubtables{};



        void clear() noexcept
        {
            glyphRanges.clear();
            glyphSets.clear();

            gdefGlyphClasses.clear();
            gdefMarkAttachClasses.clear();

            lookups.clear();

            features.clear();
            featureLookups.clear();

            gsubSingleSubtables.clear();
            gsubSinglePairs.clear();

            gsubMultipleGlyphs.clear();
            gsubMultipleSequences.clear();
            gsubMultiplePairs.clear();
            gsubMultipleSubtables.clear();


            gsubAlternateGlyphs.clear();
            gsubAlternateSets.clear();
            gsubAlternatePairs.clear();
            gsubAlternateSubtables.clear();

            gsubLigatureComponents.clear();
            gsubLigatures.clear();
            gsubLigaturePairs.clear();
            gsubLigatureSubtables.clear();


            gsubContextInputSets.clear();
            gsubContextLookups.clear();
            gsubContextRules.clear();
            gsubContextSubtables.clear();

            gsubChainContextSets.clear();
            gsubChainContextLookups.clear();
            gsubChainContextRules.clear();
            gsubChainContextSubtables.clear();

            gsubReverseChainSingleSets.clear();
            gsubReverseChainSinglePairs.clear();
            gsubReverseChainSingleSubtables.clear();
        }


        [[nodiscard]] bool empty() const noexcept
        {
            return lookups.empty();
        }


        [[nodiscard]] const OpenTypeShapingIRLookup* lookup(OpenTypeShapingIRLookupId id) const noexcept
        {
            return id < lookups.size() ? &lookups[id] : nullptr;
        }


        [[nodiscard]] const OpenTypeShapingIRFeature* feature(OpenTypeShapingIRFeatureId id) const noexcept
        {
            return id < features.size() ? &features[id] : nullptr;
        }


        [[nodiscard]] const OpenTypeShapingIRGlyphSet* glyphSet(OpenTypeShapingIRGlyphSetId id) const noexcept
        {
            return id < glyphSets.size() ? &glyphSets[id] : nullptr;
        }
    };

} // namespace waavs