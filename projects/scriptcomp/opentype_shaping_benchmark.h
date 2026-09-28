// opentype_shaping_benchmark.h
#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "opentype_shaping_ir.h"
#include "opentype_shaping_execution_stats.h"
#include "opentype_shaping_workloads.h"

namespace waavs
{

    enum class OpenTypeShapingBenchmarkSelection : uint8_t
    {
        ExtremeOnly,
        TypicalAndExtreme,
        TypicalHighExtreme
    };

    struct OpenTypeShapingBenchmarkOptions
    {
        uint32_t warmupIterations{16};
        uint32_t iterations{1000};
        uint32_t minimumIterations{1};
        OpenTypeShapingBenchmarkSelection selection{OpenTypeShapingBenchmarkSelection::ExtremeOnly};
        bool collectStats{true};
    };

    enum class OpenTypeShapingBenchmarkStatus : uint8_t
    {
        Success,
        InvalidWorkload,
        ExecutionFailure
    };

    struct OpenTypeShapingBenchmarkResult
    {
        std::string face{};
        std::string workload{};

        OpenTypeShapingIRLookupId lookup{kOpenTypeShapingIRInvalid};
        OpenTypeShapingIROp op{OpenTypeShapingIROp::Invalid};
        OpenTypeShapingWorkloadKind kind{OpenTypeShapingWorkloadKind::HitEarly};

        uint32_t structuralValue{0};
        uint32_t inputGlyphs{0};
        uint32_t targetOffset{0};
        uint32_t expectedLogicalLength{0};

        uint64_t iterations{0};
        uint64_t totalNs{0};

        double nsPerIteration{0.0};
        double nsPerInputGlyph{0.0};

        OpenTypeShapingBenchmarkStatus status{OpenTypeShapingBenchmarkStatus::ExecutionFailure};
        bool executionSucceeded{false};
        OpenTypeShapingExecutionStats stats{};
    };

    struct OpenTypeShapingBenchmarkSelectionEntry
    {
        size_t workloadIndex{0};
        const char* tier{nullptr};
    };

    enum class OpenTypeShapingBenchmarkExpectedResult : uint8_t
    {
        Any,
        Match,
        NoMatch
    };

    [[nodiscard]] static inline OpenTypeShapingBenchmarkExpectedResult openTypeShapingBenchmarkExpectedResult(const OpenTypeShapingWorkload& workload) noexcept
    {
        switch (workload.kind)
        {
        case OpenTypeShapingWorkloadKind::HitEarly:
        case OpenTypeShapingWorkloadKind::HitLate:
        case OpenTypeShapingWorkloadKind::ExpansionStress:
        case OpenTypeShapingWorkloadKind::ContractionStress:
            return OpenTypeShapingBenchmarkExpectedResult::Match;

        case OpenTypeShapingWorkloadKind::MissEarly:
        case OpenTypeShapingWorkloadKind::MissLate:
        case OpenTypeShapingWorkloadKind::FilterStress:
            return OpenTypeShapingBenchmarkExpectedResult::NoMatch;

        case OpenTypeShapingWorkloadKind::Mixed:
        default:
            return OpenTypeShapingBenchmarkExpectedResult::Any;
        }
    }

    [[nodiscard]] static inline bool validateOpenTypeShapingBenchmarkExecution(const OpenTypeShapingWorkload& workload,
        const OpenTypeShapingExecutionStats& stats) noexcept
    {
        const auto expected = openTypeShapingBenchmarkExpectedResult(workload);
        if (expected == OpenTypeShapingBenchmarkExpectedResult::Any) return true;

        const bool matched = stats.lookupMatches != 0;
        return expected == OpenTypeShapingBenchmarkExpectedResult::Match ? matched : !matched;
    }

    [[nodiscard]] static inline const char* openTypeShapingBenchmarkStatusName(OpenTypeShapingBenchmarkStatus value) noexcept
    {
        switch (value)
        {
        case OpenTypeShapingBenchmarkStatus::Success: return "success";
        case OpenTypeShapingBenchmarkStatus::InvalidWorkload: return "invalid_workload";
        case OpenTypeShapingBenchmarkStatus::ExecutionFailure: return "execution_failure";
        default: return "unknown";
        }
    }

    [[nodiscard]] static inline const char* openTypeShapingBenchmarkSelectionName(OpenTypeShapingBenchmarkSelection value) noexcept
    {
        switch (value)
        {
        case OpenTypeShapingBenchmarkSelection::ExtremeOnly: return "extreme_only";
        case OpenTypeShapingBenchmarkSelection::TypicalAndExtreme: return "typical_and_extreme";
        case OpenTypeShapingBenchmarkSelection::TypicalHighExtreme: return "typical_high_extreme";
        default: return "unknown";
        }
    }

    [[nodiscard]] static inline bool openTypeShapingBenchmarkSameFamily(const OpenTypeShapingWorkload& a, const OpenTypeShapingWorkload& b) noexcept
    {
        return a.name == b.name && a.kind == b.kind;
    }

    static inline void selectOpenTypeShapingBenchmarkWorkloads(const std::vector<OpenTypeShapingWorkload>& workloads,
        OpenTypeShapingBenchmarkSelection policy, std::vector<OpenTypeShapingBenchmarkSelectionEntry>& out)
    {
        out.clear();
        if (workloads.empty()) return;

        std::vector<size_t> order(workloads.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;

        std::sort(order.begin(), order.end(), [&](size_t a, size_t b)
        {
            const auto& wa = workloads[a];
            const auto& wb = workloads[b];
            if (wa.name != wb.name) return wa.name < wb.name;
            if (wa.kind != wb.kind) return static_cast<uint8_t>(wa.kind) < static_cast<uint8_t>(wb.kind);
            if (wa.structuralValue != wb.structuralValue) return wa.structuralValue < wb.structuralValue;
            if (wa.glyphs.size() != wb.glyphs.size()) return wa.glyphs.size() < wb.glyphs.size();
            return wa.lookup < wb.lookup;
        });

        for (size_t begin = 0; begin < order.size();)
        {
            size_t end = begin + 1;
            while (end < order.size() && openTypeShapingBenchmarkSameFamily(workloads[order[begin]], workloads[order[end]])) ++end;

            const size_t count = end - begin;
            const auto add = [&](size_t localIndex, const char* tier)
            {
                const size_t index = order[begin + localIndex];
                for (const auto& existing : out) if (existing.workloadIndex == index) return;
                out.push_back({index, tier});
            };

            if (policy == OpenTypeShapingBenchmarkSelection::TypicalHighExtreme)
            {
                add((count - 1) / 2, "typical");
                add((count - 1) * 9 / 10, "high");
            }
            else if (policy == OpenTypeShapingBenchmarkSelection::TypicalAndExtreme)
            {
                add((count - 1) / 2, "typical");
            }

            add(count - 1, "extreme");
            begin = end;
        }

        std::sort(out.begin(), out.end(), [&](const auto& a, const auto& b)
        {
            const auto& wa = workloads[a.workloadIndex];
            const auto& wb = workloads[b.workloadIndex];
            if (wa.name != wb.name) return wa.name < wb.name;
            if (wa.structuralValue != wb.structuralValue) return wa.structuralValue < wb.structuralValue;
            return wa.lookup < wb.lookup;
        });
    }

    template<class Runner>
    [[nodiscard]] static inline bool invokeOpenTypeShapingBenchmarkRunner(Runner&& runner, const OpenTypeShapingIR& ir,
        const OpenTypeShapingWorkload& workload, OpenTypeShapingExecutionStats* stats)
    {
        static_assert(std::is_invocable_r_v<bool, Runner, const OpenTypeShapingIR&, const OpenTypeShapingWorkload&, OpenTypeShapingExecutionStats*>,
            "Benchmark runner must be callable as bool(const OpenTypeShapingIR&, const OpenTypeShapingWorkload&, OpenTypeShapingExecutionStats*)");
        return std::forward<Runner>(runner)(ir, workload, stats);
    }

    template<class Runner>
    [[nodiscard]] static inline OpenTypeShapingBenchmarkResult benchmarkOpenTypeShapingWorkload(const char* face,
        const OpenTypeShapingIR& ir, const OpenTypeShapingWorkload& workload, Runner&& runner,
        const OpenTypeShapingBenchmarkOptions& options = {})
    {
        OpenTypeShapingBenchmarkResult result;
        result.face = face ? face : "";
        result.workload = workload.name;
        result.lookup = workload.lookup;
        result.kind = workload.kind;
        result.structuralValue = workload.structuralValue;
        result.inputGlyphs = static_cast<uint32_t>(workload.glyphs.size());
        result.targetOffset = workload.targetOffset;
        result.expectedLogicalLength = workload.expectedLogicalLength;

        const auto* lookup = ir.lookup(workload.lookup);
        if (!lookup || workload.glyphs.empty() || workload.targetOffset >= workload.glyphs.size()) return result;
        result.op = lookup->op;

        // Always perform one untimed execution with stats. Besides collecting
        // counters, this validates the semantic claim made by the workload:
        // hit workloads must Match and miss/filter workloads must NoMatch.
        OpenTypeShapingExecutionStats validationStats;
        if (!invokeOpenTypeShapingBenchmarkRunner(runner, ir, workload, &validationStats))
        {
            result.status = OpenTypeShapingBenchmarkStatus::ExecutionFailure;
            return result;
        }

        if (!validateOpenTypeShapingBenchmarkExecution(workload, validationStats))
        {
            result.status = OpenTypeShapingBenchmarkStatus::InvalidWorkload;
            result.stats = validationStats;
            return result;
        }

        if (options.collectStats) result.stats = validationStats;

        for (uint32_t i = 0; i < options.warmupIterations; ++i)
            if (!invokeOpenTypeShapingBenchmarkRunner(runner, ir, workload, nullptr)) return result;

        const uint64_t iterations = std::max<uint64_t>(options.iterations, options.minimumIterations);
        const auto start = std::chrono::steady_clock::now();

        for (uint64_t i = 0; i < iterations; ++i)
            if (!invokeOpenTypeShapingBenchmarkRunner(runner, ir, workload, nullptr)) return result;

        const auto end = std::chrono::steady_clock::now();
        const uint64_t totalNs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

        result.iterations = iterations;
        result.totalNs = totalNs;
        result.nsPerIteration = iterations ? static_cast<double>(totalNs) / static_cast<double>(iterations) : 0.0;
        result.nsPerInputGlyph = result.inputGlyphs ? result.nsPerIteration / static_cast<double>(result.inputGlyphs) : 0.0;
        result.status = OpenTypeShapingBenchmarkStatus::Success;
        result.executionSucceeded = true;
        return result;
    }

    template<class Runner>
    static inline bool benchmarkOpenTypeShapingIR(const char* face, const OpenTypeShapingIR& ir, Runner&& runner,
        std::vector<OpenTypeShapingBenchmarkResult>& results, const OpenTypeShapingBenchmarkOptions& options = {})
    {
        std::vector<OpenTypeShapingWorkload> workloads;
        makeOpenTypeShapingWorkloads(ir, workloads);
        if (workloads.empty()) return false;

        std::vector<OpenTypeShapingBenchmarkSelectionEntry> selected;
        selectOpenTypeShapingBenchmarkWorkloads(workloads, options.selection, selected);
        if (selected.empty()) return false;

        bool allSucceeded = true;
        for (const auto& entry : selected)
        {
            const auto result = benchmarkOpenTypeShapingWorkload(face, ir, workloads[entry.workloadIndex], runner, options);
            if (result.status == OpenTypeShapingBenchmarkStatus::ExecutionFailure) allSucceeded = false;
            results.push_back(result);
        }
        return allSucceeded;
    }
}
