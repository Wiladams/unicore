// test_opentype_shaping_workloads.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "test_core.h"
#include "opentype_shaping_workloads.h"

namespace waavs
{
    [[nodiscard]] static inline const char* openTypeShapingWorkloadKindName(OpenTypeShapingWorkloadKind kind) noexcept
    {
        switch (kind)
        {
        case OpenTypeShapingWorkloadKind::HitEarly: return "hit_early";
        case OpenTypeShapingWorkloadKind::HitLate: return "hit_late";
        case OpenTypeShapingWorkloadKind::MissEarly: return "miss_early";
        case OpenTypeShapingWorkloadKind::MissLate: return "miss_late";
        case OpenTypeShapingWorkloadKind::Mixed: return "mixed";
        case OpenTypeShapingWorkloadKind::FilterStress: return "filter_stress";
        case OpenTypeShapingWorkloadKind::ExpansionStress: return "expansion_stress";
        case OpenTypeShapingWorkloadKind::ContractionStress: return "contraction_stress";
        default: return "unknown";
        }
    }

    [[nodiscard]] static inline bool validateOpenTypeShapingWorkload(const OpenTypeShapingIR& ir, const OpenTypeShapingWorkload& workload) noexcept
    {
        if (workload.lookup >= ir.lookups.size() || workload.glyphs.empty()) return false;
        if (workload.targetOffset >= workload.glyphs.size()) return false;
        if (!workload.repeatCount) return false;

        const auto& lookup = ir.lookups[workload.lookup];
        if (workload.kind == OpenTypeShapingWorkloadKind::FilterStress)
            return openTypeShapingLookupHasFilter(lookup) && workload.structuralValue != 0;

        return lookup.op != OpenTypeShapingIROp::Invalid;
    }

    static inline bool testOpenTypeShapingWorkloads(const OpenTypeShapingIR& ir, const char* label = nullptr, FILE* out = stdout)
    {
        std::vector<OpenTypeShapingWorkload> workloads;
        makeOpenTypeShapingWorkloads(ir, workloads);

        std::fprintf(out, "REPORT_BEGIN\n");
        std::fprintf(out, "report_type: OpenTypeShapingWorkloads\n");
        std::fprintf(out, "schema_version: 1\n");
        std::fprintf(out, "label: %s\n", label ? label : "");
        std::fprintf(out, "lookup_count: %zu\n", ir.lookups.size());
        std::fprintf(out, "workload_count: %zu\n\n", workloads.size());

        uint64_t validCount = 0;
        uint64_t invalidCount = 0;

        std::fprintf(out, "SECTION_BEGIN name=workloads\n");
        for (const auto& workload : workloads)
        {
            const bool valid = validateOpenTypeShapingWorkload(ir, workload);
            valid ? ++validCount : ++invalidCount;

            std::fprintf(out,
                "WORKLOAD name=%s | lookup=%u | op=%u | kind=%s | glyphs=%zu | target_offset=%u | expected_logical_length=%u | structural_value=%u | valid=%u\n",
                workload.name.c_str(),
                workload.lookup,
                workload.lookup < ir.lookups.size() ? static_cast<unsigned>(ir.lookups[workload.lookup].op) : 0u,
                openTypeShapingWorkloadKindName(workload.kind),
                workload.glyphs.size(),
                workload.targetOffset,
                workload.expectedLogicalLength,
                workload.structuralValue,
                valid ? 1u : 0u);
        }
        std::fprintf(out, "SECTION_END\n\n");

        std::fprintf(out, "SECTION_BEGIN name=summary\n");
        std::fprintf(out, "METRIC valid_workloads: %llu\n", static_cast<unsigned long long>(validCount));
        std::fprintf(out, "METRIC invalid_workloads: %llu\n", static_cast<unsigned long long>(invalidCount));
        std::fprintf(out, "SECTION_END\n");
        std::fprintf(out, "REPORT_END\n");

        return !workloads.empty() && invalidCount == 0;
    }

    static inline bool testOpenTypeShapingWorkloads(const OpenTypeShapingIR& gsubIR, const OpenTypeShapingIR& gposIR, const char* label, FILE* out = stdout)
    {
        const bool gsubOk = gsubIR.empty() || testOpenTypeShapingWorkloads(gsubIR, label, out);
        const bool gposOk = gposIR.empty() || testOpenTypeShapingWorkloads(gposIR, label, out);
        return gsubOk && gposOk;
    }
}
