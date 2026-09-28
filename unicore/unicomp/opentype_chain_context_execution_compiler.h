// opentype_chain_context_execution_compiler.h
#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

#include "opentype_chain_context_execution_ir.h"
#include "opentype_shaping_rule_trie.h"

namespace waavs
{
    namespace OpenTypeChainContextExecutionCompilerDetail
    {
        inline constexpr uint32_t kBacktrackDispatchMinRules = 8;
        inline constexpr uint32_t kBacktrackDispatchMaxMemberships = 4096;
        inline constexpr uint32_t kLookaheadDispatchMinRules = 8;
        inline constexpr uint32_t kLookaheadDispatchMaxMemberships = 4096;
        inline constexpr uint32_t kBacktrackTrieMinRules = 4;
        inline constexpr uint32_t kBacktrackTrieMinPositions = 8;
        inline constexpr uint32_t kBacktrackTrieMaxPositions = 4096;

        static inline bool appendExecutionPredicateForGlyphSet(
            const OpenTypeShapingIR& ir, OpenTypeShapingIRGlyphSetId setId,
            OpenTypeChainContextExecutionIR& exec,
            std::map<OpenTypeShapingIRGlyphSetId, OpenTypeChainExecPredicateId>& predicateMap,
            OpenTypeChainExecPredicateId& result)
        {
            const auto found = predicateMap.find(setId);
            if (found != predicateMap.end())
            {
                result = found->second;
                return true;
            }

            const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(setId);
            if (!set || set->rangeCount == 0 ||
                uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size() ||
                exec.predicateRanges.size() > std::numeric_limits<uint32_t>::max() ||
                exec.predicates.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            const uint32_t rangeOffset = static_cast<uint32_t>(exec.predicateRanges.size());
            exec.predicateRanges.insert(exec.predicateRanges.end(),
                ir.glyphRanges.begin() + set->rangeOffset,
                ir.glyphRanges.begin() + set->rangeOffset + set->rangeCount);

            result = static_cast<OpenTypeChainExecPredicateId>(exec.predicates.size());
            exec.predicates.push_back({rangeOffset, set->rangeCount});
            predicateMap.emplace(setId, result);
            return true;
        }

        static inline bool compileExecutionRules(
            const OpenTypeShapingIR& ir, OpenTypeChainContextExecutionIR& exec)
        {
            if (ir.gsubChainContextRules.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            exec.rules.clear();
            exec.rulePredicates.clear();
            exec.rules.reserve(ir.gsubChainContextRules.size());

            std::map<OpenTypeShapingIRGlyphSetId, OpenTypeChainExecPredicateId> predicateMap;

            for (const auto& rule : ir.gsubChainContextRules)
            {
                if (rule.inputCount == 0 ||
                    uint64_t(rule.backtrackSetOffset) + rule.backtrackCount > ir.gsubChainContextSets.size() ||
                    uint64_t(rule.inputSetOffset) + rule.inputCount > ir.gsubChainContextSets.size() ||
                    uint64_t(rule.lookaheadSetOffset) + rule.lookaheadCount > ir.gsubChainContextSets.size() ||
                    uint64_t(rule.lookupOffset) + rule.lookupCount > ir.gsubChainContextLookups.size() ||
                    exec.rulePredicates.size() > std::numeric_limits<uint32_t>::max()) return false;

                OpenTypeChainExecRule compiled;
                compiled.backtrackPredicateOffset = static_cast<uint32_t>(exec.rulePredicates.size());
                compiled.backtrackCount = rule.backtrackCount;
                compiled.inputCount = rule.inputCount;

                for (uint32_t i = 0; i < rule.backtrackCount; ++i)
                {
                    OpenTypeChainExecPredicateId predicate = kOpenTypeChainExecInvalid;
                    if (!appendExecutionPredicateForGlyphSet(
                        ir, ir.gsubChainContextSets[rule.backtrackSetOffset + i],
                        exec, predicateMap, predicate)) return false;
                    exec.rulePredicates.push_back(predicate);
                }

                if (exec.rulePredicates.size() > std::numeric_limits<uint32_t>::max()) return false;
                compiled.lookaheadPredicateOffset = static_cast<uint32_t>(exec.rulePredicates.size());
                compiled.lookaheadCount = rule.lookaheadCount;

                for (uint32_t i = 0; i < rule.lookaheadCount; ++i)
                {
                    OpenTypeChainExecPredicateId predicate = kOpenTypeChainExecInvalid;
                    if (!appendExecutionPredicateForGlyphSet(
                        ir, ir.gsubChainContextSets[rule.lookaheadSetOffset + i],
                        exec, predicateMap, predicate)) return false;
                    exec.rulePredicates.push_back(predicate);
                }

                compiled.lookupOffset = rule.lookupOffset;
                compiled.lookupCount = rule.lookupCount;
                exec.rules.push_back(compiled);
            }

            return true;
        }

        static inline bool appendDirectResolver(const OpenTypeShapingRuleTrie& trie,
            const OpenTypeShapingRuleTrieNode& node, OpenTypeChainContextExecutionIR& exec,
            OpenTypeChainExecResolverId& result)
        {
            result = kOpenTypeChainExecInvalid;
            if (!node.acceptCount) return true;
            if (uint64_t(node.acceptOffset) + node.acceptCount > trie.accepts.size() ||
                exec.ruleIds.size() > std::numeric_limits<uint32_t>::max() ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            const uint32_t ruleOffset = static_cast<uint32_t>(exec.ruleIds.size());
            exec.ruleIds.insert(exec.ruleIds.end(), trie.accepts.begin() + node.acceptOffset,
                trie.accepts.begin() + node.acceptOffset + node.acceptCount);

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({OpenTypeChainExecResolverKind::Direct, 0, 0, ruleOffset, node.acceptCount});
            return true;
        }

        static inline bool buildBacktrackDispatchBuckets(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            std::map<uint16_t, std::vector<uint32_t>>& buckets)
        {
            buckets.clear();
            if (node.acceptCount < kBacktrackDispatchMinRules ||
                uint64_t(node.acceptOffset) + node.acceptCount > trie.accepts.size()) return true;

            uint64_t memberships = 0;

            for (uint32_t i = 0; i < node.acceptCount; ++i)
            {
                const uint32_t ruleId = trie.accepts[node.acceptOffset + i];
                if (ruleId >= ir.gsubChainContextRules.size()) return false;

                const auto& rule = ir.gsubChainContextRules[ruleId];
                if (rule.backtrackCount != 1 || rule.lookaheadCount != 0 ||
                    rule.backtrackSetOffset >= ir.gsubChainContextSets.size())
                {
                    buckets.clear();
                    return true;
                }

                const OpenTypeShapingIRGlyphSetId setId = ir.gsubChainContextSets[rule.backtrackSetOffset];
                const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(setId);
                if (!set || set->rangeCount == 0 ||
                    uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

                for (uint32_t r = 0; r < set->rangeCount; ++r)
                {
                    const auto& range = ir.glyphRanges[set->rangeOffset + r];
                    if (range.first > range.last) return false;

                    memberships += uint32_t(range.last) - uint32_t(range.first) + 1u;
                    if (memberships > kBacktrackDispatchMaxMemberships)
                    {
                        buckets.clear();
                        return true;
                    }

                    for (uint32_t glyph = range.first; glyph <= range.last; ++glyph)
                    {
                        buckets[static_cast<uint16_t>(glyph)].push_back(ruleId);
                        if (glyph == 0xFFFFu) break;
                    }
                }
            }

            if (buckets.size() < 2)
            {
                buckets.clear();
                return true;
            }

            uint32_t maxBucket = 0;
            for (const auto& bucket : buckets)
                maxBucket = std::max(maxBucket, static_cast<uint32_t>(bucket.second.size()));

            if (maxBucket >= node.acceptCount)
                buckets.clear();

            return true;
        }

        static inline bool appendBacktrackDispatchResolver(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result, bool& used)
        {
            result = kOpenTypeChainExecInvalid;
            used = false;

            std::map<uint16_t, std::vector<uint32_t>> buckets;
            if (!buildBacktrackDispatchBuckets(ir, trie, node, buckets)) return false;
            if (buckets.empty()) return true;

            if (exec.backtrackDispatchEntries.size() > std::numeric_limits<uint32_t>::max() ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            const uint32_t entryOffset = static_cast<uint32_t>(exec.backtrackDispatchEntries.size());

            for (const auto& bucket : buckets)
            {
                if (exec.ruleIds.size() > std::numeric_limits<uint32_t>::max() ||
                    bucket.second.size() > std::numeric_limits<uint32_t>::max()) return false;

                const uint32_t ruleOffset = static_cast<uint32_t>(exec.ruleIds.size());
                exec.ruleIds.insert(exec.ruleIds.end(), bucket.second.begin(), bucket.second.end());

                exec.backtrackDispatchEntries.push_back({
                    bucket.first, 0, ruleOffset, static_cast<uint32_t>(bucket.second.size())
                });
            }

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::BacktrackDispatch, 0, 0,
                entryOffset, static_cast<uint32_t>(buckets.size())
            });
            used = true;
            return true;
        }

        static inline bool buildLookaheadDispatchBuckets(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            std::vector<uint32_t>& defaults, std::map<uint16_t, std::vector<uint32_t>>& buckets)
        {
            defaults.clear();
            buckets.clear();

            if (node.acceptCount < kLookaheadDispatchMinRules ||
                uint64_t(node.acceptOffset) + node.acceptCount > trie.accepts.size()) return true;

            uint64_t memberships = 0;

            for (uint32_t i = 0; i < node.acceptCount; ++i)
            {
                const uint32_t ruleId = trie.accepts[node.acceptOffset + i];
                if (ruleId >= ir.gsubChainContextRules.size()) return false;

                const auto& rule = ir.gsubChainContextRules[ruleId];

                if (rule.lookaheadCount == 0)
                {
                    defaults.push_back(ruleId);
                    continue;
                }

                if (rule.lookaheadSetOffset >= ir.gsubChainContextSets.size()) return false;

                const OpenTypeShapingIRGlyphSetId setId = ir.gsubChainContextSets[rule.lookaheadSetOffset];
                const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(setId);

                if (!set || set->rangeCount == 0 ||
                    uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

                for (uint32_t r = 0; r < set->rangeCount; ++r)
                {
                    const auto& range = ir.glyphRanges[set->rangeOffset + r];
                    if (range.first > range.last) return false;

                    memberships += uint32_t(range.last) - uint32_t(range.first) + 1u;
                    if (memberships > kLookaheadDispatchMaxMemberships)
                    {
                        defaults.clear();
                        buckets.clear();
                        return true;
                    }

                    for (uint32_t glyph = range.first; glyph <= range.last; ++glyph)
                    {
                        buckets[static_cast<uint16_t>(glyph)].push_back(ruleId);
                        if (glyph == 0xFFFFu) break;
                    }
                }
            }

            if (buckets.empty())
            {
                defaults.clear();
                return true;
            }

            uint32_t maxSelected = static_cast<uint32_t>(defaults.size());

            for (const auto& bucket : buckets)
            {
                const uint32_t selected = static_cast<uint32_t>(defaults.size() + bucket.second.size());
                maxSelected = std::max(maxSelected, selected);
            }

            if (maxSelected >= node.acceptCount)
            {
                defaults.clear();
                buckets.clear();
            }

            return true;
        }

        static inline bool appendLookaheadDispatchResolver(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result, bool& used)
        {
            result = kOpenTypeChainExecInvalid;
            used = false;

            std::vector<uint32_t> defaults;
            std::map<uint16_t, std::vector<uint32_t>> buckets;

            if (!buildLookaheadDispatchBuckets(ir, trie, node, defaults, buckets)) return false;
            if (buckets.empty()) return true;

            if (exec.lookaheadDispatches.size() >= uint64_t(kOpenTypeChainExecInvalid) ||
                exec.lookaheadDispatchEntries.size() > std::numeric_limits<uint32_t>::max() ||
                exec.ruleIds.size() > std::numeric_limits<uint32_t>::max() ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            const uint32_t defaultRuleOffset = static_cast<uint32_t>(exec.ruleIds.size());
            exec.ruleIds.insert(exec.ruleIds.end(), defaults.begin(), defaults.end());
            const uint32_t defaultRuleCount = static_cast<uint32_t>(defaults.size());

            const uint32_t entryOffset = static_cast<uint32_t>(exec.lookaheadDispatchEntries.size());

            for (const auto& bucket : buckets)
            {
                std::vector<uint32_t> selected;
                selected.reserve(defaults.size() + bucket.second.size());

                auto a = defaults.begin();
                auto b = bucket.second.begin();

                while (a != defaults.end() || b != bucket.second.end())
                {
                    if (b == bucket.second.end() || (a != defaults.end() && *a < *b))
                    {
                        selected.push_back(*a++);
                    }
                    else if (a == defaults.end() || *b < *a)
                    {
                        selected.push_back(*b++);
                    }
                    else
                    {
                        selected.push_back(*a);
                        ++a;
                        ++b;
                    }
                }

                if (exec.ruleIds.size() > std::numeric_limits<uint32_t>::max() ||
                    selected.size() > std::numeric_limits<uint32_t>::max()) return false;

                const uint32_t ruleOffset = static_cast<uint32_t>(exec.ruleIds.size());
                exec.ruleIds.insert(exec.ruleIds.end(), selected.begin(), selected.end());

                exec.lookaheadDispatchEntries.push_back({
                    bucket.first, 0, ruleOffset, static_cast<uint32_t>(selected.size())
                });
            }

            const uint32_t dispatchId = static_cast<uint32_t>(exec.lookaheadDispatches.size());
            exec.lookaheadDispatches.push_back({
                entryOffset, static_cast<uint32_t>(buckets.size()), defaultRuleOffset, defaultRuleCount
            });

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::LookaheadDispatch, 0, 0, dispatchId, 1
            });

            used = true;
            return true;
        }

        static inline bool appendDirectRuleResolver(const std::vector<uint32_t>& rules,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result)
        {
            result = kOpenTypeChainExecInvalid;
            if (rules.empty()) return true;
            if (exec.ruleIds.size() > std::numeric_limits<uint32_t>::max() ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            const uint32_t ruleOffset = static_cast<uint32_t>(exec.ruleIds.size());
            exec.ruleIds.insert(exec.ruleIds.end(), rules.begin(), rules.end());

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::Direct, 0, 0,
                ruleOffset, static_cast<uint32_t>(rules.size())
            });
            return true;
        }

        static inline bool buildBacktrackDispatchBucketsForRules(const OpenTypeShapingIR& ir,
            const std::vector<uint32_t>& rules, std::map<uint16_t, std::vector<uint32_t>>& buckets)
        {
            buckets.clear();
            if (rules.size() < 2) return true;

            uint64_t memberships = 0;

            for (uint32_t ruleId : rules)
            {
                if (ruleId >= ir.gsubChainContextRules.size()) return false;
                const auto& rule = ir.gsubChainContextRules[ruleId];

                if (rule.backtrackCount != 1 || rule.backtrackSetOffset >= ir.gsubChainContextSets.size())
                {
                    buckets.clear();
                    return true;
                }

                const OpenTypeShapingIRGlyphSetId setId = ir.gsubChainContextSets[rule.backtrackSetOffset];
                const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(setId);
                if (!set || set->rangeCount == 0 ||
                    uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

                for (uint32_t r = 0; r < set->rangeCount; ++r)
                {
                    const auto& range = ir.glyphRanges[set->rangeOffset + r];
                    if (range.first > range.last) return false;

                    memberships += uint32_t(range.last) - uint32_t(range.first) + 1u;
                    if (memberships > kBacktrackDispatchMaxMemberships)
                    {
                        buckets.clear();
                        return true;
                    }

                    for (uint32_t glyph = range.first; glyph <= range.last; ++glyph)
                    {
                        buckets[static_cast<uint16_t>(glyph)].push_back(ruleId);
                        if (glyph == 0xFFFFu) break;
                    }
                }
            }

            if (buckets.size() < 2)
            {
                buckets.clear();
                return true;
            }

            uint32_t maxBucket = 0;
            for (const auto& bucket : buckets)
                maxBucket = std::max(maxBucket, static_cast<uint32_t>(bucket.second.size()));

            if (maxBucket >= rules.size()) buckets.clear();
            return true;
        }

        static inline bool appendBacktrackOrDirectRuleResolver(const OpenTypeShapingIR& ir,
            const std::vector<uint32_t>& rules, OpenTypeChainContextExecutionIR& exec,
            OpenTypeChainExecResolverId& result, bool& usedBacktrack)
        {
            result = kOpenTypeChainExecInvalid;
            usedBacktrack = false;
            if (rules.empty()) return true;

            std::map<uint16_t, std::vector<uint32_t>> buckets;
            if (!buildBacktrackDispatchBucketsForRules(ir, rules, buckets)) return false;

            if (buckets.empty()) return appendDirectRuleResolver(rules, exec, result);

            if (exec.backtrackDispatchEntries.size() > std::numeric_limits<uint32_t>::max() ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            const uint32_t entryOffset = static_cast<uint32_t>(exec.backtrackDispatchEntries.size());

            for (const auto& bucket : buckets)
            {
                if (exec.ruleIds.size() > std::numeric_limits<uint32_t>::max()) return false;
                const uint32_t ruleOffset = static_cast<uint32_t>(exec.ruleIds.size());
                exec.ruleIds.insert(exec.ruleIds.end(), bucket.second.begin(), bucket.second.end());
                exec.backtrackDispatchEntries.push_back({
                    bucket.first, 0, ruleOffset, static_cast<uint32_t>(bucket.second.size())
                });
            }

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::BacktrackDispatch, 0, 0,
                entryOffset, static_cast<uint32_t>(buckets.size())
            });
            usedBacktrack = true;
            return true;
        }

        struct OpenTypeChainExecTempBacktrackTrieNode
        {
            std::map<OpenTypeShapingIRGlyphSetId, uint32_t> children{};
            std::vector<uint32_t> accepts{};
        };

        static inline bool buildBacktrackTrieForRules(const OpenTypeShapingIR& ir,
            const std::vector<uint32_t>& rules, std::vector<OpenTypeChainExecTempBacktrackTrieNode>& nodes)
        {
            nodes.clear();
            if (rules.size() < kBacktrackTrieMinRules) return true;

            nodes.emplace_back();
            uint32_t sourcePositions = 0;
            uint32_t maxDepth = 0;

            for (uint32_t ruleId : rules)
            {
                if (ruleId >= ir.gsubChainContextRules.size()) return false;
                const auto& rule = ir.gsubChainContextRules[ruleId];

                if (uint64_t(rule.backtrackSetOffset) + rule.backtrackCount > ir.gsubChainContextSets.size())
                    return false;

                if (rule.backtrackCount > kBacktrackTrieMaxPositions ||
                    sourcePositions > kBacktrackTrieMaxPositions - rule.backtrackCount)
                {
                    nodes.clear();
                    return true;
                }

                sourcePositions += rule.backtrackCount;
                maxDepth = std::max(maxDepth, rule.backtrackCount);

                uint32_t nodeId = 0;
                for (uint32_t i = 0; i < rule.backtrackCount; ++i)
                {
                    const OpenTypeShapingIRGlyphSetId setId =
                        ir.gsubChainContextSets[rule.backtrackSetOffset + i];
                    const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(setId);

                    if (!set || set->rangeCount == 0 ||
                        uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size()) return false;

                    auto found = nodes[nodeId].children.find(setId);
                    if (found != nodes[nodeId].children.end())
                    {
                        nodeId = found->second;
                        continue;
                    }

                    if (nodes.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;
                    const uint32_t child = static_cast<uint32_t>(nodes.size());
                    nodes.emplace_back();
                    nodes[nodeId].children.emplace(setId, child);
                    nodeId = child;
                }

                nodes[nodeId].accepts.push_back(ruleId);
            }

            if (maxDepth < 2 || sourcePositions < kBacktrackTrieMinPositions)
            {
                nodes.clear();
                return true;
            }

            const uint32_t trieEdges = static_cast<uint32_t>(nodes.size() - 1);

            // Require at least 25% structural sharing versus the source positions.
            if (uint64_t(trieEdges) * 4u > uint64_t(sourcePositions) * 3u)
                nodes.clear();

            return true;
        }

        static inline bool appendBacktrackTrieResolverForRules(const OpenTypeShapingIR& ir,
            const std::vector<uint32_t>& rules, OpenTypeChainContextExecutionIR& exec,
            OpenTypeChainExecResolverId& result, bool& usedTrie)
        {
            result = kOpenTypeChainExecInvalid;
            usedTrie = false;
            if (rules.empty()) return true;

            std::vector<OpenTypeChainExecTempBacktrackTrieNode> nodes;
            if (!buildBacktrackTrieForRules(ir, rules, nodes)) return false;
            if (nodes.empty()) return true;

            if (exec.backtrackTrieStates.size() > std::numeric_limits<uint32_t>::max() ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            std::map<OpenTypeShapingIRGlyphSetId, OpenTypeChainExecPredicateId> predicateMap;

            for (const auto& node : nodes)
            {
                for (const auto& edge : node.children)
                {
                    if (predicateMap.find(edge.first) != predicateMap.end()) continue;

                    const OpenTypeShapingIRGlyphSet* set = ir.glyphSet(edge.first);
                    if (!set || uint64_t(set->rangeOffset) + set->rangeCount > ir.glyphRanges.size() ||
                        exec.predicateRanges.size() > std::numeric_limits<uint32_t>::max() ||
                        exec.predicates.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

                    const uint32_t rangeOffset = static_cast<uint32_t>(exec.predicateRanges.size());
                    exec.predicateRanges.insert(exec.predicateRanges.end(),
                        ir.glyphRanges.begin() + set->rangeOffset,
                        ir.glyphRanges.begin() + set->rangeOffset + set->rangeCount);

                    const OpenTypeChainExecPredicateId predicate =
                        static_cast<OpenTypeChainExecPredicateId>(exec.predicates.size());
                    exec.predicates.push_back({rangeOffset, set->rangeCount});
                    predicateMap.emplace(edge.first, predicate);
                }
            }

            const uint32_t stateBase = static_cast<uint32_t>(exec.backtrackTrieStates.size());
            exec.backtrackTrieStates.reserve(exec.backtrackTrieStates.size() + nodes.size());

            for (const auto& node : nodes)
            {
                if (node.children.size() > std::numeric_limits<uint16_t>::max() ||
                    exec.backtrackTrieEdges.size() > std::numeric_limits<uint32_t>::max() ||
                    exec.ruleIds.size() > std::numeric_limits<uint32_t>::max()) return false;

                OpenTypeChainExecBacktrackTrieState state;
                state.edgeOffset = static_cast<uint32_t>(exec.backtrackTrieEdges.size());
                state.edgeCount = static_cast<uint16_t>(node.children.size());
                state.acceptOffset = static_cast<uint32_t>(exec.ruleIds.size());
                state.acceptCount = static_cast<uint32_t>(node.accepts.size());

                exec.ruleIds.insert(exec.ruleIds.end(), node.accepts.begin(), node.accepts.end());

                for (const auto& edge : node.children)
                {
                    const auto predicate = predicateMap.find(edge.first);
                    if (predicate == predicateMap.end() ||
                        uint64_t(stateBase) + edge.second > std::numeric_limits<uint32_t>::max()) return false;

                    exec.backtrackTrieEdges.push_back({
                        predicate->second, stateBase + edge.second
                    });
                }

                exec.backtrackTrieStates.push_back(state);
            }

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::BacktrackTrie, 0, 0,
                stateBase, static_cast<uint32_t>(nodes.size())
            });
            usedTrie = true;
            return true;
        }

        static inline bool appendBacktrackTrieOrDispatchOrDirectRuleResolver(
            const OpenTypeShapingIR& ir, const std::vector<uint32_t>& rules,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result,
            bool& usedTrie, bool& usedBacktrackDispatch)
        {
            result = kOpenTypeChainExecInvalid;
            usedTrie = false;
            usedBacktrackDispatch = false;

            if (!appendBacktrackTrieResolverForRules(ir, rules, exec, result, usedTrie)) return false;
            if (usedTrie) return true;

            return appendBacktrackOrDirectRuleResolver(
                ir, rules, exec, result, usedBacktrackDispatch);
        }

        static inline void mergeOrderedRuleIds(const std::vector<uint32_t>& a,
            const std::vector<uint32_t>& b, std::vector<uint32_t>& result)
        {
            result.clear();
            result.reserve(a.size() + b.size());
            auto ia = a.begin();
            auto ib = b.begin();

            while (ia != a.end() || ib != b.end())
            {
                if (ib == b.end() || (ia != a.end() && *ia < *ib)) result.push_back(*ia++);
                else if (ia == a.end() || *ib < *ia) result.push_back(*ib++);
                else
                {
                    result.push_back(*ia);
                    ++ia;
                    ++ib;
                }
            }
        }

        static inline bool appendLookaheadThenBacktrackTrieResolver(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result, bool& used)
        {
            result = kOpenTypeChainExecInvalid;
            used = false;

            std::vector<uint32_t> defaults;
            std::map<uint16_t, std::vector<uint32_t>> buckets;
            if (!buildLookaheadDispatchBuckets(ir, trie, node, defaults, buckets)) return false;
            if (buckets.empty()) return true;

            struct PlannedGroup
            {
                uint16_t glyph{0};
                std::vector<uint32_t> rules{};
                bool usesTrie{false};
            };

            std::vector<PlannedGroup> groups;
            groups.reserve(buckets.size());
            bool anyTrie = false;

            for (const auto& bucket : buckets)
            {
                PlannedGroup group;
                group.glyph = bucket.first;
                mergeOrderedRuleIds(defaults, bucket.second, group.rules);

                std::vector<OpenTypeChainExecTempBacktrackTrieNode> trieNodes;
                if (!buildBacktrackTrieForRules(ir, group.rules, trieNodes)) return false;
                group.usesTrie = !trieNodes.empty();
                anyTrie |= group.usesTrie;
                groups.push_back(std::move(group));
            }

            bool defaultUsesTrie = false;
            if (!defaults.empty())
            {
                std::vector<OpenTypeChainExecTempBacktrackTrieNode> trieNodes;
                if (!buildBacktrackTrieForRules(ir, defaults, trieNodes)) return false;
                defaultUsesTrie = !trieNodes.empty();
                anyTrie |= defaultUsesTrie;
            }

            if (!anyTrie) return true;

            if (exec.lookaheadResolverEntries.size() > std::numeric_limits<uint32_t>::max() ||
                exec.lookaheadThenBacktrackTries.size() >= uint64_t(kOpenTypeChainExecInvalid) ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            OpenTypeChainExecResolverId defaultResolver = kOpenTypeChainExecInvalid;
            if (!defaults.empty())
            {
                bool usedTrie = false;
                bool usedBacktrackDispatch = false;
                if (!appendBacktrackTrieOrDispatchOrDirectRuleResolver(
                    ir, defaults, exec, defaultResolver, usedTrie, usedBacktrackDispatch)) return false;
            }

            const uint32_t entryOffset = static_cast<uint32_t>(exec.lookaheadResolverEntries.size());

            for (const PlannedGroup& group : groups)
            {
                OpenTypeChainExecResolverId childResolver = kOpenTypeChainExecInvalid;
                bool usedTrie = false;
                bool usedBacktrackDispatch = false;
                if (!appendBacktrackTrieOrDispatchOrDirectRuleResolver(
                    ir, group.rules, exec, childResolver, usedTrie, usedBacktrackDispatch)) return false;

                exec.lookaheadResolverEntries.push_back({group.glyph, 0, childResolver});
            }

            const uint32_t dispatchId = static_cast<uint32_t>(exec.lookaheadThenBacktrackTries.size());
            exec.lookaheadThenBacktrackTries.push_back({
                entryOffset, static_cast<uint32_t>(groups.size()), defaultResolver
            });

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::LookaheadThenBacktrackTrie, 0, 0,
                dispatchId, 1
            });
            used = true;
            return true;
        }

        static inline bool appendLookaheadThenBacktrackDispatchResolver(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result, bool& used)
        {
            result = kOpenTypeChainExecInvalid;
            used = false;

            std::vector<uint32_t> defaults;
            std::map<uint16_t, std::vector<uint32_t>> buckets;
            if (!buildLookaheadDispatchBuckets(ir, trie, node, defaults, buckets)) return false;
            if (buckets.empty()) return true;

            struct PlannedGroup
            {
                uint16_t glyph{0};
                std::vector<uint32_t> rules{};
                bool usesBacktrack{false};
            };

            std::vector<PlannedGroup> groups;
            groups.reserve(buckets.size());
            bool anyBacktrack = false;

            for (const auto& bucket : buckets)
            {
                PlannedGroup group;
                group.glyph = bucket.first;
                mergeOrderedRuleIds(defaults, bucket.second, group.rules);

                std::map<uint16_t, std::vector<uint32_t>> backtrackBuckets;
                if (!buildBacktrackDispatchBucketsForRules(ir, group.rules, backtrackBuckets)) return false;
                group.usesBacktrack = !backtrackBuckets.empty();
                anyBacktrack |= group.usesBacktrack;
                groups.push_back(std::move(group));
            }

            bool defaultUsesBacktrack = false;
            if (!defaults.empty())
            {
                std::map<uint16_t, std::vector<uint32_t>> backtrackBuckets;
                if (!buildBacktrackDispatchBucketsForRules(ir, defaults, backtrackBuckets)) return false;
                defaultUsesBacktrack = !backtrackBuckets.empty();
                anyBacktrack |= defaultUsesBacktrack;
            }

            if (!anyBacktrack) return true;

            if (exec.lookaheadResolverEntries.size() > std::numeric_limits<uint32_t>::max() ||
                exec.lookaheadThenBacktrackDispatches.size() >= uint64_t(kOpenTypeChainExecInvalid) ||
                exec.resolvers.size() >= uint64_t(kOpenTypeChainExecInvalid)) return false;

            OpenTypeChainExecResolverId defaultResolver = kOpenTypeChainExecInvalid;
            if (!defaults.empty())
            {
                bool usedBacktrack = false;
                if (!appendBacktrackOrDirectRuleResolver(
                    ir, defaults, exec, defaultResolver, usedBacktrack)) return false;
            }

            const uint32_t entryOffset = static_cast<uint32_t>(exec.lookaheadResolverEntries.size());

            for (const PlannedGroup& group : groups)
            {
                OpenTypeChainExecResolverId childResolver = kOpenTypeChainExecInvalid;
                bool usedBacktrack = false;
                if (!appendBacktrackOrDirectRuleResolver(
                    ir, group.rules, exec, childResolver, usedBacktrack)) return false;

                exec.lookaheadResolverEntries.push_back({group.glyph, 0, childResolver});
            }

            const uint32_t dispatchId = static_cast<uint32_t>(exec.lookaheadThenBacktrackDispatches.size());
            exec.lookaheadThenBacktrackDispatches.push_back({
                entryOffset, static_cast<uint32_t>(groups.size()), defaultResolver
            });

            result = static_cast<OpenTypeChainExecResolverId>(exec.resolvers.size());
            exec.resolvers.push_back({
                OpenTypeChainExecResolverKind::LookaheadThenBacktrackDispatch, 0, 0,
                dispatchId, 1
            });
            used = true;
            return true;
        }

        static inline bool appendResolver(const OpenTypeShapingIR& ir,
            const OpenTypeShapingRuleTrie& trie, const OpenTypeShapingRuleTrieNode& node,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecResolverId& result)
        {
            bool used = false;

            if (!appendBacktrackDispatchResolver(ir, trie, node, exec, result, used)) return false;
            if (used) return true;

            if (!appendLookaheadThenBacktrackTrieResolver(ir, trie, node, exec, result, used)) return false;
            if (used) return true;

            if (!appendLookaheadThenBacktrackDispatchResolver(ir, trie, node, exec, result, used)) return false;
            if (used) return true;

            if (!appendLookaheadDispatchResolver(ir, trie, node, exec, result, used)) return false;
            if (used) return true;

            return appendDirectResolver(trie, node, exec, result);
        }

        static inline bool appendMachineSubtable(const OpenTypeShapingIR& ir,
            uint32_t semanticSubtable, const OpenTypeShapingIRGsubChainContextSubtable& subtable,
            OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecSubtable& result)
        {
            OpenTypeShapingRuleTrie trie;
            OpenTypeShapingRuleTrieAnalysis analysis;

            if (!compileOpenTypeShapingRuleTrieChainContextSubtable(ir, subtable, trie, analysis)) return false;

            result = {};
            result.semanticSubtable = semanticSubtable;

            if (analysis.siblingOverlapPairs != 0)
            {
                result.flags = OpenTypeChainExecSubtableSemanticFallback;
                return true;
            }

            if (trie.nodes.empty() || exec.inputStates.size() > std::numeric_limits<uint32_t>::max() ||
                exec.predicates.size() > std::numeric_limits<uint32_t>::max() ||
                exec.predicateRanges.size() > std::numeric_limits<uint32_t>::max()) return false;

            const uint32_t predicateBase = static_cast<uint32_t>(exec.predicates.size());
            const uint32_t stateBase = static_cast<uint32_t>(exec.inputStates.size());
            const uint32_t rangeBase = static_cast<uint32_t>(exec.predicateRanges.size());

            exec.predicateRanges.insert(exec.predicateRanges.end(), trie.predicateRanges.begin(), trie.predicateRanges.end());
            exec.predicates.reserve(exec.predicates.size() + trie.predicates.size());

            for (const auto& predicate : trie.predicates)
            {
                if (uint64_t(rangeBase) + predicate.rangeOffset > std::numeric_limits<uint32_t>::max()) return false;
                exec.predicates.push_back({rangeBase + predicate.rangeOffset, predicate.rangeCount});
            }

            exec.inputStates.reserve(exec.inputStates.size() + trie.nodes.size());

            for (const auto& node : trie.nodes)
            {
                if (exec.transitions.size() > std::numeric_limits<uint32_t>::max() ||
                    node.edgeCount > std::numeric_limits<uint16_t>::max()) return false;

                OpenTypeChainExecResolverId resolver = kOpenTypeChainExecInvalid;
                if (!appendResolver(ir, trie, node, exec, resolver)) return false;

                OpenTypeChainExecInputState state;
                state.edgeOffset = static_cast<uint32_t>(exec.transitions.size());
                state.edgeCount = static_cast<uint16_t>(node.edgeCount);
                state.resolver = resolver;

                for (uint32_t i = 0; i < node.edgeCount; ++i)
                {
                    const auto& edge = trie.edges[node.edgeOffset + i];
                    if (uint64_t(predicateBase) + edge.predicate > std::numeric_limits<uint32_t>::max() ||
                        uint64_t(stateBase) + edge.child > std::numeric_limits<uint32_t>::max()) return false;
                    exec.transitions.push_back({predicateBase + edge.predicate, stateBase + edge.child});
                }

                exec.inputStates.push_back(state);
            }

            result.rootState = stateBase;
            return true;
        }
    }

    static inline bool compileOpenTypeChainContextExecutionLookup(const OpenTypeShapingIR& ir,
        OpenTypeShapingIRLookupId semanticLookup, OpenTypeChainContextExecutionIR& exec)
    {
        using namespace OpenTypeChainContextExecutionCompilerDetail;

        const OpenTypeShapingIRLookup* lookup = ir.lookup(semanticLookup);
        if (!lookup || lookup->op != OpenTypeShapingIROp::GsubChainContext ||
            uint64_t(lookup->payloadOffset) + lookup->payloadCount > ir.gsubChainContextSubtables.size() ||
            lookup->payloadCount == 0) return false;

        if (exec.lookupMap.size() < ir.lookups.size()) exec.lookupMap.resize(ir.lookups.size(), kOpenTypeChainExecInvalid);
        if (exec.lookupMap[semanticLookup] != kOpenTypeChainExecInvalid) return true;
        if (exec.lookups.size() >= uint64_t(kOpenTypeChainExecInvalid) ||
            exec.subtables.size() > std::numeric_limits<uint32_t>::max()) return false;

        const OpenTypeChainExecLookupId execLookupId = static_cast<OpenTypeChainExecLookupId>(exec.lookups.size());
        const uint32_t subtableOffset = static_cast<uint32_t>(exec.subtables.size());

        for (uint32_t i = 0; i < lookup->payloadCount; ++i)
        {
            const uint32_t semanticSubtable = lookup->payloadOffset + i;
            OpenTypeChainExecSubtable compiled;
            if (!appendMachineSubtable(ir, semanticSubtable,
                ir.gsubChainContextSubtables[semanticSubtable], exec, compiled)) return false;
            exec.subtables.push_back(compiled);
        }

        exec.lookups.push_back({semanticLookup, subtableOffset, lookup->payloadCount});
        exec.lookupMap[semanticLookup] = execLookupId;
        return true;
    }

    static inline bool compileOpenTypeChainContextExecutionIR(const OpenTypeShapingIR& ir,
        OpenTypeChainContextExecutionIR& exec)
    {
        exec.clear();
        if (!OpenTypeChainContextExecutionCompilerDetail::compileExecutionRules(ir, exec)) return false;
        exec.lookupMap.resize(ir.lookups.size(), kOpenTypeChainExecInvalid);

        for (OpenTypeShapingIRLookupId lookupId = 0; lookupId < ir.lookups.size(); ++lookupId)
        {
            if (ir.lookups[lookupId].op != OpenTypeShapingIROp::GsubChainContext) continue;
            if (!compileOpenTypeChainContextExecutionLookup(ir, lookupId, exec)) return false;
        }

        return true;
    }
}
