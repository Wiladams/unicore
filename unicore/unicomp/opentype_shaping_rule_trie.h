// opentype_shaping_rule_trie.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "opentype_shaping_ir.h"

namespace waavs
{
    using OpenTypeShapingRuleTriePredicateId = uint32_t;
    using OpenTypeShapingRuleTrieNodeId = uint32_t;

    inline constexpr uint32_t kOpenTypeShapingRuleTrieInvalid = std::numeric_limits<uint32_t>::max();

    struct OpenTypeShapingRuleTriePredicate
    {
        uint32_t rangeOffset{0};
        uint32_t rangeCount{0};
        uint32_t memberCount{0};
    };

    struct OpenTypeShapingRuleTrieEdge
    {
        OpenTypeShapingRuleTriePredicateId predicate{kOpenTypeShapingRuleTrieInvalid};
        OpenTypeShapingRuleTrieNodeId child{kOpenTypeShapingRuleTrieInvalid};
    };

    struct OpenTypeShapingRuleTrieNode
    {
        uint32_t edgeOffset{0};
        uint32_t edgeCount{0};
        uint32_t acceptOffset{0};
        uint32_t acceptCount{0};
        uint32_t subtreeRuleCount{0};
        uint32_t depth{0};
    };

    struct OpenTypeShapingRuleTrie
    {
        std::vector<OpenTypeShapingIRGlyphRange> predicateRanges{};
        std::vector<OpenTypeShapingRuleTriePredicate> predicates{};
        std::vector<OpenTypeShapingRuleTrieNode> nodes{};
        std::vector<OpenTypeShapingRuleTrieEdge> edges{};
        std::vector<uint32_t> accepts{};

        void clear()
        {
            predicateRanges.clear();
            predicates.clear();
            nodes.clear();
            edges.clear();
            accepts.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return nodes.empty(); }
    };

    struct OpenTypeShapingRuleTrieAnalysis
    {
        uint32_t ruleCount{0};
        uint64_t sourcePredicatePositions{0};

        uint32_t canonicalPredicateCount{0};
        uint32_t nodeCount{0};
        uint32_t edgeCount{0};
        uint32_t acceptingNodeCount{0};
        uint32_t duplicatePatternRules{0};

        uint32_t maxDepth{0};
        uint32_t rootFanout{0};
        uint32_t maxNodeFanout{0};

        uint64_t sharedPrefixPositions{0};

        uint32_t rootMinRules{0};
        uint32_t rootMaxRules{0};
        double rootMeanRules{0.0};

        uint32_t nodesWithOverlappingEdges{0};
        uint64_t siblingOverlapPairs{0};
        uint64_t rootOverlapPairs{0};

        double edgeToSourcePredicateRatio{0.0};
        double sharedPrefixFraction{0.0};
    };

    struct OpenTypeShapingRuleTrieRootBucket
    {
        OpenTypeShapingRuleTriePredicateId predicate{kOpenTypeShapingRuleTrieInvalid};
        uint32_t subtreeRuleCount{0};
        uint32_t rangeCount{0};
        uint32_t memberCount{0};
    };

    namespace OpenTypeShapingRuleTrieDetail
    {
        struct TempNode
        {
            std::map<OpenTypeShapingRuleTriePredicateId, OpenTypeShapingRuleTrieNodeId> edges{};
            std::vector<uint32_t> accepts{};
            uint32_t depth{0};
        };

        class Builder
        {
        public:
            explicit Builder(const OpenTypeShapingIR& ir) : fIR(ir)
            {
                fNodes.push_back({});
            }

            bool insert(const OpenTypeShapingIRGlyphSetId* sets, uint32_t count, uint32_t ruleId)
            {
                if (!sets || count == 0)
                    return false;

                OpenTypeShapingRuleTrieNodeId node = 0;

                for (uint32_t i = 0; i < count; ++i)
                {
                    OpenTypeShapingRuleTriePredicateId predicate = kOpenTypeShapingRuleTrieInvalid;

                    if (!canonicalize(sets[i], predicate))
                        return false;

                    auto& edges = fNodes[node].edges;
                    auto it = edges.find(predicate);

                    if (it == edges.end())
                    {
                        if (fNodes.size() >= uint64_t(kOpenTypeShapingRuleTrieInvalid))
                            return false;

                        const OpenTypeShapingRuleTrieNodeId child =
                            static_cast<OpenTypeShapingRuleTrieNodeId>(fNodes.size());

                        TempNode next;
                        next.depth = fNodes[node].depth + 1;
                        fNodes.push_back(std::move(next));

                        fNodes[node].edges.emplace(predicate, child);
                        node = child;
                    }
                    else
                    {
                        node = it->second;
                    }
                }

                fNodes[node].accepts.push_back(ruleId);
                return true;
            }

            bool finalize(OpenTypeShapingRuleTrie& out, OpenTypeShapingRuleTrieAnalysis& analysis)
            {
                out.clear();
                analysis = {};

                if (fNodes.empty())
                    return false;

                std::vector<uint32_t> subtree(fNodes.size(), 0);

                for (size_t i = fNodes.size(); i != 0; --i)
                {
                    const size_t nodeIndex = i - 1;
                    uint64_t total = fNodes[nodeIndex].accepts.size();

                    for (const auto& edge : fNodes[nodeIndex].edges)
                        total += subtree[edge.second];

                    if (total > std::numeric_limits<uint32_t>::max())
                        return false;

                    subtree[nodeIndex] = static_cast<uint32_t>(total);
                }

                out.predicateRanges = std::move(fPredicateRanges);
                out.predicates = std::move(fPredicates);
                out.nodes.reserve(fNodes.size());

                for (size_t nodeIndex = 0; nodeIndex < fNodes.size(); ++nodeIndex)
                {
                    const TempNode& src = fNodes[nodeIndex];

                    if (out.edges.size() > std::numeric_limits<uint32_t>::max() ||
                        out.accepts.size() > std::numeric_limits<uint32_t>::max() ||
                        src.edges.size() > std::numeric_limits<uint32_t>::max() ||
                        src.accepts.size() > std::numeric_limits<uint32_t>::max())
                    {
                        return false;
                    }

                    OpenTypeShapingRuleTrieNode node;
                    node.edgeOffset = static_cast<uint32_t>(out.edges.size());
                    node.edgeCount = static_cast<uint32_t>(src.edges.size());
                    node.acceptOffset = static_cast<uint32_t>(out.accepts.size());
                    node.acceptCount = static_cast<uint32_t>(src.accepts.size());
                    node.subtreeRuleCount = subtree[nodeIndex];
                    node.depth = src.depth;

                    for (const auto& edge : src.edges)
                        out.edges.push_back({edge.first, edge.second});

                    out.accepts.insert(out.accepts.end(), src.accepts.begin(), src.accepts.end());
                    out.nodes.push_back(node);
                }

                return analyze(out, analysis);
            }

        private:
            bool canonicalize(OpenTypeShapingIRGlyphSetId setId, OpenTypeShapingRuleTriePredicateId& result)
            {
                result = kOpenTypeShapingRuleTrieInvalid;

                const OpenTypeShapingIRGlyphSet* set = fIR.glyphSet(setId);
                if (!set || set->rangeCount == 0)
                    return false;

                const uint64_t end = uint64_t(set->rangeOffset) + uint64_t(set->rangeCount);
                if (end > fIR.glyphRanges.size())
                    return false;

                std::vector<uint32_t> key;
                key.reserve(set->rangeCount);

                uint64_t memberCount = 0;

                for (uint32_t i = 0; i < set->rangeCount; ++i)
                {
                    const OpenTypeShapingIRGlyphRange& range =
                        fIR.glyphRanges[set->rangeOffset + i];

                    if (range.first > range.last)
                        return false;

                    key.push_back((uint32_t(range.first) << 16) | uint32_t(range.last));
                    memberCount += uint32_t(range.last) - uint32_t(range.first) + 1u;
                }

                const auto found = fPredicateMap.find(key);
                if (found != fPredicateMap.end())
                {
                    result = found->second;
                    return true;
                }

                if (fPredicates.size() >= uint64_t(kOpenTypeShapingRuleTrieInvalid) ||
                    fPredicateRanges.size() > std::numeric_limits<uint32_t>::max() ||
                    memberCount > std::numeric_limits<uint32_t>::max())
                {
                    return false;
                }

                OpenTypeShapingRuleTriePredicate predicate;
                predicate.rangeOffset = static_cast<uint32_t>(fPredicateRanges.size());
                predicate.rangeCount = set->rangeCount;
                predicate.memberCount = static_cast<uint32_t>(memberCount);

                for (uint32_t i = 0; i < set->rangeCount; ++i)
                    fPredicateRanges.push_back(fIR.glyphRanges[set->rangeOffset + i]);

                result = static_cast<OpenTypeShapingRuleTriePredicateId>(fPredicates.size());
                fPredicates.push_back(predicate);
                fPredicateMap.emplace(std::move(key), result);
                return true;
            }

            static bool predicateIntersects(const OpenTypeShapingRuleTrie& trie,
                OpenTypeShapingRuleTriePredicateId aId,
                OpenTypeShapingRuleTriePredicateId bId) noexcept
            {
                if (aId >= trie.predicates.size() || bId >= trie.predicates.size())
                    return false;

                const OpenTypeShapingRuleTriePredicate& a = trie.predicates[aId];
                const OpenTypeShapingRuleTriePredicate& b = trie.predicates[bId];

                size_t ai = 0;
                size_t bi = 0;

                while (ai < a.rangeCount && bi < b.rangeCount)
                {
                    const auto& ar = trie.predicateRanges[a.rangeOffset + ai];
                    const auto& br = trie.predicateRanges[b.rangeOffset + bi];

                    if (ar.last < br.first)
                        ++ai;
                    else if (br.last < ar.first)
                        ++bi;
                    else
                        return true;
                }

                return false;
            }

            static bool analyze(const OpenTypeShapingRuleTrie& trie,
                OpenTypeShapingRuleTrieAnalysis& analysis)
            {
                analysis.canonicalPredicateCount = static_cast<uint32_t>(trie.predicates.size());
                analysis.nodeCount = static_cast<uint32_t>(trie.nodes.size());
                analysis.edgeCount = static_cast<uint32_t>(trie.edges.size());

                if (trie.nodes.empty())
                    return false;

                analysis.ruleCount = trie.nodes[0].subtreeRuleCount;
                analysis.rootFanout = trie.nodes[0].edgeCount;

                for (const OpenTypeShapingRuleTrieNode& node : trie.nodes)
                {
                    analysis.sourcePredicatePositions += uint64_t(node.acceptCount) * uint64_t(node.depth);
                    analysis.maxDepth = std::max(analysis.maxDepth, node.depth);
                    analysis.maxNodeFanout = std::max(analysis.maxNodeFanout, node.edgeCount);

                    if (node.acceptCount)
                    {
                        ++analysis.acceptingNodeCount;
                        if (node.acceptCount > 1)
                            analysis.duplicatePatternRules += node.acceptCount - 1;
                    }

                    bool nodeOverlaps = false;

                    for (uint32_t i = 0; i < node.edgeCount; ++i)
                    {
                        const auto& a = trie.edges[node.edgeOffset + i];

                        for (uint32_t j = i + 1; j < node.edgeCount; ++j)
                        {
                            const auto& b = trie.edges[node.edgeOffset + j];

                            if (!predicateIntersects(trie, a.predicate, b.predicate))
                                continue;

                            ++analysis.siblingOverlapPairs;
                            nodeOverlaps = true;

                            if (node.depth == 0)
                                ++analysis.rootOverlapPairs;
                        }
                    }

                    if (nodeOverlaps)
                        ++analysis.nodesWithOverlappingEdges;
                }

                if (analysis.sourcePredicatePositions >= analysis.edgeCount)
                    analysis.sharedPrefixPositions = analysis.sourcePredicatePositions - analysis.edgeCount;

                if (analysis.sourcePredicatePositions)
                {
                    analysis.edgeToSourcePredicateRatio =
                        double(analysis.edgeCount) / double(analysis.sourcePredicatePositions);

                    analysis.sharedPrefixFraction =
                        double(analysis.sharedPrefixPositions) / double(analysis.sourcePredicatePositions);
                }

                const OpenTypeShapingRuleTrieNode& root = trie.nodes[0];

                if (root.edgeCount)
                {
                    uint64_t total = 0;
                    analysis.rootMinRules = std::numeric_limits<uint32_t>::max();

                    for (uint32_t i = 0; i < root.edgeCount; ++i)
                    {
                        const auto& edge = trie.edges[root.edgeOffset + i];
                        if (edge.child >= trie.nodes.size())
                            return false;

                        const uint32_t rules = trie.nodes[edge.child].subtreeRuleCount;
                        analysis.rootMinRules = std::min(analysis.rootMinRules, rules);
                        analysis.rootMaxRules = std::max(analysis.rootMaxRules, rules);
                        total += rules;
                    }

                    analysis.rootMeanRules = double(total) / double(root.edgeCount);
                }

                return true;
            }

            const OpenTypeShapingIR& fIR;
            std::vector<TempNode> fNodes{};
            std::map<std::vector<uint32_t>, OpenTypeShapingRuleTriePredicateId> fPredicateMap{};
            std::vector<OpenTypeShapingIRGlyphRange> fPredicateRanges{};
            std::vector<OpenTypeShapingRuleTriePredicate> fPredicates{};
        };
    }

    static inline bool compileOpenTypeShapingRuleTrieContextSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubContextSubtable& subtable,
        OpenTypeShapingRuleTrie& trie,
        OpenTypeShapingRuleTrieAnalysis& analysis)
    {
        trie.clear();
        analysis = {};

        const uint64_t ruleEnd = uint64_t(subtable.ruleOffset) + uint64_t(subtable.ruleCount);
        if (subtable.ruleCount == 0 || ruleEnd > ir.gsubContextRules.size())
            return false;

        OpenTypeShapingRuleTrieDetail::Builder builder(ir);

        for (uint32_t i = 0; i < subtable.ruleCount; ++i)
        {
            const uint32_t ruleId = subtable.ruleOffset + i;
            const auto& rule = ir.gsubContextRules[ruleId];

            const uint64_t setEnd = uint64_t(rule.inputSetOffset) + uint64_t(rule.inputCount);
            if (rule.inputCount == 0 || setEnd > ir.gsubContextInputSets.size())
                return false;

            if (!builder.insert(ir.gsubContextInputSets.data() + rule.inputSetOffset, rule.inputCount, ruleId))
                return false;
        }

        return builder.finalize(trie, analysis);
    }

    static inline bool compileOpenTypeShapingRuleTrieChainContextSubtable(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubChainContextSubtable& subtable,
        OpenTypeShapingRuleTrie& trie,
        OpenTypeShapingRuleTrieAnalysis& analysis)
    {
        trie.clear();
        analysis = {};

        const uint64_t ruleEnd = uint64_t(subtable.ruleOffset) + uint64_t(subtable.ruleCount);
        if (subtable.ruleCount == 0 || ruleEnd > ir.gsubChainContextRules.size())
            return false;

        OpenTypeShapingRuleTrieDetail::Builder builder(ir);

        for (uint32_t i = 0; i < subtable.ruleCount; ++i)
        {
            const uint32_t ruleId = subtable.ruleOffset + i;
            const auto& rule = ir.gsubChainContextRules[ruleId];

            const uint64_t setEnd = uint64_t(rule.inputSetOffset) + uint64_t(rule.inputCount);
            if (rule.inputCount == 0 || setEnd > ir.gsubChainContextSets.size())
                return false;

            if (!builder.insert(ir.gsubChainContextSets.data() + rule.inputSetOffset, rule.inputCount, ruleId))
                return false;
        }

        return builder.finalize(trie, analysis);
    }

    static inline std::vector<OpenTypeShapingRuleTrieRootBucket>
        collectOpenTypeShapingRuleTrieRootBuckets(const OpenTypeShapingRuleTrie& trie)
    {
        std::vector<OpenTypeShapingRuleTrieRootBucket> result;

        if (trie.nodes.empty())
            return result;

        const auto& root = trie.nodes[0];
        result.reserve(root.edgeCount);

        for (uint32_t i = 0; i < root.edgeCount; ++i)
        {
            const auto& edge = trie.edges[root.edgeOffset + i];
            if (edge.child >= trie.nodes.size() || edge.predicate >= trie.predicates.size())
                continue;

            const auto& predicate = trie.predicates[edge.predicate];
            result.push_back({
                edge.predicate,
                trie.nodes[edge.child].subtreeRuleCount,
                predicate.rangeCount,
                predicate.memberCount
            });
        }

        std::sort(result.begin(), result.end(),
            [](const OpenTypeShapingRuleTrieRootBucket& a, const OpenTypeShapingRuleTrieRootBucket& b)
            {
                if (a.subtreeRuleCount != b.subtreeRuleCount)
                    return a.subtreeRuleCount > b.subtreeRuleCount;
                return a.predicate < b.predicate;
            });

        return result;
    }
}
