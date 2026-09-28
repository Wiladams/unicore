// test_opentype_shaping_benchmark_corpus.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <utility>

#include "test_core.h"
#include "test_opentype_shaping_workload_corpus.h"
#include "test_opentype_shaping_benchmark.h"
#include "opentype_gsub_ir_executor.h"
#include "opentype_chain_context_execution_compiler.h"
#include "opentype_gpos_ir_executor.h"

namespace waavs
{

    static inline bool runOpenTypeGsubBenchmarkWorkload(const OpenTypeShapingIR& ir,
        const OpenTypeShapingWorkload& workload, OpenTypeShapingExecutionStats* stats,
        const OpenTypeChainContextExecutionIR* chainContextExec = nullptr)
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

        OpenTypeGsubApplyState state;
        state.stats = stats;
        state.chainContextExec = chainContextExec;
        OpenTypeGsubEditLog edits;

        const auto result = applyOpenTypeGsubIRLookupAt(
            ir, workload.lookup, buffer, workload.targetOffset, state, edits);

        return result != OpenTypeGsubApplyAtResult::Invalid;
    }

    static inline bool runOpenTypeGposBenchmarkWorkload(const OpenTypeShapingIR& ir,
        const OpenTypeShapingWorkload& workload, OpenTypeShapingExecutionStats* stats)
    {
        ShapedGlyphBuffer buffer;

        for (size_t i = 0; i < workload.glyphs.size(); ++i)
        {
            ShapedGlyph glyph{};
            glyph.shaping.glyphId = workload.glyphs[i];
            glyph.shaping.scalarOffset = static_cast<uint32_t>(i);
            glyph.shaping.scalarCount = 1;
            buffer.pushBack(glyph);
        }

        OpenTypeGposAttachmentState attachments;
        attachments.reset(buffer.size());

        const OpenTypeShapingIRLookup* lookup = ir.lookup(workload.lookup);
        if (!lookup) return false;

        const bool runRightToLeft =
            (lookup->filter.flags & OpenTypeShapingIRRightToLeft) != 0;

        const auto result = applyOpenTypeGposIRLookupAt(
            ir, workload.lookup, buffer, workload.targetOffset,
            runRightToLeft, attachments, nullptr, stats);

        if (result == decltype(result)::Invalid) return false;

        if (result == decltype(result)::Match)
        {
            if (!resolveOpenTypeGposAttachments(buffer, attachments, runRightToLeft)) return false;
        }

        return true;
    }

    struct OpenTypeShapingBenchmarkCorpusSummary
    {
        uint64_t filesRequested{0};
        uint64_t filesOpened{0};
        uint64_t missingFiles{0};

        uint64_t facesSeen{0};
        uint64_t facesCompiled{0};
        uint64_t facesBenchmarked{0};
        uint64_t facesFailed{0};

        uint64_t benchmarkWorkloads{0};
        uint64_t benchmarkPassed{0};
        uint64_t invalidWorkloads{0};
        uint64_t benchmarkExecutionFailures{0};

        uint64_t malformedGsubTables{0};
        uint64_t malformedGposTables{0};
        uint64_t malformedGdefTables{0};

        uint64_t gsubLookupCompileFailures{0};
        uint64_t gposLookupCompileFailures{0};
    };

    static inline void mergeOpenTypeShapingBenchmarkCompileSummary(OpenTypeShapingBenchmarkCorpusSummary& dst, const OpenTypeShapingWorkloadCorpusSummary& src) noexcept
    {
        dst.malformedGsubTables += src.malformedGsubTables;
        dst.malformedGposTables += src.malformedGposTables;
        dst.malformedGdefTables += src.malformedGdefTables;
        dst.gsubLookupCompileFailures += src.gsubLookupCompileFailures;
        dst.gposLookupCompileFailures += src.gposLookupCompileFailures;
    }

    template<class GsubRunner, class GposRunner>
    static inline bool testOpenTypeShapingBenchmarkFile(const std::filesystem::path& filename, const char* roles,
        GsubRunner& gsubRunner, GposRunner& gposRunner, OpenTypeShapingBenchmarkCorpusSummary& summary,
        FILE* out = stdout, const OpenTypeShapingBenchmarkOptions& options = {})
    {
        ++summary.filesRequested;

        FontResource resource;
        if (!readByteResource(filename, resource))
        {
            ++summary.missingFiles;
            std::fprintf(out, "BENCHMARK_CORPUS_FILE status=missing | file=%s | roles=%s\n",
                filename.string().c_str(), roles ? roles : "");
            return false;
        }

        OpenTypeContainer container(std::move(resource));
        if (!container.isValid())
        {
            ++summary.facesFailed;
            std::fprintf(out, "BENCHMARK_CORPUS_FILE status=invalid | file=%s | roles=%s\n",
                filename.string().c_str(), roles ? roles : "");
            return false;
        }

        ++summary.filesOpened;

        bool filePassed = false;
        FontFaceView view;

        while (container(view))
        {
            ++summary.facesSeen;

            FontFace face = parseFontFace(std::move(view));
            if (!face)
            {
                ++summary.facesFailed;
                continue;
            }

            OpenTypeShapingIR gsubIR;
            OpenTypeShapingIR gposIR;
            OpenTypeShapingWorkloadCorpusSummary compileSummary;

            if (!compileOpenTypeShapingWorkloadFace(face, gsubIR, gposIR, compileSummary))
            {
                mergeOpenTypeShapingBenchmarkCompileSummary(summary, compileSummary);
                ++summary.facesFailed;
                continue;
            }

            OpenTypeChainContextExecutionIR chainContextExec;
            if (!compileOpenTypeChainContextExecutionIR(gsubIR, chainContextExec))
            {
                mergeOpenTypeShapingBenchmarkCompileSummary(summary, compileSummary);
                ++summary.facesFailed;
                continue;
            }

            mergeOpenTypeShapingBenchmarkCompileSummary(summary, compileSummary);
            ++summary.facesCompiled;

            const char* fullName = face.fullName();
            const char* familyName = face.familyName();
            const char* label = fullName && *fullName ? fullName : familyName && *familyName ? familyName : "(unnamed)";

            std::fprintf(out,
                "\nBENCHMARK_CORPUS_FACE_BEGIN name=%s | file=%s | roles=%s\n",
                label, filename.string().c_str(), roles ? roles : "");

            auto machineGsubRunner =
                [&](const OpenTypeShapingIR& ir, const OpenTypeShapingWorkload& workload,
                    OpenTypeShapingExecutionStats* stats)
                {
                    return runOpenTypeGsubBenchmarkWorkload(
                        ir, workload, stats, &chainContextExec);
                };

            OpenTypeShapingBenchmarkTestSummary benchmarkSummary;
            const bool passed = testOpenTypeShapingBenchmark(
                gsubIR, gposIR, label, machineGsubRunner, gposRunner,
                out, options, &benchmarkSummary);

            summary.benchmarkWorkloads += benchmarkSummary.benchmarkCount;
            summary.benchmarkPassed += benchmarkSummary.passed;
            summary.invalidWorkloads += benchmarkSummary.invalidWorkloads;
            summary.benchmarkExecutionFailures += benchmarkSummary.failed;

            std::fprintf(out,
                "BENCHMARK_CORPUS_FACE_END name=%s | status=%s\n",
                label, passed ? "pass" : "fail");

            if (passed)
            {
                ++summary.facesBenchmarked;
                filePassed = true;
            }
            else
            {
                ++summary.facesFailed;
            }
        }

        return filePassed;
    }

    static inline void printOpenTypeShapingBenchmarkCorpusSummary(const OpenTypeShapingBenchmarkCorpusSummary& summary, FILE* out = stdout)
    {
        std::fprintf(out, "\nBENCHMARK_CORPUS_SUMMARY_BEGIN\n");
        std::fprintf(out, "METRIC files_requested: %llu\n", static_cast<unsigned long long>(summary.filesRequested));
        std::fprintf(out, "METRIC files_opened: %llu\n", static_cast<unsigned long long>(summary.filesOpened));
        std::fprintf(out, "METRIC missing_files: %llu\n", static_cast<unsigned long long>(summary.missingFiles));
        std::fprintf(out, "METRIC faces_seen: %llu\n", static_cast<unsigned long long>(summary.facesSeen));
        std::fprintf(out, "METRIC faces_compiled: %llu\n", static_cast<unsigned long long>(summary.facesCompiled));
        std::fprintf(out, "METRIC faces_benchmarked: %llu\n", static_cast<unsigned long long>(summary.facesBenchmarked));
        std::fprintf(out, "METRIC faces_failed: %llu\n", static_cast<unsigned long long>(summary.facesFailed));
        std::fprintf(out, "METRIC benchmark_workloads: %llu\n", static_cast<unsigned long long>(summary.benchmarkWorkloads));
        std::fprintf(out, "METRIC benchmark_passed: %llu\n", static_cast<unsigned long long>(summary.benchmarkPassed));
        std::fprintf(out, "METRIC invalid_workloads: %llu\n", static_cast<unsigned long long>(summary.invalidWorkloads));
        std::fprintf(out, "METRIC benchmark_execution_failures: %llu\n", static_cast<unsigned long long>(summary.benchmarkExecutionFailures));
        std::fprintf(out, "METRIC malformed_gsub_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGsubTables));
        std::fprintf(out, "METRIC malformed_gpos_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGposTables));
        std::fprintf(out, "METRIC malformed_gdef_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGdefTables));
        std::fprintf(out, "METRIC gsub_lookup_compile_failures: %llu\n", static_cast<unsigned long long>(summary.gsubLookupCompileFailures));
        std::fprintf(out, "METRIC gpos_lookup_compile_failures: %llu\n", static_cast<unsigned long long>(summary.gposLookupCompileFailures));
        std::fprintf(out, "BENCHMARK_CORPUS_SUMMARY_END\n");
    }

    static inline bool testOpenTypeShapingBenchmarkCorpus(const char* fontRoot,
        FILE* out = stdout, const OpenTypeShapingBenchmarkOptions& options = {})
    {
        if (!fontRoot || !*fontRoot) return false;

        const std::filesystem::path root(fontRoot);
        OpenTypeShapingBenchmarkCorpusSummary summary;
        bool allPassed = true;

        auto gsubRunner = runOpenTypeGsubBenchmarkWorkload;
        auto gposRunner = runOpenTypeGposBenchmarkWorkload;

        std::fprintf(out, "BENCHMARK_CORPUS_BEGIN\n");
        std::fprintf(out, "font_root: %s\n", root.string().c_str());
        std::fprintf(out, "candidate_files: %zu\n", std::size(kOpenTypeShapingWorkloadCorpus));
        std::fprintf(out, "selection: %s\n", openTypeShapingBenchmarkSelectionName(options.selection));
        std::fprintf(out, "warmup_iterations: %u\n", options.warmupIterations);
        std::fprintf(out, "timed_iterations: %u\n", options.iterations);

        for (const auto& entry : kOpenTypeShapingWorkloadCorpus)
        {
            const std::filesystem::path filename = root / entry.relativePath;

            if (!testOpenTypeShapingBenchmarkFile(
                filename, entry.roles, gsubRunner, gposRunner, summary, out, options))
            {
                allPassed = false;
            }
        }

        printOpenTypeShapingBenchmarkCorpusSummary(summary, out);
        std::fprintf(out, "BENCHMARK_CORPUS_END\n");

        return allPassed &&
            summary.filesOpened == std::size(kOpenTypeShapingWorkloadCorpus) &&
            summary.facesBenchmarked != 0;
    }
}
