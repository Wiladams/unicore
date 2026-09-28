// opentype_chain_context_execution_executor.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_chain_context_execution_ir.h"
#include "opentype_chain_context_match.h"

namespace waavs
{
    struct OpenTypeChainExecSelectionStats
    {
        uint32_t statesVisited{0};
        uint32_t transitionsTested{0};
        uint32_t acceptsVisited{0};
        uint32_t candidatesCollected{0};
        uint32_t candidatesTested{0};
        uint32_t compiledRuleTests{0};
        uint32_t compiledBacktrackPositionTests{0};
        uint32_t compiledLookaheadPositionTests{0};
        uint32_t provenBacktrackPositionsSkipped{0};
        uint32_t provenLookaheadPositionsSkipped{0};
        uint32_t semanticFallbackCalls{0};
        uint32_t backtrackDispatches{0};
        uint32_t backtrackSearches{0};
        uint32_t backtrackDispatchProbes{0};
        uint32_t backtrackDispatchHits{0};
        uint32_t backtrackTries{0};
        uint32_t backtrackTrieSearches{0};
        uint32_t backtrackTrieStatesVisited{0};
        uint32_t backtrackTrieEdgesTested{0};
        uint32_t backtrackTrieAccepts{0};
        uint32_t lookaheadDispatches{0};
        uint32_t lookaheadSearches{0};
        uint32_t lookaheadDispatchProbes{0};
        uint32_t lookaheadDispatchHits{0};
        uint32_t lookaheadDefaultHits{0};
        uint32_t lookaheadThenBacktrackDispatches{0};
        uint32_t lookaheadThenBacktrackTries{0};
        uint32_t combinedLookaheadSearches{0};
        uint32_t combinedLookaheadDispatchProbes{0};
        uint32_t combinedLookaheadDispatchHits{0};
        uint32_t combinedLookaheadDefaultHits{0};
    };

    struct OpenTypeChainExecCandidate
    {
        uint32_t ruleId{kOpenTypeChainExecInvalid};
        uint32_t provenBacktrackCount{0};
        uint32_t provenLookaheadCount{0};
        size_t backtrackPosition{0};
        size_t lookaheadPosition{0};
    };


    [[nodiscard]] static inline bool openTypeChainExecPredicateContains(
        const OpenTypeChainContextExecutionIR& exec, OpenTypeChainExecPredicateId predicateId,
        uint16_t glyph) noexcept
    {
        if (predicateId >= exec.predicates.size()) return false;
        const auto& predicate = exec.predicates[predicateId];
        if (uint64_t(predicate.rangeOffset) + predicate.rangeCount > exec.predicateRanges.size()) return false;

        uint32_t lo = 0;
        uint32_t hi = predicate.rangeCount;
        while (lo < hi)
        {
            const uint32_t mid = lo + ((hi - lo) >> 1);
            const auto& range = exec.predicateRanges[predicate.rangeOffset + mid];
            if (glyph < range.first) hi = mid;
            else if (glyph > range.last) lo = mid + 1;
            else return true;
        }
        return false;
    }

    static inline bool appendOpenTypeChainExecRuleSlice(
        const OpenTypeChainContextExecutionIR& exec, uint32_t ruleOffset, uint32_t ruleCount,
        std::vector<OpenTypeChainExecCandidate>& candidates,
        uint32_t provenBacktrackCount = 0, size_t backtrackPosition = 0,
        uint32_t provenLookaheadCount = 0, size_t lookaheadPosition = 0)
    {
        if (uint64_t(ruleOffset) + ruleCount > exec.ruleIds.size()) return false;

        candidates.reserve(candidates.size() + ruleCount);

        for (uint32_t i = 0; i < ruleCount; ++i)
        {
            const uint32_t ruleId = exec.ruleIds[ruleOffset + i];
            if (ruleId >= exec.rules.size()) return false;

            OpenTypeChainExecCandidate candidate;
            candidate.ruleId = ruleId;
            candidate.provenBacktrackCount =
                std::min(provenBacktrackCount, exec.rules[ruleId].backtrackCount);
            candidate.provenLookaheadCount =
                std::min(provenLookaheadCount, exec.rules[ruleId].lookaheadCount);
            candidate.backtrackPosition = backtrackPosition;
            candidate.lookaheadPosition = lookaheadPosition;
            candidates.push_back(candidate);
        }

        return true;
    }

    static inline void annotateOpenTypeChainExecLookaheadProof(
        const OpenTypeChainContextExecutionIR& exec,
        std::vector<OpenTypeChainExecCandidate>& candidates, size_t begin,
        size_t lookaheadPosition)
    {
        for (size_t i = begin; i < candidates.size(); ++i)
        {
            auto& candidate = candidates[i];
            if (candidate.ruleId >= exec.rules.size()) continue;
            if (exec.rules[candidate.ruleId].lookaheadCount == 0) continue;

            if (candidate.provenLookaheadCount < 1)
            {
                candidate.provenLookaheadCount = 1;
                candidate.lookaheadPosition = lookaheadPosition;
            }
        }
    }

    static inline bool appendOpenTypeChainExecResolverCandidates(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeChainContextExecutionIR& exec, const OpenTypeChainExecConstraintResolver& resolver,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex, size_t inputEndPosition,
        std::vector<OpenTypeChainExecCandidate>& candidates, OpenTypeChainExecSelectionStats* stats = nullptr)
    {
        switch (resolver.kind)
        {
        case OpenTypeChainExecResolverKind::Direct:
        {
            return appendOpenTypeChainExecRuleSlice(
                exec, resolver.payloadOffset, resolver.payloadCount, candidates);
        }

        case OpenTypeChainExecResolverKind::BacktrackDispatch:
        {
            if (uint64_t(resolver.payloadOffset) + resolver.payloadCount > exec.backtrackDispatchEntries.size())
                return false;
            if (stats) ++stats->backtrackDispatches;

            size_t previousPosition = 0;
            if (stats) ++stats->backtrackSearches;
            const auto search = openTypeShapingIRLookupPrevious(ir, filter, buffer, glyphIndex, previousPosition);
            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid) return false;
            if (search == OpenTypeShapingIRGlyphSearchResult::End) return true;

            const uint32_t glyphId = buffer[previousPosition].glyphId;
            if (glyphId > 0xFFFFu) return false;
            const uint16_t glyph = static_cast<uint16_t>(glyphId);

            uint32_t lo = 0;
            uint32_t hi = resolver.payloadCount;

            while (lo < hi)
            {
                if (stats) ++stats->backtrackDispatchProbes;
                const uint32_t mid = lo + ((hi - lo) >> 1);
                const auto& entry = exec.backtrackDispatchEntries[resolver.payloadOffset + mid];

                if (glyph < entry.glyph)
                {
                    hi = mid;
                }
                else if (glyph > entry.glyph)
                {
                    lo = mid + 1;
                }
                else
                {
                    if (!appendOpenTypeChainExecRuleSlice(
                        exec, entry.ruleOffset, entry.ruleCount, candidates,
                        1, previousPosition)) return false;
                    if (stats) ++stats->backtrackDispatchHits;
                    return true;
                }
            }

            return true;
        }

        case OpenTypeChainExecResolverKind::BacktrackTrie:
        {
            if (resolver.payloadCount == 0 ||
                uint64_t(resolver.payloadOffset) + resolver.payloadCount > exec.backtrackTrieStates.size())
                return false;

            if (stats) ++stats->backtrackTries;

            const uint32_t firstState = resolver.payloadOffset;
            const uint32_t stateEnd = resolver.payloadOffset + resolver.payloadCount;
            std::vector<uint32_t> active{firstState};
            std::vector<uint32_t> next;

            const auto appendAccepts = [&](uint32_t stateId, uint32_t depth, size_t provenPosition) -> bool
            {
                if (stateId < firstState || stateId >= stateEnd) return false;
                const auto& state = exec.backtrackTrieStates[stateId];

                if (!appendOpenTypeChainExecRuleSlice(
                    exec, state.acceptOffset, state.acceptCount, candidates,
                    depth, provenPosition)) return false;

                if (stats) stats->backtrackTrieAccepts += state.acceptCount;
                return true;
            };

            if (!appendAccepts(firstState, 0, glyphIndex)) return false;

            size_t position = glyphIndex;
            uint32_t depth = 0;
            while (!active.empty())
            {
                size_t previousPosition = 0;
                if (stats) ++stats->backtrackTrieSearches;
                const auto search = openTypeShapingIRLookupPrevious(
                    ir, filter, buffer, position, previousPosition, nullptr);

                if (search == OpenTypeShapingIRGlyphSearchResult::Invalid) return false;
                if (search == OpenTypeShapingIRGlyphSearchResult::End) break;

                const uint32_t glyphId = buffer[previousPosition].glyphId;
                if (glyphId > 0xFFFFu) return false;
                const uint16_t glyph = static_cast<uint16_t>(glyphId);

                next.clear();

                for (uint32_t stateId : active)
                {
                    if (stateId < firstState || stateId >= stateEnd) return false;
                    const auto& state = exec.backtrackTrieStates[stateId];
                    if (uint64_t(state.edgeOffset) + state.edgeCount > exec.backtrackTrieEdges.size()) return false;
                    if (stats) ++stats->backtrackTrieStatesVisited;

                    for (uint32_t i = 0; i < state.edgeCount; ++i)
                    {
                        const auto& edge = exec.backtrackTrieEdges[state.edgeOffset + i];
                        if (stats) ++stats->backtrackTrieEdgesTested;
                        if (!openTypeChainExecPredicateContains(exec, edge.predicate, glyph)) continue;
                        if (edge.nextState < firstState || edge.nextState >= stateEnd) return false;
                        next.push_back(edge.nextState);
                    }
                }

                if (next.empty()) break;

                ++depth;

                for (uint32_t stateId : next)
                    if (!appendAccepts(stateId, depth, previousPosition)) return false;

                active.swap(next);
                position = previousPosition;
            }

            return true;
        }

        case OpenTypeChainExecResolverKind::LookaheadDispatch:
        {
            if (resolver.payloadCount != 1 || resolver.payloadOffset >= exec.lookaheadDispatches.size()) return false;

            const auto& dispatch = exec.lookaheadDispatches[resolver.payloadOffset];

            if (uint64_t(dispatch.entryOffset) + dispatch.entryCount > exec.lookaheadDispatchEntries.size() ||
                uint64_t(dispatch.defaultRuleOffset) + dispatch.defaultRuleCount > exec.ruleIds.size()) return false;

            if (stats) ++stats->lookaheadDispatches;

            size_t nextPosition = 0;
            if (stats) ++stats->lookaheadSearches;

            const auto search = openTypeShapingIRLookupNext(
                ir, filter, buffer, inputEndPosition, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid) return false;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                if (!appendOpenTypeChainExecRuleSlice(
                    exec, dispatch.defaultRuleOffset, dispatch.defaultRuleCount,
                    candidates)) return false;

                if (stats && dispatch.defaultRuleCount) ++stats->lookaheadDefaultHits;
                return true;
            }

            const uint32_t glyphId = buffer[nextPosition].glyphId;
            if (glyphId > 0xFFFFu) return false;
            const uint16_t glyph = static_cast<uint16_t>(glyphId);

            uint32_t lo = 0;
            uint32_t hi = dispatch.entryCount;

            while (lo < hi)
            {
                if (stats) ++stats->lookaheadDispatchProbes;

                const uint32_t mid = lo + ((hi - lo) >> 1);
                const auto& entry = exec.lookaheadDispatchEntries[dispatch.entryOffset + mid];

                if (glyph < entry.glyph)
                {
                    hi = mid;
                }
                else if (glyph > entry.glyph)
                {
                    lo = mid + 1;
                }
                else
                {
                    const size_t candidateBegin = candidates.size();

                    if (!appendOpenTypeChainExecRuleSlice(
                        exec, entry.ruleOffset, entry.ruleCount, candidates))
                        return false;

                    annotateOpenTypeChainExecLookaheadProof(
                        exec, candidates, candidateBegin, nextPosition);

                    if (stats) ++stats->lookaheadDispatchHits;
                    return true;
                }
            }

            if (!appendOpenTypeChainExecRuleSlice(
                exec, dispatch.defaultRuleOffset, dispatch.defaultRuleCount,
                candidates)) return false;

            if (stats && dispatch.defaultRuleCount) ++stats->lookaheadDefaultHits;
            return true;
        }

        case OpenTypeChainExecResolverKind::LookaheadThenBacktrackDispatch:
        {
            if (resolver.payloadCount != 1 ||
                resolver.payloadOffset >= exec.lookaheadThenBacktrackDispatches.size()) return false;

            const auto& dispatch = exec.lookaheadThenBacktrackDispatches[resolver.payloadOffset];
            if (uint64_t(dispatch.entryOffset) + dispatch.entryCount > exec.lookaheadResolverEntries.size())
                return false;

            if (stats) ++stats->lookaheadThenBacktrackDispatches;

            OpenTypeChainExecResolverId childResolver = dispatch.defaultResolver;
            bool lookaheadMatchedBucket = false;
            size_t nextPosition = 0;
            if (stats) ++stats->combinedLookaheadSearches;

            const auto search = openTypeShapingIRLookupNext(
                ir, filter, buffer, inputEndPosition, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid) return false;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                if (stats && childResolver != kOpenTypeChainExecInvalid)
                    ++stats->combinedLookaheadDefaultHits;
            }
            else
            {
                const uint32_t glyphId = buffer[nextPosition].glyphId;
                if (glyphId > 0xFFFFu) return false;
                const uint16_t glyph = static_cast<uint16_t>(glyphId);

                uint32_t lo = 0;
                uint32_t hi = dispatch.entryCount;

                while (lo < hi)
                {
                    if (stats) ++stats->combinedLookaheadDispatchProbes;
                    const uint32_t mid = lo + ((hi - lo) >> 1);
                    const auto& entry = exec.lookaheadResolverEntries[dispatch.entryOffset + mid];

                    if (glyph < entry.glyph) hi = mid;
                    else if (glyph > entry.glyph) lo = mid + 1;
                    else
                    {
                        childResolver = entry.resolver;
                        lookaheadMatchedBucket = true;
                        if (stats) ++stats->combinedLookaheadDispatchHits;
                        break;
                    }
                }

                if (lo == hi && (lo >= dispatch.entryCount ||
                    exec.lookaheadResolverEntries[dispatch.entryOffset + lo].glyph != glyph))
                {
                    childResolver = dispatch.defaultResolver;
                    if (stats && childResolver != kOpenTypeChainExecInvalid)
                        ++stats->combinedLookaheadDefaultHits;
                }
            }

            if (childResolver == kOpenTypeChainExecInvalid) return true;
            if (childResolver >= exec.resolvers.size()) return false;

            const auto& child = exec.resolvers[childResolver];
            if (child.kind != OpenTypeChainExecResolverKind::Direct &&
                child.kind != OpenTypeChainExecResolverKind::BacktrackDispatch) return false;

            const size_t candidateBegin = candidates.size();

            if (!appendOpenTypeChainExecResolverCandidates(
                ir, filter, exec, child, buffer, glyphIndex, inputEndPosition,
                candidates, stats)) return false;

            if (lookaheadMatchedBucket)
                annotateOpenTypeChainExecLookaheadProof(
                    exec, candidates, candidateBegin, nextPosition);

            return true;
        }

        case OpenTypeChainExecResolverKind::LookaheadThenBacktrackTrie:
        {
            if (resolver.payloadCount != 1 ||
                resolver.payloadOffset >= exec.lookaheadThenBacktrackTries.size()) return false;

            const auto& dispatch = exec.lookaheadThenBacktrackTries[resolver.payloadOffset];
            if (uint64_t(dispatch.entryOffset) + dispatch.entryCount > exec.lookaheadResolverEntries.size())
                return false;

            if (stats) ++stats->lookaheadThenBacktrackTries;

            OpenTypeChainExecResolverId childResolver = dispatch.defaultResolver;
            bool lookaheadMatchedBucket = false;
            size_t nextPosition = 0;
            if (stats) ++stats->combinedLookaheadSearches;

            const auto search = openTypeShapingIRLookupNext(
                ir, filter, buffer, inputEndPosition, nextPosition);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid) return false;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
            {
                if (stats && childResolver != kOpenTypeChainExecInvalid)
                    ++stats->combinedLookaheadDefaultHits;
            }
            else
            {
                const uint32_t glyphId = buffer[nextPosition].glyphId;
                if (glyphId > 0xFFFFu) return false;
                const uint16_t glyph = static_cast<uint16_t>(glyphId);

                uint32_t lo = 0;
                uint32_t hi = dispatch.entryCount;

                while (lo < hi)
                {
                    if (stats) ++stats->combinedLookaheadDispatchProbes;
                    const uint32_t mid = lo + ((hi - lo) >> 1);
                    const auto& entry = exec.lookaheadResolverEntries[dispatch.entryOffset + mid];

                    if (glyph < entry.glyph) hi = mid;
                    else if (glyph > entry.glyph) lo = mid + 1;
                    else
                    {
                        childResolver = entry.resolver;
                        lookaheadMatchedBucket = true;
                        if (stats) ++stats->combinedLookaheadDispatchHits;
                        break;
                    }
                }

                if (lo == hi && (lo >= dispatch.entryCount ||
                    exec.lookaheadResolverEntries[dispatch.entryOffset + lo].glyph != glyph))
                {
                    childResolver = dispatch.defaultResolver;
                    if (stats && childResolver != kOpenTypeChainExecInvalid)
                        ++stats->combinedLookaheadDefaultHits;
                }
            }

            if (childResolver == kOpenTypeChainExecInvalid) return true;
            if (childResolver >= exec.resolvers.size()) return false;

            const auto& child = exec.resolvers[childResolver];
            if (child.kind != OpenTypeChainExecResolverKind::Direct &&
                child.kind != OpenTypeChainExecResolverKind::BacktrackDispatch &&
                child.kind != OpenTypeChainExecResolverKind::BacktrackTrie) return false;

            const size_t candidateBegin = candidates.size();

            if (!appendOpenTypeChainExecResolverCandidates(
                ir, filter, exec, child, buffer, glyphIndex, inputEndPosition,
                candidates, stats)) return false;

            if (lookaheadMatchedBucket)
                annotateOpenTypeChainExecLookaheadProof(
                    exec, candidates, candidateBegin, nextPosition);

            return true;
        }

        default:
            return false;
        }
    }

    static inline bool collectOpenTypeChainExecInputCandidates(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeChainContextExecutionIR& exec, const OpenTypeChainExecSubtable& subtable,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        std::vector<OpenTypeChainExecCandidate>& candidates,
        std::vector<size_t>& inputPositions, OpenTypeChainExecSelectionStats* stats = nullptr)
    {
        candidates.clear();
        inputPositions.clear();
        if (subtable.flags & OpenTypeChainExecSubtableSemanticFallback) return true;
        if (subtable.rootState >= exec.inputStates.size() || glyphIndex >= buffer.size()) return false;

        OpenTypeChainExecStateId stateId = subtable.rootState;
        size_t position = glyphIndex;
        bool first = true;
        inputPositions.push_back(glyphIndex);

        for (;;)
        {
            if (stateId >= exec.inputStates.size()) return false;
            const auto& state = exec.inputStates[stateId];
            if (stats) ++stats->statesVisited;

            if (state.resolver != kOpenTypeChainExecInvalid)
            {
                if (state.resolver >= exec.resolvers.size()) return false;
                if (stats) ++stats->acceptsVisited;

                if (!appendOpenTypeChainExecResolverCandidates(
                    ir, filter, exec, exec.resolvers[state.resolver],
                    buffer, glyphIndex, position, candidates, stats)) return false;
            }

            uint32_t glyphId = 0;
            if (first)
            {
                glyphId = buffer[position].glyphId;
                first = false;
            }
            else
            {
                size_t nextPosition = 0;
                const auto search = openTypeShapingIRLookupNext(ir, filter, buffer, position, nextPosition, nullptr);
                if (search == OpenTypeShapingIRGlyphSearchResult::Invalid) return false;
                if (search == OpenTypeShapingIRGlyphSearchResult::End) break;
                position = nextPosition;
                inputPositions.push_back(position);
                glyphId = buffer[position].glyphId;
            }

            if (glyphId > 0xFFFFu) return false;

            OpenTypeChainExecStateId nextState = kOpenTypeChainExecInvalid;
            uint32_t matches = 0;

            if (uint64_t(state.edgeOffset) + state.edgeCount > exec.transitions.size()) return false;
            for (uint32_t i = 0; i < state.edgeCount; ++i)
            {
                const auto& edge = exec.transitions[state.edgeOffset + i];
                if (stats) ++stats->transitionsTested;
                if (!openTypeChainExecPredicateContains(exec, edge.predicate, static_cast<uint16_t>(glyphId))) continue;
                nextState = edge.nextState;
                ++matches;
            }

            if (matches == 0) break;
            if (matches != 1) return false;
            stateId = nextState;
        }

        if (!candidates.empty())
        {
            std::sort(candidates.begin(), candidates.end(),
                [](const OpenTypeChainExecCandidate& a, const OpenTypeChainExecCandidate& b)
                {
                    return a.ruleId < b.ruleId;
                });

            size_t write = 0;

            for (size_t read = 0; read < candidates.size(); ++read)
            {
                if (write == 0 || candidates[read].ruleId != candidates[write - 1].ruleId)
                {
                    candidates[write++] = candidates[read];
                    continue;
                }

                auto& destination = candidates[write - 1];
                const auto& source = candidates[read];

                if (source.provenBacktrackCount > destination.provenBacktrackCount)
                {
                    destination.provenBacktrackCount = source.provenBacktrackCount;
                    destination.backtrackPosition = source.backtrackPosition;
                }

                if (source.provenLookaheadCount > destination.provenLookaheadCount)
                {
                    destination.provenLookaheadCount = source.provenLookaheadCount;
                    destination.lookaheadPosition = source.lookaheadPosition;
                }
            }

            candidates.resize(write);
        }

        if (stats) stats->candidatesCollected += static_cast<uint32_t>(candidates.size());
        return true;
    }

    static inline OpenTypeShapingIRResult matchOpenTypeChainExecRule(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeChainContextExecutionIR& exec,
        const OpenTypeChainExecCandidate& candidate,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        const std::vector<size_t>& inputPositions, OpenTypeGsubIRChainContextMatch& match,
        OpenTypeChainExecSelectionStats* stats = nullptr)
    {
        match.clear();

        if (candidate.ruleId >= exec.rules.size()) return OpenTypeShapingIRResult::Invalid;
        const auto& rule = exec.rules[candidate.ruleId];

        if (candidate.provenBacktrackCount > rule.backtrackCount ||
            candidate.provenLookaheadCount > rule.lookaheadCount ||
            rule.inputCount == 0 || rule.inputCount > inputPositions.size() ||
            uint64_t(rule.backtrackPredicateOffset) + rule.backtrackCount > exec.rulePredicates.size() ||
            uint64_t(rule.lookaheadPredicateOffset) + rule.lookaheadCount > exec.rulePredicates.size())
            return OpenTypeShapingIRResult::Invalid;

        if (stats) ++stats->compiledRuleTests;

        if (stats)
        {
            stats->provenBacktrackPositionsSkipped += candidate.provenBacktrackCount;
            stats->provenLookaheadPositionsSkipped += candidate.provenLookaheadCount;
        }

        size_t position = candidate.provenBacktrackCount
            ? candidate.backtrackPosition
            : glyphIndex;

        for (uint32_t i = candidate.provenBacktrackCount; i < rule.backtrackCount; ++i)
        {
            if (stats) ++stats->compiledBacktrackPositionTests;

            size_t previousPosition = 0;
            const auto search = openTypeShapingIRLookupPrevious(
                ir, filter, buffer, position, previousPosition, nullptr);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
                return OpenTypeShapingIRResult::NoMatch;

            const uint32_t glyphId = buffer[previousPosition].glyphId;
            if (glyphId > 0xFFFFu) return OpenTypeShapingIRResult::Invalid;

            const OpenTypeChainExecPredicateId predicate =
                exec.rulePredicates[rule.backtrackPredicateOffset + i];

            if (!openTypeChainExecPredicateContains(
                exec, predicate, static_cast<uint16_t>(glyphId)))
                return OpenTypeShapingIRResult::NoMatch;

            position = previousPosition;
        }

        position = candidate.provenLookaheadCount
            ? candidate.lookaheadPosition
            : inputPositions[rule.inputCount - 1];

        for (uint32_t i = candidate.provenLookaheadCount; i < rule.lookaheadCount; ++i)
        {
            if (stats) ++stats->compiledLookaheadPositionTests;

            size_t nextPosition = 0;
            const auto search = openTypeShapingIRLookupNext(
                ir, filter, buffer, position, nextPosition, nullptr);

            if (search == OpenTypeShapingIRGlyphSearchResult::Invalid)
                return OpenTypeShapingIRResult::Invalid;

            if (search == OpenTypeShapingIRGlyphSearchResult::End)
                return OpenTypeShapingIRResult::NoMatch;

            const uint32_t glyphId = buffer[nextPosition].glyphId;
            if (glyphId > 0xFFFFu) return OpenTypeShapingIRResult::Invalid;

            const OpenTypeChainExecPredicateId predicate =
                exec.rulePredicates[rule.lookaheadPredicateOffset + i];

            if (!openTypeChainExecPredicateContains(
                exec, predicate, static_cast<uint16_t>(glyphId)))
                return OpenTypeShapingIRResult::NoMatch;

            position = nextPosition;
        }

        match.inputPositions.assign(
            inputPositions.begin(), inputPositions.begin() + rule.inputCount);
        match.lookupOffset = rule.lookupOffset;
        match.lookupCount = rule.lookupCount;

        return OpenTypeShapingIRResult::Match;
    }

    static inline OpenTypeShapingIRResult resolveOpenTypeChainExecSubtable(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeChainContextExecutionIR& exec, const OpenTypeChainExecSubtable& execSubtable,
        const OpenTypeShapingIRGsubChainContextSubtable& semanticSubtable,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubIRChainContextMatch& match, OpenTypeChainExecSelectionStats* stats = nullptr)
    {
        match.clear();

        if (execSubtable.flags & OpenTypeChainExecSubtableSemanticFallback)
        {
            if (stats) ++stats->semanticFallbackCalls;
            return resolveOpenTypeGsubIRChainContextSubtableUnchecked(
                ir, filter, semanticSubtable, buffer, glyphIndex, match, nullptr);
        }

        std::vector<OpenTypeChainExecCandidate> candidates;
        std::vector<size_t> inputPositions;

        if (!collectOpenTypeChainExecInputCandidates(
            ir, filter, exec, execSubtable, buffer, glyphIndex,
            candidates, inputPositions, stats))
            return OpenTypeShapingIRResult::Invalid;

        const uint64_t ruleEnd =
            uint64_t(semanticSubtable.ruleOffset) + semanticSubtable.ruleCount;

        for (const OpenTypeChainExecCandidate& candidate : candidates)
        {
            if (candidate.ruleId < semanticSubtable.ruleOffset ||
                candidate.ruleId >= ruleEnd ||
                candidate.ruleId >= exec.rules.size())
                return OpenTypeShapingIRResult::Invalid;

            if (stats) ++stats->candidatesTested;

            const auto result = matchOpenTypeChainExecRule(
                ir, filter, exec, candidate, buffer, glyphIndex,
                inputPositions, match, stats);

            if (result == OpenTypeShapingIRResult::Invalid ||
                result == OpenTypeShapingIRResult::Match)
                return result;
        }

        return OpenTypeShapingIRResult::NoMatch;
    }

    static inline OpenTypeShapingIRResult resolveOpenTypeChainExecLookup(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId semanticLookup,
        const OpenTypeChainContextExecutionIR& exec, const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubIRChainContextMatch& match,
        OpenTypeChainExecSelectionStats* stats = nullptr)
    {
        match.clear();
        const OpenTypeShapingIRLookup* lookup = ir.lookup(semanticLookup);
        const OpenTypeChainExecLookup* execLookup = exec.lookup(semanticLookup);
        if (!lookup || !execLookup || lookup->op != OpenTypeShapingIROp::GsubChainContext ||
            execLookup->subtableCount != lookup->payloadCount ||
            uint64_t(execLookup->subtableOffset) + execLookup->subtableCount > exec.subtables.size())
            return OpenTypeShapingIRResult::Invalid;

        for (uint32_t i = 0; i < execLookup->subtableCount; ++i)
        {
            const auto& execSubtable = exec.subtables[execLookup->subtableOffset + i];
            if (execSubtable.semanticSubtable >= ir.gsubChainContextSubtables.size())
                return OpenTypeShapingIRResult::Invalid;

            const auto result = resolveOpenTypeChainExecSubtable(ir, lookup->filter, exec,
                execSubtable, ir.gsubChainContextSubtables[execSubtable.semanticSubtable],
                buffer, glyphIndex, match, stats);
            if (result == OpenTypeShapingIRResult::Invalid || result == OpenTypeShapingIRResult::Match) return result;
        }

        return OpenTypeShapingIRResult::NoMatch;
    }

    // Compatibility alias for the Phase-A test name.
    static inline OpenTypeShapingIRResult resolveOpenTypeChainExecLookupDirect(
        const OpenTypeShapingIR& ir, OpenTypeShapingIRLookupId semanticLookup,
        const OpenTypeChainContextExecutionIR& exec, const OpenTypeShapingBuffer& buffer,
        size_t glyphIndex, OpenTypeGsubIRChainContextMatch& match,
        OpenTypeChainExecSelectionStats* stats = nullptr)
    {
        return resolveOpenTypeChainExecLookup(
            ir, semanticLookup, exec, buffer, glyphIndex, match, stats);
    }
}
