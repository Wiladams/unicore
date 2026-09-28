// test_opentype_shaping_rule_trie.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

#include "test_core.h"
#include "test_opentype_shaping_workload_corpus.h"
#include "opentype_shaping_rule_trie.h"

namespace waavs
{
    struct OpenTypeShapingRuleTrieSelection
    {
        OpenTypeShapingIRLookupId lookup{kOpenTypeShapingIRInvalid};
        uint32_t subtableIndex{0};
        uint32_t ruleCount{0};
    };

    static inline bool selectOpenTypeShapingRuleTrieContextSubtable(
        const OpenTypeShapingIR& ir, OpenTypeShapingRuleTrieSelection& result) noexcept
    {
        result = {};

        for (uint32_t lookupId = 0; lookupId < ir.lookups.size(); ++lookupId)
        {
            const auto& lookup = ir.lookups[lookupId];
            if (lookup.op != OpenTypeShapingIROp::GsubContext)
                continue;

            const uint64_t end = uint64_t(lookup.payloadOffset) + uint64_t(lookup.payloadCount);
            if (end > ir.gsubContextSubtables.size())
                return false;

            for (uint32_t i = 0; i < lookup.payloadCount; ++i)
            {
                const auto& subtable = ir.gsubContextSubtables[lookup.payloadOffset + i];

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

    static inline bool selectOpenTypeShapingRuleTrieChainContextSubtable(
        const OpenTypeShapingIR& ir, OpenTypeShapingRuleTrieSelection& result) noexcept
    {
        result = {};

        for (uint32_t lookupId = 0; lookupId < ir.lookups.size(); ++lookupId)
        {
            const auto& lookup = ir.lookups[lookupId];
            if (lookup.op != OpenTypeShapingIROp::GsubChainContext)
                continue;

            const uint64_t end = uint64_t(lookup.payloadOffset) + uint64_t(lookup.payloadCount);
            if (end > ir.gsubChainContextSubtables.size())
                return false;

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

    static inline void printOpenTypeShapingRuleTrieAnalysis(
        const char* face, const char* opName,
        const OpenTypeShapingRuleTrieSelection& selection,
        const OpenTypeShapingRuleTrie& trie,
        const OpenTypeShapingRuleTrieAnalysis& analysis,
        FILE* out = stdout)
    {
        std::fprintf(out, "\nRULE_TRIE_BEGIN face=%s | op=%s | lookup=%u | subtable=%u\n",
            face ? face : "(unnamed)", opName ? opName : "",
            selection.lookup, selection.subtableIndex);

        std::fprintf(out, "METRIC source_rules: %u\n", analysis.ruleCount);
        std::fprintf(out, "METRIC source_predicate_positions: %llu\n",
            static_cast<unsigned long long>(analysis.sourcePredicatePositions));
        std::fprintf(out, "METRIC canonical_predicates: %u\n", analysis.canonicalPredicateCount);
        std::fprintf(out, "METRIC trie_nodes: %u\n", analysis.nodeCount);
        std::fprintf(out, "METRIC trie_edges: %u\n", analysis.edgeCount);
        std::fprintf(out, "METRIC accepting_nodes: %u\n", analysis.acceptingNodeCount);
        std::fprintf(out, "METRIC duplicate_pattern_rules: %u\n", analysis.duplicatePatternRules);
        std::fprintf(out, "METRIC max_depth: %u\n", analysis.maxDepth);
        std::fprintf(out, "METRIC root_fanout: %u\n", analysis.rootFanout);
        std::fprintf(out, "METRIC max_node_fanout: %u\n", analysis.maxNodeFanout);
        std::fprintf(out, "METRIC shared_prefix_positions: %llu\n",
            static_cast<unsigned long long>(analysis.sharedPrefixPositions));
        std::fprintf(out, "METRIC edge_to_source_predicate_ratio: %.6f\n",
            analysis.edgeToSourcePredicateRatio);
        std::fprintf(out, "METRIC shared_prefix_fraction: %.6f\n",
            analysis.sharedPrefixFraction);
        std::fprintf(out, "METRIC root_min_rules_per_edge: %u\n", analysis.rootMinRules);
        std::fprintf(out, "METRIC root_mean_rules_per_edge: %.3f\n", analysis.rootMeanRules);
        std::fprintf(out, "METRIC root_max_rules_per_edge: %u\n", analysis.rootMaxRules);
        std::fprintf(out, "METRIC nodes_with_overlapping_edges: %u\n",
            analysis.nodesWithOverlappingEdges);
        std::fprintf(out, "METRIC sibling_overlap_pairs: %llu\n",
            static_cast<unsigned long long>(analysis.siblingOverlapPairs));
        std::fprintf(out, "METRIC root_overlap_pairs: %llu\n",
            static_cast<unsigned long long>(analysis.rootOverlapPairs));

        const auto buckets = collectOpenTypeShapingRuleTrieRootBuckets(trie);
        const size_t bucketCount = std::min<size_t>(buckets.size(), 12);

        std::fprintf(out, "SECTION_BEGIN name=root_buckets\n");

        for (size_t i = 0; i < bucketCount; ++i)
        {
            const auto& bucket = buckets[i];

            std::fprintf(out,
                "ROOT_BUCKET rank=%zu | predicate=%u | rules=%u | ranges=%u | members=%u\n",
                i + 1, bucket.predicate, bucket.subtreeRuleCount,
                bucket.rangeCount, bucket.memberCount);
        }

        std::fprintf(out, "SECTION_END\n");
        std::fprintf(out, "RULE_TRIE_END\n");
    }

    static inline bool testOpenTypeShapingRuleTrieFace(
        const FontFace& face, FILE* out = stdout)
    {
        OpenTypeShapingIR gsubIR;
        OpenTypeShapingIR gposIR;
        OpenTypeShapingWorkloadCorpusSummary compileSummary;

        if (!compileOpenTypeShapingWorkloadFace(face, gsubIR, gposIR, compileSummary))
            return false;

        const char* fullName = face.fullName();
        const char* familyName = face.familyName();
        const char* label =
            fullName && *fullName ? fullName :
            familyName && *familyName ? familyName :
            "(unnamed)";

        bool any = false;

        OpenTypeShapingRuleTrieSelection contextSelection;

        if (selectOpenTypeShapingRuleTrieContextSubtable(gsubIR, contextSelection))
        {
            const auto& lookup = gsubIR.lookups[contextSelection.lookup];
            const auto& subtable =
                gsubIR.gsubContextSubtables[lookup.payloadOffset + contextSelection.subtableIndex];

            OpenTypeShapingRuleTrie trie;
            OpenTypeShapingRuleTrieAnalysis analysis;

            if (!compileOpenTypeShapingRuleTrieContextSubtable(
                gsubIR, subtable, trie, analysis))
            {
                return false;
            }

            printOpenTypeShapingRuleTrieAnalysis(
                label, "GSUB_CONTEXT",
                contextSelection, trie, analysis, out);

            any = true;
        }

        OpenTypeShapingRuleTrieSelection chainSelection;

        if (selectOpenTypeShapingRuleTrieChainContextSubtable(gsubIR, chainSelection))
        {
            const auto& lookup = gsubIR.lookups[chainSelection.lookup];
            const auto& subtable =
                gsubIR.gsubChainContextSubtables[lookup.payloadOffset + chainSelection.subtableIndex];

            OpenTypeShapingRuleTrie trie;
            OpenTypeShapingRuleTrieAnalysis analysis;

            if (!compileOpenTypeShapingRuleTrieChainContextSubtable(
                gsubIR, subtable, trie, analysis))
            {
                return false;
            }

            printOpenTypeShapingRuleTrieAnalysis(
                label, "GSUB_CHAIN_CONTEXT",
                chainSelection, trie, analysis, out);

            any = true;
        }

        return any;
    }

    static inline bool testOpenTypeShapingRuleTrieFile(
        const std::filesystem::path& filename, FILE* out = stdout)
    {
        FontResource resource;

        if (!readByteResource(filename, resource))
        {
            std::fprintf(out, "RULE_TRIE_FILE status=missing | file=%s\n",
                filename.string().c_str());
            return false;
        }

        OpenTypeContainer container(std::move(resource));

        if (!container.isValid())
        {
            std::fprintf(out, "RULE_TRIE_FILE status=invalid | file=%s\n",
                filename.string().c_str());
            return false;
        }

        bool passed = false;
        FontFaceView view;

        while (container(view))
        {
            FontFace face = parseFontFace(std::move(view));

            if (!face)
                continue;

            passed |= testOpenTypeShapingRuleTrieFace(face, out);
        }

        return passed;
    }

    static inline bool testOpenTypeShapingRuleTrieExperiment(
        const char* fontRoot, FILE* out = stdout)
    {
        if (!fontRoot || !*fontRoot)
            return false;

        const std::filesystem::path root(fontRoot);

        const std::filesystem::path dives =
            root / "ofl" / "notoserifdivesakuru" /
            "NotoSerifDivesAkuru-Regular.ttf";

        const std::filesystem::path nastaliq =
            root / "ofl" / "notonastaliqurdu" /
            "NotoNastaliqUrdu[wght].ttf";

        std::fprintf(out, "RULE_TRIE_EXPERIMENT_BEGIN\n");
        std::fprintf(out, "font_root: %s\n", root.string().c_str());

        const bool divesPassed =
            testOpenTypeShapingRuleTrieFile(dives, out);

        const bool nastaliqPassed =
            testOpenTypeShapingRuleTrieFile(nastaliq, out);

        std::fprintf(out,
            "RULE_TRIE_EXPERIMENT_END status=%s\n",
            divesPassed && nastaliqPassed ? "pass" : "fail");

        return divesPassed && nastaliqPassed;
    }
}
