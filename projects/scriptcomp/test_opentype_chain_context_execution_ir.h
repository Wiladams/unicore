// test_opentype_chain_context_execution_ir.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <utility>
#include <vector>

#include "test_core.h"
#include "test_opentype_shaping_workload_corpus.h"
#include "opentype_shaping_workloads.h"
#include "opentype_gsub_ir_executor.h"
#include "opentype_chain_context_execution_compiler.h"
#include "opentype_chain_context_execution_executor.h"

namespace waavs
{
    struct OpenTypeChainContextExecutionTestSummary
    {
        uint64_t workloadsSeen{0};
        uint64_t chainWorkloads{0};
        uint64_t compared{0};
        uint64_t mismatches{0};
        uint64_t invalid{0};
        uint64_t semanticFallbackSubtables{0};
        uint64_t machineSubtables{0};
        uint64_t directResolvers{0};
        uint64_t backtrackDispatchResolvers{0};
        uint64_t backtrackTrieResolvers{0};
        uint64_t lookaheadDispatchResolvers{0};
        uint64_t lookaheadThenBacktrackResolvers{0};
        uint64_t lookaheadThenBacktrackTrieResolvers{0};
        uint64_t statesVisited{0};
        uint64_t transitionsTested{0};
        uint64_t acceptsVisited{0};
        uint64_t candidatesCollected{0};
        uint64_t candidatesTested{0};
        uint64_t compiledRuleTests{0};
        uint64_t compiledBacktrackPositionTests{0};
        uint64_t compiledLookaheadPositionTests{0};
        uint64_t provenBacktrackPositionsSkipped{0};
        uint64_t provenLookaheadPositionsSkipped{0};
        uint64_t semanticFallbackCalls{0};
        uint64_t backtrackDispatches{0};
        uint64_t backtrackSearches{0};
        uint64_t backtrackDispatchProbes{0};
        uint64_t backtrackDispatchHits{0};
        uint64_t backtrackTries{0};
        uint64_t backtrackTrieSearches{0};
        uint64_t backtrackTrieStatesVisited{0};
        uint64_t backtrackTrieEdgesTested{0};
        uint64_t backtrackTrieAccepts{0};
        uint64_t lookaheadDispatches{0};
        uint64_t lookaheadSearches{0};
        uint64_t lookaheadDispatchProbes{0};
        uint64_t lookaheadDispatchHits{0};
        uint64_t lookaheadDefaultHits{0};
        uint64_t lookaheadThenBacktrackDispatches{0};
        uint64_t lookaheadThenBacktrackTries{0};
        uint64_t combinedLookaheadSearches{0};
        uint64_t combinedLookaheadDispatchProbes{0};
        uint64_t combinedLookaheadDispatchHits{0};
        uint64_t combinedLookaheadDefaultHits{0};
    };

    static inline OpenTypeShapingBuffer makeOpenTypeChainContextTestBuffer(
        const OpenTypeShapingWorkload& workload)
    {
        OpenTypeShapingBuffer buffer;
        for (size_t i = 0; i < workload.glyphs.size(); ++i)
        {
            OpenTypeShapingGlyph glyph{};
            glyph.glyphId = workload.glyphs[i];
            glyph.scalarOffset = static_cast<uint32_t>(i);
            glyph.scalarCount = 1;
            buffer.pushBack(glyph);
        }
        return buffer;
    }

    [[nodiscard]] static inline bool openTypeChainContextMatchesEquivalent(
        OpenTypeShapingIRResult aResult, const OpenTypeGsubIRChainContextMatch& a,
        OpenTypeShapingIRResult bResult, const OpenTypeGsubIRChainContextMatch& b) noexcept
    {
        if (aResult != bResult) return false;
        if (aResult != OpenTypeShapingIRResult::Match) return true;
        return a.inputPositions == b.inputPositions &&
            a.lookupOffset == b.lookupOffset &&
            a.lookupCount == b.lookupCount;
    }

    static inline bool testOpenTypeChainContextExecutionFace(
        const FontFace& face, FILE* out = stdout)
    {
        OpenTypeShapingIR gsubIR;
        OpenTypeShapingIR gposIR;
        OpenTypeShapingWorkloadCorpusSummary compileSummary;

        if (!compileOpenTypeShapingWorkloadFace(face, gsubIR, gposIR, compileSummary)) return false;

        OpenTypeChainContextExecutionIR exec;
        if (!compileOpenTypeChainContextExecutionIR(gsubIR, exec)) return false;

        OpenTypeChainContextExecutionTestSummary summary;
        for (const auto& subtable : exec.subtables)
        {
            if (subtable.flags & OpenTypeChainExecSubtableSemanticFallback) ++summary.semanticFallbackSubtables;
            else ++summary.machineSubtables;
        }

        for (const auto& resolver : exec.resolvers)
            if (resolver.kind == OpenTypeChainExecResolverKind::BacktrackTrie) ++summary.backtrackTrieResolvers;

        for (const auto& state : exec.inputStates)
        {
            if (state.resolver == kOpenTypeChainExecInvalid || state.resolver >= exec.resolvers.size()) continue;
            switch (exec.resolvers[state.resolver].kind)
            {
            case OpenTypeChainExecResolverKind::Direct:
                ++summary.directResolvers;
                break;
            case OpenTypeChainExecResolverKind::BacktrackDispatch:
                ++summary.backtrackDispatchResolvers;
                break;
            case OpenTypeChainExecResolverKind::LookaheadDispatch:
                ++summary.lookaheadDispatchResolvers;
                break;
            case OpenTypeChainExecResolverKind::LookaheadThenBacktrackDispatch:
                ++summary.lookaheadThenBacktrackResolvers;
                break;
            case OpenTypeChainExecResolverKind::LookaheadThenBacktrackTrie:
                ++summary.lookaheadThenBacktrackTrieResolvers;
                break;
            default:
                break;
            }
        }

        std::vector<OpenTypeShapingWorkload> workloads;
        makeOpenTypeShapingWorkloads(gsubIR, workloads);
        summary.workloadsSeen = workloads.size();

        const char* fullName = face.fullName();
        const char* familyName = face.familyName();
        const char* label = fullName && *fullName ? fullName : familyName && *familyName ? familyName : "(unnamed)";

        std::fprintf(out, "CHAIN_EXEC_FACE_BEGIN name=%s\n", label);
        std::fprintf(out, "METRIC machine_subtables: %llu\n",
            static_cast<unsigned long long>(summary.machineSubtables));
        std::fprintf(out, "METRIC semantic_fallback_subtables: %llu\n",
            static_cast<unsigned long long>(summary.semanticFallbackSubtables));
        std::fprintf(out, "METRIC direct_resolvers: %llu\n",
            static_cast<unsigned long long>(summary.directResolvers));
        std::fprintf(out, "METRIC backtrack_dispatch_resolvers: %llu\n",
            static_cast<unsigned long long>(summary.backtrackDispatchResolvers));
        std::fprintf(out, "METRIC backtrack_trie_resolvers: %llu\n",
            static_cast<unsigned long long>(summary.backtrackTrieResolvers));
        std::fprintf(out, "METRIC lookahead_dispatch_resolvers: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadDispatchResolvers));
        std::fprintf(out, "METRIC lookahead_then_backtrack_resolvers: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadThenBacktrackResolvers));
        std::fprintf(out, "METRIC lookahead_then_backtrack_trie_resolvers: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadThenBacktrackTrieResolvers));

        for (const OpenTypeShapingWorkload& workload : workloads)
        {
            const OpenTypeShapingIRLookup* lookup = gsubIR.lookup(workload.lookup);
            if (!lookup || lookup->op != OpenTypeShapingIROp::GsubChainContext) continue;
            ++summary.chainWorkloads;

            if (workload.glyphs.empty() || workload.targetOffset >= workload.glyphs.size())
            {
                ++summary.invalid;
                continue;
            }

            const OpenTypeShapingBuffer buffer = makeOpenTypeChainContextTestBuffer(workload);
            OpenTypeGsubIRChainContextMatch semanticMatch;
            OpenTypeGsubIRChainContextMatch execMatch;

            const OpenTypeShapingIRResult semanticResult =
                resolveOpenTypeGsubIRChainContextLookup(gsubIR, *lookup, buffer,
                    workload.targetOffset, semanticMatch);

            OpenTypeChainExecSelectionStats stats;
            const OpenTypeShapingIRResult execResult =
                resolveOpenTypeChainExecLookup(gsubIR, workload.lookup, exec,
                    buffer, workload.targetOffset, execMatch, &stats);

            summary.statesVisited += stats.statesVisited;
            summary.transitionsTested += stats.transitionsTested;
            summary.acceptsVisited += stats.acceptsVisited;
            summary.candidatesCollected += stats.candidatesCollected;
            summary.candidatesTested += stats.candidatesTested;
            summary.compiledRuleTests += stats.compiledRuleTests;
            summary.compiledBacktrackPositionTests += stats.compiledBacktrackPositionTests;
            summary.compiledLookaheadPositionTests += stats.compiledLookaheadPositionTests;
            summary.provenBacktrackPositionsSkipped += stats.provenBacktrackPositionsSkipped;
            summary.provenLookaheadPositionsSkipped += stats.provenLookaheadPositionsSkipped;
            summary.semanticFallbackCalls += stats.semanticFallbackCalls;
            summary.backtrackDispatches += stats.backtrackDispatches;
            summary.backtrackSearches += stats.backtrackSearches;
            summary.backtrackDispatchProbes += stats.backtrackDispatchProbes;
            summary.backtrackDispatchHits += stats.backtrackDispatchHits;
            summary.backtrackTries += stats.backtrackTries;
            summary.backtrackTrieSearches += stats.backtrackTrieSearches;
            summary.backtrackTrieStatesVisited += stats.backtrackTrieStatesVisited;
            summary.backtrackTrieEdgesTested += stats.backtrackTrieEdgesTested;
            summary.backtrackTrieAccepts += stats.backtrackTrieAccepts;
            summary.lookaheadDispatches += stats.lookaheadDispatches;
            summary.lookaheadSearches += stats.lookaheadSearches;
            summary.lookaheadDispatchProbes += stats.lookaheadDispatchProbes;
            summary.lookaheadDispatchHits += stats.lookaheadDispatchHits;
            summary.lookaheadDefaultHits += stats.lookaheadDefaultHits;
            summary.lookaheadThenBacktrackDispatches += stats.lookaheadThenBacktrackDispatches;
            summary.lookaheadThenBacktrackTries += stats.lookaheadThenBacktrackTries;
            summary.combinedLookaheadSearches += stats.combinedLookaheadSearches;
            summary.combinedLookaheadDispatchProbes += stats.combinedLookaheadDispatchProbes;
            summary.combinedLookaheadDispatchHits += stats.combinedLookaheadDispatchHits;
            summary.combinedLookaheadDefaultHits += stats.combinedLookaheadDefaultHits;
            ++summary.compared;

            if (!openTypeChainContextMatchesEquivalent(
                semanticResult, semanticMatch, execResult, execMatch))
            {
                ++summary.mismatches;
                std::fprintf(out,
                    "MISMATCH workload=%s | lookup=%u | target=%u | semantic=%u | exec=%u | semantic_positions=%zu | exec_positions=%zu\n",
                    workload.name.c_str(), workload.lookup, workload.targetOffset,
                    static_cast<unsigned>(semanticResult), static_cast<unsigned>(execResult),
                    semanticMatch.inputPositions.size(), execMatch.inputPositions.size());
            }
        }

        std::fprintf(out, "METRIC chain_workloads: %llu\n",
            static_cast<unsigned long long>(summary.chainWorkloads));
        std::fprintf(out, "METRIC compared: %llu\n",
            static_cast<unsigned long long>(summary.compared));
        std::fprintf(out, "METRIC mismatches: %llu\n",
            static_cast<unsigned long long>(summary.mismatches));
        std::fprintf(out, "METRIC states_visited: %llu\n",
            static_cast<unsigned long long>(summary.statesVisited));
        std::fprintf(out, "METRIC transitions_tested: %llu\n",
            static_cast<unsigned long long>(summary.transitionsTested));
        std::fprintf(out, "METRIC accepts_visited: %llu\n",
            static_cast<unsigned long long>(summary.acceptsVisited));
        std::fprintf(out, "METRIC candidates_collected: %llu\n",
            static_cast<unsigned long long>(summary.candidatesCollected));
        std::fprintf(out, "METRIC candidates_tested: %llu\n",
            static_cast<unsigned long long>(summary.candidatesTested));
        std::fprintf(out, "METRIC compiled_rule_tests: %llu\n",
            static_cast<unsigned long long>(summary.compiledRuleTests));
        std::fprintf(out, "METRIC compiled_backtrack_position_tests: %llu\n",
            static_cast<unsigned long long>(summary.compiledBacktrackPositionTests));
        std::fprintf(out, "METRIC compiled_lookahead_position_tests: %llu\n",
            static_cast<unsigned long long>(summary.compiledLookaheadPositionTests));
        std::fprintf(out, "METRIC proven_backtrack_positions_skipped: %llu\n",
            static_cast<unsigned long long>(summary.provenBacktrackPositionsSkipped));
        std::fprintf(out, "METRIC proven_lookahead_positions_skipped: %llu\n",
            static_cast<unsigned long long>(summary.provenLookaheadPositionsSkipped));
        std::fprintf(out, "METRIC semantic_fallback_calls: %llu\n",
            static_cast<unsigned long long>(summary.semanticFallbackCalls));
        std::fprintf(out, "METRIC backtrack_dispatches: %llu\n",
            static_cast<unsigned long long>(summary.backtrackDispatches));
        std::fprintf(out, "METRIC backtrack_searches: %llu\n",
            static_cast<unsigned long long>(summary.backtrackSearches));
        std::fprintf(out, "METRIC backtrack_dispatch_probes: %llu\n",
            static_cast<unsigned long long>(summary.backtrackDispatchProbes));
        std::fprintf(out, "METRIC backtrack_dispatch_hits: %llu\n",
            static_cast<unsigned long long>(summary.backtrackDispatchHits));
        std::fprintf(out, "METRIC backtrack_tries: %llu\n",
            static_cast<unsigned long long>(summary.backtrackTries));
        std::fprintf(out, "METRIC backtrack_trie_searches: %llu\n",
            static_cast<unsigned long long>(summary.backtrackTrieSearches));
        std::fprintf(out, "METRIC backtrack_trie_states_visited: %llu\n",
            static_cast<unsigned long long>(summary.backtrackTrieStatesVisited));
        std::fprintf(out, "METRIC backtrack_trie_edges_tested: %llu\n",
            static_cast<unsigned long long>(summary.backtrackTrieEdgesTested));
        std::fprintf(out, "METRIC backtrack_trie_accepts: %llu\n",
            static_cast<unsigned long long>(summary.backtrackTrieAccepts));
        std::fprintf(out, "METRIC lookahead_dispatches: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadDispatches));
        std::fprintf(out, "METRIC lookahead_searches: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadSearches));
        std::fprintf(out, "METRIC lookahead_dispatch_probes: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadDispatchProbes));
        std::fprintf(out, "METRIC lookahead_dispatch_hits: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadDispatchHits));
        std::fprintf(out, "METRIC lookahead_default_hits: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadDefaultHits));
        std::fprintf(out, "METRIC lookahead_then_backtrack_dispatches: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadThenBacktrackDispatches));
        std::fprintf(out, "METRIC lookahead_then_backtrack_tries: %llu\n",
            static_cast<unsigned long long>(summary.lookaheadThenBacktrackTries));
        std::fprintf(out, "METRIC combined_lookahead_searches: %llu\n",
            static_cast<unsigned long long>(summary.combinedLookaheadSearches));
        std::fprintf(out, "METRIC combined_lookahead_dispatch_probes: %llu\n",
            static_cast<unsigned long long>(summary.combinedLookaheadDispatchProbes));
        std::fprintf(out, "METRIC combined_lookahead_dispatch_hits: %llu\n",
            static_cast<unsigned long long>(summary.combinedLookaheadDispatchHits));
        std::fprintf(out, "METRIC combined_lookahead_default_hits: %llu\n",
            static_cast<unsigned long long>(summary.combinedLookaheadDefaultHits));
        std::fprintf(out, "CHAIN_EXEC_FACE_END name=%s | status=%s\n",
            label, summary.mismatches == 0 && summary.invalid == 0 ? "pass" : "fail");

        return summary.mismatches == 0 && summary.invalid == 0;
    }

    static inline bool testOpenTypeChainContextExecutionFile(
        const std::filesystem::path& filename, FILE* out = stdout)
    {
        FontResource resource;
        if (!readByteResource(filename, resource)) return false;

        OpenTypeContainer container(std::move(resource));
        if (!container.isValid()) return false;

        bool passed = false;
        FontFaceView view;
        while (container(view))
        {
            FontFace face = parseFontFace(std::move(view));
            if (!face) continue;
            passed |= testOpenTypeChainContextExecutionFace(face, out);
        }
        return passed;
    }

    static inline bool testOpenTypeChainContextExecutionExperiment(
        const char* fontRoot, FILE* out = stdout)
    {
        if (!fontRoot || !*fontRoot) return false;
        const std::filesystem::path root(fontRoot);

        const std::filesystem::path dives = root / "ofl" / "notoserifdivesakuru" /
            "NotoSerifDivesAkuru-Regular.ttf";
        const std::filesystem::path nastaliq = root / "ofl" / "notonastaliqurdu" /
            "NotoNastaliqUrdu[wght].ttf";

        std::fprintf(out, "CHAIN_EXEC_EXPERIMENT_BEGIN\n");
        std::fprintf(out, "font_root: %s\n", root.string().c_str());

        const bool divesPassed = testOpenTypeChainContextExecutionFile(dives, out);
        const bool nastaliqPassed = testOpenTypeChainContextExecutionFile(nastaliq, out);

        std::fprintf(out, "CHAIN_EXEC_EXPERIMENT_END status=%s\n",
            divesPassed && nastaliqPassed ? "pass" : "fail");
        return divesPassed && nastaliqPassed;
    }
}
