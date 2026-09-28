// test_opentype_shaping_benchmark.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "test_core.h"
#include "opentype_shaping_benchmark.h"

namespace waavs
{
    struct OpenTypeShapingBenchmarkTestSummary
    {
        uint64_t benchmarkCount{0};
        uint64_t passed{0};
        uint64_t invalidWorkloads{0};
        uint64_t failed{0};

        OpenTypeShapingBenchmarkTestSummary& operator+=(const OpenTypeShapingBenchmarkTestSummary& other) noexcept
        {
            benchmarkCount += other.benchmarkCount;
            passed += other.passed;
            invalidWorkloads += other.invalidWorkloads;
            failed += other.failed;
            return *this;
        }
    };

    [[nodiscard]] static inline const char* openTypeShapingBenchmarkOpName(OpenTypeShapingIROp op) noexcept
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

    static inline void printOpenTypeShapingExecutionStats(FILE* out, const OpenTypeShapingExecutionStats& stats)
    {
        std::fprintf(out, "COUNTERS");
        std::fprintf(out, " glyph_visits=%llu", static_cast<unsigned long long>(stats.glyphVisits));
        std::fprintf(out, " lookup_attempts=%llu", static_cast<unsigned long long>(stats.lookupAttempts));
        std::fprintf(out, " lookup_matches=%llu", static_cast<unsigned long long>(stats.lookupMatches));
        std::fprintf(out, " subtable_attempts=%llu", static_cast<unsigned long long>(stats.subtableAttempts));
        std::fprintf(out, " glyph_set_tests=%llu", static_cast<unsigned long long>(stats.glyphSetTests));
        std::fprintf(out, " glyph_range_tests=%llu", static_cast<unsigned long long>(stats.glyphRangeTests));
        std::fprintf(out, " filter_tests=%llu", static_cast<unsigned long long>(stats.filterTests));
        std::fprintf(out, " filter_skips=%llu", static_cast<unsigned long long>(stats.filterSkips));
        std::fprintf(out, " context_rule_attempts=%llu", static_cast<unsigned long long>(stats.contextRuleAttempts));
        std::fprintf(out, " context_position_tests=%llu", static_cast<unsigned long long>(stats.contextPositionTests));
        std::fprintf(out, " context_matches=%llu", static_cast<unsigned long long>(stats.contextMatches));
        std::fprintf(out, " nested_lookup_calls=%llu", static_cast<unsigned long long>(stats.nestedLookupCalls));
        std::fprintf(out, " ligature_candidates=%llu", static_cast<unsigned long long>(stats.ligatureCandidates));
        std::fprintf(out, " ligature_component_tests=%llu", static_cast<unsigned long long>(stats.ligatureComponentTests));
        std::fprintf(out, " buffer_insertions=%llu", static_cast<unsigned long long>(stats.bufferInsertions));
        std::fprintf(out, " buffer_erasures=%llu", static_cast<unsigned long long>(stats.bufferErasures));
        std::fprintf(out, " glyphs_moved=%llu", static_cast<unsigned long long>(stats.glyphsMoved));
        std::fprintf(out, " attachment_edges=%llu", static_cast<unsigned long long>(stats.attachmentEdges));
        std::fprintf(out, " chain_exec_lookups=%llu", static_cast<unsigned long long>(stats.chainExecLookups));
        std::fprintf(out, " chain_exec_fallback_lookups=%llu", static_cast<unsigned long long>(stats.chainExecFallbackLookups));
        std::fprintf(out, " chain_exec_states_visited=%llu", static_cast<unsigned long long>(stats.chainExecStatesVisited));
        std::fprintf(out, " chain_exec_transitions_tested=%llu", static_cast<unsigned long long>(stats.chainExecTransitionsTested));
        std::fprintf(out, " chain_exec_accepts_visited=%llu", static_cast<unsigned long long>(stats.chainExecAcceptsVisited));
        std::fprintf(out, " chain_exec_candidates_collected=%llu", static_cast<unsigned long long>(stats.chainExecCandidatesCollected));
        std::fprintf(out, " chain_exec_candidates_tested=%llu", static_cast<unsigned long long>(stats.chainExecCandidatesTested));
        std::fprintf(out, " chain_exec_compiled_rule_tests=%llu", static_cast<unsigned long long>(stats.chainExecCompiledRuleTests));
        std::fprintf(out, " chain_exec_backtrack_position_tests=%llu", static_cast<unsigned long long>(stats.chainExecBacktrackPositionTests));
        std::fprintf(out, " chain_exec_lookahead_position_tests=%llu", static_cast<unsigned long long>(stats.chainExecLookaheadPositionTests));
        std::fprintf(out, " chain_exec_proven_backtrack_skipped=%llu", static_cast<unsigned long long>(stats.chainExecProvenBacktrackSkipped));
        std::fprintf(out, " chain_exec_proven_lookahead_skipped=%llu", static_cast<unsigned long long>(stats.chainExecProvenLookaheadSkipped));
        std::fprintf(out, " chain_exec_semantic_fallback_calls=%llu\n", static_cast<unsigned long long>(stats.chainExecSemanticFallbackCalls));
    }

    static inline void printOpenTypeShapingBenchmarkResult(FILE* out, const OpenTypeShapingBenchmarkResult& result)
    {
        std::fprintf(out,
            "BENCHMARK face=%s | workload=%s | lookup=%u | op=%s | structural_value=%u | input_glyphs=%u | target_offset=%u | expected_logical_length=%u | iterations=%llu | total_ns=%llu | ns_per_iteration=%.3f | ns_per_input_glyph=%.3f | status=%s | success=%u\n",
            result.face.c_str(),
            result.workload.c_str(),
            result.lookup,
            openTypeShapingBenchmarkOpName(result.op),
            result.structuralValue,
            result.inputGlyphs,
            result.targetOffset,
            result.expectedLogicalLength,
            static_cast<unsigned long long>(result.iterations),
            static_cast<unsigned long long>(result.totalNs),
            result.nsPerIteration,
            result.nsPerInputGlyph,
            openTypeShapingBenchmarkStatusName(result.status),
            result.executionSucceeded ? 1u : 0u);

        printOpenTypeShapingExecutionStats(out, result.stats);
    }

    template<class Runner>
    static inline bool testOpenTypeShapingBenchmark(const OpenTypeShapingIR& ir, const char* label, Runner&& runner,
        FILE* out = stdout, const OpenTypeShapingBenchmarkOptions& options = {},
        OpenTypeShapingBenchmarkTestSummary* summaryOut = nullptr)
    {
        std::vector<OpenTypeShapingBenchmarkResult> results;

        std::fprintf(out, "REPORT_BEGIN\n");
        std::fprintf(out, "report_type: OpenTypeShapingBenchmark\n");
        std::fprintf(out, "schema_version: 1\n");
        std::fprintf(out, "label: %s\n", label ? label : "");
        std::fprintf(out, "lookup_count: %zu\n", ir.lookups.size());
        std::fprintf(out, "selection: %s\n", openTypeShapingBenchmarkSelectionName(options.selection));
        std::fprintf(out, "warmup_iterations: %u\n", options.warmupIterations);
        std::fprintf(out, "timed_iterations: %u\n\n", options.iterations);

        const bool ok = benchmarkOpenTypeShapingIR(label, ir, std::forward<Runner>(runner), results, options);

        uint64_t passed = 0;
        uint64_t rejected = 0;
        uint64_t failed = 0;

        std::fprintf(out, "SECTION_BEGIN name=benchmarks\n");
        for (const auto& result : results)
        {
            printOpenTypeShapingBenchmarkResult(out, result);

            switch (result.status)
            {
            case OpenTypeShapingBenchmarkStatus::Success: ++passed; break;
            case OpenTypeShapingBenchmarkStatus::InvalidWorkload: ++rejected; break;
            case OpenTypeShapingBenchmarkStatus::ExecutionFailure: ++failed; break;
            }
        }
        std::fprintf(out, "SECTION_END\n\n");

        std::fprintf(out, "SECTION_BEGIN name=summary\n");
        std::fprintf(out, "METRIC benchmark_count: %zu\n", results.size());
        std::fprintf(out, "METRIC passed: %llu\n", static_cast<unsigned long long>(passed));
        std::fprintf(out, "METRIC invalid_workloads: %llu\n", static_cast<unsigned long long>(rejected));
        std::fprintf(out, "METRIC failed: %llu\n", static_cast<unsigned long long>(failed));
        std::fprintf(out, "SECTION_END\n");
        std::fprintf(out, "REPORT_END\n");

        if (summaryOut)
        {
            summaryOut->benchmarkCount += results.size();
            summaryOut->passed += passed;
            summaryOut->invalidWorkloads += rejected;
            summaryOut->failed += failed;
        }

        return ok && !results.empty() && failed == 0;
    }

    template<class GsubRunner, class GposRunner>
    static inline bool testOpenTypeShapingBenchmark(const OpenTypeShapingIR& gsubIR, const OpenTypeShapingIR& gposIR,
        const char* label, GsubRunner&& gsubRunner, GposRunner&& gposRunner, FILE* out = stdout,
        const OpenTypeShapingBenchmarkOptions& options = {},
        OpenTypeShapingBenchmarkTestSummary* summaryOut = nullptr)
    {
        bool any = false;
        bool ok = true;
        OpenTypeShapingBenchmarkTestSummary localSummary;

        if (!gsubIR.lookups.empty())
        {
            any = true;
            ok &= testOpenTypeShapingBenchmark(gsubIR, label, std::forward<GsubRunner>(gsubRunner), out, options, &localSummary);
        }

        if (!gposIR.lookups.empty())
        {
            any = true;
            ok &= testOpenTypeShapingBenchmark(gposIR, label, std::forward<GposRunner>(gposRunner), out, options, &localSummary);
        }

        if (summaryOut) *summaryOut += localSummary;
        return any && ok;
    }
}
