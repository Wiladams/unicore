// test_opentype_shaping_workload_corpus.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

#include "test_core.h"
#include "byte_resource_file.h"
#include "opentype_container.h"
#include "opentype_face.h"
#include "opentype_types.h"
#include "opentype_layout_view.h"
#include "opentype_gdef_view.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_shaping_ir.h"
#include "test_opentype_shaping_workloads.h"

namespace waavs
{
    struct OpenTypeShapingWorkloadCorpusEntry
    {
        const char* relativePath{nullptr};
        const char* roles{nullptr};
    };

    inline constexpr OpenTypeShapingWorkloadCorpusEntry kOpenTypeShapingWorkloadCorpus[] =
    {
        {"ofl/kiranghaerang/KirangHaerang-Regular.ttf", "gsub_single_max_mappings_per_subtable"},
        {"ofl/notosansduployan/NotoSansDuployan-Regular.ttf", "gsub_multiple_max_expansion,combined_filter_lookups"},
        {"ofl/notosanskannada/NotoSansKannada[wdth,wght].ttf", "gsub_alternate_max_set"},
        {"ofl/notosansgrantha/NotoSansGrantha-Regular.ttf", "gsub_ligature_max_fanout"},
        {"ofl/shantellsans/ShantellSans[BNCE,INFM,SPAC,wght].ttf", "gsub_ligature_max_components"},
        {"ofl/notoemoji/NotoEmoji[wght].ttf", "gsub_ligature_max_shared_prefix2"},
        {"ofl/signikanegative/SignikaNegative[wght].ttf", "gsub_ligature_max_shared_prefix3"},
        {"ofl/notonastaliqurdu/NotoNastaliqUrdu[wght].ttf", "gsub_context_max_rules_per_subtable,gsub_context_max_input,gsub_chain_max_prefix2_same_set_fanout"},
        {"ofl/notoserifdivesakuru/NotoSerifDivesAkuru-Regular.ttf", "gsub_context_max_first_fanout,gsub_context_max_prefix2_same_set_fanout"},
        {"ofl/rosario/Rosario-Italic[wght].ttf", "gsub_chain_max_backtrack"},
        {"ofl/inspiration/Inspiration-Regular.ttf", "gsub_chain_max_lookahead"},
        {"ofl/eczar/Eczar[wght].ttf", "gsub_chain_max_first_fanout"},
        {"ofl/stixtwotext/STIXTwoText-Italic[wght].ttf", "gpos_pair_max_explicit_pairs"},
        {"ofl/merriweather/Merriweather-Italic[opsz,wdth,wght].ttf", "gpos_pair_max_second_fanout"},
        {"ofl/lalezar/Lalezar-Regular.ttf", "gpos_pair_class_max_nonzero_row_fanout"},
        {"ofl/alata/Alata-Regular.ttf", "gpos_pair_max_class_cells"},
        {"ufl/ubuntusans/UbuntuSans[wdth,wght].ttf", "gpos_pair_max_class_dimension"},
        {"ofl/mada/Mada[wght].ttf", "gpos_mark_base_max_classes"},
        {"ofl/outfit/Outfit[wght].ttf", "gpos_mark_ligature_max_components"},
        {"ofl/tirodevanagarihindi/TiroDevanagariHindi-Regular.ttf", "gpos_mark_mark_max_classes"},
        {"ofl/notosansbengaliui/NotoSansBengaliUI[wdth,wght].ttf", "max_glyph_set_members"},
        {"ofl/jomolhari/Jomolhari-Regular.ttf", "max_glyph_set_ranges"}
    };

    struct OpenTypeShapingWorkloadCorpusSummary
    {
        uint64_t filesRequested{0};
        uint64_t filesOpened{0};
        uint64_t facesSeen{0};
        uint64_t facesCompiled{0};
        uint64_t facesPassed{0};
        uint64_t facesFailed{0};
        uint64_t missingFiles{0};
        uint64_t malformedGsubTables{0};
        uint64_t malformedGposTables{0};
        uint64_t malformedGdefTables{0};
        uint64_t gsubLookupCompileFailures{0};
        uint64_t gposLookupCompileFailures{0};
    };

    static inline bool compileOpenTypeShapingWorkloadGsubIR(const OpenTypeLayoutLookupListView& lookups, const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir, OpenTypeShapingWorkloadCorpusSummary& summary)
    {
        OpenTypeGsubIRCompilerContext context;
        context.reset(lookups.size());

        bool any = false;
        for (uint32_t i = 0; i < lookups.size(); ++i)
        {
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            if (compileOpenTypeGsubIRLookupByIndex(lookups, static_cast<uint16_t>(i), gdef, ir, context, lookupId)) any = true;
            else ++summary.gsubLookupCompileFailures;
        }
        return any;
    }

    static inline bool compileOpenTypeShapingWorkloadGposIR(const OpenTypeLayoutLookupListView& lookups, const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir, OpenTypeShapingWorkloadCorpusSummary& summary)
    {
        OpenTypeGposIRCompilerContext context;
        context.reset(lookups.size());

        bool any = false;
        for (uint32_t i = 0; i < lookups.size(); ++i)
        {
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            if (compileOpenTypeGposIRLookupByIndex(lookups, static_cast<uint16_t>(i), gdef, ir, context, lookupId)) any = true;
            else ++summary.gposLookupCompileFailures;
        }
        return any;
    }

    static inline bool compileOpenTypeShapingWorkloadFace(const FontFace& face, OpenTypeShapingIR& gsubIR, OpenTypeShapingIR& gposIR, OpenTypeShapingWorkloadCorpusSummary& summary)
    {
        gsubIR = {};
        gposIR = {};

        const auto* tables = openTypeTableProvider(face);
        if (!tables) return false;

        const TableRecord* gsubTable = tables->getTable(OTAG("GSUB"));
        const TableRecord* gposTable = tables->getTable(OTAG("GPOS"));
        const TableRecord* gdefTable = tables->getTable(OTAG("GDEF"));
        if (!gsubTable && !gposTable) return false;

        OpenTypeGdefView gdef;
        if (gdefTable)
        {
            gdef = OpenTypeGdefView(gdefTable->data);
            if (!gdef)
            {
                ++summary.malformedGdefTables;
                gdef = {};
            }
        }

        bool any = false;

        if (gsubTable)
        {
            const OpenTypeLayoutView layout(gsubTable->data);
            if (!layout) ++summary.malformedGsubTables;
            else
            {
                const OpenTypeLayoutLookupListView lookups = layout.lookups();
                if (!lookups) ++summary.malformedGsubTables;
                else any |= compileOpenTypeShapingWorkloadGsubIR(lookups, gdef, gsubIR, summary);
            }
        }

        if (gposTable)
        {
            const OpenTypeLayoutView layout(gposTable->data);
            if (!layout) ++summary.malformedGposTables;
            else
            {
                const OpenTypeLayoutLookupListView lookups = layout.lookups();
                if (!lookups) ++summary.malformedGposTables;
                else any |= compileOpenTypeShapingWorkloadGposIR(lookups, gdef, gposIR, summary);
            }
        }

        return any || !gsubIR.empty() || !gposIR.empty();
    }

    static inline bool testOpenTypeShapingWorkloadFile(const std::filesystem::path& filename, const char* roles, OpenTypeShapingWorkloadCorpusSummary& summary, FILE* out = stdout)
    {
        ++summary.filesRequested;

        FontResource resource;
        if (!readByteResource(filename, resource))
        {
            ++summary.missingFiles;
            std::fprintf(out, "CORPUS_FILE status=missing | file=%s | roles=%s\n", filename.string().c_str(), roles ? roles : "");
            return false;
        }

        OpenTypeContainer container(std::move(resource));
        if (!container.isValid())
        {
            ++summary.facesFailed;
            std::fprintf(out, "CORPUS_FILE status=invalid | file=%s | roles=%s\n", filename.string().c_str(), roles ? roles : "");
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
            if (!compileOpenTypeShapingWorkloadFace(face, gsubIR, gposIR, summary))
            {
                ++summary.facesFailed;
                continue;
            }

            ++summary.facesCompiled;

            const char* fullName = face.fullName();
            const char* familyName = face.familyName();
            const char* label = fullName && *fullName ? fullName : familyName && *familyName ? familyName : "(unnamed)";

            std::fprintf(out, "\nCORPUS_FACE_BEGIN name=%s | file=%s | roles=%s\n", label, filename.string().c_str(), roles ? roles : "");
            const bool passed = testOpenTypeShapingWorkloads(gsubIR, gposIR, label, out);
            std::fprintf(out, "CORPUS_FACE_END name=%s | status=%s\n", label, passed ? "pass" : "fail");

            if (passed) ++summary.facesPassed;
            else ++summary.facesFailed;

            filePassed |= passed;
        }

        return filePassed;
    }

    static inline void printOpenTypeShapingWorkloadCorpusSummary(const OpenTypeShapingWorkloadCorpusSummary& summary, FILE* out = stdout)
    {
        std::fprintf(out, "\nCORPUS_SUMMARY_BEGIN\n");
        std::fprintf(out, "METRIC files_requested: %llu\n", static_cast<unsigned long long>(summary.filesRequested));
        std::fprintf(out, "METRIC files_opened: %llu\n", static_cast<unsigned long long>(summary.filesOpened));
        std::fprintf(out, "METRIC missing_files: %llu\n", static_cast<unsigned long long>(summary.missingFiles));
        std::fprintf(out, "METRIC faces_seen: %llu\n", static_cast<unsigned long long>(summary.facesSeen));
        std::fprintf(out, "METRIC faces_compiled: %llu\n", static_cast<unsigned long long>(summary.facesCompiled));
        std::fprintf(out, "METRIC faces_passed: %llu\n", static_cast<unsigned long long>(summary.facesPassed));
        std::fprintf(out, "METRIC faces_failed: %llu\n", static_cast<unsigned long long>(summary.facesFailed));
        std::fprintf(out, "METRIC malformed_gsub_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGsubTables));
        std::fprintf(out, "METRIC malformed_gpos_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGposTables));
        std::fprintf(out, "METRIC malformed_gdef_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGdefTables));
        std::fprintf(out, "METRIC gsub_lookup_compile_failures: %llu\n", static_cast<unsigned long long>(summary.gsubLookupCompileFailures));
        std::fprintf(out, "METRIC gpos_lookup_compile_failures: %llu\n", static_cast<unsigned long long>(summary.gposLookupCompileFailures));
        std::fprintf(out, "CORPUS_SUMMARY_END\n");
    }

    static inline bool testOpenTypeShapingWorkloadCorpus(const char* fontRoot, FILE* out = stdout)
    {
        if (!fontRoot || !*fontRoot) return false;

        const std::filesystem::path root(fontRoot);
        OpenTypeShapingWorkloadCorpusSummary summary;
        bool allPassed = true;

        std::fprintf(out, "CORPUS_BEGIN\n");
        std::fprintf(out, "font_root: %s\n", root.string().c_str());
        std::fprintf(out, "candidate_files: %zu\n", std::size(kOpenTypeShapingWorkloadCorpus));

        for (const auto& entry : kOpenTypeShapingWorkloadCorpus)
        {
            const std::filesystem::path filename = root / entry.relativePath;
            if (!testOpenTypeShapingWorkloadFile(filename, entry.roles, summary, out)) allPassed = false;
        }

        printOpenTypeShapingWorkloadCorpusSummary(summary, out);
        std::fprintf(out, "CORPUS_END\n");

        return allPassed && summary.filesOpened == std::size(kOpenTypeShapingWorkloadCorpus) && summary.facesPassed != 0;
    }
}
