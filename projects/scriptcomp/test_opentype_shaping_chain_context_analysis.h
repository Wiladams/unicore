// test_opentype_shaping_chain_context_analysis.h
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <utility>

#include "test_core.h"
#include "test_opentype_shaping_workload_corpus.h"
#include "opentype_shaping_rule_trie.h"
#include "opentype_shaping_chain_context_analysis.h"

namespace waavs
{
    struct OpenTypeShapingChainContextSelection
    {
        OpenTypeShapingIRLookupId lookup{kOpenTypeShapingIRInvalid};
        uint32_t subtableIndex{0};
        uint32_t ruleCount{0};
    };

    static inline bool selectOpenTypeShapingChainContextSubtable(
        const OpenTypeShapingIR& ir,
        OpenTypeShapingChainContextSelection& result) noexcept
    {
        result = {};

        for (uint32_t lookupId = 0; lookupId < ir.lookups.size(); ++lookupId)
        {
            const auto& lookup = ir.lookups[lookupId];
            if (lookup.op != OpenTypeShapingIROp::GsubChainContext) continue;

            const uint64_t end = uint64_t(lookup.payloadOffset) + uint64_t(lookup.payloadCount);
            if (end > ir.gsubChainContextSubtables.size()) return false;

            for (uint32_t i = 0; i < lookup.payloadCount; ++i)
            {
                const auto& subtable = ir.gsubChainContextSubtables[lookup.payloadOffset + i];

                if (subtable.ruleCount > result.ruleCount)
                {
                    result.lookup = lookupId;
                    result.subtableIndex = i;
                    result.ruleCount = subtable.ruleCount;
                }
            }
        }

        return result.lookup != kOpenTypeShapingIRInvalid;
    }

    static inline void printOpenTypeShapingChainConstraintTrieMetrics(
        const char* prefix,
        const OpenTypeShapingChainConstraintTrieMetrics& metrics,
        FILE* out)
    {
        std::fprintf(out,
            "%s_sequences=%u | %s_predicates=%llu | %s_nodes=%u | %s_edges=%u | "
            "%s_shared=%llu | %s_shared_fraction=%.6f",
            prefix, metrics.sequenceCount,
            prefix, static_cast<unsigned long long>(metrics.sourcePredicatePositions),
            prefix, metrics.nodeCount,
            prefix, metrics.edgeCount,
            prefix, static_cast<unsigned long long>(metrics.sharedPrefixPositions),
            prefix, metrics.sharedPrefixFraction);
    }

    static inline void printOpenTypeShapingChainContextAnalysis(
        const char* face,
        const OpenTypeShapingChainContextSelection& selection,
        const OpenTypeShapingRuleTrieAnalysis& inputAnalysis,
        const OpenTypeShapingChainContextAnalysis& analysis,
        FILE* out = stdout)
    {
        std::fprintf(out,
            "\nCHAIN_CONSTRAINT_ANALYSIS_BEGIN face=%s | lookup=%u | subtable=%u\n",
            face ? face : "(unnamed)", selection.lookup, selection.subtableIndex);

        std::fprintf(out, "METRIC source_rules: %u\n", analysis.sourceRules);
        std::fprintf(out, "METRIC input_trie_nodes: %u\n", inputAnalysis.nodeCount);
        std::fprintf(out, "METRIC input_trie_edges: %u\n", inputAnalysis.edgeCount);
        std::fprintf(out, "METRIC input_shared_prefix_fraction: %.6f\n", inputAnalysis.sharedPrefixFraction);
        std::fprintf(out, "METRIC input_accept_states: %u\n", analysis.inputAcceptStates);
        std::fprintf(out, "METRIC min_candidate_rules_per_accept: %u\n", analysis.minCandidateRules);
        std::fprintf(out, "METRIC mean_candidate_rules_per_accept: %.3f\n", analysis.meanCandidateRules);
        std::fprintf(out, "METRIC max_candidate_rules_per_accept: %u\n", analysis.maxCandidateRules);

        std::fprintf(out, "METRIC total_backtrack_predicate_positions: %llu\n",
            static_cast<unsigned long long>(analysis.totalBacktrackPredicatePositions));
        std::fprintf(out, "METRIC total_lookahead_predicate_positions: %llu\n",
            static_cast<unsigned long long>(analysis.totalLookaheadPredicatePositions));

        std::fprintf(out, "METRIC total_distinct_backtrack_patterns: %u\n", analysis.totalDistinctBacktrackPatterns);
        std::fprintf(out, "METRIC total_distinct_lookahead_patterns: %u\n", analysis.totalDistinctLookaheadPatterns);
        std::fprintf(out, "METRIC total_distinct_constraint_pairs: %u\n", analysis.totalDistinctConstraintPairs);
        std::fprintf(out, "METRIC total_duplicate_constraint_rules: %u\n", analysis.totalDuplicateConstraintRules);

        std::fprintf(out, "METRIC total_backtrack_trie_edges: %llu\n",
            static_cast<unsigned long long>(analysis.totalBacktrackTrieEdges));
        std::fprintf(out, "METRIC total_lookahead_trie_edges: %llu\n",
            static_cast<unsigned long long>(analysis.totalLookaheadTrieEdges));

        const double backtrackShared = analysis.totalBacktrackPredicatePositions ?
            double(analysis.totalBacktrackSharedPrefixPositions) /
            double(analysis.totalBacktrackPredicatePositions) : 0.0;

        const double lookaheadShared = analysis.totalLookaheadPredicatePositions ?
            double(analysis.totalLookaheadSharedPrefixPositions) /
            double(analysis.totalLookaheadPredicatePositions) : 0.0;

        std::fprintf(out, "METRIC backtrack_shared_prefix_fraction: %.6f\n", backtrackShared);
        std::fprintf(out, "METRIC lookahead_shared_prefix_fraction: %.6f\n", lookaheadShared);
        std::fprintf(out, "METRIC max_distinct_backtrack_patterns: %u\n", analysis.maxDistinctBacktrackPatterns);
        std::fprintf(out, "METRIC max_distinct_lookahead_patterns: %u\n", analysis.maxDistinctLookaheadPatterns);
        std::fprintf(out, "METRIC max_distinct_constraint_pairs: %u\n", analysis.maxDistinctConstraintPairs);

        std::fprintf(out, "SECTION_BEGIN name=largest_accept_states\n");

        const size_t count = std::min<size_t>(analysis.acceptStates.size(), 16);

        for (size_t i = 0; i < count; ++i)
        {
            const auto& state = analysis.acceptStates[i];

            std::fprintf(out,
                "ACCEPT_STATE rank=%zu | input_node=%u | input_depth=%u | "
                "candidate_rules=%u | backtrack_patterns=%u | lookahead_patterns=%u | "
                "constraint_pairs=%u | duplicate_constraint_rules=%u | ",
                i + 1, state.inputNode, state.inputDepth, state.candidateRules,
                state.distinctBacktrackPatterns, state.distinctLookaheadPatterns,
                state.distinctConstraintPairs, state.duplicateConstraintRules);

            printOpenTypeShapingChainConstraintTrieMetrics("backtrack", state.backtrackTrie, out);
            std::fprintf(out, " | ");
            printOpenTypeShapingChainConstraintTrieMetrics("lookahead", state.lookaheadTrie, out);
            std::fprintf(out, "\n");
        }

        std::fprintf(out, "SECTION_END\n");
        std::fprintf(out, "CHAIN_CONSTRAINT_ANALYSIS_END\n");
    }

    static inline bool testOpenTypeShapingChainContextAnalysisFace(
        const FontFace& face, FILE* out = stdout)
    {
        OpenTypeShapingIR gsubIR;
        OpenTypeShapingIR gposIR;
        OpenTypeShapingWorkloadCorpusSummary compileSummary;

        if (!compileOpenTypeShapingWorkloadFace(face, gsubIR, gposIR, compileSummary)) return false;

        OpenTypeShapingChainContextSelection selection;
        if (!selectOpenTypeShapingChainContextSubtable(gsubIR, selection)) return false;

        const auto& lookup = gsubIR.lookups[selection.lookup];
        const auto& subtable = gsubIR.gsubChainContextSubtables[
            lookup.payloadOffset + selection.subtableIndex];

        OpenTypeShapingRuleTrie inputTrie;
        OpenTypeShapingRuleTrieAnalysis inputAnalysis;

        if (!compileOpenTypeShapingRuleTrieChainContextSubtable(
            gsubIR, subtable, inputTrie, inputAnalysis)) return false;

        OpenTypeShapingChainContextAnalysis analysis;

        if (!analyzeOpenTypeShapingChainContextConstraints(
            gsubIR, subtable, inputTrie, analysis)) return false;

        const char* fullName = face.fullName();
        const char* familyName = face.familyName();
        const char* label = fullName && *fullName ? fullName :
            familyName && *familyName ? familyName : "(unnamed)";

        printOpenTypeShapingChainContextAnalysis(
            label, selection, inputAnalysis, analysis, out);

        return true;
    }

    static inline bool testOpenTypeShapingChainContextAnalysisFile(
        const std::filesystem::path& filename, FILE* out = stdout)
    {
        FontResource resource;

        if (!readByteResource(filename, resource))
        {
            std::fprintf(out, "CHAIN_CONSTRAINT_FILE status=missing | file=%s\n",
                filename.string().c_str());
            return false;
        }

        OpenTypeContainer container(std::move(resource));

        if (!container.isValid())
        {
            std::fprintf(out, "CHAIN_CONSTRAINT_FILE status=invalid | file=%s\n",
                filename.string().c_str());
            return false;
        }

        bool passed = false;
        FontFaceView view;

        while (container(view))
        {
            FontFace face = parseFontFace(std::move(view));
            if (!face) continue;
            passed |= testOpenTypeShapingChainContextAnalysisFace(face, out);
        }

        return passed;
    }

    static inline bool testOpenTypeShapingChainContextAnalysisExperiment(
        const char* fontRoot, FILE* out = stdout)
    {
        if (!fontRoot || !*fontRoot) return false;

        const std::filesystem::path root(fontRoot);

        const std::filesystem::path dives =
            root / "ofl" / "notoserifdivesakuru" / "NotoSerifDivesAkuru-Regular.ttf";

        const std::filesystem::path nastaliq =
            root / "ofl" / "notonastaliqurdu" / "NotoNastaliqUrdu[wght].ttf";

        std::fprintf(out, "CHAIN_CONSTRAINT_EXPERIMENT_BEGIN\n");
        std::fprintf(out, "font_root: %s\n", root.string().c_str());

        const bool divesPassed = testOpenTypeShapingChainContextAnalysisFile(dives, out);
        const bool nastaliqPassed = testOpenTypeShapingChainContextAnalysisFile(nastaliq, out);

        std::fprintf(out, "CHAIN_CONSTRAINT_EXPERIMENT_END status=%s\n",
            divesPassed && nastaliqPassed ? "pass" : "fail");

        return divesPassed && nastaliqPassed;
    }
}
