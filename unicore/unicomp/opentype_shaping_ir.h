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
    using OpenTypeShapingIRGlyphClassMapId = uint32_t;
    using OpenTypeShapingIRAnchorId = uint32_t;


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
    // Normalized glyph class maps
    //
    // Canonical semantic replacement for OpenType ClassDef Format 1/2.
    //
    // Ranges are inclusive, sorted, and non-overlapping. Class zero is the
    // implicit default and does not need to be stored explicitly.
    // ========================================================================

    struct OpenTypeShapingIRGlyphClassRange
    {
        uint16_t first{ 0 };
        uint16_t last{ 0 };
        uint16_t value{ 0 };
        uint16_t reserved{ 0 };
    };


    struct OpenTypeShapingIRGlyphClassMap
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
    // GPOS Position Adjustment
    //
    // Semantic positioning delta in font design units. This deliberately does
    // not preserve OpenType ValueFormat bits or ValueRecord serialization.
    //
    // Device / VariationIndex adjustments are not represented yet because the
    // current raw GPOS executor does not apply them. They remain a separate
    // future semantic extension rather than leaking parent-relative offsets
    // into the compiled runtime representation.
    // ========================================================================

    struct OpenTypeShapingIRPositionAdjustment
    {
        int32_t offsetX{ 0 };
        int32_t offsetY{ 0 };
        int32_t advanceX{ 0 };
        int32_t advanceY{ 0 };

        [[nodiscard]] bool empty() const noexcept
        {
            return offsetX == 0 && offsetY == 0 &&
                advanceX == 0 && advanceY == 0;
        }
    };

    // ========================================================================
    // GPOS Anchor
    //
    // Semantic anchor position in font design units.
    //
    // OpenType Anchor table formats are removed during compilation. Device and
    // VariationIndex adjustments are not represented yet, matching the current
    // raw GPOS execution path.
    // ========================================================================

    struct OpenTypeShapingIRAnchor
    {
        int32_t x{ 0 };
        int32_t y{ 0 };
    };

    // ========================================================================
    // GPOS Single
    //
    // Canonical semantic representation:
    //
    //     glyph -> positioning adjustment
    //
    // SinglePos Format 1 and Format 2 both compile here. Pairs within one
    // subtable are sorted by glyph. Subtable order is preserved because the
    // first matching subtable wins.
    // ========================================================================

    struct OpenTypeShapingIRGposSinglePair
    {
        uint16_t glyph{ 0 };
        uint16_t reserved{ 0 };
        OpenTypeShapingIRPositionAdjustment adjustment{};
    };


    struct OpenTypeShapingIRGposSingleSubtable
    {
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };


    // ========================================================================
    // GPOS Pair
    //
    // PairPos has two useful semantic forms, both retained without preserving
    // OpenType source formats:
    //
    //   Explicit
    //       first glyph + second glyph -> two adjustments
    //
    //   Class
    //       first class + second class -> two adjustments
    //
    // PairPos Format 1 compiles to Explicit. PairPos Format 2 compiles to
    // Class using normalized glyph-class maps. The original Coverage still has
    // semantic meaning for the first glyph in the class form, so it is retained
    // as a normalized glyph set.
    //
    // secondParticipates records whether the source subtable contains a second
    // ValueRecord. This is not equivalent to checking whether the second
    // adjustment is numerically zero: it controls PairPos scan/resume behavior.
    // ========================================================================

    enum class OpenTypeShapingIRGposPairKind : uint8_t
    {
        Invalid = 0,
        Explicit,
        Class
    };


    struct OpenTypeShapingIRGposPairExplicit
    {
        uint16_t first{ 0 };
        uint16_t second{ 0 };
        OpenTypeShapingIRPositionAdjustment firstAdjustment{};
        OpenTypeShapingIRPositionAdjustment secondAdjustment{};
    };


    struct OpenTypeShapingIRGposPairExplicitSubtable
    {
        uint32_t pairOffset{ 0 };
        uint32_t pairCount{ 0 };
    };


    struct OpenTypeShapingIRGposPairClassValue
    {
        OpenTypeShapingIRPositionAdjustment firstAdjustment{};
        OpenTypeShapingIRPositionAdjustment secondAdjustment{};
    };


    struct OpenTypeShapingIRGposPairClassSubtable
    {
        OpenTypeShapingIRGlyphSetId firstCoverage{ kOpenTypeShapingIRInvalid };
        OpenTypeShapingIRGlyphClassMapId firstClassMap{ kOpenTypeShapingIRInvalid };
        OpenTypeShapingIRGlyphClassMapId secondClassMap{ kOpenTypeShapingIRInvalid };

        uint32_t valueOffset{ 0 };
        uint16_t firstClassCount{ 0 };
        uint16_t secondClassCount{ 0 };
    };


    struct OpenTypeShapingIRGposPairSubtable
    {
        OpenTypeShapingIRGposPairKind kind{ OpenTypeShapingIRGposPairKind::Invalid };
        uint8_t secondParticipates{ 0 };
        uint16_t reserved{ 0 };
        uint32_t payloadIndex{ 0 };
    };

    // ========================================================================
    // GPOS Cursive
    //
    // Canonical semantic representation:
    //
    //     glyph -> optional entry anchor + optional exit anchor
    //
    // CursivePos Coverage indexing and Anchor table serialization disappear
    // during compilation.
    //
    // The entry/exit presence flags are semantic. An anchor at (0, 0) is a valid
    // anchor and must not be confused with an absent anchor.
    //
    // Records within one subtable are sorted by glyph. Subtable order remains
    // significant because the first matching subtable wins.
    // ========================================================================

    struct OpenTypeShapingIRGposCursiveRecord
    {
        uint16_t glyph{ 0 };

        uint8_t hasEntry{ 0 };
        uint8_t hasExit{ 0 };

        OpenTypeShapingIRAnchor entry{};
        OpenTypeShapingIRAnchor exit{};
    };


    struct OpenTypeShapingIRGposCursiveSubtable
    {
        uint32_t recordOffset{ 0 };
        uint32_t recordCount{ 0 };
    };

    // ========================================================================
    // GPOS shared mark record
    //
    // Used by MarkBase, and later by MarkLigature and MarkMark.
    //
    // Source MarkCoverage indexing and MarkArray serialization disappear during
    // compilation. Records are sorted by glyph within the owning subtable.
    // ========================================================================

    struct OpenTypeShapingIRGposMarkRecord
    {
        uint16_t glyph{ 0 };
        uint16_t markClass{ 0 };
        OpenTypeShapingIRAnchorId anchor{ kOpenTypeShapingIRInvalid };
    };


    // ========================================================================
    // GPOS MarkBase
    //
    // Canonical semantic representation:
    //
    //     mark glyph -> markClass + markAnchor
    //
    //     base glyph -> anchor[markClass]
    //
    // The base-anchor matrix is row-major:
    //
    //     localBaseIndex * markClassCount + markClass
    //
    // Each matrix cell contains an AnchorId. The invalid ID represents a NULL
    // BaseAnchor.
    //
    // Marks and bases are independently sorted by glyph.
    // ========================================================================

    struct OpenTypeShapingIRGposBaseRecord
    {
        uint16_t glyph{ 0 };
        uint16_t reserved{ 0 };
    };


    struct OpenTypeShapingIRGposMarkBaseSubtable
    {
        uint32_t markOffset{ 0 };
        uint32_t markCount{ 0 };

        uint32_t baseOffset{ 0 };
        uint32_t baseCount{ 0 };

        uint32_t baseAnchorOffset{ 0 };

        uint16_t markClassCount{ 0 };
        uint16_t reserved{ 0 };
    };


    // ========================================================================
    // GPOS MarkLigature
    //
    // Canonical semantic representation:
    //
    //     mark glyph -> markClass + markAnchor
    //
    //     ligature glyph
    //         -> component[]
    //             -> anchor[markClass]
    //
    // Each ligature may have a different component count.
    //
    // componentAnchorOffset addresses a contiguous row-major slice of
    // gposMarkLigatureAnchorRefs:
    //
    //     componentIndex * markClassCount + markClass
    //
    // Each cell is an AnchorId. The invalid ID represents a NULL anchor.
    // ========================================================================

    struct OpenTypeShapingIRGposLigatureRecord
    {
        uint16_t glyph{ 0 };
        uint16_t componentCount{ 0 };

        uint32_t componentAnchorOffset{ 0 };
    };


    struct OpenTypeShapingIRGposMarkLigatureSubtable
    {
        uint32_t markOffset{ 0 };
        uint32_t markCount{ 0 };

        uint32_t ligatureOffset{ 0 };
        uint32_t ligatureCount{ 0 };

        uint16_t markClassCount{ 0 };
        uint16_t reserved{ 0 };
    };


    // ========================================================================
    // GPOS MarkMark
    //
    // Canonical semantic representation:
    //
    //     Mark1 glyph -> markClass + Mark1 anchor
    //
    //     Mark2 glyph -> anchor[Mark1 class]
    //
    // Mark1 records reuse the shared gposMarkRecords pool.
    //
    // The Mark2 anchor matrix is row-major:
    //
    //     localMark2Index * markClassCount + markClass
    //
    // Each cell contains an AnchorId. The invalid ID represents a NULL
    // Mark2 anchor.
    // ========================================================================

    struct OpenTypeShapingIRGposMark2Record
    {
        uint16_t glyph{ 0 };
        uint16_t reserved{ 0 };
    };


    struct OpenTypeShapingIRGposMarkMarkSubtable
    {
        uint32_t mark1Offset{ 0 };
        uint32_t mark1Count{ 0 };

        uint32_t mark2Offset{ 0 };
        uint32_t mark2Count{ 0 };

        uint32_t mark2AnchorOffset{ 0 };

        uint16_t markClassCount{ 0 };
        uint16_t reserved{ 0 };
    };


    // ========================================================================
    // GPOS Context
    //
    // Canonical semantic representation for GPOS LookupType 7.
    //
    // ContextPos Formats 1, 2, and 3 all compile to:
    //
    //     ordered rules
    //         ordered input glyph sets
    //         ordered positioning lookup actions
    //
    // Position 0 is the current glyph and is matched directly.
    // Positions 1..N-1 are reached using the parent lookup filter.
    //
    // GPOS never changes glyph topology, so sequenceIndex remains stable while
    // nested positioning lookups execute.
    // ========================================================================

    struct OpenTypeShapingIRGposContextRule
    {
        uint32_t inputSetOffset{ 0 };
        uint32_t inputCount{ 0 };

        uint32_t lookupOffset{ 0 };
        uint32_t lookupCount{ 0 };
    };


    struct OpenTypeShapingIRGposContextSubtable
    {
        uint32_t ruleOffset{ 0 };
        uint32_t ruleCount{ 0 };
    };



    // ========================================================================
    // GPOS Chain Context
    //
    // Canonical semantic representation for GPOS LookupType 8.
    //
    // ChainContextPos Formats 1, 2, and 3 all compile to:
    //
    //     ordered rules
    //         ordered backtrack glyph sets
    //         ordered input glyph sets
    //         ordered lookahead glyph sets
    //         ordered positioning lookup actions
    //
    // Backtrack sets are stored nearest-first.
    // Input set 0 is the current glyph.
    // Lookahead sets are stored nearest-first after the input sequence.
    //
    // Backtrack and lookahead are match-only constraints. Nested positioning
    // executes only within the physical span occupied by the matched input
    // sequence.
    // ========================================================================

    struct OpenTypeShapingIRGposChainContextRule
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


    struct OpenTypeShapingIRGposChainContextSubtable
    {
        uint32_t ruleOffset{ 0 };
        uint32_t ruleCount{ 0 };
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
    //   GposSingle
    //       payloadOffset -> gposSingleSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GposPair
    //       payloadOffset -> gposPairSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GposCursive
    //       payloadOffset -> gposCursiveSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GposMarkBase
    //       payloadOffset -> gposMarkBaseSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GposMarkLigature
    //       payloadOffset -> gposMarkLigatureSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GposMarkMark
    //       payloadOffset -> gposMarkMarkSubtables[]
    //       payloadCount  -> number of ordered subtables
    // 
    //   GposContext
    //       payloadOffset -> gposContextSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
    //   GposChainContext
    //       payloadOffset -> gposChainContextSubtables[]
    //       payloadCount  -> number of ordered subtables
    //
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

        std::vector<OpenTypeShapingIRGlyphClassRange> glyphClassRanges{};
        std::vector<OpenTypeShapingIRGlyphClassMap> glyphClassMaps{};

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

        // GPOS Single semantic pools.
        std::vector<OpenTypeShapingIRGposSinglePair> gposSinglePairs{};
        std::vector<OpenTypeShapingIRGposSingleSubtable> gposSingleSubtables{};

        // GPOS Pair semantic pools.
        std::vector<OpenTypeShapingIRGposPairExplicit> gposPairExplicitPairs{};
        std::vector<OpenTypeShapingIRGposPairExplicitSubtable> gposPairExplicitSubtables{};
        std::vector<OpenTypeShapingIRGposPairClassValue> gposPairClassValues{};
        std::vector<OpenTypeShapingIRGposPairClassSubtable> gposPairClassSubtables{};
        std::vector<OpenTypeShapingIRGposPairSubtable> gposPairSubtables{};

        // GPOS Cursive semantic pools.
        std::vector<OpenTypeShapingIRGposCursiveRecord> gposCursiveRecords{};
        std::vector<OpenTypeShapingIRGposCursiveSubtable> gposCursiveSubtables{};


        // Semantic anchor pool shared by GPOS attachment lookups.
        std::vector<OpenTypeShapingIRAnchor> anchors{};

        // Shared GPOS mark records used by Types 4, 5 and 6.
        std::vector<OpenTypeShapingIRGposMarkRecord> gposMarkRecords{};

        // GPOS MarkBase semantic pools.
        std::vector<OpenTypeShapingIRGposBaseRecord> gposMarkBaseRecords{};
        std::vector<OpenTypeShapingIRAnchorId> gposMarkBaseAnchorRefs{};
        std::vector<OpenTypeShapingIRGposMarkBaseSubtable> gposMarkBaseSubtables{};

        // GPOS MarkLigature semantic pools.
        std::vector<OpenTypeShapingIRGposLigatureRecord> gposMarkLigatureRecords{};
        std::vector<OpenTypeShapingIRAnchorId> gposMarkLigatureAnchorRefs{};
        std::vector<OpenTypeShapingIRGposMarkLigatureSubtable> gposMarkLigatureSubtables{};

        // GPOS MarkMark semantic pools.
        std::vector<OpenTypeShapingIRGposMark2Record> gposMark2Records{};
        std::vector<OpenTypeShapingIRAnchorId> gposMark2AnchorRefs{};
        std::vector<OpenTypeShapingIRGposMarkMarkSubtable> gposMarkMarkSubtables{};

        // GPOS Context semantic pools.
        std::vector<OpenTypeShapingIRGlyphSetId> gposContextInputSets{};
        std::vector<OpenTypeShapingIRSequenceLookup> gposContextLookups{};
        std::vector<OpenTypeShapingIRGposContextRule> gposContextRules{};
        std::vector<OpenTypeShapingIRGposContextSubtable> gposContextSubtables{};

        // GPOS ChainContext semantic pools.
        std::vector<OpenTypeShapingIRGlyphSetId> gposChainContextBacktrackSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> gposChainContextInputSets{};
        std::vector<OpenTypeShapingIRGlyphSetId> gposChainContextLookaheadSets{};
        std::vector<OpenTypeShapingIRSequenceLookup> gposChainContextLookups{};
        std::vector<OpenTypeShapingIRGposChainContextRule> gposChainContextRules{};
        std::vector<OpenTypeShapingIRGposChainContextSubtable> gposChainContextSubtables{};




        void clear() noexcept
        {
            glyphRanges.clear();
            glyphSets.clear();

            glyphClassRanges.clear();
            glyphClassMaps.clear();

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

            gposSinglePairs.clear();
            gposSingleSubtables.clear();

            gposPairExplicitPairs.clear();
            gposPairExplicitSubtables.clear();
            gposPairClassValues.clear();
            gposPairClassSubtables.clear();
            gposPairSubtables.clear();

            gposCursiveRecords.clear();
            gposCursiveSubtables.clear();

            anchors.clear();
            gposMarkRecords.clear();

            gposMarkBaseRecords.clear();
            gposMarkBaseAnchorRefs.clear();
            gposMarkBaseSubtables.clear();

            gposMarkLigatureRecords.clear();
            gposMarkLigatureAnchorRefs.clear();
            gposMarkLigatureSubtables.clear();

            gposMark2Records.clear();
            gposMark2AnchorRefs.clear();
            gposMarkMarkSubtables.clear();

            gposContextInputSets.clear();
            gposContextLookups.clear();
            gposContextRules.clear();
            gposContextSubtables.clear();

            gposChainContextBacktrackSets.clear();
            gposChainContextInputSets.clear();
            gposChainContextLookaheadSets.clear();
            gposChainContextLookups.clear();
            gposChainContextRules.clear();
            gposChainContextSubtables.clear();
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


        [[nodiscard]] const OpenTypeShapingIRGlyphClassMap* glyphClassMap(OpenTypeShapingIRGlyphClassMapId id) const noexcept
        {
            return id < glyphClassMaps.size() ? &glyphClassMaps[id] : nullptr;
        }

        [[nodiscard]] const OpenTypeShapingIRAnchor* anchor(OpenTypeShapingIRAnchorId id) const noexcept
        {
            return id < anchors.size() ? &anchors[id] : nullptr;
        }
    };

} // namespace waavs