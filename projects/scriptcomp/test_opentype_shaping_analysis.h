// test_opentype_shaping_analysis.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "test_core.h"
#include "font_directory_view.h"
#include "font_interfaces.h"
#include "opentype_layout_view.h"
#include "opentype_gdef_view.h"
#include "opentype_gsub_ir_compiler.h"
#include "opentype_gpos_ir_compiler.h"
#include "opentype_shaping_ir.h"
#include "opentype_shaping_analysis.h"

namespace waavs
{
    struct OpenTypeShapingAnalysisCompileSummary
    {
        uint64_t facesSeen{0};
        uint64_t facesAnalyzed{0};
        uint64_t facesWithGsub{0};
        uint64_t facesWithGpos{0};
        uint64_t gsubSourceLookups{0};
        uint64_t gposSourceLookups{0};
        uint64_t gsubLookupCompileFailures{0};
        uint64_t gposLookupCompileFailures{0};
        uint64_t malformedGsubTables{0};
        uint64_t malformedGposTables{0};
        uint64_t malformedGdefTables{0};
    };

    static inline bool compileOpenTypeGsubAnalysisIR(const OpenTypeLayoutLookupListView& lookups, const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir, OpenTypeShapingAnalysisCompileSummary& summary)
    {
        OpenTypeGsubIRCompilerContext context;
        context.reset(lookups.size());
        summary.gsubSourceLookups += lookups.size();

        bool any = false;
        for (uint32_t i = 0; i < lookups.size(); ++i)
        {
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            if (compileOpenTypeGsubIRLookupByIndex(lookups, static_cast<uint16_t>(i), gdef, ir, context, lookupId)) any = true;
            else ++summary.gsubLookupCompileFailures;
        }
        return any;
    }

    static inline bool compileOpenTypeGposAnalysisIR(const OpenTypeLayoutLookupListView& lookups, const OpenTypeGdefView& gdef, OpenTypeShapingIR& ir, OpenTypeShapingAnalysisCompileSummary& summary)
    {
        OpenTypeGposIRCompilerContext context;
        context.reset(lookups.size());
        summary.gposSourceLookups += lookups.size();

        bool any = false;
        for (uint32_t i = 0; i < lookups.size(); ++i)
        {
            OpenTypeShapingIRLookupId lookupId = kOpenTypeShapingIRInvalid;
            if (compileOpenTypeGposIRLookupByIndex(lookups, static_cast<uint16_t>(i), gdef, ir, context, lookupId)) any = true;
            else ++summary.gposLookupCompileFailures;
        }
        return any;
    }

    static inline bool analyzeOpenTypeShapingFace(const FontFace& face, OpenTypeShapingCorpusAnalysis& analysis, OpenTypeShapingAnalysisCompileSummary& summary)
    {
        ++summary.facesSeen;

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
            if (!gdef.isValid()) { ++summary.malformedGdefTables; gdef = OpenTypeGdefView{}; }
        }

        OpenTypeShapingIR gsubIR;
        OpenTypeShapingIR gposIR;
        bool compiledAny = false;

        if (gsubTable)
        {
            OpenTypeLayoutView layout(gsubTable->data);
            if (!layout.isValid()) ++summary.malformedGsubTables;
            else
            {
                const auto lookups = layout.lookups();
                if (!lookups.isValid()) ++summary.malformedGsubTables;
                else
                {
                    ++summary.facesWithGsub;
                    compiledAny |= compileOpenTypeGsubAnalysisIR(lookups, gdef, gsubIR, summary);
                }
            }
        }

        if (gposTable)
        {
            OpenTypeLayoutView layout(gposTable->data);
            if (!layout.isValid()) ++summary.malformedGposTables;
            else
            {
                const auto lookups = layout.lookups();
                if (!lookups.isValid()) ++summary.malformedGposTables;
                else
                {
                    ++summary.facesWithGpos;
                    compiledAny |= compileOpenTypeGposAnalysisIR(lookups, gdef, gposIR, summary);
                }
            }
        }

        if (!compiledAny && gsubIR.lookups.empty() && gposIR.lookups.empty()) return false;

        ++summary.facesAnalyzed;
        std::string label = face.fullName();
        if (label.empty()) label = face.familyName();
        analysis.add(label, face.sourceLocation(), gsubIR, gposIR);
        return true;
    }

    static inline void printOpenTypeShapingAnalysisCompileSummary(FILE* out, const OpenTypeShapingAnalysisCompileSummary& summary)
    {
        std::fprintf(out, "SECTION_BEGIN name=compiler_census\n");
        std::fprintf(out, "METRIC faces_seen: %llu\n", static_cast<unsigned long long>(summary.facesSeen));
        std::fprintf(out, "METRIC faces_analyzed: %llu\n", static_cast<unsigned long long>(summary.facesAnalyzed));
        std::fprintf(out, "METRIC faces_with_gsub: %llu\n", static_cast<unsigned long long>(summary.facesWithGsub));
        std::fprintf(out, "METRIC faces_with_gpos: %llu\n", static_cast<unsigned long long>(summary.facesWithGpos));
        std::fprintf(out, "METRIC gsub_source_lookups: %llu\n", static_cast<unsigned long long>(summary.gsubSourceLookups));
        std::fprintf(out, "METRIC gpos_source_lookups: %llu\n", static_cast<unsigned long long>(summary.gposSourceLookups));
        std::fprintf(out, "METRIC gsub_lookup_compile_failures: %llu\n", static_cast<unsigned long long>(summary.gsubLookupCompileFailures));
        std::fprintf(out, "METRIC gpos_lookup_compile_failures: %llu\n", static_cast<unsigned long long>(summary.gposLookupCompileFailures));
        std::fprintf(out, "METRIC malformed_gsub_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGsubTables));
        std::fprintf(out, "METRIC malformed_gpos_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGposTables));
        std::fprintf(out, "METRIC malformed_gdef_tables: %llu\n", static_cast<unsigned long long>(summary.malformedGdefTables));
        std::fprintf(out, "SECTION_END\n");
    }

    static inline bool testOpenTypeShapingAnalysis(const char* fontDirectory, FILE* out = stdout)
    {
        FontDirectoryView fonts(fontDirectory, true);
        FontFace face;
        OpenTypeShapingCorpusAnalysis analysis;
        OpenTypeShapingAnalysisCompileSummary summary;

        while (fonts(face)) analyzeOpenTypeShapingFace(face, analysis, summary);

        analysis.print(out);
        std::fputc('\n', out);
        printOpenTypeShapingAnalysisCompileSummary(out, summary);
        return summary.facesAnalyzed != 0;
    }
}
