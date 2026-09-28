// opentype_shaping_analysis.h
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "opentype_shaping_ir.h"

namespace waavs
{
    struct OpenTypeShapingAnalysisFace
    {
        std::string label{};
        std::string source{};

        uint64_t gsubLookups{0};
        uint64_t gposLookups{0};
        std::array<uint64_t, 16> opCounts{};

        uint64_t glyphSets{0};
        uint64_t glyphSetRanges{0};
        uint64_t glyphSetMembers{0};
        uint64_t maxGlyphSetRanges{0};
        uint64_t maxGlyphSetMembers{0};

        uint64_t filteredLookups{0};
        uint64_t ignoreBaseLookups{0};
        uint64_t ignoreLigatureLookups{0};
        uint64_t ignoreMarkLookups{0};
        uint64_t markFilteringSetLookups{0};
        uint64_t markAttachmentTypeLookups{0};
        uint64_t combinedFilterLookups{0};

        uint64_t gsubSingleMappings{0};
        uint64_t gsubSingleMaxMappingsPerSubtable{0};
        uint64_t gsubMultipleMappings{0};
        uint64_t gsubMultipleOutputGlyphs{0};
        uint64_t gsubMultipleMaxExpansion{0};
        uint64_t gsubAlternateMappings{0};
        uint64_t gsubAlternateGlyphs{0};
        uint64_t gsubAlternateMaxSet{0};

        uint64_t gsubLigatureFirstGlyphs{0};
        uint64_t gsubLigatureRules{0};
        uint64_t gsubLigatureComponents{0};
        uint64_t gsubLigatureMaxComponents{0};
        uint64_t gsubLigatureMaxFanout{0};
        uint64_t gsubLigatureMaxSharedPrefix2{0};
        uint64_t gsubLigatureMaxSharedPrefix3{0};

        uint64_t gsubContextRules{0};
        uint64_t gsubContextMaxRulesPerSubtable{0};
        uint64_t gsubContextMaxInput{0};
        uint64_t gsubContextMaxActions{0};
        uint64_t gsubContextMaxFirstSetMembers{0};
        uint64_t gsubContextMaxFirstFanoutApprox{0};
        uint64_t gsubContextMaxFirstFanout{0};
        uint64_t gsubContextMaxPrefix2SameSetFanout{0};

        uint64_t gsubChainRules{0};
        uint64_t gsubChainMaxRulesPerSubtable{0};
        uint64_t gsubChainMaxBacktrack{0};
        uint64_t gsubChainMaxInput{0};
        uint64_t gsubChainMaxLookahead{0};
        uint64_t gsubChainMaxActions{0};
        uint64_t gsubChainMaxFirstFanout{0};
        uint64_t gsubChainMaxPrefix2SameSetFanout{0};

        uint64_t gsubReverseSubtables{0};
        uint64_t gsubReversePairs{0};
        uint64_t gsubReverseMaxBacktrack{0};
        uint64_t gsubReverseMaxLookahead{0};

        uint64_t gposSingleMappings{0};
        uint64_t gposPairExplicitSubtables{0};
        uint64_t gposPairExplicitPairs{0};
        uint64_t gposPairClassSubtables{0};
        uint64_t gposPairClassCells{0};
        uint64_t gposPairClassNonZeroCells{0};
        uint64_t gposPairMaxExplicitPairs{0};
        uint64_t gposPairMaxClassCells{0};
        uint64_t gposPairMaxClassDimension{0};
        uint64_t gposPairMaxSecondFanout{0};
        uint64_t gposPairClassDensitySamples{0};
        uint64_t gposPairClassMinDensityPermille{0};
        uint64_t gposPairClassMaxDensityPermille{0};
        uint64_t gposPairClassMaxNonZeroRowFanout{0};

        uint64_t gposCursiveRecords{0};
        uint64_t gposCursiveBothAnchors{0};

        uint64_t gposMarkBaseSubtables{0};
        uint64_t gposMarkBaseMarks{0};
        uint64_t gposMarkBaseBases{0};
        uint64_t gposMarkBaseAnchorCells{0};
        uint64_t gposMarkBasePresentAnchors{0};
        uint64_t gposMarkBaseMaxClasses{0};

        uint64_t gposMarkLigatureSubtables{0};
        uint64_t gposMarkLigatureMarks{0};
        uint64_t gposMarkLigatureLigatures{0};
        uint64_t gposMarkLigatureComponents{0};
        uint64_t gposMarkLigatureMaxComponents{0};
        uint64_t gposMarkLigatureMaxClasses{0};

        uint64_t gposMarkMarkSubtables{0};
        uint64_t gposMark1Records{0};
        uint64_t gposMark2Records{0};
        uint64_t gposMarkMarkAnchorCells{0};
        uint64_t gposMarkMarkPresentAnchors{0};
        uint64_t gposMarkMarkMaxClasses{0};

        uint64_t gposContextRules{0};
        uint64_t gposContextMaxRulesPerSubtable{0};
        uint64_t gposContextMaxInput{0};
        uint64_t gposContextMaxActions{0};

        uint64_t gposChainRules{0};
        uint64_t gposChainMaxRulesPerSubtable{0};
        uint64_t gposChainMaxBacktrack{0};
        uint64_t gposChainMaxInput{0};
        uint64_t gposChainMaxLookahead{0};
        uint64_t gposChainMaxActions{0};
    };

    struct OpenTypeShapingAnalysisBucket
    {
        const char* label;
        uint64_t min;
        uint64_t max;
    };

    struct OpenTypeShapingAnalysisExtreme
    {
        std::string metric{};
        uint64_t value{0};
        std::string face{};
        std::string source{};
    };

    static inline const char* openTypeShapingIROpName(OpenTypeShapingIROp op) noexcept
    {
        switch (op)
        {
        case OpenTypeShapingIROp::GsubSingle: return "GSUB_SINGLE";
        case OpenTypeShapingIROp::GsubMultiple: return "GSUB_MULTIPLE";
        case OpenTypeShapingIROp::GsubAlternate: return "GSUB_ALTERNATE";
        case OpenTypeShapingIROp::GsubLigature: return "GSUB_LIGATURE";
        case OpenTypeShapingIROp::GsubContext: return "GSUB_CONTEXT";
        case OpenTypeShapingIROp::GsubChainContext: return "GSUB_CHAIN_CONTEXT";
        case OpenTypeShapingIROp::GsubReverseChainSingle: return "GSUB_REVERSE_CHAIN_SINGLE";
        case OpenTypeShapingIROp::GposSingle: return "GPOS_SINGLE";
        case OpenTypeShapingIROp::GposPair: return "GPOS_PAIR";
        case OpenTypeShapingIROp::GposCursive: return "GPOS_CURSIVE";
        case OpenTypeShapingIROp::GposMarkBase: return "GPOS_MARK_BASE";
        case OpenTypeShapingIROp::GposMarkLigature: return "GPOS_MARK_LIGATURE";
        case OpenTypeShapingIROp::GposMarkMark: return "GPOS_MARK_MARK";
        case OpenTypeShapingIROp::GposContext: return "GPOS_CONTEXT";
        case OpenTypeShapingIROp::GposChainContext: return "GPOS_CHAIN_CONTEXT";
        default: return "INVALID";
        }
    }

    [[nodiscard]] static inline uint64_t openTypeShapingAnalysisGlyphSetMembers(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id) noexcept
    {
        const auto* set = ir.glyphSet(id);
        if (!set) return 0;
        uint64_t total = 0;
        const uint64_t end = uint64_t(set->rangeOffset) + set->rangeCount;
        if (end > ir.glyphRanges.size()) return 0;
        for (uint32_t i = 0; i < set->rangeCount; ++i)
        {
            const auto& r = ir.glyphRanges[set->rangeOffset + i];
            if (r.first <= r.last) total += uint64_t(r.last) - r.first + 1;
        }
        return total;
    }

    [[nodiscard]] static inline bool openTypeShapingAnalysisAdjustmentNonZero(const OpenTypeShapingIRPositionAdjustment& a) noexcept
    {
        return a.offsetX != 0 || a.offsetY != 0 || a.advanceX != 0 || a.advanceY != 0;
    }


    template<class RuleT>
    [[nodiscard]] static inline uint64_t openTypeShapingAnalysisMaxFirstFanout(const OpenTypeShapingIR& ir, const std::vector<OpenTypeShapingIRGlyphSetId>& sets, uint32_t ruleOffset, uint32_t ruleCount, const std::vector<RuleT>& rules)
    {
        const uint64_t ruleEnd = uint64_t(ruleOffset) + ruleCount;
        if (ruleEnd > rules.size()) return 0;

        std::vector<std::pair<uint32_t, int32_t>> events;
        for (uint32_t i = 0; i < ruleCount; ++i)
        {
            const auto& rule = rules[ruleOffset + i];
            if (!rule.inputCount || rule.inputSetOffset >= sets.size()) continue;

            const auto* set = ir.glyphSet(sets[rule.inputSetOffset]);
            if (!set || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) continue;

            for (uint32_t r = 0; r < set->rangeCount; ++r)
            {
                const auto& range = ir.glyphRanges[set->rangeOffset + r];
                if (range.first > range.last) continue;
                events.emplace_back(range.first, 1);
                events.emplace_back(uint32_t(range.last) + 1u, -1);
            }
        }

        if (events.empty()) return 0;
        std::sort(events.begin(), events.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

        uint64_t best = 0;
        int64_t active = 0;
        for (size_t i = 0; i < events.size();)
        {
            const uint32_t position = events[i].first;
            int64_t delta = 0;
            do { delta += events[i].second; ++i; } while (i < events.size() && events[i].first == position);
            active += delta;
            if (active > 0) best = std::max<uint64_t>(best, static_cast<uint64_t>(active));
        }
        return best;
    }

    template<class RuleT>
    [[nodiscard]] static inline uint64_t openTypeShapingAnalysisMaxPrefix2SameSetFanout(const std::vector<OpenTypeShapingIRGlyphSetId>& sets, uint32_t ruleOffset, uint32_t ruleCount, const std::vector<RuleT>& rules)
    {
        const uint64_t ruleEnd = uint64_t(ruleOffset) + ruleCount;
        if (ruleEnd > rules.size()) return 0;

        std::vector<std::pair<OpenTypeShapingIRGlyphSetId, OpenTypeShapingIRGlyphSetId>> prefixes;
        prefixes.reserve(ruleCount);

        for (uint32_t i = 0; i < ruleCount; ++i)
        {
            const auto& rule = rules[ruleOffset + i];
            if (rule.inputCount < 2 || uint64_t(rule.inputSetOffset) + 2u > sets.size()) continue;
            prefixes.emplace_back(sets[rule.inputSetOffset], sets[rule.inputSetOffset + 1u]);
        }

        if (prefixes.empty()) return 0;
        std::sort(prefixes.begin(), prefixes.end());

        uint64_t best = 1;
        uint64_t run = 1;
        for (size_t i = 1; i < prefixes.size(); ++i)
        {
            if (prefixes[i] == prefixes[i - 1]) ++run;
            else run = 1;
            best = std::max(best, run);
        }
        return best;
    }

    static inline void analyzeOpenTypeShapingIR(const OpenTypeShapingIR& ir, OpenTypeShapingAnalysisFace& out)
    {
        for (const auto& set : ir.glyphSets)
        {
            ++out.glyphSets;
            out.glyphSetRanges += set.rangeCount;
            out.maxGlyphSetRanges = std::max<uint64_t>(out.maxGlyphSetRanges, set.rangeCount);
            const uint64_t members = [&]() {
                uint64_t total = 0;
                const uint64_t end = uint64_t(set.rangeOffset) + set.rangeCount;
                if (end > ir.glyphRanges.size()) return uint64_t(0);
                for (uint32_t i = 0; i < set.rangeCount; ++i)
                {
                    const auto& r = ir.glyphRanges[set.rangeOffset + i];
                    if (r.first <= r.last) total += uint64_t(r.last) - r.first + 1;
                }
                return total;
            }();
            out.glyphSetMembers += members;
            out.maxGlyphSetMembers = std::max(out.maxGlyphSetMembers, members);
        }

        for (const auto& lookup : ir.lookups)
        {
            if (isOpenTypeShapingIRGsub(lookup.op)) ++out.gsubLookups;
            if (isOpenTypeShapingIRGpos(lookup.op)) ++out.gposLookups;
            const size_t opIndex = static_cast<size_t>(lookup.op);
            if (opIndex < out.opCounts.size()) ++out.opCounts[opIndex];

            const auto& f = lookup.filter;
            const bool filtered = f.flags != OpenTypeShapingIRFilterNone || f.markFilteringSet != kOpenTypeShapingIRInvalid || f.markAttachmentType != 0;
            out.filteredLookups += filtered ? 1 : 0;
            out.ignoreBaseLookups += (f.flags & OpenTypeShapingIRIgnoreBaseGlyphs) ? 1 : 0;
            out.ignoreLigatureLookups += (f.flags & OpenTypeShapingIRIgnoreLigatures) ? 1 : 0;
            out.ignoreMarkLookups += (f.flags & OpenTypeShapingIRIgnoreMarks) ? 1 : 0;
            out.markFilteringSetLookups += f.markFilteringSet != kOpenTypeShapingIRInvalid ? 1 : 0;
            out.markAttachmentTypeLookups += f.markAttachmentType != 0 ? 1 : 0;
            const uint32_t filterModes = ((f.flags & OpenTypeShapingIRIgnoreBaseGlyphs) ? 1u : 0u) + ((f.flags & OpenTypeShapingIRIgnoreLigatures) ? 1u : 0u) + ((f.flags & OpenTypeShapingIRIgnoreMarks) ? 1u : 0u) + (f.markFilteringSet != kOpenTypeShapingIRInvalid ? 1u : 0u) + (f.markAttachmentType != 0 ? 1u : 0u);
            out.combinedFilterLookups += filterModes > 1u ? 1u : 0u;
        }

        for (const auto& s : ir.gsubSingleSubtables)
        {
            out.gsubSingleMappings += s.pairCount;
            out.gsubSingleMaxMappingsPerSubtable = std::max<uint64_t>(out.gsubSingleMaxMappingsPerSubtable, s.pairCount);
        }

        for (const auto& s : ir.gsubMultipleSubtables) out.gsubMultipleMappings += s.pairCount;
        for (const auto& seq : ir.gsubMultipleSequences)
        {
            out.gsubMultipleOutputGlyphs += seq.glyphCount;
            out.gsubMultipleMaxExpansion = std::max<uint64_t>(out.gsubMultipleMaxExpansion, seq.glyphCount);
        }

        for (const auto& s : ir.gsubAlternateSubtables) out.gsubAlternateMappings += s.pairCount;
        for (const auto& set : ir.gsubAlternateSets)
        {
            out.gsubAlternateGlyphs += set.glyphCount;
            out.gsubAlternateMaxSet = std::max<uint64_t>(out.gsubAlternateMaxSet, set.glyphCount);
        }

        for (const auto& pair : ir.gsubLigaturePairs)
        {
            ++out.gsubLigatureFirstGlyphs;
            out.gsubLigatureRules += pair.ligatureCount;
            out.gsubLigatureMaxFanout = std::max<uint64_t>(out.gsubLigatureMaxFanout, pair.ligatureCount);

            const uint64_t end = uint64_t(pair.ligatureOffset) + pair.ligatureCount;
            if (end > ir.gsubLigatures.size()) continue;
            for (uint32_t i = 0; i < pair.ligatureCount; ++i)
            {
                const auto& lig = ir.gsubLigatures[pair.ligatureOffset + i];
                out.gsubLigatureComponents += lig.componentCount;
                out.gsubLigatureMaxComponents = std::max<uint64_t>(out.gsubLigatureMaxComponents, lig.componentCount);
            }

            // Approximate shared-prefix fanout by counting identical first one/two trailing components.
            for (uint32_t i = 0; i < pair.ligatureCount; ++i)
            {
                const auto& a = ir.gsubLigatures[pair.ligatureOffset + i];
                if (a.componentCount < 2 || a.componentOffset >= ir.gsubLigatureComponents.size()) continue;
                uint64_t prefix2 = 0;
                uint64_t prefix3 = 0;
                for (uint32_t j = 0; j < pair.ligatureCount; ++j)
                {
                    const auto& b = ir.gsubLigatures[pair.ligatureOffset + j];
                    if (b.componentCount < 2 || b.componentOffset >= ir.gsubLigatureComponents.size()) continue;
                    if (ir.gsubLigatureComponents[a.componentOffset] == ir.gsubLigatureComponents[b.componentOffset]) ++prefix2;
                    if (a.componentCount >= 3 && b.componentCount >= 3 && a.componentOffset + 1 < ir.gsubLigatureComponents.size() && b.componentOffset + 1 < ir.gsubLigatureComponents.size() && ir.gsubLigatureComponents[a.componentOffset] == ir.gsubLigatureComponents[b.componentOffset] && ir.gsubLigatureComponents[a.componentOffset + 1] == ir.gsubLigatureComponents[b.componentOffset + 1]) ++prefix3;
                }
                out.gsubLigatureMaxSharedPrefix2 = std::max(out.gsubLigatureMaxSharedPrefix2, prefix2);
                out.gsubLigatureMaxSharedPrefix3 = std::max(out.gsubLigatureMaxSharedPrefix3, prefix3);
            }
        }

        for (const auto& s : ir.gsubContextSubtables)
        {
            out.gsubContextRules += s.ruleCount;
            out.gsubContextMaxRulesPerSubtable = std::max<uint64_t>(out.gsubContextMaxRulesPerSubtable, s.ruleCount);
            const uint64_t end = uint64_t(s.ruleOffset) + s.ruleCount;
            if (end > ir.gsubContextRules.size()) continue;
            for (uint32_t i = 0; i < s.ruleCount; ++i)
            {
                const auto& r = ir.gsubContextRules[s.ruleOffset + i];
                out.gsubContextMaxInput = std::max<uint64_t>(out.gsubContextMaxInput, r.inputCount);
                out.gsubContextMaxActions = std::max<uint64_t>(out.gsubContextMaxActions, r.lookupCount);
                if (r.inputCount && r.inputSetOffset < ir.gsubContextInputSets.size())
                {
                    const auto setId = ir.gsubContextInputSets[r.inputSetOffset];
                    out.gsubContextMaxFirstSetMembers = std::max(out.gsubContextMaxFirstSetMembers, openTypeShapingAnalysisGlyphSetMembers(ir, setId));
                }
            }
            // Conservative ambiguity proxy retained for comparison with schema v1.
            out.gsubContextMaxFirstFanoutApprox = std::max<uint64_t>(out.gsubContextMaxFirstFanoutApprox, s.ruleCount);
            out.gsubContextMaxFirstFanout = std::max(out.gsubContextMaxFirstFanout, openTypeShapingAnalysisMaxFirstFanout(ir, ir.gsubContextInputSets, s.ruleOffset, s.ruleCount, ir.gsubContextRules));
            out.gsubContextMaxPrefix2SameSetFanout = std::max(out.gsubContextMaxPrefix2SameSetFanout, openTypeShapingAnalysisMaxPrefix2SameSetFanout(ir.gsubContextInputSets, s.ruleOffset, s.ruleCount, ir.gsubContextRules));
        }

        for (const auto& s : ir.gsubChainContextSubtables)
        {
            out.gsubChainRules += s.ruleCount;
            out.gsubChainMaxRulesPerSubtable = std::max<uint64_t>(out.gsubChainMaxRulesPerSubtable, s.ruleCount);
            const uint64_t end = uint64_t(s.ruleOffset) + s.ruleCount;
            if (end > ir.gsubChainContextRules.size()) continue;
            for (uint32_t i = 0; i < s.ruleCount; ++i)
            {
                const auto& r = ir.gsubChainContextRules[s.ruleOffset + i];
                out.gsubChainMaxBacktrack = std::max<uint64_t>(out.gsubChainMaxBacktrack, r.backtrackCount);
                out.gsubChainMaxInput = std::max<uint64_t>(out.gsubChainMaxInput, r.inputCount);
                out.gsubChainMaxLookahead = std::max<uint64_t>(out.gsubChainMaxLookahead, r.lookaheadCount);
                out.gsubChainMaxActions = std::max<uint64_t>(out.gsubChainMaxActions, r.lookupCount);
            }
            out.gsubChainMaxFirstFanout = std::max(out.gsubChainMaxFirstFanout, openTypeShapingAnalysisMaxFirstFanout(ir, ir.gsubChainContextSets, s.ruleOffset, s.ruleCount, ir.gsubChainContextRules));
            out.gsubChainMaxPrefix2SameSetFanout = std::max(out.gsubChainMaxPrefix2SameSetFanout, openTypeShapingAnalysisMaxPrefix2SameSetFanout(ir.gsubChainContextSets, s.ruleOffset, s.ruleCount, ir.gsubChainContextRules));
        }

        for (const auto& s : ir.gsubReverseChainSingleSubtables)
        {
            ++out.gsubReverseSubtables;
            out.gsubReversePairs += s.pairCount;
            out.gsubReverseMaxBacktrack = std::max<uint64_t>(out.gsubReverseMaxBacktrack, s.backtrackCount);
            out.gsubReverseMaxLookahead = std::max<uint64_t>(out.gsubReverseMaxLookahead, s.lookaheadCount);
        }

        for (const auto& s : ir.gposSingleSubtables) out.gposSingleMappings += s.pairCount;

        for (const auto& s : ir.gposPairSubtables)
        {
            if (s.kind == OpenTypeShapingIRGposPairKind::Explicit && s.payloadIndex < ir.gposPairExplicitSubtables.size())
            {
                const auto& p = ir.gposPairExplicitSubtables[s.payloadIndex];
                ++out.gposPairExplicitSubtables;
                out.gposPairExplicitPairs += p.pairCount;
                out.gposPairMaxExplicitPairs = std::max<uint64_t>(out.gposPairMaxExplicitPairs, p.pairCount);
                const uint64_t pairEnd = uint64_t(p.pairOffset) + p.pairCount;
                if (p.pairCount && pairEnd <= ir.gposPairExplicitPairs.size())
                {
                    std::vector<uint16_t> firstGlyphs;
                    firstGlyphs.reserve(p.pairCount);
                    for (uint32_t i = 0; i < p.pairCount; ++i) firstGlyphs.push_back(ir.gposPairExplicitPairs[p.pairOffset + i].first);
                    std::sort(firstGlyphs.begin(), firstGlyphs.end());

                    uint64_t run = 1;
                    out.gposPairMaxSecondFanout = std::max<uint64_t>(out.gposPairMaxSecondFanout, 1);
                    for (size_t i = 1; i < firstGlyphs.size(); ++i)
                    {
                        if (firstGlyphs[i] == firstGlyphs[i - 1]) ++run;
                        else run = 1;
                        out.gposPairMaxSecondFanout = std::max(out.gposPairMaxSecondFanout, run);
                    }
                }
            }
            else if (s.kind == OpenTypeShapingIRGposPairKind::Class && s.payloadIndex < ir.gposPairClassSubtables.size())
            {
                const auto& p = ir.gposPairClassSubtables[s.payloadIndex];
                ++out.gposPairClassSubtables;
                const uint64_t cells = uint64_t(p.firstClassCount) * p.secondClassCount;
                out.gposPairClassCells += cells;
                out.gposPairMaxClassCells = std::max(out.gposPairMaxClassCells, cells);
                out.gposPairMaxClassDimension = std::max<uint64_t>(out.gposPairMaxClassDimension, std::max(p.firstClassCount, p.secondClassCount));
                const uint64_t end = uint64_t(p.valueOffset) + cells;
                if (cells && end <= ir.gposPairClassValues.size())
                {
                    uint64_t nonZero = 0;
                    uint64_t maxRow = 0;
                    for (uint32_t row = 0; row < p.firstClassCount; ++row)
                    {
                        uint64_t rowNonZero = 0;
                        for (uint32_t col = 0; col < p.secondClassCount; ++col)
                        {
                            const auto& v = ir.gposPairClassValues[p.valueOffset + uint64_t(row) * p.secondClassCount + col];
                            if (openTypeShapingAnalysisAdjustmentNonZero(v.firstAdjustment) || openTypeShapingAnalysisAdjustmentNonZero(v.secondAdjustment)) { ++nonZero; ++rowNonZero; }
                        }
                        maxRow = std::max(maxRow, rowNonZero);
                    }
                    out.gposPairClassNonZeroCells += nonZero;
                    out.gposPairClassMaxNonZeroRowFanout = std::max(out.gposPairClassMaxNonZeroRowFanout, maxRow);
                    const uint64_t densityPermille = (nonZero * 1000u + cells / 2u) / cells;
                    if (!out.gposPairClassDensitySamples) out.gposPairClassMinDensityPermille = densityPermille;
                    else out.gposPairClassMinDensityPermille = std::min(out.gposPairClassMinDensityPermille, densityPermille);
                    out.gposPairClassMaxDensityPermille = std::max(out.gposPairClassMaxDensityPermille, densityPermille);
                    ++out.gposPairClassDensitySamples;
                }
            }
        }

        for (const auto& r : ir.gposCursiveRecords)
        {
            ++out.gposCursiveRecords;
            if (r.hasEntry && r.hasExit) ++out.gposCursiveBothAnchors;
        }

        for (const auto& s : ir.gposMarkBaseSubtables)
        {
            ++out.gposMarkBaseSubtables;
            out.gposMarkBaseMarks += s.markCount;
            out.gposMarkBaseBases += s.baseCount;
            out.gposMarkBaseMaxClasses = std::max<uint64_t>(out.gposMarkBaseMaxClasses, s.markClassCount);
            const uint64_t cells = uint64_t(s.baseCount) * s.markClassCount;
            out.gposMarkBaseAnchorCells += cells;
            const uint64_t end = uint64_t(s.baseAnchorOffset) + cells;
            if (end <= ir.gposMarkBaseAnchorRefs.size()) for (uint64_t i = 0; i < cells; ++i) if (ir.gposMarkBaseAnchorRefs[s.baseAnchorOffset + i] != kOpenTypeShapingIRInvalid) ++out.gposMarkBasePresentAnchors;
        }

        for (const auto& s : ir.gposMarkLigatureSubtables)
        {
            ++out.gposMarkLigatureSubtables;
            out.gposMarkLigatureMarks += s.markCount;
            out.gposMarkLigatureLigatures += s.ligatureCount;
            out.gposMarkLigatureMaxClasses = std::max<uint64_t>(out.gposMarkLigatureMaxClasses, s.markClassCount);
            const uint64_t end = uint64_t(s.ligatureOffset) + s.ligatureCount;
            if (end <= ir.gposMarkLigatureRecords.size()) for (uint32_t i = 0; i < s.ligatureCount; ++i)
            {
                const auto& r = ir.gposMarkLigatureRecords[s.ligatureOffset + i];
                out.gposMarkLigatureComponents += r.componentCount;
                out.gposMarkLigatureMaxComponents = std::max<uint64_t>(out.gposMarkLigatureMaxComponents, r.componentCount);
            }
        }

        for (const auto& s : ir.gposMarkMarkSubtables)
        {
            ++out.gposMarkMarkSubtables;
            out.gposMark1Records += s.mark1Count;
            out.gposMark2Records += s.mark2Count;
            out.gposMarkMarkMaxClasses = std::max<uint64_t>(out.gposMarkMarkMaxClasses, s.markClassCount);
            const uint64_t cells = uint64_t(s.mark2Count) * s.markClassCount;
            out.gposMarkMarkAnchorCells += cells;
            const uint64_t end = uint64_t(s.mark2AnchorOffset) + cells;
            if (end <= ir.gposMark2AnchorRefs.size()) for (uint64_t i = 0; i < cells; ++i) if (ir.gposMark2AnchorRefs[s.mark2AnchorOffset + i] != kOpenTypeShapingIRInvalid) ++out.gposMarkMarkPresentAnchors;
        }

        for (const auto& s : ir.gposContextSubtables)
        {
            out.gposContextRules += s.ruleCount;
            out.gposContextMaxRulesPerSubtable = std::max<uint64_t>(out.gposContextMaxRulesPerSubtable, s.ruleCount);
            const uint64_t end = uint64_t(s.ruleOffset) + s.ruleCount;
            if (end > ir.gposContextRules.size()) continue;
            for (uint32_t i = 0; i < s.ruleCount; ++i)
            {
                const auto& r = ir.gposContextRules[s.ruleOffset + i];
                out.gposContextMaxInput = std::max<uint64_t>(out.gposContextMaxInput, r.inputCount);
                out.gposContextMaxActions = std::max<uint64_t>(out.gposContextMaxActions, r.lookupCount);
            }
        }

        for (const auto& s : ir.gposChainContextSubtables)
        {
            out.gposChainRules += s.ruleCount;
            out.gposChainMaxRulesPerSubtable = std::max<uint64_t>(out.gposChainMaxRulesPerSubtable, s.ruleCount);
            const uint64_t end = uint64_t(s.ruleOffset) + s.ruleCount;
            if (end > ir.gposChainContextRules.size()) continue;
            for (uint32_t i = 0; i < s.ruleCount; ++i)
            {
                const auto& r = ir.gposChainContextRules[s.ruleOffset + i];
                out.gposChainMaxBacktrack = std::max<uint64_t>(out.gposChainMaxBacktrack, r.backtrackCount);
                out.gposChainMaxInput = std::max<uint64_t>(out.gposChainMaxInput, r.inputCount);
                out.gposChainMaxLookahead = std::max<uint64_t>(out.gposChainMaxLookahead, r.lookaheadCount);
                out.gposChainMaxActions = std::max<uint64_t>(out.gposChainMaxActions, r.lookupCount);
            }
        }
    }

    class OpenTypeShapingCorpusAnalysis
    {
    public:
        void add(std::string_view label, std::string_view source, const OpenTypeShapingIR& gsub, const OpenTypeShapingIR& gpos)
        {
            OpenTypeShapingAnalysisFace face;
            face.label.assign(label);
            face.source.assign(source);
            analyzeOpenTypeShapingIR(gsub, face);
            analyzeOpenTypeShapingIR(gpos, face);
            fFaces.push_back(std::move(face));
        }

        void add(std::string_view label, std::string_view source, const OpenTypeShapingIR& ir)
        {
            OpenTypeShapingAnalysisFace face;
            face.label.assign(label);
            face.source.assign(source);
            analyzeOpenTypeShapingIR(ir, face);
            fFaces.push_back(std::move(face));
        }

        [[nodiscard]] const std::vector<OpenTypeShapingAnalysisFace>& faces() const noexcept { return fFaces; }

        void print(FILE* out = stdout) const
        {
            std::fprintf(out, "REPORT_BEGIN\n");
            std::fprintf(out, "report_type: OpenTypeShapingStructuralAnalysis\n");
            std::fprintf(out, "schema_version: 2\n");
            std::fprintf(out, "purpose: structural census for GSUB/GPOS workload selection and optimization\n");
            std::fprintf(out, "face_count: %zu\n", fFaces.size());
            std::fprintf(out, "units: counts unless otherwise stated\n\n");

            printSummary(out);
            printOperationHistogram(out);
            printFilterSummary(out);
            printMetricHistograms(out);
            printExtremes(out);
            printFaceRecords(out);

            std::fprintf(out, "REPORT_END\n");
        }

    private:
        template<typename Fn>
        static uint64_t sum(const std::vector<OpenTypeShapingAnalysisFace>& faces, Fn fn)
        {
            uint64_t v = 0;
            for (const auto& f : faces) v += fn(f);
            return v;
        }

        template<typename Fn>
        static uint64_t maximum(const std::vector<OpenTypeShapingAnalysisFace>& faces, Fn fn)
        {
            uint64_t v = 0;
            for (const auto& f : faces) v = std::max(v, fn(f));
            return v;
        }

        template<typename Fn>
        void printHistogram(FILE* out, const char* name, const OpenTypeShapingAnalysisBucket* buckets, size_t bucketCount, Fn fn) const
        {
            std::fprintf(out, "HISTOGRAM_BEGIN name=%s\n", name);
            for (size_t b = 0; b < bucketCount; ++b)
            {
                uint64_t count = 0;
                for (const auto& f : fFaces)
                {
                    const uint64_t v = fn(f);
                    if (v >= buckets[b].min && v <= buckets[b].max) ++count;
                }
                std::fprintf(out, "bucket: %s | faces: %llu\n", buckets[b].label, static_cast<unsigned long long>(count));
            }
            std::fprintf(out, "HISTOGRAM_END\n\n");
        }

        template<typename Fn>
        void printExtreme(FILE* out, const char* metric, Fn fn) const
        {
            if (fFaces.empty()) return;
            const OpenTypeShapingAnalysisFace* best = &fFaces.front();
            for (const auto& f : fFaces) if (fn(f) > fn(*best)) best = &f;
            std::fprintf(out, "EXTREME metric=%s | value=%llu | face=%s | source=%s\n", metric,
                static_cast<unsigned long long>(fn(*best)), best->label.c_str(), best->source.c_str());
        }

        void printSummary(FILE* out) const
        {
            std::fprintf(out, "SECTION_BEGIN name=corpus_summary\n");
            std::fprintf(out, "METRIC faces: %zu\n", fFaces.size());
            std::fprintf(out, "METRIC gsub_lookups: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.gsubLookups; })));
            std::fprintf(out, "METRIC gpos_lookups: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.gposLookups; })));
            std::fprintf(out, "METRIC glyph_sets: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.glyphSets; })));
            std::fprintf(out, "METRIC glyph_set_ranges: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.glyphSetRanges; })));
            std::fprintf(out, "METRIC filtered_lookups: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.filteredLookups; })));
            std::fprintf(out, "SECTION_END\n\n");
        }

        void printOperationHistogram(FILE* out) const
        {
            std::fprintf(out, "SECTION_BEGIN name=lookup_operation_totals\n");
            for (uint32_t op = static_cast<uint32_t>(OpenTypeShapingIROp::GsubSingle); op <= static_cast<uint32_t>(OpenTypeShapingIROp::GposLAST); ++op)
            {
                uint64_t total = 0;
                for (const auto& f : fFaces) if (op < f.opCounts.size()) total += f.opCounts[op];
                std::fprintf(out, "OP name=%s | count=%llu\n", openTypeShapingIROpName(static_cast<OpenTypeShapingIROp>(op)), static_cast<unsigned long long>(total));
            }
            std::fprintf(out, "SECTION_END\n\n");
        }

        void printFilterSummary(FILE* out) const
        {
            std::fprintf(out, "SECTION_BEGIN name=filter_mode_totals\n");
            std::fprintf(out, "METRIC ignore_bases: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.ignoreBaseLookups; })));
            std::fprintf(out, "METRIC ignore_ligatures: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.ignoreLigatureLookups; })));
            std::fprintf(out, "METRIC ignore_marks: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.ignoreMarkLookups; })));
            std::fprintf(out, "METRIC mark_attachment_type: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.markAttachmentTypeLookups; })));
            std::fprintf(out, "METRIC mark_filtering_set: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.markFilteringSetLookups; })));
            std::fprintf(out, "METRIC combined: %llu\n", static_cast<unsigned long long>(sum(fFaces, [](const auto& f){ return f.combinedFilterLookups; })));
            std::fprintf(out, "SECTION_END\n\n");
        }

        void printMetricHistograms(FILE* out) const
        {
            static constexpr OpenTypeShapingAnalysisBucket smallBuckets[] = {
                {"0",0,0},{"1",1,1},{"2-4",2,4},{"5-8",5,8},{"9-16",9,16},{"17-32",17,32},{"33-64",33,64},{"65-128",65,128},{"129-256",129,256},{"257+",257,std::numeric_limits<uint64_t>::max()}
            };
            static constexpr OpenTypeShapingAnalysisBucket largeBuckets[] = {
                {"0",0,0},{"1-8",1,8},{"9-32",9,32},{"33-128",33,128},{"129-512",129,512},{"513-2048",513,2048},{"2049-8192",2049,8192},{"8193+",8193,std::numeric_limits<uint64_t>::max()}
            };

            printHistogram(out, "gsub_ligature_max_fanout", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gsubLigatureMaxFanout; });
            printHistogram(out, "gsub_ligature_max_components", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gsubLigatureMaxComponents; });
            printHistogram(out, "gsub_context_max_rules_per_subtable", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gsubContextMaxRulesPerSubtable; });
            printHistogram(out, "gsub_context_max_input", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gsubContextMaxInput; });
            printHistogram(out, "gsub_context_max_first_fanout", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gsubContextMaxFirstFanout; });
            printHistogram(out, "gsub_context_max_prefix2_same_set_fanout", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gsubContextMaxPrefix2SameSetFanout; });
            printHistogram(out, "gsub_chain_max_backtrack", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gsubChainMaxBacktrack; });
            printHistogram(out, "gsub_chain_max_lookahead", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gsubChainMaxLookahead; });
            printHistogram(out, "gsub_chain_max_first_fanout", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gsubChainMaxFirstFanout; });
            printHistogram(out, "gsub_chain_max_prefix2_same_set_fanout", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gsubChainMaxPrefix2SameSetFanout; });
            printHistogram(out, "gpos_pair_max_explicit_pairs", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gposPairMaxExplicitPairs; });
            printHistogram(out, "gpos_pair_max_second_fanout", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gposPairMaxSecondFanout; });
            printHistogram(out, "gpos_pair_max_class_cells", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.gposPairMaxClassCells; });
            printHistogram(out, "gpos_mark_base_max_classes", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gposMarkBaseMaxClasses; });
            printHistogram(out, "gpos_mark_ligature_max_components", smallBuckets, std::size(smallBuckets), [](const auto& f){ return f.gposMarkLigatureMaxComponents; });
            printHistogram(out, "max_glyph_set_members", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.maxGlyphSetMembers; });
            printHistogram(out, "max_glyph_set_ranges", largeBuckets, std::size(largeBuckets), [](const auto& f){ return f.maxGlyphSetRanges; });
        }

        void printExtremes(FILE* out) const
        {
            std::fprintf(out, "SECTION_BEGIN name=extremes\n");
            printExtreme(out, "gsub_single_max_mappings_per_subtable", [](const auto& f){ return f.gsubSingleMaxMappingsPerSubtable; });
            printExtreme(out, "gsub_multiple_max_expansion", [](const auto& f){ return f.gsubMultipleMaxExpansion; });
            printExtreme(out, "gsub_alternate_max_set", [](const auto& f){ return f.gsubAlternateMaxSet; });
            printExtreme(out, "gsub_ligature_max_fanout", [](const auto& f){ return f.gsubLigatureMaxFanout; });
            printExtreme(out, "gsub_ligature_max_components", [](const auto& f){ return f.gsubLigatureMaxComponents; });
            printExtreme(out, "gsub_ligature_max_shared_prefix2", [](const auto& f){ return f.gsubLigatureMaxSharedPrefix2; });
            printExtreme(out, "gsub_ligature_max_shared_prefix3", [](const auto& f){ return f.gsubLigatureMaxSharedPrefix3; });
            printExtreme(out, "gsub_context_max_rules_per_subtable", [](const auto& f){ return f.gsubContextMaxRulesPerSubtable; });
            printExtreme(out, "gsub_context_max_input", [](const auto& f){ return f.gsubContextMaxInput; });
            printExtreme(out, "gsub_context_max_first_fanout", [](const auto& f){ return f.gsubContextMaxFirstFanout; });
            printExtreme(out, "gsub_context_max_prefix2_same_set_fanout", [](const auto& f){ return f.gsubContextMaxPrefix2SameSetFanout; });
            printExtreme(out, "gsub_chain_max_backtrack", [](const auto& f){ return f.gsubChainMaxBacktrack; });
            printExtreme(out, "gsub_chain_max_lookahead", [](const auto& f){ return f.gsubChainMaxLookahead; });
            printExtreme(out, "gsub_chain_max_first_fanout", [](const auto& f){ return f.gsubChainMaxFirstFanout; });
            printExtreme(out, "gsub_chain_max_prefix2_same_set_fanout", [](const auto& f){ return f.gsubChainMaxPrefix2SameSetFanout; });
            printExtreme(out, "gpos_pair_max_explicit_pairs", [](const auto& f){ return f.gposPairMaxExplicitPairs; });
            printExtreme(out, "gpos_pair_max_second_fanout", [](const auto& f){ return f.gposPairMaxSecondFanout; });
            printExtreme(out, "gpos_pair_class_max_nonzero_row_fanout", [](const auto& f){ return f.gposPairClassMaxNonZeroRowFanout; });
            printExtreme(out, "combined_filter_lookups", [](const auto& f){ return f.combinedFilterLookups; });
            printExtreme(out, "gpos_pair_max_class_cells", [](const auto& f){ return f.gposPairMaxClassCells; });
            printExtreme(out, "gpos_pair_max_class_dimension", [](const auto& f){ return f.gposPairMaxClassDimension; });
            printExtreme(out, "gpos_mark_base_max_classes", [](const auto& f){ return f.gposMarkBaseMaxClasses; });
            printExtreme(out, "gpos_mark_ligature_max_components", [](const auto& f){ return f.gposMarkLigatureMaxComponents; });
            printExtreme(out, "gpos_mark_mark_max_classes", [](const auto& f){ return f.gposMarkMarkMaxClasses; });
            printExtreme(out, "max_glyph_set_members", [](const auto& f){ return f.maxGlyphSetMembers; });
            printExtreme(out, "max_glyph_set_ranges", [](const auto& f){ return f.maxGlyphSetRanges; });
            std::fprintf(out, "SECTION_END\n\n");
        }

        void printFaceRecords(FILE* out) const
        {
            std::fprintf(out, "SECTION_BEGIN name=face_records\n");
            for (const auto& f : fFaces)
            {
                std::fprintf(out, "FACE_BEGIN name=%s | source=%s\n", f.label.c_str(), f.source.c_str());
                std::fprintf(out, "METRIC gsub_lookups=%llu\n", static_cast<unsigned long long>(f.gsubLookups));
                std::fprintf(out, "METRIC gpos_lookups=%llu\n", static_cast<unsigned long long>(f.gposLookups));
                std::fprintf(out, "METRIC filtered_lookups=%llu\n", static_cast<unsigned long long>(f.filteredLookups));
                std::fprintf(out, "METRIC filter_ignore_bases=%llu\n", static_cast<unsigned long long>(f.ignoreBaseLookups));
                std::fprintf(out, "METRIC filter_ignore_ligatures=%llu\n", static_cast<unsigned long long>(f.ignoreLigatureLookups));
                std::fprintf(out, "METRIC filter_ignore_marks=%llu\n", static_cast<unsigned long long>(f.ignoreMarkLookups));
                std::fprintf(out, "METRIC filter_mark_attachment_type=%llu\n", static_cast<unsigned long long>(f.markAttachmentTypeLookups));
                std::fprintf(out, "METRIC filter_mark_filtering_set=%llu\n", static_cast<unsigned long long>(f.markFilteringSetLookups));
                std::fprintf(out, "METRIC filter_combined=%llu\n", static_cast<unsigned long long>(f.combinedFilterLookups));
                std::fprintf(out, "METRIC max_glyph_set_members=%llu\n", static_cast<unsigned long long>(f.maxGlyphSetMembers));
                std::fprintf(out, "METRIC max_glyph_set_ranges=%llu\n", static_cast<unsigned long long>(f.maxGlyphSetRanges));
                std::fprintf(out, "METRIC gsub_single_mappings=%llu\n", static_cast<unsigned long long>(f.gsubSingleMappings));
                std::fprintf(out, "METRIC gsub_multiple_max_expansion=%llu\n", static_cast<unsigned long long>(f.gsubMultipleMaxExpansion));
                std::fprintf(out, "METRIC gsub_ligature_rules=%llu\n", static_cast<unsigned long long>(f.gsubLigatureRules));
                std::fprintf(out, "METRIC gsub_ligature_max_fanout=%llu\n", static_cast<unsigned long long>(f.gsubLigatureMaxFanout));
                std::fprintf(out, "METRIC gsub_ligature_max_components=%llu\n", static_cast<unsigned long long>(f.gsubLigatureMaxComponents));
                std::fprintf(out, "METRIC gsub_context_rules=%llu\n", static_cast<unsigned long long>(f.gsubContextRules));
                std::fprintf(out, "METRIC gsub_context_max_rules_per_subtable=%llu\n", static_cast<unsigned long long>(f.gsubContextMaxRulesPerSubtable));
                std::fprintf(out, "METRIC gsub_context_max_input=%llu\n", static_cast<unsigned long long>(f.gsubContextMaxInput));
                std::fprintf(out, "METRIC gsub_context_max_first_fanout=%llu\n", static_cast<unsigned long long>(f.gsubContextMaxFirstFanout));
                std::fprintf(out, "METRIC gsub_context_max_prefix2_same_set_fanout=%llu\n", static_cast<unsigned long long>(f.gsubContextMaxPrefix2SameSetFanout));
                std::fprintf(out, "METRIC gsub_chain_rules=%llu\n", static_cast<unsigned long long>(f.gsubChainRules));
                std::fprintf(out, "METRIC gsub_chain_max_backtrack=%llu\n", static_cast<unsigned long long>(f.gsubChainMaxBacktrack));
                std::fprintf(out, "METRIC gsub_chain_max_lookahead=%llu\n", static_cast<unsigned long long>(f.gsubChainMaxLookahead));
                std::fprintf(out, "METRIC gsub_chain_max_first_fanout=%llu\n", static_cast<unsigned long long>(f.gsubChainMaxFirstFanout));
                std::fprintf(out, "METRIC gsub_chain_max_prefix2_same_set_fanout=%llu\n", static_cast<unsigned long long>(f.gsubChainMaxPrefix2SameSetFanout));
                std::fprintf(out, "METRIC gpos_pair_explicit_pairs=%llu\n", static_cast<unsigned long long>(f.gposPairExplicitPairs));
                std::fprintf(out, "METRIC gpos_pair_max_second_fanout=%llu\n", static_cast<unsigned long long>(f.gposPairMaxSecondFanout));
                std::fprintf(out, "METRIC gpos_pair_class_cells=%llu\n", static_cast<unsigned long long>(f.gposPairClassCells));
                std::fprintf(out, "METRIC gpos_pair_class_nonzero_cells=%llu\n", static_cast<unsigned long long>(f.gposPairClassNonZeroCells));
                std::fprintf(out, "METRIC gpos_pair_class_min_density_permille=%llu\n", static_cast<unsigned long long>(f.gposPairClassMinDensityPermille));
                std::fprintf(out, "METRIC gpos_pair_class_max_density_permille=%llu\n", static_cast<unsigned long long>(f.gposPairClassMaxDensityPermille));
                std::fprintf(out, "METRIC gpos_pair_class_max_nonzero_row_fanout=%llu\n", static_cast<unsigned long long>(f.gposPairClassMaxNonZeroRowFanout));
                std::fprintf(out, "METRIC gpos_mark_base_marks=%llu\n", static_cast<unsigned long long>(f.gposMarkBaseMarks));
                std::fprintf(out, "METRIC gpos_mark_base_bases=%llu\n", static_cast<unsigned long long>(f.gposMarkBaseBases));
                std::fprintf(out, "METRIC gpos_mark_base_max_classes=%llu\n", static_cast<unsigned long long>(f.gposMarkBaseMaxClasses));
                std::fprintf(out, "METRIC gpos_mark_ligature_max_components=%llu\n", static_cast<unsigned long long>(f.gposMarkLigatureMaxComponents));
                std::fprintf(out, "METRIC gpos_mark_mark_max_classes=%llu\n", static_cast<unsigned long long>(f.gposMarkMarkMaxClasses));
                std::fprintf(out, "FACE_END\n");
            }
            std::fprintf(out, "SECTION_END\n\n");
        }

        std::vector<OpenTypeShapingAnalysisFace> fFaces{};
    };
}
