// test_font_run_itemizer.h

#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <vector>

#include "font_run_itemizer.h"


namespace waavs
{
    // ========================================================================
    // FontRunItemizerTestFaceData
    //
    // Minimal deterministic IProvideFontFaceData implementation.
    //
    // Every code point listed in mGlyphs maps to a synthetic non-zero glyph
    // index. Everything else maps to glyph zero.
    // ========================================================================

    class FontRunItemizerTestFaceData final : public IProvideFontFaceData
    {
    public:
        explicit FontRunItemizerTestFaceData(
            std::initializer_list<uint32_t> glyphs)
            : mGlyphs(glyphs)
        {}


        FontName sourceLocation() const noexcept override
        {
            return nullptr;
        }


        FontName familyName() const noexcept override
        {
            return nullptr;
        }


        FontName subfamilyName() const noexcept override
        {
            return nullptr;
        }


        FontName fullName() const noexcept override
        {
            return nullptr;
        }


        FontName postScriptName() const noexcept override
        {
            return nullptr;
        }


        FontFaceProperties properties() const noexcept override
        {
            return {};
        }


        uint32_t glyphCount() const noexcept override
        {
            return static_cast<uint32_t>(mGlyphs.size()) + 1u;
        }


        uint16_t unitsPerEm() const noexcept override
        {
            return 1000;
        }


        uint32_t glyphIndex(uint32_t codepoint) const noexcept override
        {
            for (size_t i = 0; i < mGlyphs.size(); ++i)
            {
                if (mGlyphs[i] == codepoint)
                    return static_cast<uint32_t>(i) + 1u;
            }

            return 0;
        }


        bool supportsCodepoint(uint32_t codepoint) const noexcept override
        {
            return glyphIndex(codepoint) != 0;
        }


        const UnicodeCoverage& unicodeCoverage() const noexcept override
        {
            return mCoverage;
        }


    private:
        std::vector<uint32_t> mGlyphs{};
        UnicodeCoverage mCoverage{};
    };


    // ========================================================================
    // makeFontRunItemizerTestFace
    // ========================================================================

    static FontFace makeFontRunItemizerTestFace(
        std::initializer_list<uint32_t> glyphs)
    {
        return FontFace(
            std::make_shared<FontRunItemizerTestFaceData>(glyphs));
    }


    // ========================================================================
    // FontRunItemizerTestCandidates
    //
    // Minimal deterministic IProvideFontFaces implementation.
    //
    // Candidate order is insertion order.
    // ========================================================================

    class FontRunItemizerTestCandidates final : public IProvideFontFaces
    {
    public:
        void add(const FontFace& face)
        {
            if (face)
                mFaces.push_back(face);
        }


        size_t fontFaceCount() const noexcept override
        {
            return mFaces.size();
        }


        FontFace fontFace(size_t index) const noexcept override
        {
            if (index >= mFaces.size())
                return {};

            return mFaces[index];
        }


    private:
        std::vector<FontFace> mFaces{};
    };


    // ========================================================================
    // FontRunItemizerTestInput
    //
    // Own the scalar and cluster storage used by a synthetic ShapingRunView.
    // ========================================================================

    struct FontRunItemizerTestInput
    {
        std::vector<UnicodeScalar> scalars{};
        std::vector<ShapingCluster> clusters{};

        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };
        uint8_t bidiLevel{ 0 };


        void addCluster(std::initializer_list<uint32_t> values,
            ScalarIndex normalizedBegin, SourceRange source)
        {
            ShapingCluster cluster{};

            cluster.scalarOffset =
                static_cast<uint32_t>(scalars.size());

            cluster.scalarCount =
                static_cast<uint32_t>(values.size());

            cluster.normalizedBegin =
                normalizedBegin;

            cluster.source =
                source;


            for (uint32_t value : values)
            {
                UnicodeScalar scalar{};

                scalar.value = value;
                scalar.source = source;

                scalars.push_back(scalar);
            }


            clusters.push_back(cluster);
        }


        [[nodiscard]]
        ShapingRunView view() const noexcept
        {
            ShapingRunView result{};

            result.scalars =
                scalars.empty()
                ? nullptr
                : scalars.data();

            result.scalarCount =
                static_cast<uint32_t>(scalars.size());

            result.clusters =
                clusters.empty()
                ? nullptr
                : clusters.data();

            result.clusterCount =
                static_cast<uint32_t>(clusters.size());

            result.script =
                script;

            result.bidiLevel =
                bidiLevel;


            if (!clusters.empty())
            {
                result.normalizedBegin =
                    clusters.front().normalizedBegin;

                result.source =
                    clusters.front().source;


                for (size_t i = 1; i < clusters.size(); ++i)
                {
                    if (!clusters[i].source.valid())
                        continue;

                    if (!result.source.valid())
                    {
                        result.source =
                            clusters[i].source;

                        continue;
                    }

                    if (clusters[i].source.begin < result.source.begin)
                        result.source.begin = clusters[i].source.begin;

                    if (clusters[i].source.end > result.source.end)
                        result.source.end = clusters[i].source.end;
                }
            }


            return result;
        }
    };


    // ========================================================================
    // testFontRunItemizer
    // ========================================================================

    static bool testFontRunItemizer(const ByteSpan& databaseData)
    {
        UnicodeDatabase database(databaseData);


        if (!database)
        {
            std::printf(
                "Font run itemizer: FAIL\n"
                "  Unable to initialize Unicode database\n");

            return false;
        }


        uint32_t cases = 0;
        uint32_t passed = 0;


        auto fail = [&](const char* message) -> bool
            {
                std::printf(
                    "Font run itemizer: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Preferred-face restoration.
        //
        // Candidate priority:
        //
        //      A
        //      B
        //
        // Coverage:
        //
        //      clusters:  0 1 2 3 4 5
        //      A:         + + - - + +
        //      B:         - - + + - -
        //
        // Expected:
        //
        //      A A | B B | A A
        //
        // This proves candidate search restarts at candidate zero for every
        // cluster.
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041,
                    0x0042,
                    0x0043,
                    0x0044
                    });

            const FontFace faceB =
                makeFontRunItemizerTestFace({
                    0x03B1,
                    0x03B2
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);
            candidates.add(faceB);


            FontRunItemizerTestInput input;

            input.bidiLevel = 0;

            input.addCluster(
                { 0x0041 },
                100,
                SourceRange{ 10, 11 });

            input.addCluster(
                { 0x0042 },
                101,
                SourceRange{ 11, 12 });

            input.addCluster(
                { 0x03B1 },
                102,
                SourceRange{ 12, 14 });

            input.addCluster(
                { 0x03B2 },
                103,
                SourceRange{ 14, 16 });

            input.addCluster(
                { 0x0043 },
                104,
                SourceRange{ 16, 17 });

            input.addCluster(
                { 0x0044 },
                105,
                SourceRange{ 17, 18 });


            const ShapingRunView shapingRun =
                input.view();


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            FontRunView run{};


            // ---------------------------------------------------------------
            // Run 1: A A
            // ---------------------------------------------------------------

            if (!itemizer(run))
                return fail("case 1 did not emit first run");

            if (run.face != faceA)
                return fail("case 1 first run wrong face");

            if (!run.completeCoverage)
                return fail("case 1 first run incomplete coverage");

            if (run.scalarCount != 2 ||
                run.clusterCount != 2)
            {
                return fail("case 1 first run size");
            }

            if (run.scalars[0].value != 0x0041 ||
                run.scalars[1].value != 0x0042)
            {
                return fail("case 1 first run scalar contents");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[1].scalarOffset != 1)
            {
                return fail("case 1 first run cluster offsets");
            }

            if (run.normalizedBegin != 100)
                return fail("case 1 first run normalizedBegin");

            if (run.source.begin != 10 ||
                run.source.end != 12)
            {
                return fail("case 1 first run source range");
            }

            if (run.script != shapingRun.script ||
                run.bidiLevel != shapingRun.bidiLevel)
            {
                return fail("case 1 first run shaping metadata");
            }


            // ---------------------------------------------------------------
            // Run 2: B B
            //
            // Original scalar offsets were 2 and 3.
            // They must be rebased to 0 and 1.
            // ---------------------------------------------------------------

            if (!itemizer(run))
                return fail("case 1 did not emit second run");

            if (run.face != faceB)
                return fail("case 1 second run wrong face");

            if (!run.completeCoverage)
                return fail("case 1 second run incomplete coverage");

            if (run.scalarCount != 2 ||
                run.clusterCount != 2)
            {
                return fail("case 1 second run size");
            }

            if (run.scalars[0].value != 0x03B1 ||
                run.scalars[1].value != 0x03B2)
            {
                return fail("case 1 second run scalar contents");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[1].scalarOffset != 1)
            {
                return fail("case 1 second run cluster rebasing");
            }

            if (run.normalizedBegin != 102)
                return fail("case 1 second run normalizedBegin");

            if (run.source.begin != 12 ||
                run.source.end != 16)
            {
                return fail("case 1 second run source range");
            }


            // ---------------------------------------------------------------
            // Run 3: A A
            //
            // This proves the preferred candidate was restored.
            // ---------------------------------------------------------------

            if (!itemizer(run))
                return fail("case 1 did not emit third run");

            if (run.face != faceA)
                return fail("case 1 preferred face was not restored");

            if (!run.completeCoverage)
                return fail("case 1 third run incomplete coverage");

            if (run.scalarCount != 2 ||
                run.clusterCount != 2)
            {
                return fail("case 1 third run size");
            }

            if (run.scalars[0].value != 0x0043 ||
                run.scalars[1].value != 0x0044)
            {
                return fail("case 1 third run scalar contents");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[1].scalarOffset != 1)
            {
                return fail("case 1 third run cluster rebasing");
            }

            if (run.normalizedBegin != 104)
                return fail("case 1 third run normalizedBegin");

            if (run.source.begin != 16 ||
                run.source.end != 18)
            {
                return fail("case 1 third run source range");
            }


            if (itemizer(run))
                return fail("case 1 emitted unexpected fourth run");

            if (itemizer.status() != TextStreamStatus::End)
                return fail("case 1 did not enter End state");


            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // Whole-cluster fallback.
        //
        // A supports the base but not the combining mark.
        // B supports both.
        //
        // The entire cluster must be assigned to B.
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041,
                    0x0042
                    });

            const FontFace faceB =
                makeFontRunItemizerTestFace({
                    0x0041,
                    0x0301
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);
            candidates.add(faceB);


            FontRunItemizerTestInput input;

            input.addCluster(
                { 0x0041, 0x0301 },
                200,
                SourceRange{ 20, 23 });


            const ShapingRunView shapingRun =
                input.view();


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            FontRunView run{};


            if (!itemizer(run))
                return fail("case 2 did not emit run");

            if (run.face != faceB)
                return fail("case 2 did not fallback whole cluster to B");

            if (!run.completeCoverage)
                return fail("case 2 unexpectedly incomplete");

            if (run.scalarCount != 2 ||
                run.clusterCount != 1)
            {
                return fail("case 2 run size");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[0].scalarCount != 2)
            {
                return fail("case 2 cluster geometry");
            }

            if (run.scalars[0].value != 0x0041 ||
                run.scalars[1].value != 0x0301)
            {
                return fail("case 2 scalar contents");
            }


            if (itemizer(run))
                return fail("case 2 emitted unexpected second run");

            if (itemizer.status() != TextStreamStatus::End)
                return fail("case 2 did not enter End state");


            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // Unsupported middle cluster.
        //
        // No candidate supports U+2603.
        //
        // Candidate A is the first valid candidate, so it becomes the
        // last-resort face. Since A is also selected before and after the
        // unsupported cluster, all three clusters remain one FontRunView.
        //
        // completeCoverage must be false.
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041,
                    0x0042
                    });

            const FontFace faceB =
                makeFontRunItemizerTestFace({
                    0x03B1
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);
            candidates.add(faceB);


            FontRunItemizerTestInput input;

            input.addCluster(
                { 0x0041 },
                300,
                SourceRange{ 30, 31 });

            input.addCluster(
                { 0x2603 },
                301,
                SourceRange{ 31, 34 });

            input.addCluster(
                { 0x0042 },
                302,
                SourceRange{ 34, 35 });


            const ShapingRunView shapingRun =
                input.view();


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            FontRunView run{};


            if (!itemizer(run))
                return fail("case 3 did not emit run");

            if (run.face != faceA)
                return fail("case 3 last-resort face was not A");

            if (run.completeCoverage)
                return fail("case 3 failed to record incomplete coverage");

            if (run.scalarCount != 3 ||
                run.clusterCount != 3)
            {
                return fail("case 3 run was unnecessarily split");
            }

            if (run.scalars[0].value != 0x0041 ||
                run.scalars[1].value != 0x2603 ||
                run.scalars[2].value != 0x0042)
            {
                return fail("case 3 scalar contents");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[1].scalarOffset != 1 ||
                run.clusters[2].scalarOffset != 2)
            {
                return fail("case 3 cluster offsets");
            }

            if (run.source.begin != 30 ||
                run.source.end != 35)
            {
                return fail("case 3 source envelope");
            }


            if (itemizer(run))
                return fail("case 3 emitted unexpected second run");

            if (itemizer.status() != TextStreamStatus::End)
                return fail("case 3 did not enter End state");


            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Multi-scalar cluster rebasing after an earlier font run.
        //
        // Original:
        //
        //      cluster 0: offset 0, count 1 -> A
        //      cluster 1: offset 1, count 2 -> B
        //
        // The B run must expose:
        //
        //      offset 0, count 2
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041
                    });

            const FontFace faceB =
                makeFontRunItemizerTestFace({
                    0x0065,
                    0x0301
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);
            candidates.add(faceB);


            FontRunItemizerTestInput input;

            input.addCluster(
                { 0x0041 },
                400,
                SourceRange{ 40, 41 });

            input.addCluster(
                { 0x0065, 0x0301 },
                401,
                SourceRange{ 41, 44 });


            const ShapingRunView shapingRun =
                input.view();


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            FontRunView run{};


            if (!itemizer(run))
                return fail("case 4 did not emit first run");

            if (run.face != faceA ||
                run.scalarCount != 1 ||
                run.clusterCount != 1)
            {
                return fail("case 4 first run");
            }


            if (!itemizer(run))
                return fail("case 4 did not emit second run");

            if (run.face != faceB)
                return fail("case 4 second run wrong face");

            if (run.scalarCount != 2 ||
                run.clusterCount != 1)
            {
                return fail("case 4 second run size");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[0].scalarCount != 2)
            {
                return fail("case 4 multi-scalar cluster was not rebased");
            }

            if (run.scalars[0].value != 0x0065 ||
                run.scalars[1].value != 0x0301)
            {
                return fail("case 4 second run scalar contents");
            }

            if (run.normalizedBegin != 401)
                return fail("case 4 second run normalizedBegin");

            if (run.source.begin != 41 ||
                run.source.end != 44)
            {
                return fail("case 4 second run source range");
            }


            if (itemizer(run))
                return fail("case 4 emitted unexpected third run");

            if (itemizer.status() != TextStreamStatus::End)
                return fail("case 4 did not enter End state");


            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // ZWJ does not independently require a nominal glyph.
        //
        // A supports the visible scalars but not U+200D.
        // The cluster should still remain fully covered by A.
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041,
                    0x0042
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);


            FontRunItemizerTestInput input;

            input.addCluster(
                { 0x0041, 0x200D, 0x0042 },
                500,
                SourceRange{ 50, 55 });


            const ShapingRunView shapingRun =
                input.view();


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            FontRunView run{};


            if (!itemizer(run))
                return fail("case 5 did not emit run");

            if (run.face != faceA)
                return fail("case 5 wrong face");

            if (!run.completeCoverage)
                return fail("case 5 ZWJ caused incomplete coverage");

            if (run.scalarCount != 3 ||
                run.clusterCount != 1)
            {
                return fail("case 5 run size");
            }

            if (run.clusters[0].scalarOffset != 0 ||
                run.clusters[0].scalarCount != 3)
            {
                return fail("case 5 cluster geometry");
            }


            if (itemizer(run))
                return fail("case 5 emitted unexpected second run");

            if (itemizer.status() != TextStreamStatus::End)
                return fail("case 5 did not enter End state");


            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Empty candidate provider is a configuration failure.
        // ====================================================================

        {
            ++cases;


            FontRunItemizerTestCandidates candidates;


            FontRunItemizerTestInput input;

            input.addCluster(
                { 0x0041 },
                600,
                SourceRange{ 60, 61 });


            const ShapingRunView shapingRun =
                input.view();


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            if (itemizer.status() != TextStreamStatus::InvalidInput)
                return fail("case 6 empty candidates not rejected");


            FontRunView run{};

            if (itemizer(run))
                return fail("case 6 emitted run with no candidates");


            ++passed;
        }


        // ====================================================================
        // Case 7
        //
        // Malformed cluster partition.
        //
        // Cluster 1 begins at scalar offset 2, leaving scalar 1 uncovered.
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041,
                    0x0042,
                    0x0043
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);


            UnicodeScalar scalars[3]{};

            scalars[0].value = 0x0041;
            scalars[1].value = 0x0042;
            scalars[2].value = 0x0043;


            ShapingCluster clusters[2]{};

            clusters[0].scalarOffset = 0;
            clusters[0].scalarCount = 1;

            clusters[1].scalarOffset = 2;
            clusters[1].scalarCount = 1;


            ShapingRunView shapingRun{};

            shapingRun.scalars = scalars;
            shapingRun.scalarCount = 3;

            shapingRun.clusters = clusters;
            shapingRun.clusterCount = 2;


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            if (itemizer.status() != TextStreamStatus::InvalidInput)
                return fail("case 7 malformed partition not rejected");


            FontRunView run{};

            if (itemizer(run))
                return fail("case 7 emitted malformed run");


            ++passed;
        }


        // ====================================================================
        // Case 8
        //
        // Empty shaping run is clean End.
        // ====================================================================

        {
            ++cases;


            const FontFace faceA =
                makeFontRunItemizerTestFace({
                    0x0041
                    });


            FontRunItemizerTestCandidates candidates;

            candidates.add(faceA);


            ShapingRunView shapingRun{};


            FontRunItemizer itemizer(
                shapingRun,
                candidates,
                database);


            if (itemizer.status() != TextStreamStatus::End)
                return fail("case 8 empty run did not begin in End state");


            FontRunView run{};

            if (itemizer(run))
                return fail("case 8 empty run emitted output");


            ++passed;
        }


        // ====================================================================
        // Diagnostics
        // ====================================================================

        std::printf(
            "Font run itemizer: PASS\n"
            "  Cases:                   %u\n"
            "  Passed:                  %u\n"
            "  Priority restoration:    PASS\n"
            "  Whole-cluster fallback:  PASS\n"
            "  Incomplete coverage:     PASS\n"
            "  Cluster rebasing:        PASS\n"
            "  Provenance:              PASS\n"
            "  Failure paths:           PASS\n",
            cases,
            passed);


        return passed == cases;
    }


    // ========================================================================
    // Convenience filename overload
    // ========================================================================

    static bool testFontRunItemizer(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;


        if (!readFileData(
            databaseFilename,
            fileData))
        {
            std::printf(
                "Font run itemizer: FAIL: unable to read Unicode database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testFontRunItemizer(
            databaseData);
    }

} // namespace waavs