// opentype_chain_context_execution_ir.h
#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "opentype_shaping_ir.h"

namespace waavs
{
    using OpenTypeChainExecStateId = uint32_t;
    using OpenTypeChainExecPredicateId = uint32_t;
    using OpenTypeChainExecResolverId = uint32_t;
    using OpenTypeChainExecLookupId = uint32_t;

    inline constexpr uint32_t kOpenTypeChainExecInvalid = std::numeric_limits<uint32_t>::max();

    enum class OpenTypeChainExecResolverKind : uint8_t
    {
        Direct = 0,
        BacktrackDispatch,
        BacktrackTrie,
        LookaheadDispatch,
        LookaheadThenBacktrackDispatch,
        LookaheadThenBacktrackTrie
    };

    enum OpenTypeChainExecSubtableFlags : uint16_t
    {
        OpenTypeChainExecSubtableNone = 0,
        OpenTypeChainExecSubtableSemanticFallback = 1u << 0
    };

    struct OpenTypeChainExecPredicate
    {
        uint32_t rangeOffset{0};
        uint32_t rangeCount{0};
    };

    struct OpenTypeChainExecTransition
    {
        OpenTypeChainExecPredicateId predicate{kOpenTypeChainExecInvalid};
        OpenTypeChainExecStateId nextState{kOpenTypeChainExecInvalid};
    };

    struct OpenTypeChainExecInputState
    {
        uint32_t edgeOffset{0};
        uint16_t edgeCount{0};
        uint16_t reserved{0};
        OpenTypeChainExecResolverId resolver{kOpenTypeChainExecInvalid};
    };

    struct OpenTypeChainExecConstraintResolver
    {
        OpenTypeChainExecResolverKind kind{OpenTypeChainExecResolverKind::Direct};
        uint8_t reserved0{0};
        uint16_t reserved1{0};
        uint32_t payloadOffset{0};
        uint32_t payloadCount{0};
    };

    struct OpenTypeChainExecRule
    {
        uint32_t backtrackPredicateOffset{0};
        uint32_t backtrackCount{0};
        uint32_t inputCount{0};
        uint32_t lookaheadPredicateOffset{0};
        uint32_t lookaheadCount{0};
        uint32_t lookupOffset{0};
        uint32_t lookupCount{0};
    };

    struct OpenTypeChainExecBacktrackDispatchEntry
    {
        uint16_t glyph{0};
        uint16_t reserved{0};
        uint32_t ruleOffset{0};
        uint32_t ruleCount{0};
    };

    struct OpenTypeChainExecBacktrackTrieEdge
    {
        OpenTypeChainExecPredicateId predicate{kOpenTypeChainExecInvalid};
        OpenTypeChainExecStateId nextState{kOpenTypeChainExecInvalid};
    };

    struct OpenTypeChainExecBacktrackTrieState
    {
        uint32_t edgeOffset{0};
        uint16_t edgeCount{0};
        uint16_t reserved{0};
        uint32_t acceptOffset{0};
        uint32_t acceptCount{0};
    };

    struct OpenTypeChainExecLookaheadDispatchEntry
    {
        uint16_t glyph{0};
        uint16_t reserved{0};
        uint32_t ruleOffset{0};
        uint32_t ruleCount{0};
    };

    struct OpenTypeChainExecLookaheadDispatch
    {
        uint32_t entryOffset{0};
        uint32_t entryCount{0};
        uint32_t defaultRuleOffset{0};
        uint32_t defaultRuleCount{0};
    };

    struct OpenTypeChainExecLookaheadResolverEntry
    {
        uint16_t glyph{0};
        uint16_t reserved{0};
        OpenTypeChainExecResolverId resolver{kOpenTypeChainExecInvalid};
    };

    struct OpenTypeChainExecLookaheadThenBacktrackDispatch
    {
        uint32_t entryOffset{0};
        uint32_t entryCount{0};
        OpenTypeChainExecResolverId defaultResolver{kOpenTypeChainExecInvalid};
    };

    struct OpenTypeChainExecLookaheadThenBacktrackTrie
    {
        uint32_t entryOffset{0};
        uint32_t entryCount{0};
        OpenTypeChainExecResolverId defaultResolver{kOpenTypeChainExecInvalid};
    };

    struct OpenTypeChainExecSubtable
    {
        OpenTypeChainExecStateId rootState{kOpenTypeChainExecInvalid};
        uint32_t semanticSubtable{0};
        uint16_t flags{OpenTypeChainExecSubtableNone};
        uint16_t reserved{0};
    };

    struct OpenTypeChainExecLookup
    {
        OpenTypeShapingIRLookupId semanticLookup{kOpenTypeShapingIRInvalid};
        uint32_t subtableOffset{0};
        uint32_t subtableCount{0};
    };

    struct OpenTypeChainContextExecutionIR
    {
        std::vector<OpenTypeShapingIRGlyphRange> predicateRanges{};
        std::vector<OpenTypeChainExecPredicate> predicates{};
        std::vector<OpenTypeChainExecTransition> transitions{};
        std::vector<OpenTypeChainExecInputState> inputStates{};
        std::vector<OpenTypeChainExecConstraintResolver> resolvers{};
        std::vector<OpenTypeChainExecPredicateId> rulePredicates{};
        std::vector<OpenTypeChainExecRule> rules{};
        std::vector<OpenTypeChainExecBacktrackDispatchEntry> backtrackDispatchEntries{};
        std::vector<OpenTypeChainExecBacktrackTrieEdge> backtrackTrieEdges{};
        std::vector<OpenTypeChainExecBacktrackTrieState> backtrackTrieStates{};
        std::vector<OpenTypeChainExecLookaheadDispatchEntry> lookaheadDispatchEntries{};
        std::vector<OpenTypeChainExecLookaheadDispatch> lookaheadDispatches{};
        std::vector<OpenTypeChainExecLookaheadResolverEntry> lookaheadResolverEntries{};
        std::vector<OpenTypeChainExecLookaheadThenBacktrackDispatch> lookaheadThenBacktrackDispatches{};
        std::vector<OpenTypeChainExecLookaheadThenBacktrackTrie> lookaheadThenBacktrackTries{};
        std::vector<uint32_t> ruleIds{};
        std::vector<OpenTypeChainExecSubtable> subtables{};
        std::vector<OpenTypeChainExecLookup> lookups{};
        std::vector<OpenTypeChainExecLookupId> lookupMap{};

        void clear()
        {
            predicateRanges.clear();
            predicates.clear();
            transitions.clear();
            inputStates.clear();
            resolvers.clear();
            rulePredicates.clear();
            rules.clear();
            backtrackDispatchEntries.clear();
            backtrackTrieEdges.clear();
            backtrackTrieStates.clear();
            lookaheadDispatchEntries.clear();
            lookaheadDispatches.clear();
            lookaheadResolverEntries.clear();
            lookaheadThenBacktrackDispatches.clear();
            lookaheadThenBacktrackTries.clear();
            ruleIds.clear();
            subtables.clear();
            lookups.clear();
            lookupMap.clear();
        }

        [[nodiscard]] const OpenTypeChainExecLookup* lookup(OpenTypeShapingIRLookupId semanticLookup) const noexcept
        {
            if (semanticLookup >= lookupMap.size()) return nullptr;
            const OpenTypeChainExecLookupId id = lookupMap[semanticLookup];
            return id < lookups.size() ? &lookups[id] : nullptr;
        }
    };
}
