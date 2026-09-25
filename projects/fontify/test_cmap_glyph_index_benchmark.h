// test_cmap_glyph_index_benchmark.h
#pragma once

#include "test_core.h"

#include "font_directory_view.h"
#include "opentype_cmap_view.h"
#include "opentype_name_view.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>


namespace waavs
{
    struct CmapBenchmarkCandidate
    {
        FontResource resource;
        size_t faceOffset{ 0 };

        std::string name;
        std::string location;

        uint16_t format{ 0 };
        uint32_t entryCount{ 0 };

        explicit operator bool() const noexcept
        {
            return bool(resource);
        }

        FontFaceView view() const noexcept
        {
            return FontFaceView(resource, faceOffset);
        }
    };


    struct CmapBenchmarkSelection
    {
        CmapBenchmarkCandidate smallestFormat4;
        CmapBenchmarkCandidate largestFormat4;
        CmapBenchmarkCandidate smallestFormat12;
        CmapBenchmarkCandidate largestFormat12;
    };


    struct CmapBenchmarkHistogram
    {
        static constexpr size_t kBucketCount = 9;

        std::array<size_t, kBucketCount> format4{};
        std::array<size_t, kBucketCount> format12{};

        static size_t bucket(uint32_t count) noexcept
        {
            if (count <= 8) return 0;
            if (count <= 16) return 1;
            if (count <= 32) return 2;
            if (count <= 64) return 3;
            if (count <= 128) return 4;
            if (count <= 256) return 5;
            if (count <= 512) return 6;
            if (count <= 1024) return 7;
            return 8;
        }

        void add(uint16_t format, uint32_t count) noexcept
        {
            if (format == 4)
                ++format4[bucket(count)];
            else if (format == 12)
                ++format12[bucket(count)];
        }

        static const char* bucketName(size_t index) noexcept
        {
            static constexpr const char* kNames[kBucketCount] =
            {
                "1-8",
                "9-16",
                "17-32",
                "33-64",
                "65-128",
                "129-256",
                "257-512",
                "513-1024",
                "1025+"
            };

            return index < kBucketCount ? kNames[index] : "?";
        }

        void print() const noexcept
        {
            std::printf(
                "\ncmap structure histogram\n"
                "\n"
                "  Entries       Format 4    Format 12\n"
                "  -----------------------------------\n");

            for (size_t i = 0; i < kBucketCount; ++i)
            {
                std::printf(
                    "  %-10s %10zu %12zu\n",
                    bucketName(i),
                    format4[i],
                    format12[i]);
            }
        }
    };


    static bool cmapBenchmarkEntryCount(const ByteSpan& cmapData, uint32_t offset, uint32_t length, uint16_t format, uint32_t& count) noexcept
    {
        count = 0;

        if (offset > cmapData.size() || length > cmapData.size() - offset)
            return false;

        OpenTypeByteStream cmap(cmapData);
        auto subtable = cmap.subStream(offset, length);

        if (!subtable.isValid())
            return false;

        if (format == 4)
        {
            auto header = subtable.subStream(6);

            if (!header.isValid())
                return false;

            uint16_t segCountX2 = 0;

            if (!header.readUInt16(segCountX2))
                return false;

            if (segCountX2 == 0 || (segCountX2 & 1u) != 0)
                return false;

            count = uint32_t(segCountX2) >> 1;
            return true;
        }

        if (format == 12)
        {
            auto header = subtable.subStream(12);

            if (!header.isValid())
                return false;

            uint32_t numGroups = 0;

            if (!header.readUInt32(numGroups))
                return false;

            if (numGroups > header.remaining() / 12u)
                return false;

            count = numGroups;
            return true;
        }

        return false;
    }


    static void cmapBenchmarkUpdateCandidate(CmapBenchmarkCandidate& candidate, const FontFaceView& view, uint16_t format, uint32_t entryCount, bool selectSmallest)
    {
        if (candidate)
        {
            if (selectSmallest && entryCount >= candidate.entryCount)
                return;

            if (!selectSmallest && entryCount <= candidate.entryCount)
                return;
        }

        NameView name(view);

        const char* fullName =
            name && name.fullName()
            ? name.fullName()
            : "(unnamed)";

        const char* location =
            view.sourceLocation()
            ? view.sourceLocation()
            : "(unknown)";

        candidate.resource = view.resource();
        candidate.faceOffset = view.faceOffset();
        candidate.name = fullName;
        candidate.location = location;
        candidate.format = format;
        candidate.entryCount = entryCount;
    }


    static void cmapBenchmarkPrintCandidate(const char* label, const CmapBenchmarkCandidate& candidate)
    {
        if (!candidate)
        {
            std::printf(
                "  %-20s (none)\n",
                label);

            return;
        }

        std::printf(
            "  %-20s format=%u  entries=%u\n"
            "    Face: %s\n"
            "    File: %s\n",
            label,
            static_cast<unsigned>(candidate.format),
            static_cast<unsigned>(candidate.entryCount),
            candidate.name.c_str(),
            candidate.location.c_str());
    }


    static bool cmapBenchmarkFindMappedCodepoints(const CmapView& cmap, std::vector<uint32_t>& codepoints, size_t desiredCount)
    {
        codepoints.clear();

        if (!cmap || desiredCount == 0)
            return false;

        static constexpr uint32_t kUnicodeLimit = 0x110000;
        static constexpr uint32_t kStride = 4093;

        uint32_t cp = 0;

        for (uint32_t i = 0; i < kUnicodeLimit && codepoints.size() < desiredCount; ++i)
        {
            cp += kStride;

            if (cp >= kUnicodeLimit)
                cp -= kUnicodeLimit;

            if (cp >= 0xD800u && cp <= 0xDFFFu)
                continue;

            if (cmap.hasGlyph(cp))
                codepoints.push_back(cp);
        }

        // Sparse fonts may not produce enough samples using the stride.
        if (codepoints.size() < desiredCount)
        {
            for (uint32_t scalar = 0; scalar < kUnicodeLimit && codepoints.size() < desiredCount; ++scalar)
            {
                if (scalar >= 0xD800u && scalar <= 0xDFFFu)
                    continue;

                if (cmap.hasGlyph(scalar))
                    codepoints.push_back(scalar);
            }
        }

        return !codepoints.empty();
    }


    static bool cmapBenchmarkFindMissingCodepoints(const CmapView& cmap, std::vector<uint32_t>& codepoints, size_t desiredCount)
    {
        codepoints.clear();

        if (!cmap || desiredCount == 0)
            return false;

        static constexpr uint32_t kUnicodeLimit = 0x110000;
        static constexpr uint32_t kStride = 6151;

        uint32_t cp = 0;

        for (uint32_t i = 0; i < kUnicodeLimit && codepoints.size() < desiredCount; ++i)
        {
            cp += kStride;

            if (cp >= kUnicodeLimit)
                cp -= kUnicodeLimit;

            if (cp >= 0xD800u && cp <= 0xDFFFu)
                continue;

            if (!cmap.hasGlyph(cp))
                codepoints.push_back(cp);
        }

        return !codepoints.empty();
    }


    static double cmapBenchmarkRunLookups(const CmapView& cmap, const std::vector<uint32_t>& codepoints, size_t iterations, uint64_t& checksum)
    {
        using Clock = std::chrono::steady_clock;

        if (!cmap || codepoints.empty() || iterations == 0)
            return 0.0;

        uint64_t sum = 0;

        const auto start = Clock::now();

        for (size_t iteration = 0; iteration < iterations; ++iteration)
        {
            for (uint32_t cp : codepoints)
                sum += cmap.glyphIndex(cp);
        }

        const auto end = Clock::now();

        checksum += sum;

        const size_t lookupCount = iterations * codepoints.size();

        const double elapsedNs =
            std::chrono::duration<double, std::nano>(end - start).count();

        return elapsedNs / static_cast<double>(lookupCount);
    }


    static void cmapBenchmarkRunCandidate(const char* label, const CmapBenchmarkCandidate& candidate, size_t iterations, uint64_t& checksum)
    {
        if (!candidate)
            return;

        FontFaceView view = candidate.view();

        if (!view)
            return;

        CmapView cmap(view);

        if (!cmap)
            return;

        std::vector<uint32_t> hits;
        std::vector<uint32_t> misses;

        cmapBenchmarkFindMappedCodepoints(cmap, hits, 256);
        cmapBenchmarkFindMissingCodepoints(cmap, misses, 256);

        const double hitNs =
            cmapBenchmarkRunLookups(
                cmap,
                hits,
                iterations,
                checksum);

        const double missNs =
            cmapBenchmarkRunLookups(
                cmap,
                misses,
                iterations,
                checksum);

        std::vector<uint32_t> mixed;
        const size_t mixedCount = std::min(hits.size(), misses.size());

        mixed.reserve(mixedCount * 2);

        for (size_t i = 0; i < mixedCount; ++i)
        {
            mixed.push_back(hits[i]);
            mixed.push_back(misses[i]);
        }

        const double mixedNs =
            cmapBenchmarkRunLookups(
                cmap,
                mixed,
                iterations,
                checksum);

        std::printf(
            "\n%s\n"
            "  Face:          %s\n"
            "  Format:        %u\n"
            "  Entries:       %u\n"
            "  Hit samples:   %zu\n"
            "  Miss samples:  %zu\n"
            "  Hit lookup:    %8.3f ns\n"
            "  Miss lookup:   %8.3f ns\n"
            "  Mixed lookup:  %8.3f ns\n",
            label,
            candidate.name.c_str(),
            static_cast<unsigned>(candidate.format),
            static_cast<unsigned>(candidate.entryCount),
            hits.size(),
            misses.size(),
            hitNs,
            missNs,
            mixedNs);
    }


    static bool testCmapGlyphIndexBenchmark(const char* fontDirectory, size_t iterations = 10000)
    {
        if (!fontDirectory || !*fontDirectory)
            return false;

        FontDirectoryView fonts(fontDirectory, true);
        FontFaceView view;

        CmapBenchmarkSelection selection;
        CmapBenchmarkHistogram histogram;

        size_t facesScanned = 0;
        size_t cmapFaces = 0;
        size_t format4Subtables = 0;
        size_t format12Subtables = 0;

        while (fonts(view))
        {
            ++facesScanned;

            CmapView cmap(view);

            if (!cmap)
                continue;

            ++cmapFaces;

            const TableRecord* cmapTable = view.getTable(TagConstants::CMAP);

            if (!cmapTable)
                continue;

            cmap.forEachEncoding(
                [&](uint16_t, uint16_t, uint16_t format, uint32_t offset, uint32_t length) noexcept
                {
                    if (format != 4 && format != 12)
                        return;

                    uint32_t entryCount = 0;

                    if (!cmapBenchmarkEntryCount(cmapTable->data, offset, length, format, entryCount))
                        return;

                    histogram.add(format, entryCount);

                    if (format == 4)
                    {
                        ++format4Subtables;

                        cmapBenchmarkUpdateCandidate(
                            selection.smallestFormat4,
                            view,
                            format,
                            entryCount,
                            true);

                        cmapBenchmarkUpdateCandidate(
                            selection.largestFormat4,
                            view,
                            format,
                            entryCount,
                            false);
                    }
                    else
                    {
                        ++format12Subtables;

                        cmapBenchmarkUpdateCandidate(
                            selection.smallestFormat12,
                            view,
                            format,
                            entryCount,
                            true);

                        cmapBenchmarkUpdateCandidate(
                            selection.largestFormat12,
                            view,
                            format,
                            entryCount,
                            false);
                    }
                });
        }

        if (cmapFaces == 0)
        {
            std::printf(
                "cmap glyphIndex benchmark: FAIL: no readable cmap faces found\n"
                "  Directory: %s\n",
                fontDirectory);

            return false;
        }

        std::printf(
            "cmap glyphIndex benchmark discovery\n"
            "  Directory:           %s\n"
            "  Faces scanned:       %zu\n"
            "  Cmap faces:          %zu\n"
            "  Format 4 subtables:  %zu\n"
            "  Format 12 subtables: %zu\n",
            fontDirectory,
            facesScanned,
            cmapFaces,
            format4Subtables,
            format12Subtables);

        histogram.print();

        std::printf("\nSelected benchmark fonts\n\n");

        cmapBenchmarkPrintCandidate(
            "Small format 4",
            selection.smallestFormat4);

        cmapBenchmarkPrintCandidate(
            "Large format 4",
            selection.largestFormat4);

        cmapBenchmarkPrintCandidate(
            "Small format 12",
            selection.smallestFormat12);

        cmapBenchmarkPrintCandidate(
            "Large format 12",
            selection.largestFormat12);

        uint64_t checksum = 0;

        std::printf(
            "\nglyphIndex benchmark\n"
            "  Iterations/sample: %zu\n",
            iterations);

        cmapBenchmarkRunCandidate(
            "Small format 4",
            selection.smallestFormat4,
            iterations,
            checksum);

        cmapBenchmarkRunCandidate(
            "Large format 4",
            selection.largestFormat4,
            iterations,
            checksum);

        cmapBenchmarkRunCandidate(
            "Small format 12",
            selection.smallestFormat12,
            iterations,
            checksum);

        cmapBenchmarkRunCandidate(
            "Large format 12",
            selection.largestFormat12,
            iterations,
            checksum);

        std::printf(
            "\ncmap glyphIndex benchmark: PASS\n"
            "  Checksum: %llu\n",
            static_cast<unsigned long long>(checksum));

        return true;
    }
}