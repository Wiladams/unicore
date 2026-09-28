// opentype_shaping_workloads.h
#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "opentype_shaping_ir.h"

namespace waavs
{
    enum class OpenTypeShapingWorkloadKind : uint8_t
    {
        HitEarly,
        HitLate,
        MissEarly,
        MissLate,
        Mixed,
        FilterStress,
        ExpansionStress,
        ContractionStress
    };

    struct OpenTypeShapingWorkload
    {
        std::string name{};
        OpenTypeShapingWorkloadKind kind{OpenTypeShapingWorkloadKind::HitEarly};
        OpenTypeShapingIRLookupId lookup{kOpenTypeShapingIRInvalid};
        std::vector<uint16_t> glyphs{};
        uint32_t targetOffset{0};
        uint32_t repeatCount{1};
        uint32_t expectedLogicalLength{0};
        uint32_t structuralValue{0};
    };

    [[nodiscard]] static inline bool openTypeShapingGlyphSetContains(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id, uint16_t glyph) noexcept
    {
        const auto* set = ir.glyphSet(id);
        if (!set || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

        uint32_t lo = 0;
        uint32_t hi = set->rangeCount;
        while (lo < hi)
        {
            const uint32_t mid = lo + ((hi - lo) >> 1);
            const auto& range = ir.glyphRanges[set->rangeOffset + mid];
            if (glyph < range.first) hi = mid;
            else if (glyph > range.last) lo = mid + 1;
            else return true;
        }
        return false;
    }

    [[nodiscard]] static inline bool openTypeShapingFirstGlyphFromSet(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id, uint16_t& glyph) noexcept
    {
        const auto* set = ir.glyphSet(id);
        if (!set || !set->rangeCount || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;
        glyph = ir.glyphRanges[set->rangeOffset].first;
        return true;
    }

    [[nodiscard]] static inline bool openTypeShapingLastGlyphFromSet(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id, uint16_t& glyph) noexcept
    {
        const auto* set = ir.glyphSet(id);
        if (!set || !set->rangeCount || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;
        glyph = ir.glyphRanges[set->rangeOffset + set->rangeCount - 1].last;
        return true;
    }

    [[nodiscard]] static inline bool openTypeShapingGlyphOutsideSet(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId id, uint16_t& glyph) noexcept
    {
        const auto* set = ir.glyphSet(id);
        if (!set || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

        uint32_t candidate = 0;
        for (uint32_t i = 0; i < set->rangeCount; ++i)
        {
            const auto& range = ir.glyphRanges[set->rangeOffset + i];
            if (candidate < range.first) { glyph = static_cast<uint16_t>(candidate); return true; }
            if (candidate <= range.last) candidate = uint32_t(range.last) + 1u;
            if (candidate > 0xFFFFu) return false;
        }

        glyph = static_cast<uint16_t>(candidate);
        return true;
    }

    [[nodiscard]] static inline uint16_t openTypeShapingGlyphClassValue(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphClassMapId id, uint16_t glyph) noexcept
    {
        const auto* map = ir.glyphClassMap(id);
        if (!map || uint64_t(map->rangeOffset) + map->rangeCount > ir.glyphClassRanges.size()) return 0;

        uint32_t lo = 0;
        uint32_t hi = map->rangeCount;
        while (lo < hi)
        {
            const uint32_t mid = lo + ((hi - lo) >> 1);
            const auto& range = ir.glyphClassRanges[map->rangeOffset + mid];
            if (glyph < range.first) hi = mid;
            else if (glyph > range.last) lo = mid + 1;
            else return range.value;
        }
        return 0;
    }

    [[nodiscard]] static inline bool openTypeShapingGlyphForClass(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphClassMapId id, uint16_t value, uint16_t& glyph) noexcept
    {
        if (value == 0)
        {
            for (uint32_t g = 0; g <= 0xFFFFu; ++g)
            {
                if (openTypeShapingGlyphClassValue(ir, id, static_cast<uint16_t>(g)) == 0)
                {
                    glyph = static_cast<uint16_t>(g);
                    return true;
                }
            }
            return false;
        }

        const auto* map = ir.glyphClassMap(id);
        if (!map || uint64_t(map->rangeOffset) + map->rangeCount > ir.glyphClassRanges.size()) return false;
        for (uint32_t i = 0; i < map->rangeCount; ++i)
        {
            const auto& range = ir.glyphClassRanges[map->rangeOffset + i];
            if (range.value == value) { glyph = range.first; return true; }
        }
        return false;
    }

    [[nodiscard]] static inline bool openTypeShapingGlyphForClassInSet(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphClassMapId classMap, uint16_t value, OpenTypeShapingIRGlyphSetId setId, uint16_t& glyph) noexcept
    {
        const auto* set = ir.glyphSet(setId);
        if (!set || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

        for (uint32_t i = 0; i < set->rangeCount; ++i)
        {
            const auto& range = ir.glyphRanges[set->rangeOffset + i];
            for (uint32_t g = range.first; g <= range.last; ++g)
            {
                if (openTypeShapingGlyphClassValue(ir, classMap, static_cast<uint16_t>(g)) == value)
                {
                    glyph = static_cast<uint16_t>(g);
                    return true;
                }
            }
        }
        return false;
    }

    [[nodiscard]] static inline bool openTypeShapingAdjustmentNonZero(const OpenTypeShapingIRGposPairClassValue& value) noexcept
    {
        return !value.firstAdjustment.empty() || !value.secondAdjustment.empty();
    }

    static inline void appendOpenTypeShapingWorkload(std::vector<OpenTypeShapingWorkload>& out, const char* name, OpenTypeShapingWorkloadKind kind, OpenTypeShapingIRLookupId lookup, std::vector<uint16_t> glyphs, uint32_t targetOffset, uint32_t expectedLogicalLength, uint32_t structuralValue)
    {
        if (glyphs.empty()) return;
        OpenTypeShapingWorkload w;
        w.name = name;
        w.kind = kind;
        w.lookup = lookup;
        w.glyphs = std::move(glyphs);
        w.targetOffset = targetOffset;
        w.expectedLogicalLength = expectedLogicalLength;
        w.structuralValue = structuralValue;
        out.push_back(std::move(w));
    }

    static inline bool makeOpenTypeGsubSingleWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubSingle || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubSingleSubtables.size()) return false;

        const OpenTypeShapingIRGsubSingleSubtable* best = nullptr;
        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubSingleSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.pairOffset) + sub.pairCount > ir.gsubSinglePairs.size()) continue;
            if (!best || sub.pairCount > best->pairCount) best = &sub;
        }
        if (!best || !best->pairCount) return false;

        const auto& first = ir.gsubSinglePairs[best->pairOffset];
        const auto& last = ir.gsubSinglePairs[best->pairOffset + best->pairCount - 1];
        appendOpenTypeShapingWorkload(out, "single_hit_first_mapping", OpenTypeShapingWorkloadKind::HitEarly, lookupId, {first.input}, 0, 1, best->pairCount);
        appendOpenTypeShapingWorkload(out, "single_hit_last_mapping", OpenTypeShapingWorkloadKind::HitLate, lookupId, {last.input}, 0, 1, best->pairCount);

        uint16_t miss = 0;
        bool found = false;
        for (uint32_t g = first.input; g <= last.input; ++g)
        {
            const auto begin = ir.gsubSinglePairs.begin() + best->pairOffset;
            const auto end = begin + best->pairCount;
            const auto it = std::lower_bound(begin, end, static_cast<uint16_t>(g), [](const auto& pair, uint16_t value){ return pair.input < value; });
            if (it == end || it->input != g) { miss = static_cast<uint16_t>(g); found = true; break; }
        }
        if (!found && last.input != 0xFFFFu) { miss = static_cast<uint16_t>(last.input + 1u); found = true; }
        if (found) appendOpenTypeShapingWorkload(out, "single_miss", OpenTypeShapingWorkloadKind::MissLate, lookupId, {miss}, 0, 1, best->pairCount);
        return true;
    }

    static inline bool makeOpenTypeGsubMultipleWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubMultiple || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubMultipleSubtables.size()) return false;

        const OpenTypeShapingIRGsubMultiplePair* best = nullptr;
        uint32_t expansion = 0;
        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubMultipleSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.pairOffset) + sub.pairCount > ir.gsubMultiplePairs.size()) continue;
            for (uint32_t i = 0; i < sub.pairCount; ++i)
            {
                const auto& pair = ir.gsubMultiplePairs[sub.pairOffset + i];
                if (pair.sequenceIndex >= ir.gsubMultipleSequences.size()) continue;
                const uint32_t count = ir.gsubMultipleSequences[pair.sequenceIndex].glyphCount;
                if (!best || count > expansion) { best = &pair; expansion = count; }
            }
        }
        if (!best) return false;
        appendOpenTypeShapingWorkload(out, "multiple_max_expansion", OpenTypeShapingWorkloadKind::ExpansionStress, lookupId, std::vector<uint16_t>(64, best->input), 0, 64, expansion);
        return true;
    }

    static inline bool makeOpenTypeGsubAlternateWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubAlternate || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubAlternateSubtables.size()) return false;

        const OpenTypeShapingIRGsubAlternatePair* best = nullptr;
        uint32_t count = 0;
        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubAlternateSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.pairOffset) + sub.pairCount > ir.gsubAlternatePairs.size()) continue;
            for (uint32_t i = 0; i < sub.pairCount; ++i)
            {
                const auto& pair = ir.gsubAlternatePairs[sub.pairOffset + i];
                if (pair.alternateSetIndex >= ir.gsubAlternateSets.size()) continue;
                const uint32_t n = ir.gsubAlternateSets[pair.alternateSetIndex].glyphCount;
                if (!best || n > count) { best = &pair; count = n; }
            }
        }
        if (!best) return false;
        appendOpenTypeShapingWorkload(out, "alternate_hit", OpenTypeShapingWorkloadKind::HitEarly, lookupId, {best->input}, 0, 1, count);
        return true;
    }

    static inline bool makeOpenTypeGsubLigatureWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubLigature || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubLigatureSubtables.size()) return false;

        const OpenTypeShapingIRGsubLigaturePair* bestFanout = nullptr;
        const OpenTypeShapingIRGsubLigature* longest = nullptr;
        uint16_t longestFirst = 0;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubLigatureSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.pairOffset) + sub.pairCount > ir.gsubLigaturePairs.size()) continue;
            for (uint32_t i = 0; i < sub.pairCount; ++i)
            {
                const auto& pair = ir.gsubLigaturePairs[sub.pairOffset + i];
                if (uint64_t(pair.ligatureOffset) + pair.ligatureCount > ir.gsubLigatures.size()) continue;
                if (!bestFanout || pair.ligatureCount > bestFanout->ligatureCount) bestFanout = &pair;

                for (uint32_t j = 0; j < pair.ligatureCount; ++j)
                {
                    const auto& lig = ir.gsubLigatures[pair.ligatureOffset + j];
                    if (!longest || lig.componentCount > longest->componentCount) { longest = &lig; longestFirst = pair.input; }
                }
            }
        }
        if (!bestFanout || !bestFanout->ligatureCount) return false;

        const auto makeCandidate = [&](uint32_t candidateIndex, const char* name, OpenTypeShapingWorkloadKind kind)
        {
            const auto& lig = ir.gsubLigatures[bestFanout->ligatureOffset + candidateIndex];
            if (!lig.componentCount || uint64_t(lig.componentOffset) + lig.componentCount - 1u > ir.gsubLigatureComponents.size()) return;
            std::vector<uint16_t> glyphs;
            glyphs.push_back(bestFanout->input);
            for (uint32_t c = 0; c + 1 < lig.componentCount; ++c) glyphs.push_back(ir.gsubLigatureComponents[lig.componentOffset + c]);
            appendOpenTypeShapingWorkload(out, name, kind, lookupId, std::move(glyphs), 0, lig.componentCount, bestFanout->ligatureCount);
        };

        makeCandidate(0, "ligature_hit_first_candidate", OpenTypeShapingWorkloadKind::HitEarly);
        makeCandidate(bestFanout->ligatureCount - 1, "ligature_hit_last_candidate", OpenTypeShapingWorkloadKind::HitLate);

        const auto& last = ir.gsubLigatures[bestFanout->ligatureOffset + bestFanout->ligatureCount - 1];
        if (last.componentCount > 1 && uint64_t(last.componentOffset) + last.componentCount - 1u <= ir.gsubLigatureComponents.size())
        {
            std::vector<uint16_t> miss;
            miss.push_back(bestFanout->input);
            for (uint32_t c = 0; c + 1 < last.componentCount; ++c) miss.push_back(ir.gsubLigatureComponents[last.componentOffset + c]);

            const size_t pos = miss.size() - 1;
            bool found = false;
            for (uint32_t g = 0; g <= 0xFFFFu && !found; ++g)
            {
                if (g == miss[pos]) continue;
                bool completes = false;
                for (uint32_t j = 0; j < bestFanout->ligatureCount; ++j)
                {
                    const auto& lig = ir.gsubLigatures[bestFanout->ligatureOffset + j];
                    if (lig.componentCount != miss.size()) continue;
                    bool same = true;
                    for (uint32_t c = 0; c + 2 < lig.componentCount; ++c)
                    {
                        if (ir.gsubLigatureComponents[lig.componentOffset + c] != miss[c + 1]) { same = false; break; }
                    }
                    if (same && ir.gsubLigatureComponents[lig.componentOffset + lig.componentCount - 2] == g) { completes = true; break; }
                }
                if (!completes) { miss[pos] = static_cast<uint16_t>(g); found = true; }
            }
            if (found) appendOpenTypeShapingWorkload(out, "ligature_same_prefix_miss", OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(miss), 0, static_cast<uint32_t>(last.componentCount), bestFanout->ligatureCount);
        }

        if (longest)
        {
            std::vector<uint16_t> glyphs;
            glyphs.push_back(longestFirst);
            for (uint32_t c = 0; c + 1 < longest->componentCount; ++c) glyphs.push_back(ir.gsubLigatureComponents[longest->componentOffset + c]);
            appendOpenTypeShapingWorkload(out, "ligature_longest_components", OpenTypeShapingWorkloadKind::ContractionStress, lookupId, std::move(glyphs), 0, longest->componentCount, longest->componentCount);
        }
        return true;
    }

    template<class RuleT>
    [[nodiscard]] static inline uint32_t openTypeShapingContextFirstFanout(const OpenTypeShapingIR& ir, const std::vector<OpenTypeShapingIRGlyphSetId>& sets, uint32_t ruleOffset, uint32_t ruleCount, const std::vector<RuleT>& rules, uint16_t& bestGlyph, std::vector<uint32_t>& candidates)
    {
        candidates.clear();
        uint32_t best = 0;

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
                events.emplace_back(range.first, 1);
                events.emplace_back(uint32_t(range.last) + 1u, -1);
            }
        }
        if (events.empty()) return 0;

        std::sort(events.begin(), events.end(), [](const auto& a, const auto& b){ return a.first < b.first; });
        int32_t active = 0;
        for (size_t i = 0; i < events.size();)
        {
            const uint32_t pos = events[i].first;
            int32_t delta = 0;
            do { delta += events[i].second; ++i; } while (i < events.size() && events[i].first == pos);
            active += delta;
            if (pos <= 0xFFFFu && active > static_cast<int32_t>(best)) { best = static_cast<uint32_t>(active); bestGlyph = static_cast<uint16_t>(pos); }
        }

        if (!best) return 0;
        for (uint32_t i = 0; i < ruleCount; ++i)
        {
            const auto& rule = rules[ruleOffset + i];
            if (rule.inputCount && rule.inputSetOffset < sets.size() && openTypeShapingGlyphSetContains(ir, sets[rule.inputSetOffset], bestGlyph)) candidates.push_back(ruleOffset + i);
        }
        return static_cast<uint32_t>(candidates.size());
    }

    template<class RuleT>
    [[nodiscard]] static inline uint32_t openTypeShapingContextPrefix2SameSetFanout(const std::vector<OpenTypeShapingIRGlyphSetId>& sets, uint32_t ruleOffset, uint32_t ruleCount, const std::vector<RuleT>& rules, std::vector<uint32_t>& candidates)
    {
        candidates.clear();
        std::vector<std::pair<std::pair<OpenTypeShapingIRGlyphSetId, OpenTypeShapingIRGlyphSetId>, uint32_t>> entries;
        entries.reserve(ruleCount);

        for (uint32_t i = 0; i < ruleCount; ++i)
        {
            const auto& rule = rules[ruleOffset + i];
            if (rule.inputCount < 2 || uint64_t(rule.inputSetOffset) + 2u > sets.size()) continue;
            entries.push_back({{sets[rule.inputSetOffset], sets[rule.inputSetOffset + 1]}, ruleOffset + i});
        }
        if (entries.empty()) return 0;
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

        size_t bestBegin = 0;
        size_t bestCount = 1;
        for (size_t i = 0; i < entries.size();)
        {
            size_t j = i + 1;
            while (j < entries.size() && entries[j].first == entries[i].first) ++j;
            if (j - i > bestCount) { bestBegin = i; bestCount = j - i; }
            i = j;
        }
        for (size_t i = 0; i < bestCount; ++i) candidates.push_back(entries[bestBegin + i].second);
        std::sort(candidates.begin(), candidates.end());
        return static_cast<uint32_t>(candidates.size());
    }

    template<class RuleT>
    static inline bool openTypeShapingBuildContextRuleGlyphs(const OpenTypeShapingIR& ir, const std::vector<OpenTypeShapingIRGlyphSetId>& sets, const RuleT& rule, std::vector<uint16_t>& glyphs)
    {
        glyphs.clear();
        if (!rule.inputCount || uint64_t(rule.inputSetOffset) + rule.inputCount > sets.size()) return false;
        glyphs.reserve(rule.inputCount);
        for (uint32_t i = 0; i < rule.inputCount; ++i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, sets[rule.inputSetOffset + i], glyph)) return false;
            glyphs.push_back(glyph);
        }
        return true;
    }

    static inline bool makeOpenTypeGsubContextWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubContext || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubContextSubtables.size()) return false;

        uint32_t bestFanout = 0;
        uint32_t bestPrefix2 = 0;
        std::vector<uint32_t> fanoutCandidates;
        std::vector<uint32_t> prefixCandidates;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubContextSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.ruleOffset) + sub.ruleCount > ir.gsubContextRules.size()) continue;

            uint16_t glyph = 0;
            std::vector<uint32_t> candidates;
            const uint32_t fanout = openTypeShapingContextFirstFanout(ir, ir.gsubContextInputSets, sub.ruleOffset, sub.ruleCount, ir.gsubContextRules, glyph, candidates);
            if (fanout > bestFanout) { bestFanout = fanout; fanoutCandidates = candidates; }

            candidates.clear();
            const uint32_t prefix2 = openTypeShapingContextPrefix2SameSetFanout(ir.gsubContextInputSets, sub.ruleOffset, sub.ruleCount, ir.gsubContextRules, candidates);
            if (prefix2 > bestPrefix2) { bestPrefix2 = prefix2; prefixCandidates = candidates; }
        }

        const auto emit = [&](const std::vector<uint32_t>& candidates, uint32_t structuralValue, const char* earlyName, const char* lateName, const char* missName)
        {
            if (candidates.empty()) return;
            std::vector<uint16_t> glyphs;

            const auto& early = ir.gsubContextRules[candidates.front()];
            if (openTypeShapingBuildContextRuleGlyphs(ir, ir.gsubContextInputSets, early, glyphs))
                appendOpenTypeShapingWorkload(out, earlyName, OpenTypeShapingWorkloadKind::HitEarly, lookupId, glyphs, 0, early.inputCount, structuralValue);

            const auto& late = ir.gsubContextRules[candidates.back()];
            if (openTypeShapingBuildContextRuleGlyphs(ir, ir.gsubContextInputSets, late, glyphs))
            {
                appendOpenTypeShapingWorkload(out, lateName, OpenTypeShapingWorkloadKind::HitLate, lookupId, glyphs, 0, late.inputCount, structuralValue);

                if (late.inputCount > 1)
                {
                    uint16_t replacement = 0;
                    if (openTypeShapingGlyphOutsideSet(ir, ir.gsubContextInputSets[late.inputSetOffset + late.inputCount - 1], replacement))
                    {
                        glyphs.back() = replacement;
                        appendOpenTypeShapingWorkload(out, missName, OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), 0, late.inputCount, structuralValue);
                    }
                }
            }
        };

        emit(fanoutCandidates, bestFanout, "context_first_fanout_hit_early", "context_first_fanout_hit_late", "context_first_fanout_miss_late");
        if (bestPrefix2 > 1) emit(prefixCandidates, bestPrefix2, "context_prefix2_fanout_hit_early", "context_prefix2_fanout_hit_late", "context_prefix2_fanout_miss_after_prefix");
        return !fanoutCandidates.empty();
    }

    static inline bool openTypeShapingBuildGsubChainRuleGlyphs(const OpenTypeShapingIR& ir, const OpenTypeShapingIRGsubChainContextRule& rule, std::vector<uint16_t>& glyphs, uint32_t& targetOffset)
    {
        if (uint64_t(rule.backtrackSetOffset) + rule.backtrackCount > ir.gsubChainContextSets.size() ||
            uint64_t(rule.inputSetOffset) + rule.inputCount > ir.gsubChainContextSets.size() ||
            uint64_t(rule.lookaheadSetOffset) + rule.lookaheadCount > ir.gsubChainContextSets.size()) return false;

        glyphs.clear();
        for (uint32_t i = rule.backtrackCount; i != 0; --i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gsubChainContextSets[rule.backtrackSetOffset + i - 1], glyph)) return false;
            glyphs.push_back(glyph);
        }

        targetOffset = static_cast<uint32_t>(glyphs.size());

        for (uint32_t i = 0; i < rule.inputCount; ++i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gsubChainContextSets[rule.inputSetOffset + i], glyph)) return false;
            glyphs.push_back(glyph);
        }

        for (uint32_t i = 0; i < rule.lookaheadCount; ++i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gsubChainContextSets[rule.lookaheadSetOffset + i], glyph)) return false;
            glyphs.push_back(glyph);
        }
        return true;
    }

    static inline bool makeOpenTypeGsubChainContextWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubChainContext || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubChainContextSubtables.size()) return false;

        uint32_t bestFanout = 0;
        uint32_t bestPrefix2 = 0;
        std::vector<uint32_t> fanoutCandidates;
        std::vector<uint32_t> prefixCandidates;
        const OpenTypeShapingIRGsubChainContextRule* longest = nullptr;
        uint32_t longestSpan = 0;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubChainContextSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.ruleOffset) + sub.ruleCount > ir.gsubChainContextRules.size()) continue;

            uint16_t glyph = 0;
            std::vector<uint32_t> candidates;
            const uint32_t fanout = openTypeShapingContextFirstFanout(ir, ir.gsubChainContextSets, sub.ruleOffset, sub.ruleCount, ir.gsubChainContextRules, glyph, candidates);
            if (fanout > bestFanout) { bestFanout = fanout; fanoutCandidates = candidates; }

            candidates.clear();
            const uint32_t prefix2 = openTypeShapingContextPrefix2SameSetFanout(ir.gsubChainContextSets, sub.ruleOffset, sub.ruleCount, ir.gsubChainContextRules, candidates);
            if (prefix2 > bestPrefix2) { bestPrefix2 = prefix2; prefixCandidates = candidates; }

            for (uint32_t i = 0; i < sub.ruleCount; ++i)
            {
                const auto& rule = ir.gsubChainContextRules[sub.ruleOffset + i];
                const uint32_t span = rule.backtrackCount + rule.inputCount + rule.lookaheadCount;
                if (!longest || span > longestSpan) { longest = &rule; longestSpan = span; }
            }
        }

        const auto emit = [&](const std::vector<uint32_t>& candidates, uint32_t structuralValue, const char* earlyName, const char* lateName, const char* missName)
        {
            if (candidates.empty()) return;
            std::vector<uint16_t> glyphs;
            uint32_t target = 0;

            const auto& early = ir.gsubChainContextRules[candidates.front()];
            if (openTypeShapingBuildGsubChainRuleGlyphs(ir, early, glyphs, target))
                appendOpenTypeShapingWorkload(out, earlyName, OpenTypeShapingWorkloadKind::HitEarly, lookupId, glyphs, target, early.inputCount, structuralValue);

            const auto& late = ir.gsubChainContextRules[candidates.back()];
            if (openTypeShapingBuildGsubChainRuleGlyphs(ir, late, glyphs, target))
            {
                appendOpenTypeShapingWorkload(out, lateName, OpenTypeShapingWorkloadKind::HitLate, lookupId, glyphs, target, late.inputCount, structuralValue);

                uint16_t replacement = 0;
                if (late.lookaheadCount && openTypeShapingGlyphOutsideSet(ir, ir.gsubChainContextSets[late.lookaheadSetOffset + late.lookaheadCount - 1], replacement))
                {
                    glyphs.back() = replacement;
                    appendOpenTypeShapingWorkload(out, missName, OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), target, late.inputCount, structuralValue);
                }
                else if (late.inputCount > 1 && openTypeShapingGlyphOutsideSet(ir, ir.gsubChainContextSets[late.inputSetOffset + late.inputCount - 1], replacement))
                {
                    glyphs[target + late.inputCount - 1] = replacement;
                    appendOpenTypeShapingWorkload(out, missName, OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), target, late.inputCount, structuralValue);
                }
            }
        };

        emit(fanoutCandidates, bestFanout, "chain_first_fanout_hit_early", "chain_first_fanout_hit_late", "chain_first_fanout_miss_late");
        if (bestPrefix2 > 1) emit(prefixCandidates, bestPrefix2, "chain_prefix2_fanout_hit_early", "chain_prefix2_fanout_hit_late", "chain_prefix2_fanout_miss_after_prefix");

        if (longest)
        {
            std::vector<uint16_t> glyphs;
            uint32_t target = 0;
            if (openTypeShapingBuildGsubChainRuleGlyphs(ir, *longest, glyphs, target))
                appendOpenTypeShapingWorkload(out, "chain_max_span_hit", OpenTypeShapingWorkloadKind::HitLate, lookupId, std::move(glyphs), target, longest->inputCount, longestSpan);
        }
        return !fanoutCandidates.empty() || longest != nullptr;
    }

    static inline bool makeOpenTypeGsubReverseChainSingleWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubReverseChainSingle || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubReverseChainSingleSubtables.size()) return false;

        const OpenTypeShapingIRGsubReverseChainSingleSubtable* best = nullptr;
        uint32_t span = 0;
        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gsubReverseChainSingleSubtables[lookup->payloadOffset + s];
            if (!sub.pairCount || uint64_t(sub.pairOffset) + sub.pairCount > ir.gsubReverseChainSinglePairs.size()) continue;
            const uint32_t n = sub.backtrackCount + 1u + sub.lookaheadCount;
            if (!best || n > span) { best = &sub; span = n; }
        }
        if (!best) return false;

        std::vector<uint16_t> glyphs;
        for (uint32_t i = best->backtrackCount; i != 0; --i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gsubReverseChainSingleSets[best->backtrackSetOffset + i - 1], glyph)) return false;
            glyphs.push_back(glyph);
        }
        const uint32_t target = static_cast<uint32_t>(glyphs.size());
        glyphs.push_back(ir.gsubReverseChainSinglePairs[best->pairOffset].input);
        for (uint32_t i = 0; i < best->lookaheadCount; ++i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gsubReverseChainSingleSets[best->lookaheadSetOffset + i], glyph)) return false;
            glyphs.push_back(glyph);
        }
        appendOpenTypeShapingWorkload(out, "reverse_chain_hit", OpenTypeShapingWorkloadKind::HitEarly, lookupId, glyphs, target, 1, span);

        if (best->lookaheadCount)
        {
            uint16_t replacement = 0;
            if (openTypeShapingGlyphOutsideSet(ir, ir.gsubReverseChainSingleSets[best->lookaheadSetOffset + best->lookaheadCount - 1], replacement))
            {
                glyphs.back() = replacement;
                appendOpenTypeShapingWorkload(out, "reverse_chain_miss_late", OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), target, 1, span);
            }
        }
        return true;
    }

    static inline bool makeOpenTypeGposSingleWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposSingle || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposSingleSubtables.size()) return false;

        const OpenTypeShapingIRGposSingleSubtable* best = nullptr;
        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposSingleSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.pairOffset) + sub.pairCount > ir.gposSinglePairs.size()) continue;
            if (!best || sub.pairCount > best->pairCount) best = &sub;
        }
        if (!best || !best->pairCount) return false;

        appendOpenTypeShapingWorkload(out, "gpos_single_hit_first_mapping", OpenTypeShapingWorkloadKind::HitEarly, lookupId, {ir.gposSinglePairs[best->pairOffset].glyph}, 0, 1, best->pairCount);
        appendOpenTypeShapingWorkload(out, "gpos_single_hit_last_mapping", OpenTypeShapingWorkloadKind::HitLate, lookupId, {ir.gposSinglePairs[best->pairOffset + best->pairCount - 1].glyph}, 0, 1, best->pairCount);
        return true;
    }

    static inline bool makeOpenTypeGposPairExplicitWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, const OpenTypeShapingIRGposPairExplicitSubtable& sub, std::vector<OpenTypeShapingWorkload>& out)
    {
        if (!sub.pairCount || uint64_t(sub.pairOffset) + sub.pairCount > ir.gposPairExplicitPairs.size()) return false;

        appendOpenTypeShapingWorkload(out, "pair_explicit_first_pair_hit", OpenTypeShapingWorkloadKind::HitEarly, lookupId,
            {ir.gposPairExplicitPairs[sub.pairOffset].first, ir.gposPairExplicitPairs[sub.pairOffset].second}, 0, 2, sub.pairCount);
        appendOpenTypeShapingWorkload(out, "pair_explicit_last_pair_hit", OpenTypeShapingWorkloadKind::HitLate, lookupId,
            {ir.gposPairExplicitPairs[sub.pairOffset + sub.pairCount - 1].first, ir.gposPairExplicitPairs[sub.pairOffset + sub.pairCount - 1].second}, 0, 2, sub.pairCount);

        uint32_t bestBegin = 0;
        uint32_t bestCount = 0;
        for (uint32_t i = 0; i < sub.pairCount;)
        {
            const uint16_t first = ir.gposPairExplicitPairs[sub.pairOffset + i].first;
            uint32_t j = i + 1;
            while (j < sub.pairCount && ir.gposPairExplicitPairs[sub.pairOffset + j].first == first) ++j;
            if (j - i > bestCount) { bestBegin = i; bestCount = j - i; }
            i = j;
        }
        if (!bestCount) return true;

        const uint32_t firstIndex = sub.pairOffset + bestBegin;
        const uint32_t lastIndex = firstIndex + bestCount - 1;
        const uint16_t firstGlyph = ir.gposPairExplicitPairs[firstIndex].first;
        appendOpenTypeShapingWorkload(out, "pair_explicit_same_first_hit_early", OpenTypeShapingWorkloadKind::HitEarly, lookupId,
            {firstGlyph, ir.gposPairExplicitPairs[firstIndex].second}, 0, 2, bestCount);
        appendOpenTypeShapingWorkload(out, "pair_explicit_same_first_hit_late", OpenTypeShapingWorkloadKind::HitLate, lookupId,
            {firstGlyph, ir.gposPairExplicitPairs[lastIndex].second}, 0, 2, bestCount);

        // A miss must miss the whole PairPos lookup, not merely this
        // explicit subtable. Any class subtable whose first coverage contains
        // firstGlyph will match every second glyph through some class cell,
        // including a numerically-zero cell, so no same-first miss exists.
        const auto* lookup = ir.lookup(lookupId);
        bool classSubtableCoversFirst = false;
        std::vector<uint8_t> usedSeconds(0x10000u, 0);

        if (lookup && uint64_t(lookup->payloadOffset) + lookup->payloadCount <= ir.gposPairSubtables.size())
        {
            for (uint32_t s = 0; s < lookup->payloadCount; ++s)
            {
                const auto& pairSub = ir.gposPairSubtables[lookup->payloadOffset + s];

                if (pairSub.kind == OpenTypeShapingIRGposPairKind::Class)
                {
                    if (pairSub.payloadIndex >= ir.gposPairClassSubtables.size()) continue;
                    const auto& classSub = ir.gposPairClassSubtables[pairSub.payloadIndex];
                    if (openTypeShapingGlyphSetContains(ir, classSub.firstCoverage, firstGlyph))
                    {
                        classSubtableCoversFirst = true;
                        break;
                    }
                    continue;
                }

                if (pairSub.kind != OpenTypeShapingIRGposPairKind::Explicit ||
                    pairSub.payloadIndex >= ir.gposPairExplicitSubtables.size()) continue;

                const auto& explicitSub = ir.gposPairExplicitSubtables[pairSub.payloadIndex];
                if (uint64_t(explicitSub.pairOffset) + explicitSub.pairCount > ir.gposPairExplicitPairs.size()) continue;

                for (uint32_t i = 0; i < explicitSub.pairCount; ++i)
                {
                    const auto& pair = ir.gposPairExplicitPairs[explicitSub.pairOffset + i];
                    if (pair.first == firstGlyph) usedSeconds[pair.second] = 1;
                }
            }
        }

        if (!classSubtableCoversFirst)
        {
            for (uint32_t g = 0; g <= 0xFFFFu; ++g)
            {
                if (usedSeconds[g]) continue;
                appendOpenTypeShapingWorkload(out, "pair_explicit_same_first_miss", OpenTypeShapingWorkloadKind::MissLate,
                    lookupId, {firstGlyph, static_cast<uint16_t>(g)}, 0, 2, bestCount);
                break;
            }
        }
        return true;
    }

    static inline bool makeOpenTypeGposPairClassWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, const OpenTypeShapingIRGposPairClassSubtable& sub, std::vector<OpenTypeShapingWorkload>& out)
    {
        const uint64_t cells = uint64_t(sub.firstClassCount) * sub.secondClassCount;
        if (!cells || uint64_t(sub.valueOffset) + cells > ir.gposPairClassValues.size()) return false;

        uint16_t denseClass1 = 0;
        uint32_t denseCount = 0;
        uint16_t sparseClass1 = 0;
        uint32_t sparseCount = std::numeric_limits<uint32_t>::max();

        for (uint16_t c1 = 0; c1 < sub.firstClassCount; ++c1)
        {
            uint32_t count = 0;
            for (uint16_t c2 = 0; c2 < sub.secondClassCount; ++c2)
            {
                if (openTypeShapingAdjustmentNonZero(ir.gposPairClassValues[sub.valueOffset + uint32_t(c1) * sub.secondClassCount + c2])) ++count;
            }
            if (count > denseCount) { denseCount = count; denseClass1 = c1; }
            if (count && count < sparseCount) { sparseCount = count; sparseClass1 = c1; }
        }

        const auto emitRow = [&](uint16_t c1, uint32_t rowCount, const char* name)
        {
            uint16_t first = 0;
            if (!openTypeShapingGlyphForClassInSet(ir, sub.firstClassMap, c1, sub.firstCoverage, first)) return;

            for (uint16_t c2 = 0; c2 < sub.secondClassCount; ++c2)
            {
                const auto& value = ir.gposPairClassValues[sub.valueOffset + uint32_t(c1) * sub.secondClassCount + c2];
                if (!openTypeShapingAdjustmentNonZero(value)) continue;
                uint16_t second = 0;
                if (!openTypeShapingGlyphForClass(ir, sub.secondClassMap, c2, second)) continue;
                appendOpenTypeShapingWorkload(out, name, OpenTypeShapingWorkloadKind::HitLate, lookupId, {first, second}, 0, 2, rowCount);
                break;
            }
        };

        if (denseCount) emitRow(denseClass1, denseCount, "pair_class_nonzero_dense_row_hit");
        if (sparseCount != std::numeric_limits<uint32_t>::max() && sparseClass1 != denseClass1) emitRow(sparseClass1, sparseCount, "pair_class_nonzero_sparse_row_hit");

        if (denseCount)
        {
            uint16_t first = 0;
            if (openTypeShapingGlyphForClassInSet(ir, sub.firstClassMap, denseClass1, sub.firstCoverage, first))
            {
                for (uint16_t c2 = 0; c2 < sub.secondClassCount; ++c2)
                {
                    const auto& value = ir.gposPairClassValues[sub.valueOffset + uint32_t(denseClass1) * sub.secondClassCount + c2];
                    if (openTypeShapingAdjustmentNonZero(value)) continue;
                    uint16_t second = 0;
                    if (!openTypeShapingGlyphForClass(ir, sub.secondClassMap, c2, second)) continue;
                    appendOpenTypeShapingWorkload(out, "pair_class_zero_cell_in_populated_row", OpenTypeShapingWorkloadKind::HitLate, lookupId, {first, second}, 0, 2, denseCount);
                    break;
                }
            }
        }
        return denseCount != 0;
    }

    static inline bool makeOpenTypeGposPairWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposPair || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposPairSubtables.size()) return false;

        const OpenTypeShapingIRGposPairExplicitSubtable* bestExplicit = nullptr;
        const OpenTypeShapingIRGposPairClassSubtable* bestClass = nullptr;
        uint64_t bestCells = 0;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& pairSub = ir.gposPairSubtables[lookup->payloadOffset + s];
            if (pairSub.kind == OpenTypeShapingIRGposPairKind::Explicit)
            {
                if (pairSub.payloadIndex >= ir.gposPairExplicitSubtables.size()) continue;
                const auto& sub = ir.gposPairExplicitSubtables[pairSub.payloadIndex];
                if (!bestExplicit || sub.pairCount > bestExplicit->pairCount) bestExplicit = &sub;
            }
            else if (pairSub.kind == OpenTypeShapingIRGposPairKind::Class)
            {
                if (pairSub.payloadIndex >= ir.gposPairClassSubtables.size()) continue;
                const auto& sub = ir.gposPairClassSubtables[pairSub.payloadIndex];
                const uint64_t cells = uint64_t(sub.firstClassCount) * sub.secondClassCount;
                if (!bestClass || cells > bestCells) { bestClass = &sub; bestCells = cells; }
            }
        }

        bool any = false;
        if (bestExplicit) any |= makeOpenTypeGposPairExplicitWorkloads(ir, lookupId, *bestExplicit, out);
        if (bestClass) any |= makeOpenTypeGposPairClassWorkloads(ir, lookupId, *bestClass, out);
        return any;
    }

    static inline bool makeOpenTypeGposCursiveWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposCursive || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposCursiveSubtables.size()) return false;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposCursiveSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.recordOffset) + sub.recordCount > ir.gposCursiveRecords.size()) continue;

            for (uint32_t a = 0; a < sub.recordCount; ++a)
            {
                const auto& first = ir.gposCursiveRecords[sub.recordOffset + a];
                if (!first.hasExit) continue;
                for (uint32_t b = 0; b < sub.recordCount; ++b)
                {
                    const auto& second = ir.gposCursiveRecords[sub.recordOffset + b];
                    if (!second.hasEntry) continue;
                    if (lookup->filter.flags & OpenTypeShapingIRRightToLeft)
                        appendOpenTypeShapingWorkload(out, "cursive_exit_entry_hit", OpenTypeShapingWorkloadKind::HitEarly, lookupId, {second.glyph, first.glyph}, 1, 2, sub.recordCount);
                    else
                        appendOpenTypeShapingWorkload(out, "cursive_exit_entry_hit", OpenTypeShapingWorkloadKind::HitEarly, lookupId, {first.glyph, second.glyph}, 0, 2, sub.recordCount);
                    return true;
                }
            }
        }
        return false;
    }

    static inline bool makeOpenTypeGposMarkBaseWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposMarkBase || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposMarkBaseSubtables.size()) return false;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposMarkBaseSubtables[lookup->payloadOffset + s];
            if (!sub.markCount || !sub.baseCount || !sub.markClassCount ||
                uint64_t(sub.markOffset) + sub.markCount > ir.gposMarkRecords.size() ||
                uint64_t(sub.baseOffset) + sub.baseCount > ir.gposMarkBaseRecords.size() ||
                uint64_t(sub.baseAnchorOffset) + uint64_t(sub.baseCount) * sub.markClassCount > ir.gposMarkBaseAnchorRefs.size()) continue;

            for (uint32_t m = 0; m < sub.markCount; ++m)
            {
                const auto& mark = ir.gposMarkRecords[sub.markOffset + m];
                if (mark.markClass >= sub.markClassCount || mark.anchor == kOpenTypeShapingIRInvalid) continue;
                for (uint32_t b = 0; b < sub.baseCount; ++b)
                {
                    const auto anchor = ir.gposMarkBaseAnchorRefs[sub.baseAnchorOffset + b * sub.markClassCount + mark.markClass];
                    if (anchor == kOpenTypeShapingIRInvalid) continue;
                    appendOpenTypeShapingWorkload(out, "mark_base_valid_attachment", OpenTypeShapingWorkloadKind::HitEarly, lookupId,
                        {ir.gposMarkBaseRecords[sub.baseOffset + b].glyph, mark.glyph}, 1, 2, sub.markClassCount);
                    return true;
                }
            }
        }
        return false;
    }

    static inline bool makeOpenTypeGposMarkLigatureWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposMarkLigature || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposMarkLigatureSubtables.size()) return false;

        const OpenTypeShapingIRGposLigatureRecord* bestLigature = nullptr;
        const OpenTypeShapingIRGposMarkLigatureSubtable* bestSub = nullptr;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposMarkLigatureSubtables[lookup->payloadOffset + s];
            if (!sub.markCount || !sub.ligatureCount || !sub.markClassCount ||
                uint64_t(sub.markOffset) + sub.markCount > ir.gposMarkRecords.size() ||
                uint64_t(sub.ligatureOffset) + sub.ligatureCount > ir.gposMarkLigatureRecords.size()) continue;

            for (uint32_t l = 0; l < sub.ligatureCount; ++l)
            {
                const auto& lig = ir.gposMarkLigatureRecords[sub.ligatureOffset + l];
                if (!bestLigature || lig.componentCount > bestLigature->componentCount) { bestLigature = &lig; bestSub = &sub; }
            }
        }

        if (!bestLigature || !bestSub || !bestLigature->componentCount) return false;
        const uint64_t anchors = uint64_t(bestLigature->componentCount) * bestSub->markClassCount;
        if (uint64_t(bestLigature->componentAnchorOffset) + anchors > ir.gposMarkLigatureAnchorRefs.size()) return false;

        for (uint32_t m = 0; m < bestSub->markCount; ++m)
        {
            const auto& mark = ir.gposMarkRecords[bestSub->markOffset + m];
            if (mark.markClass >= bestSub->markClassCount || mark.anchor == kOpenTypeShapingIRInvalid) continue;

            for (uint32_t c = bestLigature->componentCount; c != 0; --c)
            {
                const auto anchor = ir.gposMarkLigatureAnchorRefs[bestLigature->componentAnchorOffset + (c - 1u) * bestSub->markClassCount + mark.markClass];
                if (anchor == kOpenTypeShapingIRInvalid) continue;
                appendOpenTypeShapingWorkload(out, "mark_ligature_max_component_hit", OpenTypeShapingWorkloadKind::HitLate, lookupId,
                    {bestLigature->glyph, mark.glyph}, 1, 2, bestLigature->componentCount);
                return true;
            }
        }
        return false;
    }

    static inline bool makeOpenTypeGposMarkMarkWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposMarkMark || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposMarkMarkSubtables.size()) return false;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposMarkMarkSubtables[lookup->payloadOffset + s];
            if (!sub.mark1Count || !sub.mark2Count || !sub.markClassCount ||
                uint64_t(sub.mark1Offset) + sub.mark1Count > ir.gposMarkRecords.size() ||
                uint64_t(sub.mark2Offset) + sub.mark2Count > ir.gposMark2Records.size() ||
                uint64_t(sub.mark2AnchorOffset) + uint64_t(sub.mark2Count) * sub.markClassCount > ir.gposMark2AnchorRefs.size()) continue;

            for (uint32_t m = 0; m < sub.mark1Count; ++m)
            {
                const auto& mark1 = ir.gposMarkRecords[sub.mark1Offset + m];
                if (mark1.markClass >= sub.markClassCount || mark1.anchor == kOpenTypeShapingIRInvalid) continue;
                for (uint32_t b = 0; b < sub.mark2Count; ++b)
                {
                    const auto anchor = ir.gposMark2AnchorRefs[sub.mark2AnchorOffset + b * sub.markClassCount + mark1.markClass];
                    if (anchor == kOpenTypeShapingIRInvalid) continue;
                    appendOpenTypeShapingWorkload(out, "mark_mark_valid_attachment", OpenTypeShapingWorkloadKind::HitEarly, lookupId,
                        {ir.gposMark2Records[sub.mark2Offset + b].glyph, mark1.glyph}, 1, 2, sub.markClassCount);
                    return true;
                }
            }
        }
        return false;
    }


    static inline bool makeOpenTypeGposContextWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposContext || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposContextSubtables.size()) return false;

        uint32_t bestFanout = 0;
        uint32_t bestPrefix2 = 0;
        std::vector<uint32_t> fanoutCandidates;
        std::vector<uint32_t> prefixCandidates;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposContextSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.ruleOffset) + sub.ruleCount > ir.gposContextRules.size()) continue;

            uint16_t glyph = 0;
            std::vector<uint32_t> candidates;
            const uint32_t fanout = openTypeShapingContextFirstFanout(ir, ir.gposContextInputSets, sub.ruleOffset, sub.ruleCount, ir.gposContextRules, glyph, candidates);
            if (fanout > bestFanout) { bestFanout = fanout; fanoutCandidates = candidates; }

            candidates.clear();
            const uint32_t prefix2 = openTypeShapingContextPrefix2SameSetFanout(ir.gposContextInputSets, sub.ruleOffset, sub.ruleCount, ir.gposContextRules, candidates);
            if (prefix2 > bestPrefix2) { bestPrefix2 = prefix2; prefixCandidates = candidates; }
        }

        const auto emit = [&](const std::vector<uint32_t>& candidates, uint32_t structuralValue, const char* earlyName, const char* lateName, const char* missName)
        {
            if (candidates.empty()) return;
            std::vector<uint16_t> glyphs;

            const auto& early = ir.gposContextRules[candidates.front()];
            if (openTypeShapingBuildContextRuleGlyphs(ir, ir.gposContextInputSets, early, glyphs))
                appendOpenTypeShapingWorkload(out, earlyName, OpenTypeShapingWorkloadKind::HitEarly, lookupId, glyphs, 0, early.inputCount, structuralValue);

            const auto& late = ir.gposContextRules[candidates.back()];
            if (openTypeShapingBuildContextRuleGlyphs(ir, ir.gposContextInputSets, late, glyphs))
            {
                appendOpenTypeShapingWorkload(out, lateName, OpenTypeShapingWorkloadKind::HitLate, lookupId, glyphs, 0, late.inputCount, structuralValue);
                if (late.inputCount > 1)
                {
                    uint16_t replacement = 0;
                    if (openTypeShapingGlyphOutsideSet(ir, ir.gposContextInputSets[late.inputSetOffset + late.inputCount - 1], replacement))
                    {
                        glyphs.back() = replacement;
                        appendOpenTypeShapingWorkload(out, missName, OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), 0, late.inputCount, structuralValue);
                    }
                }
            }
        };

        emit(fanoutCandidates, bestFanout, "gpos_context_first_fanout_hit_early", "gpos_context_first_fanout_hit_late", "gpos_context_first_fanout_miss_late");
        if (bestPrefix2 > 1) emit(prefixCandidates, bestPrefix2, "gpos_context_prefix2_fanout_hit_early", "gpos_context_prefix2_fanout_hit_late", "gpos_context_prefix2_fanout_miss_after_prefix");
        return !fanoutCandidates.empty();
    }

    static inline bool openTypeShapingBuildGposChainRuleGlyphs(const OpenTypeShapingIR& ir, const OpenTypeShapingIRGposChainContextRule& rule, std::vector<uint16_t>& glyphs, uint32_t& targetOffset)
    {
        if (uint64_t(rule.backtrackSetOffset) + rule.backtrackCount > ir.gposChainContextBacktrackSets.size() ||
            uint64_t(rule.inputSetOffset) + rule.inputCount > ir.gposChainContextInputSets.size() ||
            uint64_t(rule.lookaheadSetOffset) + rule.lookaheadCount > ir.gposChainContextLookaheadSets.size()) return false;

        glyphs.clear();
        for (uint32_t i = rule.backtrackCount; i != 0; --i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gposChainContextBacktrackSets[rule.backtrackSetOffset + i - 1], glyph)) return false;
            glyphs.push_back(glyph);
        }

        targetOffset = static_cast<uint32_t>(glyphs.size());

        for (uint32_t i = 0; i < rule.inputCount; ++i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gposChainContextInputSets[rule.inputSetOffset + i], glyph)) return false;
            glyphs.push_back(glyph);
        }

        for (uint32_t i = 0; i < rule.lookaheadCount; ++i)
        {
            uint16_t glyph = 0;
            if (!openTypeShapingFirstGlyphFromSet(ir, ir.gposChainContextLookaheadSets[rule.lookaheadSetOffset + i], glyph)) return false;
            glyphs.push_back(glyph);
        }
        return true;
    }

    static inline bool makeOpenTypeGposChainContextWorkloads(const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId lookupId, std::vector<OpenTypeShapingWorkload>& out)
    {
        const auto* lookup = ir.lookup(lookupId);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GposChainContext || uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gposChainContextSubtables.size()) return false;

        const OpenTypeShapingIRGposChainContextRule* longest = nullptr;
        uint32_t longestSpan = 0;

        for (uint32_t s = 0; s < lookup->payloadCount; ++s)
        {
            const auto& sub = ir.gposChainContextSubtables[lookup->payloadOffset + s];
            if (uint64_t(sub.ruleOffset) + sub.ruleCount > ir.gposChainContextRules.size()) continue;

            for (uint32_t i = 0; i < sub.ruleCount; ++i)
            {
                const auto& rule = ir.gposChainContextRules[sub.ruleOffset + i];
                const uint32_t span = rule.backtrackCount + rule.inputCount + rule.lookaheadCount;
                if (!longest || span > longestSpan) { longest = &rule; longestSpan = span; }
            }
        }

        if (!longest) return false;
        std::vector<uint16_t> glyphs;
        uint32_t target = 0;
        if (!openTypeShapingBuildGposChainRuleGlyphs(ir, *longest, glyphs, target)) return false;

        appendOpenTypeShapingWorkload(out, "gpos_chain_max_span_hit", OpenTypeShapingWorkloadKind::HitLate, lookupId, glyphs, target, longest->inputCount, longestSpan);

        uint16_t replacement = 0;
        if (longest->lookaheadCount && openTypeShapingGlyphOutsideSet(ir, ir.gposChainContextLookaheadSets[longest->lookaheadSetOffset + longest->lookaheadCount - 1], replacement))
        {
            glyphs.back() = replacement;
            appendOpenTypeShapingWorkload(out, "gpos_chain_max_span_miss_late", OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), target, longest->inputCount, longestSpan);
        }
        else if (longest->inputCount > 1 && openTypeShapingGlyphOutsideSet(ir, ir.gposChainContextInputSets[longest->inputSetOffset + longest->inputCount - 1], replacement))
        {
            glyphs[target + longest->inputCount - 1] = replacement;
            appendOpenTypeShapingWorkload(out, "gpos_chain_max_span_miss_late", OpenTypeShapingWorkloadKind::MissLate, lookupId, std::move(glyphs), target, longest->inputCount, longestSpan);
        }
        return true;
    }

    [[nodiscard]] static inline bool openTypeShapingLookupHasFilter(const OpenTypeShapingIRLookup& lookup) noexcept
    {
        const uint8_t semanticFlags = lookup.filter.flags & (OpenTypeShapingIRIgnoreBaseGlyphs | OpenTypeShapingIRIgnoreLigatures | OpenTypeShapingIRIgnoreMarks);
        return semanticFlags || lookup.filter.markAttachmentType || lookup.filter.markFilteringSet != kOpenTypeShapingIRInvalid;
    }

    [[nodiscard]] static inline bool openTypeShapingFindFilteredGlyph(const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookup& lookup, uint16_t& glyph) noexcept
    {
        if (ir.gdefGlyphClasses.size() != kOpenTypeShapingIRGlyphDomainSize) return false;

        for (uint32_t g = 0; g <= 0xFFFFu; ++g)
        {
            const uint16_t gid = static_cast<uint16_t>(g);
            const uint16_t glyphClass = ir.gdefGlyphClasses[gid];
            bool filtered = false;

            if ((lookup.filter.flags & OpenTypeShapingIRIgnoreBaseGlyphs) && glyphClass == 1) filtered = true;
            if ((lookup.filter.flags & OpenTypeShapingIRIgnoreLigatures) && glyphClass == 2) filtered = true;
            if ((lookup.filter.flags & OpenTypeShapingIRIgnoreMarks) && glyphClass == 3) filtered = true;

            if (!filtered && glyphClass == 3 && !(lookup.filter.flags & OpenTypeShapingIRIgnoreMarks))
            {
                if (lookup.filter.markFilteringSet != kOpenTypeShapingIRInvalid)
                    filtered = !openTypeShapingGlyphSetContains(ir, lookup.filter.markFilteringSet, gid);
                else if (lookup.filter.markAttachmentType && ir.gdefMarkAttachClasses.size() == kOpenTypeShapingIRGlyphDomainSize)
                    filtered = ir.gdefMarkAttachClasses[gid] != lookup.filter.markAttachmentType;
            }

            if (filtered) { glyph = gid; return true; }
        }
        return false;
    }

    static inline void makeOpenTypeFilterStressWorkloads(const OpenTypeShapingIR& ir, std::vector<OpenTypeShapingWorkload>& out)
    {
        for (OpenTypeShapingIRLookupId id = 0; id < ir.lookups.size(); ++id)
        {
            const auto& lookup = ir.lookups[id];
            if (!openTypeShapingLookupHasFilter(lookup)) continue;

            uint16_t filteredGlyph = 0;
            if (!openTypeShapingFindFilteredGlyph(ir, lookup, filteredGlyph)) continue;

            OpenTypeShapingWorkload w;
            w.name = "filter_stress";
            w.kind = OpenTypeShapingWorkloadKind::FilterStress;
            w.lookup = id;
            w.glyphs.assign(64, filteredGlyph);
            w.targetOffset = 0;
            w.expectedLogicalLength = 64;

            uint32_t modes = 0;
            if (lookup.filter.flags & OpenTypeShapingIRIgnoreBaseGlyphs) ++modes;
            if (lookup.filter.flags & OpenTypeShapingIRIgnoreLigatures) ++modes;
            if (lookup.filter.flags & OpenTypeShapingIRIgnoreMarks) ++modes;
            if (lookup.filter.markAttachmentType) ++modes;
            if (lookup.filter.markFilteringSet != kOpenTypeShapingIRInvalid) ++modes;
            w.structuralValue = modes;
            out.push_back(std::move(w));
        }
    }

    static inline void makeOpenTypeShapingWorkloads(const OpenTypeShapingIR& ir, std::vector<OpenTypeShapingWorkload>& out)
    {
        out.clear();

        for (OpenTypeShapingIRLookupId id = 0; id < ir.lookups.size(); ++id)
        {
            switch (ir.lookups[id].op)
            {
            case OpenTypeShapingIROp::GsubSingle: makeOpenTypeGsubSingleWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GsubMultiple: makeOpenTypeGsubMultipleWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GsubAlternate: makeOpenTypeGsubAlternateWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GsubLigature: makeOpenTypeGsubLigatureWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GsubContext: makeOpenTypeGsubContextWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GsubChainContext: makeOpenTypeGsubChainContextWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GsubReverseChainSingle: makeOpenTypeGsubReverseChainSingleWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposSingle: makeOpenTypeGposSingleWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposPair: makeOpenTypeGposPairWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposCursive: makeOpenTypeGposCursiveWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposMarkBase: makeOpenTypeGposMarkBaseWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposMarkLigature: makeOpenTypeGposMarkLigatureWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposMarkMark: makeOpenTypeGposMarkMarkWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposContext: makeOpenTypeGposContextWorkloads(ir, id, out); break;
            case OpenTypeShapingIROp::GposChainContext: makeOpenTypeGposChainContextWorkloads(ir, id, out); break;
            default: break;
            }
        }

        makeOpenTypeFilterStressWorkloads(ir, out);
    }
}
