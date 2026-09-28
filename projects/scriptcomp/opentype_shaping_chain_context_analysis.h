// opentype_shaping_chain_context_analysis.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include "opentype_shaping_rule_trie.h"

namespace waavs
{
    struct OpenTypeShapingChainConstraintTrieMetrics
    {
        uint32_t sequenceCount{0};
        uint64_t sourcePredicatePositions{0};
        uint32_t nodeCount{0};
        uint32_t edgeCount{0};
        uint32_t acceptingNodeCount{0};
        uint32_t duplicateSequences{0};
        uint32_t maxDepth{0};
        uint32_t rootFanout{0};
        uint32_t maxNodeFanout{0};
        uint64_t sharedPrefixPositions{0};
        double edgeToSourcePredicateRatio{0.0};
        double sharedPrefixFraction{0.0};
    };

    struct OpenTypeShapingChainAcceptStateAnalysis
    {
        OpenTypeShapingRuleTrieNodeId inputNode{kOpenTypeShapingRuleTrieInvalid};
        uint32_t inputDepth{0};
        uint32_t candidateRules{0};

        uint32_t distinctBacktrackPatterns{0};
        uint32_t distinctLookaheadPatterns{0};
        uint32_t distinctConstraintPairs{0};
        uint32_t duplicateConstraintRules{0};

        uint64_t backtrackPredicatePositions{0};
        uint64_t lookaheadPredicatePositions{0};

        OpenTypeShapingChainConstraintTrieMetrics backtrackTrie{};
        OpenTypeShapingChainConstraintTrieMetrics lookaheadTrie{};
    };

    struct OpenTypeShapingChainContextAnalysis
    {
        uint32_t sourceRules{0};
        uint32_t inputAcceptStates{0};

        uint32_t minCandidateRules{0};
        uint32_t maxCandidateRules{0};
        double meanCandidateRules{0.0};

        uint64_t totalBacktrackPredicatePositions{0};
        uint64_t totalLookaheadPredicatePositions{0};

        uint32_t totalDistinctBacktrackPatterns{0};
        uint32_t totalDistinctLookaheadPatterns{0};
        uint32_t totalDistinctConstraintPairs{0};
        uint32_t totalDuplicateConstraintRules{0};

        uint64_t totalBacktrackTrieEdges{0};
        uint64_t totalLookaheadTrieEdges{0};
        uint64_t totalBacktrackSharedPrefixPositions{0};
        uint64_t totalLookaheadSharedPrefixPositions{0};

        uint32_t maxDistinctBacktrackPatterns{0};
        uint32_t maxDistinctLookaheadPatterns{0};
        uint32_t maxDistinctConstraintPairs{0};

        std::vector<OpenTypeShapingChainAcceptStateAnalysis> acceptStates{};
    };

    namespace OpenTypeShapingChainContextAnalysisDetail
    {
        using PredicateKey = std::vector<uint32_t>;
        using SequenceKey = std::vector<PredicateKey>;

        struct SequenceTrieNode
        {
            std::map<PredicateKey, uint32_t> edges{};
            uint32_t acceptCount{0};
            uint32_t depth{0};
        };

        static inline bool makePredicateKey(const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId setId, PredicateKey& result)
        {
            result.clear();

            const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(setId);
            if (!set || set->rangeCount == 0) return false;

            const uint64_t end = uint64_t(set->rangeOffset) + uint64_t(set->rangeCount);
            if (end > ir.glyphRanges.size()) return false;

            result.reserve(set->rangeCount);

            for (uint32_t i = 0; i < set->rangeCount; ++i)
            {
                const OpenTypeShapingIRGlyphRange& range = ir.glyphRanges[set->rangeOffset + i];
                if (range.first > range.last) return false;
                result.push_back((uint32_t(range.first) << 16) | uint32_t(range.last));
            }

            return true;
        }

        static inline bool makeSequenceKey(const OpenTypeShapingIR& ir,
            const std::vector<OpenTypeShapingIRGlyphSetId>& sets,
            uint32_t offset, uint32_t count, SequenceKey& result)
        {
            result.clear();

            const uint64_t end = uint64_t(offset) + uint64_t(count);
            if (end > sets.size()) return false;

            result.reserve(count);

            for (uint32_t i = 0; i < count; ++i)
            {
                PredicateKey predicate;
                if (!makePredicateKey(ir, sets[offset + i], predicate)) return false;
                result.push_back(std::move(predicate));
            }

            return true;
        }

        static inline bool analyzeSequenceTrie(const std::vector<SequenceKey>& sequences,
            OpenTypeShapingChainConstraintTrieMetrics& result)
        {
            result = {};
            if (sequences.empty()) return true;

            std::vector<SequenceTrieNode> nodes(1);

            for (const SequenceKey& sequence : sequences)
            {
                ++result.sequenceCount;
                result.sourcePredicatePositions += sequence.size();

                uint32_t node = 0;

                for (const PredicateKey& predicate : sequence)
                {
                    auto it = nodes[node].edges.find(predicate);

                    if (it == nodes[node].edges.end())
                    {
                        if (nodes.size() > std::numeric_limits<uint32_t>::max()) return false;

                        const uint32_t child = static_cast<uint32_t>(nodes.size());
                        SequenceTrieNode next;
                        next.depth = nodes[node].depth + 1;
                        nodes.push_back(std::move(next));
                        nodes[node].edges.emplace(predicate, child);
                        node = child;
                    }
                    else
                    {
                        node = it->second;
                    }
                }

                ++nodes[node].acceptCount;
            }

            result.nodeCount = static_cast<uint32_t>(nodes.size());
            result.edgeCount = result.nodeCount ? result.nodeCount - 1u : 0u;
            result.rootFanout = nodes.empty() ? 0u : static_cast<uint32_t>(nodes[0].edges.size());

            for (const SequenceTrieNode& node : nodes)
            {
                result.maxDepth = std::max(result.maxDepth, node.depth);
                result.maxNodeFanout = std::max(result.maxNodeFanout, static_cast<uint32_t>(node.edges.size()));

                if (node.acceptCount)
                {
                    ++result.acceptingNodeCount;
                    if (node.acceptCount > 1) result.duplicateSequences += node.acceptCount - 1;
                }
            }

            if (result.sourcePredicatePositions >= result.edgeCount)
                result.sharedPrefixPositions = result.sourcePredicatePositions - result.edgeCount;

            if (result.sourcePredicatePositions)
            {
                result.edgeToSourcePredicateRatio = double(result.edgeCount) / double(result.sourcePredicatePositions);
                result.sharedPrefixFraction = double(result.sharedPrefixPositions) / double(result.sourcePredicatePositions);
            }

            return true;
        }

        struct ConstraintPairKey
        {
            SequenceKey backtrack{};
            SequenceKey lookahead{};

            bool operator<(const ConstraintPairKey& other) const noexcept
            {
                return std::tie(backtrack, lookahead) < std::tie(other.backtrack, other.lookahead);
            }
        };
    }

    static inline bool analyzeOpenTypeShapingChainContextConstraints(
        const OpenTypeShapingIR& ir,
        const OpenTypeShapingIRGsubChainContextSubtable& subtable,
        const OpenTypeShapingRuleTrie& inputTrie,
        OpenTypeShapingChainContextAnalysis& result)
    {
        using namespace OpenTypeShapingChainContextAnalysisDetail;

        result = {};

        const uint64_t ruleEnd = uint64_t(subtable.ruleOffset) + uint64_t(subtable.ruleCount);
        if (subtable.ruleCount == 0 || ruleEnd > ir.gsubChainContextRules.size() || inputTrie.nodes.empty()) return false;

        result.sourceRules = subtable.ruleCount;
        uint64_t totalCandidates = 0;

        for (uint32_t nodeId = 0; nodeId < inputTrie.nodes.size(); ++nodeId)
        {
            const OpenTypeShapingRuleTrieNode& node = inputTrie.nodes[nodeId];
            if (node.acceptCount == 0) continue;

            const uint64_t acceptEnd = uint64_t(node.acceptOffset) + uint64_t(node.acceptCount);
            if (acceptEnd > inputTrie.accepts.size()) return false;

            OpenTypeShapingChainAcceptStateAnalysis state;
            state.inputNode = nodeId;
            state.inputDepth = node.depth;
            state.candidateRules = node.acceptCount;

            std::map<SequenceKey, uint32_t> backtrackPatterns;
            std::map<SequenceKey, uint32_t> lookaheadPatterns;
            std::map<ConstraintPairKey, uint32_t> constraintPairs;
            std::vector<SequenceKey> backtrackSequences;
            std::vector<SequenceKey> lookaheadSequences;

            backtrackSequences.reserve(node.acceptCount);
            lookaheadSequences.reserve(node.acceptCount);

            for (uint32_t i = 0; i < node.acceptCount; ++i)
            {
                const uint32_t ruleId = inputTrie.accepts[node.acceptOffset + i];
                if (ruleId < subtable.ruleOffset || ruleId >= ruleEnd) return false;

                const auto& rule = ir.gsubChainContextRules[ruleId];
                SequenceKey backtrack;
                SequenceKey lookahead;

                if (!makeSequenceKey(ir, ir.gsubChainContextSets,
                        rule.backtrackSetOffset, rule.backtrackCount, backtrack) ||
                    !makeSequenceKey(ir, ir.gsubChainContextSets,
                        rule.lookaheadSetOffset, rule.lookaheadCount, lookahead))
                {
                    return false;
                }

                state.backtrackPredicatePositions += rule.backtrackCount;
                state.lookaheadPredicatePositions += rule.lookaheadCount;

                ++backtrackPatterns[backtrack];
                ++lookaheadPatterns[lookahead];

                ConstraintPairKey pair;
                pair.backtrack = backtrack;
                pair.lookahead = lookahead;
                ++constraintPairs[pair];

                backtrackSequences.push_back(std::move(backtrack));
                lookaheadSequences.push_back(std::move(lookahead));
            }

            state.distinctBacktrackPatterns = static_cast<uint32_t>(backtrackPatterns.size());
            state.distinctLookaheadPatterns = static_cast<uint32_t>(lookaheadPatterns.size());
            state.distinctConstraintPairs = static_cast<uint32_t>(constraintPairs.size());
            state.duplicateConstraintRules = state.candidateRules - state.distinctConstraintPairs;

            if (!analyzeSequenceTrie(backtrackSequences, state.backtrackTrie) ||
                !analyzeSequenceTrie(lookaheadSequences, state.lookaheadTrie))
            {
                return false;
            }

            ++result.inputAcceptStates;
            totalCandidates += state.candidateRules;

            if (result.inputAcceptStates == 1) result.minCandidateRules = state.candidateRules;
            else result.minCandidateRules = std::min(result.minCandidateRules, state.candidateRules);

            result.maxCandidateRules = std::max(result.maxCandidateRules, state.candidateRules);
            result.totalBacktrackPredicatePositions += state.backtrackPredicatePositions;
            result.totalLookaheadPredicatePositions += state.lookaheadPredicatePositions;
            result.totalDistinctBacktrackPatterns += state.distinctBacktrackPatterns;
            result.totalDistinctLookaheadPatterns += state.distinctLookaheadPatterns;
            result.totalDistinctConstraintPairs += state.distinctConstraintPairs;
            result.totalDuplicateConstraintRules += state.duplicateConstraintRules;
            result.totalBacktrackTrieEdges += state.backtrackTrie.edgeCount;
            result.totalLookaheadTrieEdges += state.lookaheadTrie.edgeCount;
            result.totalBacktrackSharedPrefixPositions += state.backtrackTrie.sharedPrefixPositions;
            result.totalLookaheadSharedPrefixPositions += state.lookaheadTrie.sharedPrefixPositions;
            result.maxDistinctBacktrackPatterns = std::max(result.maxDistinctBacktrackPatterns, state.distinctBacktrackPatterns);
            result.maxDistinctLookaheadPatterns = std::max(result.maxDistinctLookaheadPatterns, state.distinctLookaheadPatterns);
            result.maxDistinctConstraintPairs = std::max(result.maxDistinctConstraintPairs, state.distinctConstraintPairs);
            result.acceptStates.push_back(std::move(state));
        }

        if (result.inputAcceptStates == 0) return false;

        result.meanCandidateRules = double(totalCandidates) / double(result.inputAcceptStates);

        std::sort(result.acceptStates.begin(), result.acceptStates.end(),
            [](const OpenTypeShapingChainAcceptStateAnalysis& a,
                const OpenTypeShapingChainAcceptStateAnalysis& b)
            {
                if (a.candidateRules != b.candidateRules) return a.candidateRules > b.candidateRules;
                if (a.distinctConstraintPairs != b.distinctConstraintPairs) return a.distinctConstraintPairs > b.distinctConstraintPairs;
                return a.inputNode < b.inputNode;
            });

        return true;
    }
}
