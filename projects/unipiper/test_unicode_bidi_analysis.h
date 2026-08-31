// test_unicode_bidi_analysis.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <utility>
#include <vector>


#include "../ucdbdemo/test_core.h"

#include "unicode_bidi_analysis.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // BidiTestScriptSource
    //
    // Synthetic ScriptGrapheme source used to exercise UnicodeBidiStream.
    //
    // The scalar buffer is deliberately reused on every pull. This reproduces
    // the borrowed lifetime behavior of the real upstream script stream and
    // verifies that UnicodeBidiStream promotes paragraph contents to owned
    // storage.
    // ========================================================================

    struct BidiTestSourceItem
    {
        std::vector<uint32_t> values{};

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};

        UnicodeScriptSet candidates{};
        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };
    };


    class BidiTestScriptSource
    {
    public:
        void add(std::initializer_list<uint32_t> values,
            ScalarIndex normalizedBegin, SourceRange source,
            UnicodeScriptSet candidates = {},
            UnicodeScriptIndex script = kUnicodeScriptIndexInvalid)
        {
            BidiTestSourceItem item{};

            item.values.assign(values.begin(), values.end());
            item.normalizedBegin = normalizedBegin;
            item.source = source;
            item.candidates = candidates;
            item.script = script;

            mItems.push_back(std::move(item));
        }


        void failAtEnd(bool value = true) noexcept
        {
            mFailAtEnd = value;
        }


        [[nodiscard]] TextStreamStatus status() const noexcept
        {
            return mStatus;
        }


        bool operator()(ScriptGrapheme& out)
        {
            out = {};

            if (mStatus != TextStreamStatus::Ready)
                return false;


            if (mIndex >= mItems.size())
            {
                mStatus = mFailAtEnd
                    ? TextStreamStatus::InvalidInput
                    : TextStreamStatus::End;

                return false;
            }


            const BidiTestSourceItem& item =
                mItems[mIndex++];


            // ---------------------------------------------------------------
            // Reuse this storage on every pull.
            // ---------------------------------------------------------------

            mScalars.clear();

            for (uint32_t value : item.values)
            {
                UnicodeScalar scalar{};

                scalar.value = value;
                scalar.source = item.source;

                mScalars.push_back(scalar);
            }


            out.grapheme.scalars =
                mScalars.empty()
                ? nullptr
                : mScalars.data();

            out.grapheme.scalarCount =
                static_cast<uint32_t>(mScalars.size());

            out.grapheme.normalizedBegin =
                item.normalizedBegin;

            out.grapheme.source =
                item.source;

            out.candidates =
                item.candidates;

            out.script =
                item.script;


            return true;
        }


    private:
        std::vector<BidiTestSourceItem> mItems{};
        std::vector<UnicodeScalar> mScalars{};

        size_t mIndex{ 0 };

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
        bool mFailAtEnd{ false };
    };


    // ========================================================================
    // testUnicodeBidiAnalysis
    //
    // UAX #9 milestones:
    //
    //      P1      paragraph collection
    //      P2/P3   paragraph embedding level
    //      X1-X8   explicit embeddings, overrides, and isolates
    //      X9      virtual removal of explicit formatting characters
    //      X10     isolating run sequences and sos/eos
    //      W1      nonspacing mark resolution
    //      W2      EN after AL resolution
    //
    // This intentionally does NOT test W3-W7/N/I resolution yet.
    // ========================================================================

    static bool testUnicodeBidiAnalysis(const ByteSpan& databaseData)
    {
        uint32_t helperCases = 0;
        uint32_t helperPassed = 0;

        uint32_t explicitCases = 0;
        uint32_t explicitPassed = 0;

        uint32_t x9Cases = 0;
        uint32_t x9Passed = 0;

        uint32_t x10Cases = 0;
        uint32_t x10Passed = 0;

        uint32_t w1Cases = 0;
        uint32_t w1Passed = 0;

        uint32_t w2Cases = 0;
        uint32_t w2Passed = 0;

        uint32_t w3Cases = 0;
        uint32_t w3Passed = 0;

        uint32_t streamCases = 0;
        uint32_t streamPassed = 0;


        auto fail = [](const char* message) -> bool
            {
                std::printf(
                    "Unicode bidi analysis: FAIL: %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Database
        // ====================================================================

        UnicodeDatabase database(databaseData);

        if (!database.valid())
            return fail("unable to attach Unicode database");

        if (!database.hasBidiClass())
            return fail("Unicode database has no Bidi_Class property");


        // ====================================================================
        // Standalone P2/P3 helper tests
        // ====================================================================

        auto checkParagraphLevel =
            [&](std::initializer_list<UnicodeBidiClass> types,
                UnicodeBidiParagraphDirection direction,
                UnicodeBidiLevel expected,
                const char* description) -> bool
            {
                ++helperCases;

                const UnicodeBidiLevel actual =
                    resolveBidiParagraphLevel(
                        types.begin(),
                        static_cast<uint32_t>(types.size()),
                        direction);


                if (actual != expected)
                {
                    std::printf(
                        "Unicode bidi analysis: FAIL: %s\n"
                        "  Expected paragraph level: %u\n"
                        "  Actual paragraph level:   %u\n",
                        description,
                        static_cast<unsigned>(expected),
                        static_cast<unsigned>(actual));

                    return false;
                }


                ++helperPassed;
                return true;
            };


        // --------------------------------------------------------------------
        // Basic P2/P3
        // --------------------------------------------------------------------

        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::LeftToRight
            },
            UnicodeBidiParagraphDirection::Auto,
            0,
            "Latin paragraph"))
        {
            return false;
        }


        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::ArabicLetter
            },
            UnicodeBidiParagraphDirection::Auto,
            1,
            "Arabic paragraph"))
        {
            return false;
        }


        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::ArabicLetter
            },
            UnicodeBidiParagraphDirection::Auto,
            1,
            "leading whitespace before Arabic"))
        {
            return false;
        }


        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::OtherNeutral
            },
            UnicodeBidiParagraphDirection::Auto,
            0,
            "neutral-only paragraph"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Higher-level paragraph direction
        // --------------------------------------------------------------------

        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::ArabicLetter
            },
            UnicodeBidiParagraphDirection::LeftToRight,
            0,
            "explicit LTR paragraph"))
        {
            return false;
        }


        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::LeftToRight
            },
            UnicodeBidiParagraphDirection::RightToLeft,
            1,
            "explicit RTL paragraph"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Isolate contents do not participate in P2.
        //
        // RLI L PDI R
        //
        // The L is inside an isolate. The first relevant strong type in the
        // surrounding paragraph is therefore R.
        // --------------------------------------------------------------------

        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::RightToLeft
            },
            UnicodeBidiParagraphDirection::Auto,
            1,
            "P2 isolate skipping"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Nested isolates.
        // --------------------------------------------------------------------

        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::RightToLeft
            },
            UnicodeBidiParagraphDirection::Auto,
            1,
            "nested isolate skipping"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Embedding contents remain visible to P2.
        // --------------------------------------------------------------------

        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat
            },
            UnicodeBidiParagraphDirection::Auto,
            1,
            "embedding contents participate in P2"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Unterminated isolate extends to the end of the paragraph.
        // The L is therefore ignored and P3 defaults to level zero.
        // --------------------------------------------------------------------

        if (!checkParagraphLevel(
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight
            },
            UnicodeBidiParagraphDirection::Auto,
            0,
            "unterminated isolate"))
        {
            return false;
        }


        // ====================================================================
        // skipBidiIsolate
        // ====================================================================

        {
            ++helperCases;

            const UnicodeBidiClass types[] =
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight,

                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,

                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::RightToLeft
            };


            const uint32_t matchingPdi =
                skipBidiIsolate(
                    types,
                    static_cast<uint32_t>(
                        sizeof(types) / sizeof(types[0])),
                    0);


            if (matchingPdi != 5)
                return fail("nested isolate matching PDI is incorrect");


            ++helperPassed;
        }

        // ====================================================================
// X1-X8 explicit embedding-level resolution
// ====================================================================

        auto checkExplicitResolution =
            [&](std::initializer_list<UnicodeBidiClass> inputTypeList,
                UnicodeBidiLevel paragraphLevel,
                std::initializer_list<UnicodeBidiLevel> expectedLevelList,
                std::initializer_list<UnicodeBidiClass> expectedTypeList,
                const char* description) -> bool
            {
                ++explicitCases;


                if (inputTypeList.size() != expectedLevelList.size() ||
                    inputTypeList.size() != expectedTypeList.size())
                {
                    return fail(
                        "internal X1-X8 test definition has mismatched lengths");
                }


                std::vector<UnicodeBidiClass> types(
                    inputTypeList.begin(),
                    inputTypeList.end());

                std::vector<UnicodeBidiLevel> levels(
                    types.size(),
                    kUnicodeBidiLevelInvalid);

                std::vector<UnicodeBidiLevel> expectedLevels(
                    expectedLevelList.begin(),
                    expectedLevelList.end());

                std::vector<UnicodeBidiClass> expectedTypes(
                    expectedTypeList.begin(),
                    expectedTypeList.end());


                if (!resolveExplicitBidiLevels(
                    types.empty() ? nullptr : types.data(),
                    levels.empty() ? nullptr : levels.data(),
                    static_cast<uint32_t>(types.size()),
                    paragraphLevel))
                {
                    std::printf(
                        "Unicode bidi analysis: FAIL: %s\n"
                        "  resolveExplicitBidiLevels returned false\n",
                        description);

                    return false;
                }


                for (uint32_t i = 0;
                    i < static_cast<uint32_t>(types.size());
                    ++i)
                {
                    // kUnicodeBidiLevelInvalid means that the level of this
                    // formatting character is intentionally not tested.
                    if (expectedLevels[i] != kUnicodeBidiLevelInvalid &&
                        levels[i] != expectedLevels[i])
                    {
                        std::printf(
                            "Unicode bidi analysis: FAIL: %s\n"
                            "  Index:          %u\n"
                            "  Expected level: %u\n"
                            "  Actual level:   %u\n",
                            description,
                            i,
                            static_cast<unsigned>(expectedLevels[i]),
                            static_cast<unsigned>(levels[i]));

                        return false;
                    }


                    if (types[i] != expectedTypes[i])
                    {
                        std::printf(
                            "Unicode bidi analysis: FAIL: %s\n"
                            "  Index:         %u\n"
                            "  Expected type: %u\n"
                            "  Actual type:   %u\n",
                            description,
                            i,
                            static_cast<unsigned>(expectedTypes[i]),
                            static_cast<unsigned>(types[i]));

                        return false;
                    }
                }


                ++explicitPassed;
                return true;
            };


        // --------------------------------------------------------------------
        // X2 - RLE
        //
        // Paragraph level 0 -> next odd level is 1.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            0,
            {
                kUnicodeBidiLevelInvalid,
                1,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "X2 RLE raises to odd level"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X3 - LRE
        //
        // Paragraph level 0 -> next even level is 2.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat
            },
            0,
            {
                kUnicodeBidiLevelInvalid,
                2,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "X3 LRE from level zero"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // LRE from an RTL paragraph.
        //
        // Paragraph level 1 -> next even level is 2.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            1,
            {
                kUnicodeBidiLevelInvalid,
                2,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "X3 LRE from level one"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Nested embeddings.
        //
        //      paragraph 0
        //          RLE     -> level 1
        //              L
        //              LRE -> level 2
        //                  R
        //              PDF -> level 1
        //              L
        //          PDF     -> level 0
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            0,
            {
                kUnicodeBidiLevelInvalid,
                1,
                kUnicodeBidiLevelInvalid,
                2,
                kUnicodeBidiLevelInvalid,
                1,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "nested RLE and LRE"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X4 - RLO
        //
        // The enclosed L character is changed to working type R.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            0,
            {
                kUnicodeBidiLevelInvalid,
                1,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "X4 RLO changes L to R"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X5 - LRO
        //
        // The enclosed R character is changed to working type L.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::LeftToRightOverride,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat
            },
            0,
            {
                kUnicodeBidiLevelInvalid,
                2,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::LeftToRightOverride,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "X5 LRO changes R to L"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X5a / X6a - RLI and matching PDI.
        //
        // The isolate initiator and PDI remain at the outer level.
        // The isolate contents use the new odd level.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            },
            0,
            {
                0,
                1,
                0,
                0
            },
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            },
            "X5a RLI and X6a PDI"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X5b / X6a - LRI from an RTL paragraph.
        //
        // Outer level 1 -> isolate contents level 2.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::RightToLeft
            },
            1,
            {
                1,
                2,
                1,
                1
            },
            {
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::RightToLeft
            },
            "X5b LRI from RTL paragraph"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X5c - FSI with LTR contents behaves as LRI.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate
            },
            0,
            {
                0,
                2,
                0
            },
            {
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate
            },
            "X5c FSI with LTR contents"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X5c - FSI with RTL contents behaves as RLI.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::PopDirectionalIsolate
            },
            0,
            {
                0,
                1,
                0
            },
            {
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::PopDirectionalIsolate
            },
            "X5c FSI with RTL contents"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X6a terminates embeddings inside the isolate.
        //
        //      RLI
        //          LRE
        //              L
        //      PDI
        //      L
        //
        // PDI must pop both LRE and RLI and return to paragraph level zero.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            },
            0,
            {
                0,
                kUnicodeBidiLevelInvalid,
                2,
                0,
                0
            },
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            },
            "X6a PDI terminates embedded scope"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X7 - unmatched PDF is harmless.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::LeftToRight
            },
            0,
            {
                0,
                kUnicodeBidiLevelInvalid,
                0
            },
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::LeftToRight
            },
            "X7 unmatched PDF"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // X6a - unmatched PDI is harmless.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            },
            0,
            {
                0,
                0,
                0
            },
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            },
            "X6a unmatched PDI"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Override applies to isolate initiators and PDI.
        //
        // RLO establishes an R override. The LRI itself becomes working type R.
        // The isolate contents have neutral override status. When the PDI
        // returns to the RLO scope, the PDI also becomes working type R.
        // --------------------------------------------------------------------

        if (!checkExplicitResolution(
            {
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::PopDirectionalFormat
            },
            0,
            {
                kUnicodeBidiLevelInvalid,
                1,
                2,
                1,
                kUnicodeBidiLevelInvalid
            },
            {
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalFormat
            },
            "override applies to LRI and PDI"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Explicit embedding depth overflow.
        //
        // There are 63 valid nested odd levels:
        //
        //      1, 3, 5, ... 125
        //
        // One more RLE must overflow without failing the algorithm. After the
        // corresponding PDFs are consumed, the final L must be back at level 0.
        // --------------------------------------------------------------------

        {
            ++explicitCases;


            const uint32_t embeddingCount =
                static_cast<uint32_t>(
                    (kUnicodeBidiMaxDepth + 1u) / 2u) + 1u;


            std::vector<UnicodeBidiClass> types;
            types.reserve(
                embeddingCount +
                1u +
                embeddingCount +
                1u);


            for (uint32_t i = 0; i < embeddingCount; ++i)
                types.push_back(UnicodeBidiClass::RightToLeftEmbedding);

            const uint32_t innerIndex =
                static_cast<uint32_t>(types.size());

            types.push_back(UnicodeBidiClass::LeftToRight);


            for (uint32_t i = 0; i < embeddingCount; ++i)
                types.push_back(UnicodeBidiClass::PopDirectionalFormat);

            const uint32_t outerIndex =
                static_cast<uint32_t>(types.size());

            types.push_back(UnicodeBidiClass::LeftToRight);


            std::vector<UnicodeBidiLevel> levels(
                types.size(),
                kUnicodeBidiLevelInvalid);


            if (!resolveExplicitBidiLevels(
                types.data(),
                levels.data(),
                static_cast<uint32_t>(types.size()),
                0))
            {
                return fail("embedding depth overflow returned failure");
            }


            if (levels[innerIndex] != kUnicodeBidiMaxDepth)
                return fail("embedding depth overflow inner level");

            if (levels[outerIndex] != 0)
                return fail("embedding depth overflow did not unwind");


            ++explicitPassed;
        }


        // --------------------------------------------------------------------
        // Explicit isolate depth overflow.
        //
        // This exercises overflowIsolateCount independently from
        // overflowEmbeddingCount.
        // --------------------------------------------------------------------

        {
            ++explicitCases;


            const uint32_t isolateCount =
                static_cast<uint32_t>(
                    (kUnicodeBidiMaxDepth + 1u) / 2u) + 1u;


            std::vector<UnicodeBidiClass> types;
            types.reserve(
                isolateCount +
                1u +
                isolateCount +
                1u);


            for (uint32_t i = 0; i < isolateCount; ++i)
                types.push_back(UnicodeBidiClass::RightToLeftIsolate);

            const uint32_t innerIndex =
                static_cast<uint32_t>(types.size());

            types.push_back(UnicodeBidiClass::LeftToRight);


            for (uint32_t i = 0; i < isolateCount; ++i)
                types.push_back(UnicodeBidiClass::PopDirectionalIsolate);

            const uint32_t outerIndex =
                static_cast<uint32_t>(types.size());

            types.push_back(UnicodeBidiClass::LeftToRight);


            std::vector<UnicodeBidiLevel> levels(
                types.size(),
                kUnicodeBidiLevelInvalid);


            if (!resolveExplicitBidiLevels(
                types.data(),
                levels.data(),
                static_cast<uint32_t>(types.size()),
                0))
            {
                return fail("isolate depth overflow returned failure");
            }


            if (levels[innerIndex] != kUnicodeBidiMaxDepth)
                return fail("isolate depth overflow inner level");

            if (levels[outerIndex] != 0)
                return fail("isolate depth overflow did not unwind");


            ++explicitPassed;
        }

        // ====================================================================
// X9 virtual removal
// ====================================================================

// --------------------------------------------------------------------
// X9 removal predicate.
//
// These six types must be absent from all subsequent bidi processing.
// --------------------------------------------------------------------

        {
            ++x9Cases;

            const UnicodeBidiClass removedTypes[] =
            {
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::LeftToRightOverride,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::BoundaryNeutral
            };


            for (UnicodeBidiClass type : removedTypes)
            {
                if (!isBidiRemovedByX9(type))
                    return fail("X9 removal predicate missed removed type");
            }


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // Isolate formatting characters survive X9.
        //
        // X10 still needs LRI, RLI, FSI, and PDI when constructing isolating
        // run sequences.
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            const UnicodeBidiClass retainedTypes[] =
            {
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::PopDirectionalIsolate
            };


            for (UnicodeBidiClass type : retainedTypes)
            {
                if (isBidiRemovedByX9(type))
                    return fail("X9 incorrectly removed isolate formatting type");
            }


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // Ordinary bidi types also survive X9.
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            const UnicodeBidiClass retainedTypes[] =
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::ParagraphSeparator
            };


            for (UnicodeBidiClass type : retainedTypes)
            {
                if (isBidiRemovedByX9(type))
                    return fail("X9 incorrectly removed ordinary bidi type");
            }


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // Mixed X9 index sequence.
        //
        // This one sequence contains all six X9-removed types and all four
        // isolate formatting types.
        //
        // Original indices:
        //
        //      0   L
        //      1   RLE     removed
        //      2   R
        //      3   LRE     removed
        //      4   LRI
        //      5   RLO     removed
        //      6   PDI
        //      7   LRO     removed
        //      8   FSI
        //      9   PDF     removed
        //     10   RLI
        //     11   BN      removed
        //     12   EN
        //
        // Result:
        //
        //      0, 2, 4, 6, 8, 10, 12
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            const UnicodeBidiClass types[] =
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRightOverride,
                UnicodeBidiClass::FirstStrongIsolate,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::BoundaryNeutral,
                UnicodeBidiClass::EuropeanNumber
            };

            const uint32_t expected[] =
            {
                0, 2, 4, 6, 8, 10, 12
            };


            std::vector<uint32_t> indices;

            if (!buildBidiX9Indices(
                types,
                static_cast<uint32_t>(
                    sizeof(types) / sizeof(types[0])),
                indices))
            {
                return fail("X9 mixed index construction failed");
            }


            const uint32_t expectedCount =
                static_cast<uint32_t>(
                    sizeof(expected) / sizeof(expected[0]));


            if (indices.size() != expectedCount)
                return fail("X9 mixed index count");


            for (uint32_t i = 0; i < expectedCount; ++i)
            {
                if (indices[i] != expected[i])
                    return fail("X9 mixed index sequence");
            }


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // A paragraph consisting entirely of X9 characters produces an empty
        // post-X9 processing sequence.
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            const UnicodeBidiClass types[] =
            {
                UnicodeBidiClass::RightToLeftEmbedding,
                UnicodeBidiClass::LeftToRightEmbedding,
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::LeftToRightOverride,
                UnicodeBidiClass::PopDirectionalFormat,
                UnicodeBidiClass::BoundaryNeutral
            };


            std::vector<uint32_t> indices;

            if (!buildBidiX9Indices(
                types,
                static_cast<uint32_t>(
                    sizeof(types) / sizeof(types[0])),
                indices))
            {
                return fail("X9 all-removed construction failed");
            }


            if (!indices.empty())
                return fail("X9 all-removed sequence is not empty");


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // Empty input is valid and produces an empty processing sequence.
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            std::vector<uint32_t> indices;

            indices.push_back(123u);


            if (!buildBidiX9Indices(
                nullptr,
                0,
                indices))
            {
                return fail("X9 empty input failed");
            }


            if (!indices.empty())
                return fail("X9 empty input did not clear indices");


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // Non-empty input requires a valid type array.
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            std::vector<uint32_t> indices;

            indices.push_back(123u);


            if (buildBidiX9Indices(
                nullptr,
                1,
                indices))
            {
                return fail("X9 accepted null non-empty type array");
            }


            if (!indices.empty())
                return fail("X9 invalid input did not clear indices");


            ++x9Passed;
        }


        // --------------------------------------------------------------------
        // X9 follows X1-X8 working-type resolution.
        //
        // Under RLO, the LRI and PDI working types become R. They must still
        // remain in the post-X9 sequence; RLO and PDF must disappear.
        //
        //      0   RLO     removed
        //      1   LRI -> R
        //      2   L
        //      3   PDI -> R
        //      4   PDF     removed
        //
        // Result:
        //
        //      1, 2, 3
        // --------------------------------------------------------------------

        {
            ++x9Cases;

            std::vector<UnicodeBidiClass> types =
            {
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::PopDirectionalFormat
            };

            std::vector<UnicodeBidiLevel> levels(
                types.size(),
                kUnicodeBidiLevelInvalid);


            if (!resolveExplicitBidiLevels(
                types.data(),
                levels.data(),
                static_cast<uint32_t>(types.size()),
                0))
            {
                return fail("X9 post-X1-X8 setup failed");
            }


            std::vector<uint32_t> indices;

            if (!buildBidiX9Indices(
                types.data(),
                static_cast<uint32_t>(types.size()),
                indices))
            {
                return fail("X9 post-X1-X8 index construction failed");
            }


            if (indices.size() != 3 ||
                indices[0] != 1 ||
                indices[1] != 2 ||
                indices[2] != 3)
            {
                return fail("X9 post-X1-X8 index sequence");
            }


            ++x9Passed;
        }

        // ====================================================================
        // X10 isolating run sequences
        // ====================================================================

        // --------------------------------------------------------------------
        // Embedding direction from level.
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            if (bidiTypeFromLevel(0) != UnicodeBidiClass::LeftToRight ||
                bidiTypeFromLevel(1) != UnicodeBidiClass::RightToLeft ||
                bidiTypeFromLevel(2) != UnicodeBidiClass::LeftToRight ||
                bidiTypeFromLevel(125) != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 embedding direction from level");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // One level run.
        //
        //      bidi positions: 0 1 2 3
        //      levels:         0 0 0 0
        //
        // produces one maximal run [0,4).
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 1, 2, 3
            };

            const UnicodeBidiLevel levels[] =
            {
                0, 0, 0, 0
            };


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;


            if (!buildBidiLevelRuns(
                bidiIndices,
                4,
                levels,
                4,
                runs,
                runForPosition))
            {
                return fail("X10 single level run construction failed");
            }


            if (runs.size() != 1)
                return fail("X10 single level run count");

            if (runs[0].begin != 0 ||
                runs[0].end != 4 ||
                runs[0].level != 0)
            {
                return fail("X10 single level run contents");
            }


            if (runForPosition.size() != 4 ||
                runForPosition[0] != 0 ||
                runForPosition[1] != 0 ||
                runForPosition[2] != 0 ||
                runForPosition[3] != 0)
            {
                return fail("X10 single run reverse mapping");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Multiple level runs across X9 gaps.
        //
        // Raw scalar indices 1 and 4 are absent from bidiIndices. They must not
        // affect level-run construction.
        //
        //      bidiIndices:  0 2 3 5 6
        //      levels:       0 0 1 1 0
        //
        // Runs:
        //
        //      [0,2) level 0
        //      [2,4) level 1
        //      [4,5) level 0
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 2, 3, 5, 6
            };

            const UnicodeBidiLevel levels[] =
            {
                0, 0, 0, 1, 0, 1, 0
            };


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;


            if (!buildBidiLevelRuns(
                bidiIndices,
                5,
                levels,
                7,
                runs,
                runForPosition))
            {
                return fail("X10 multiple level run construction failed");
            }


            if (runs.size() != 3)
                return fail("X10 multiple level run count");


            if (runs[0].begin != 0 ||
                runs[0].end != 2 ||
                runs[0].level != 0)
            {
                return fail("X10 first multiple level run");
            }


            if (runs[1].begin != 2 ||
                runs[1].end != 4 ||
                runs[1].level != 1)
            {
                return fail("X10 second multiple level run");
            }


            if (runs[2].begin != 4 ||
                runs[2].end != 5 ||
                runs[2].level != 0)
            {
                return fail("X10 third multiple level run");
            }


            if (runForPosition.size() != 5 ||
                runForPosition[0] != 0 ||
                runForPosition[1] != 0 ||
                runForPosition[2] != 1 ||
                runForPosition[3] != 1 ||
                runForPosition[4] != 2)
            {
                return fail("X10 multiple run reverse mapping");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Nested isolate matching.
        //
        //      0 RLI
        //      1 L
        //      2 LRI
        //      3 R
        //      4 PDI     matches 2
        //      5 PDI     matches 0
        //      6 PDI     unmatched
        //      7 L
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 1, 2, 3, 4, 5, 6, 7
            };

            const UnicodeBidiClass originalTypes[] =
            {
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            };


            std::vector<uint32_t> matches;


            if (!buildBidiIsolateMatches(
                bidiIndices,
                8,
                originalTypes,
                8,
                matches))
            {
                return fail("X10 nested isolate matching failed");
            }


            if (matches.size() != 8)
                return fail("X10 isolate match array size");


            if (matches[0] != 5 ||
                matches[2] != 4 ||
                matches[4] != 2 ||
                matches[5] != 0)
            {
                return fail("X10 nested isolate match pairs");
            }


            if (matches[1] != kBidiIndexInvalid ||
                matches[3] != kBidiIndexInvalid ||
                matches[6] != kBidiIndexInvalid ||
                matches[7] != kBidiIndexInvalid)
            {
                return fail("X10 unmatched isolate positions");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Level runs without isolates.
        //
        //      levels: 0 0 | 1 1 | 0
        //
        // With no isolates, every level run is its own isolating run sequence.
        //
        // Expected:
        //
        //      run 0: level 0, sos L, eos R
        //      run 1: level 1, sos R, eos R
        //      run 2: level 0, sos R, eos L
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 1, 2, 3, 4
            };

            const UnicodeBidiClass originalTypes[] =
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::LeftToRight
            };

            const UnicodeBidiLevel levels[] =
            {
                0, 0, 1, 1, 0
            };


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;
            std::vector<uint32_t> matches;
            std::vector<uint32_t> sequenceRunIndices;
            std::vector<BidiIsolatingRunSequence> sequences;


            if (!buildBidiLevelRuns(
                bidiIndices,
                5,
                levels,
                5,
                runs,
                runForPosition))
            {
                return fail("X10 ordinary level runs failed");
            }


            if (!buildBidiIsolateMatches(
                bidiIndices,
                5,
                originalTypes,
                5,
                matches))
            {
                return fail("X10 ordinary isolate matching failed");
            }


            if (!buildBidiIsolatingRunSequences(
                bidiIndices,
                5,
                originalTypes,
                levels,
                5,
                0,
                runs,
                runForPosition,
                matches,
                sequenceRunIndices,
                sequences))
            {
                return fail("X10 ordinary run sequences failed");
            }


            if (sequences.size() != 3 ||
                sequenceRunIndices.size() != 3)
            {
                return fail("X10 ordinary sequence count");
            }


            if (sequenceRunIndices[0] != 0 ||
                sequenceRunIndices[1] != 1 ||
                sequenceRunIndices[2] != 2)
            {
                return fail("X10 ordinary sequence run membership");
            }


            if (sequences[0].runOffset != 0 ||
                sequences[0].runCount != 1 ||
                sequences[0].level != 0 ||
                sequences[0].sos != UnicodeBidiClass::LeftToRight ||
                sequences[0].eos != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 ordinary first sequence");
            }


            if (sequences[1].runOffset != 1 ||
                sequences[1].runCount != 1 ||
                sequences[1].level != 1 ||
                sequences[1].sos != UnicodeBidiClass::RightToLeft ||
                sequences[1].eos != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 ordinary second sequence");
            }


            if (sequences[2].runOffset != 2 ||
                sequences[2].runCount != 1 ||
                sequences[2].level != 0 ||
                sequences[2].sos != UnicodeBidiClass::RightToLeft ||
                sequences[2].eos != UnicodeBidiClass::LeftToRight)
            {
                return fail("X10 ordinary third sequence");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // One matched isolate.
        //
        //      L RLI | R | PDI L
        //      0  0    1    0  0
        //
        // Level runs:
        //
        //      run 0 = L RLI
        //      run 1 = R
        //      run 2 = PDI L
        //
        // BD13 links run 0 to run 2.
        //
        // Isolating run sequences:
        //
        //      sequence 0 = run 0, run 2   level 0   L/L
        //      sequence 1 = run 1          level 1   R/R
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 1, 2, 3, 4
            };

            const UnicodeBidiClass originalTypes[] =
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            };

            const UnicodeBidiLevel levels[] =
            {
                0, 0, 1, 0, 0
            };


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;
            std::vector<uint32_t> matches;
            std::vector<uint32_t> sequenceRunIndices;
            std::vector<BidiIsolatingRunSequence> sequences;


            if (!buildBidiLevelRuns(
                bidiIndices,
                5,
                levels,
                5,
                runs,
                runForPosition))
            {
                return fail("X10 simple isolate level runs failed");
            }


            if (!buildBidiIsolateMatches(
                bidiIndices,
                5,
                originalTypes,
                5,
                matches))
            {
                return fail("X10 simple isolate matching failed");
            }


            if (!buildBidiIsolatingRunSequences(
                bidiIndices,
                5,
                originalTypes,
                levels,
                5,
                0,
                runs,
                runForPosition,
                matches,
                sequenceRunIndices,
                sequences))
            {
                return fail("X10 simple isolate sequences failed");
            }


            if (runs.size() != 3 ||
                sequences.size() != 2 ||
                sequenceRunIndices.size() != 3)
            {
                return fail("X10 simple isolate structure counts");
            }


            if (sequenceRunIndices[0] != 0 ||
                sequenceRunIndices[1] != 2 ||
                sequenceRunIndices[2] != 1)
            {
                return fail("X10 simple isolate run linkage");
            }


            if (sequences[0].runOffset != 0 ||
                sequences[0].runCount != 2 ||
                sequences[0].level != 0 ||
                sequences[0].sos != UnicodeBidiClass::LeftToRight ||
                sequences[0].eos != UnicodeBidiClass::LeftToRight)
            {
                return fail("X10 outer simple isolate sequence");
            }


            if (sequences[1].runOffset != 2 ||
                sequences[1].runCount != 1 ||
                sequences[1].level != 1 ||
                sequences[1].sos != UnicodeBidiClass::RightToLeft ||
                sequences[1].eos != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 inner simple isolate sequence");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Nested isolates.
        //
        //      L RLI | R LRI | L | PDI R | PDI L
        //      0  0    1  1    2    1  1    0  0
        //
        // Level runs:
        //
        //      0 = L RLI
        //      1 = R LRI
        //      2 = L
        //      3 = PDI R
        //      4 = PDI L
        //
        // Sequences:
        //
        //      0,4      level 0
        //      1,3      level 1
        //      2        level 2
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 1, 2, 3, 4, 5, 6, 7, 8
            };

            const UnicodeBidiClass originalTypes[] =
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::LeftToRight
            };

            const UnicodeBidiLevel levels[] =
            {
                0, 0, 1, 1, 2, 1, 1, 0, 0
            };


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;
            std::vector<uint32_t> matches;
            std::vector<uint32_t> sequenceRunIndices;
            std::vector<BidiIsolatingRunSequence> sequences;


            if (!buildBidiLevelRuns(
                bidiIndices,
                9,
                levels,
                9,
                runs,
                runForPosition))
            {
                return fail("X10 nested isolate level runs failed");
            }


            if (!buildBidiIsolateMatches(
                bidiIndices,
                9,
                originalTypes,
                9,
                matches))
            {
                return fail("X10 nested isolate matching failed");
            }


            if (!buildBidiIsolatingRunSequences(
                bidiIndices,
                9,
                originalTypes,
                levels,
                9,
                0,
                runs,
                runForPosition,
                matches,
                sequenceRunIndices,
                sequences))
            {
                return fail("X10 nested isolate sequences failed");
            }


            if (runs.size() != 5 ||
                sequences.size() != 3 ||
                sequenceRunIndices.size() != 5)
            {
                return fail("X10 nested isolate structure counts");
            }


            if (sequenceRunIndices[0] != 0 ||
                sequenceRunIndices[1] != 4 ||
                sequenceRunIndices[2] != 1 ||
                sequenceRunIndices[3] != 3 ||
                sequenceRunIndices[4] != 2)
            {
                return fail("X10 nested isolate run linkage");
            }


            if (sequences[0].runOffset != 0 ||
                sequences[0].runCount != 2 ||
                sequences[0].level != 0 ||
                sequences[0].sos != UnicodeBidiClass::LeftToRight ||
                sequences[0].eos != UnicodeBidiClass::LeftToRight)
            {
                return fail("X10 nested outer sequence");
            }


            if (sequences[1].runOffset != 2 ||
                sequences[1].runCount != 2 ||
                sequences[1].level != 1 ||
                sequences[1].sos != UnicodeBidiClass::RightToLeft ||
                sequences[1].eos != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 nested middle sequence");
            }


            if (sequences[2].runOffset != 4 ||
                sequences[2].runCount != 1 ||
                sequences[2].level != 2 ||
                sequences[2].sos != UnicodeBidiClass::LeftToRight ||
                sequences[2].eos != UnicodeBidiClass::LeftToRight)
            {
                return fail("X10 nested inner sequence");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Unmatched isolate initiator.
        //
        //      L RLI | R
        //      0  0    1
        //
        // The RLI has no matching PDI. X10 therefore uses paragraphLevel for
        // the outer sequence's eos instead of the level of the following R.
        //
        // This specifically distinguishes:
        //
        //      eos = L   correct
        //
        // from:
        //
        //      eos = R   if the following level were incorrectly used.
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const uint32_t bidiIndices[] =
            {
                0, 1, 2
            };

            const UnicodeBidiClass originalTypes[] =
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::RightToLeft
            };

            const UnicodeBidiLevel levels[] =
            {
                0, 0, 1
            };


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;
            std::vector<uint32_t> matches;
            std::vector<uint32_t> sequenceRunIndices;
            std::vector<BidiIsolatingRunSequence> sequences;


            if (!buildBidiLevelRuns(
                bidiIndices,
                3,
                levels,
                3,
                runs,
                runForPosition))
            {
                return fail("X10 unmatched isolate level runs failed");
            }


            if (!buildBidiIsolateMatches(
                bidiIndices,
                3,
                originalTypes,
                3,
                matches))
            {
                return fail("X10 unmatched isolate matching failed");
            }


            if (!buildBidiIsolatingRunSequences(
                bidiIndices,
                3,
                originalTypes,
                levels,
                3,
                0,
                runs,
                runForPosition,
                matches,
                sequenceRunIndices,
                sequences))
            {
                return fail("X10 unmatched isolate sequences failed");
            }


            if (sequences.size() != 2)
                return fail("X10 unmatched isolate sequence count");


            if (sequences[0].level != 0 ||
                sequences[0].sos != UnicodeBidiClass::LeftToRight ||
                sequences[0].eos != UnicodeBidiClass::LeftToRight)
            {
                return fail("X10 unmatched isolate outer eos");
            }


            if (sequences[1].level != 1 ||
                sequences[1].sos != UnicodeBidiClass::RightToLeft ||
                sequences[1].eos != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 unmatched isolate inner sequence");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Original bidi type survives an override.
        //
        //      RLO LRI L PDI PDF
        //
        // X1-X8 changes the working types of LRI and PDI to R because they are
        // under RLO. X10 must nevertheless recognize their original identities
        // and link their level runs.
        //
        // After X9:
        //
        //      LRI/R   level 1
        //      L       level 2
        //      PDI/R   level 1
        //
        // Isolating sequences:
        //
        //      outer = run 0, run 2
        //      inner = run 1
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            const UnicodeBidiClass originalTypes[] =
            {
                UnicodeBidiClass::RightToLeftOverride,
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::PopDirectionalFormat
            };


            std::vector<UnicodeBidiClass> workingTypes(
                originalTypes,
                originalTypes + 5);

            std::vector<UnicodeBidiLevel> levels(
                5,
                kUnicodeBidiLevelInvalid);


            if (!resolveExplicitBidiLevels(
                workingTypes.data(),
                levels.data(),
                5,
                0))
            {
                return fail("X10 override setup failed");
            }


            if (workingTypes[1] != UnicodeBidiClass::RightToLeft ||
                workingTypes[3] != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 override did not change working isolate types");
            }


            std::vector<uint32_t> bidiIndices;

            if (!buildBidiX9Indices(
                workingTypes.data(),
                5,
                bidiIndices))
            {
                return fail("X10 override X9 construction failed");
            }


            if (bidiIndices.size() != 3 ||
                bidiIndices[0] != 1 ||
                bidiIndices[1] != 2 ||
                bidiIndices[2] != 3)
            {
                return fail("X10 override X9 sequence");
            }


            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;
            std::vector<uint32_t> matches;
            std::vector<uint32_t> sequenceRunIndices;
            std::vector<BidiIsolatingRunSequence> sequences;


            if (!buildBidiLevelRuns(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                levels.data(),
                5,
                runs,
                runForPosition))
            {
                return fail("X10 override level runs failed");
            }


            if (!buildBidiIsolateMatches(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                originalTypes,
                5,
                matches))
            {
                return fail("X10 override isolate matching failed");
            }


            if (!buildBidiIsolatingRunSequences(
                bidiIndices.data(),
                static_cast<uint32_t>(bidiIndices.size()),
                originalTypes,
                levels.data(),
                5,
                0,
                runs,
                runForPosition,
                matches,
                sequenceRunIndices,
                sequences))
            {
                return fail("X10 override run sequences failed");
            }


            if (runs.size() != 3 ||
                sequences.size() != 2 ||
                sequenceRunIndices.size() != 3)
            {
                return fail("X10 override structure counts");
            }


            if (sequenceRunIndices[0] != 0 ||
                sequenceRunIndices[1] != 2 ||
                sequenceRunIndices[2] != 1)
            {
                return fail("X10 override original-type linkage");
            }


            if (sequences[0].level != 1 ||
                sequences[0].sos != UnicodeBidiClass::RightToLeft ||
                sequences[0].eos != UnicodeBidiClass::RightToLeft)
            {
                return fail("X10 override outer sequence");
            }


            if (sequences[1].level != 2 ||
                sequences[1].sos != UnicodeBidiClass::LeftToRight ||
                sequences[1].eos != UnicodeBidiClass::LeftToRight)
            {
                return fail("X10 override inner sequence");
            }


            ++x10Passed;
        }


        // --------------------------------------------------------------------
        // Empty X10 input.
        //
        // Every X10 builder should accept an empty post-X9 sequence and clear
        // any previous output.
        // --------------------------------------------------------------------

        {
            ++x10Cases;

            std::vector<BidiLevelRun> runs;
            std::vector<uint32_t> runForPosition;
            std::vector<uint32_t> matches;
            std::vector<uint32_t> sequenceRunIndices;
            std::vector<BidiIsolatingRunSequence> sequences;


            runs.push_back(
                BidiLevelRun{
                    1,
                    2,
                    3
                });

            runForPosition.push_back(7);
            matches.push_back(8);
            sequenceRunIndices.push_back(9);

            sequences.push_back(
                BidiIsolatingRunSequence{});


            if (!buildBidiLevelRuns(
                nullptr,
                0,
                nullptr,
                0,
                runs,
                runForPosition))
            {
                return fail("X10 empty level runs failed");
            }


            if (!runs.empty() ||
                !runForPosition.empty())
            {
                return fail("X10 empty level runs not cleared");
            }


            if (!buildBidiIsolateMatches(
                nullptr,
                0,
                nullptr,
                0,
                matches))
            {
                return fail("X10 empty isolate matching failed");
            }


            if (!matches.empty())
                return fail("X10 empty isolate matches not cleared");


            if (!buildBidiIsolatingRunSequences(
                nullptr,
                0,
                nullptr,
                nullptr,
                0,
                0,
                runs,
                runForPosition,
                matches,
                sequenceRunIndices,
                sequences))
            {
                return fail("X10 empty run sequences failed");
            }


            if (!sequenceRunIndices.empty() ||
                !sequences.empty())
            {
                return fail("X10 empty run sequences not cleared");
            }


            ++x10Passed;
        }

        // ====================================================================
// W1 nonspacing mark resolution
// ====================================================================

        auto checkW1 =
            [&](std::initializer_list<UnicodeBidiClass> originalTypeList,
                std::initializer_list<UnicodeBidiClass> workingTypeList,
                std::initializer_list<UnicodeBidiLevel> levelList,
                UnicodeBidiLevel paragraphLevel,
                std::initializer_list<UnicodeBidiClass> expectedTypeList,
                const char* description) -> bool
            {
                ++w1Cases;


                if (originalTypeList.size() != workingTypeList.size() ||
                    originalTypeList.size() != levelList.size() ||
                    originalTypeList.size() != expectedTypeList.size())
                {
                    return fail(
                        "internal W1 test definition has mismatched lengths");
                }


                std::vector<UnicodeBidiClass> originalTypes(
                    originalTypeList.begin(),
                    originalTypeList.end());

                std::vector<UnicodeBidiClass> types(
                    workingTypeList.begin(),
                    workingTypeList.end());

                std::vector<UnicodeBidiLevel> levels(
                    levelList.begin(),
                    levelList.end());

                std::vector<UnicodeBidiClass> expectedTypes(
                    expectedTypeList.begin(),
                    expectedTypeList.end());


                std::vector<uint32_t> bidiIndices;

                for (uint32_t i = 0;
                    i < static_cast<uint32_t>(types.size());
                    ++i)
                {
                    bidiIndices.push_back(i);
                }


                std::vector<BidiLevelRun> runs;
                std::vector<uint32_t> runForPosition;
                std::vector<uint32_t> matches;
                std::vector<uint32_t> sequenceRunIndices;
                std::vector<BidiIsolatingRunSequence> sequences;


                if (!buildBidiLevelRuns(
                    bidiIndices.empty() ? nullptr : bidiIndices.data(),
                    static_cast<uint32_t>(bidiIndices.size()),
                    levels.empty() ? nullptr : levels.data(),
                    static_cast<uint32_t>(types.size()),
                    runs,
                    runForPosition))
                {
                    return fail("W1 level-run construction failed");
                }


                if (!buildBidiIsolateMatches(
                    bidiIndices.empty() ? nullptr : bidiIndices.data(),
                    static_cast<uint32_t>(bidiIndices.size()),
                    originalTypes.empty() ? nullptr : originalTypes.data(),
                    static_cast<uint32_t>(types.size()),
                    matches))
                {
                    return fail("W1 isolate matching failed");
                }


                if (!buildBidiIsolatingRunSequences(
                    bidiIndices.empty() ? nullptr : bidiIndices.data(),
                    static_cast<uint32_t>(bidiIndices.size()),
                    originalTypes.empty() ? nullptr : originalTypes.data(),
                    levels.empty() ? nullptr : levels.data(),
                    static_cast<uint32_t>(types.size()),
                    paragraphLevel,
                    runs,
                    runForPosition,
                    matches,
                    sequenceRunIndices,
                    sequences))
                {
                    return fail("W1 isolating run sequence construction failed");
                }


                if (!resolveBidiWeakTypesW1(
                    bidiIndices.empty() ? nullptr : bidiIndices.data(),
                    static_cast<uint32_t>(bidiIndices.size()),
                    originalTypes.empty() ? nullptr : originalTypes.data(),
                    types.empty() ? nullptr : types.data(),
                    static_cast<uint32_t>(types.size()),
                    runs,
                    sequenceRunIndices,
                    sequences))
                {
                    return fail("W1 resolution failed");
                }


                for (uint32_t i = 0;
                    i < static_cast<uint32_t>(types.size());
                    ++i)
                {
                    if (types[i] == expectedTypes[i])
                        continue;


                    std::printf(
                        "Unicode bidi analysis: FAIL: %s\n"
                        "  Index:         %u\n"
                        "  Expected type: %u\n"
                        "  Actual type:   %u\n",
                        description,
                        i,
                        static_cast<unsigned>(expectedTypes[i]),
                        static_cast<unsigned>(types[i]));

                    return false;
                }


                ++w1Passed;
                return true;
            };


            // --------------------------------------------------------------------
            // W1 - NSM takes the preceding resolved type.
            //
            //      AL NSM NSM
            //
            // becomes
            //
            //      AL AL AL
            // --------------------------------------------------------------------

            if (!checkW1(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::NonspacingMark,
                    UnicodeBidiClass::NonspacingMark
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::NonspacingMark
            },
            {
                1, 1, 1
            },
                1,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter
                },
                "W1 consecutive NSM propagation"))
            {
                return false;
            }


            // --------------------------------------------------------------------
// W1 - NSM at the beginning of an isolating run sequence gets sos.
//
// The run is level 1 in a paragraph at level 0:
//
//      sos = R
//
//      NSM L
//
// becomes:
//
//      R L
// --------------------------------------------------------------------

            if (!checkW1(
                {
                    UnicodeBidiClass::NonspacingMark,
                    UnicodeBidiClass::LeftToRight
                },
            {
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::LeftToRight
            },
            {
                1, 1
            },
                0,
                {
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::LeftToRight
                },
                "W1 leading NSM uses sos"))
            {
                return false;
            }


            // --------------------------------------------------------------------
// W1 - NSM after an isolate initiator becomes ON.
//
// Equal levels model an overflow isolate initiator:
//
//      LRI NSM
//
// becomes:
//
//      LRI ON
// --------------------------------------------------------------------

            if (!checkW1(
                {
                    UnicodeBidiClass::LeftToRightIsolate,
                    UnicodeBidiClass::NonspacingMark
                },
            {
                UnicodeBidiClass::LeftToRightIsolate,
                UnicodeBidiClass::NonspacingMark
            },
            {
                0, 0
            },
                0,
                {
                    UnicodeBidiClass::LeftToRightIsolate,
                    UnicodeBidiClass::OtherNeutral
                },
                "W1 NSM after isolate initiator"))
            {
                return false;
            }


            // --------------------------------------------------------------------
// W1 - NSM after PDI becomes ON.
//
//      PDI NSM
//
// becomes:
//
//      PDI ON
// --------------------------------------------------------------------

            if (!checkW1(
                {
                    UnicodeBidiClass::PopDirectionalIsolate,
                    UnicodeBidiClass::NonspacingMark
                },
            {
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::NonspacingMark
            },
            {
                0, 0
            },
                0,
                {
                    UnicodeBidiClass::PopDirectionalIsolate,
                    UnicodeBidiClass::OtherNeutral
                },
                "W1 NSM after PDI"))
            {
                return false;
            }


            // --------------------------------------------------------------------
// W1 across constituent level runs.
//
//      L RLI | R | PDI NSM
//      0  0    1    0   0
//
// X10 makes the outer isolating run sequence:
//
//      L RLI PDI NSM
//
// Therefore the NSM follows PDI and must become ON.
// --------------------------------------------------------------------

            if (!checkW1(
                {
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::RightToLeftIsolate,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::PopDirectionalIsolate,
                    UnicodeBidiClass::NonspacingMark
                },
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::NonspacingMark
            },
            {
                0, 0, 1, 0, 0
            },
                0,
                {
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::RightToLeftIsolate,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::PopDirectionalIsolate,
                    UnicodeBidiClass::OtherNeutral
                },
                "W1 across isolating run sequence boundary"))
            {
                return false;
            }



            // --------------------------------------------------------------------
// W1 must recognize isolate identity from originalTypes.
//
// This models an overflow LRI under an R override:
//
//      original:  LRI NSM
//      working:   R   NSM
//
// Using the working type alone would incorrectly produce:
//
//      R R
//
// W1 must produce:
//
//      R ON
// --------------------------------------------------------------------

            if (!checkW1(
                {
                    UnicodeBidiClass::LeftToRightIsolate,
                    UnicodeBidiClass::NonspacingMark
                },
            {
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::NonspacingMark
            },
            {
                1, 1
            },
                1,
                {
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::OtherNeutral
                },
                "W1 uses original isolate type under override"))
            {
                return false;
            }


            // ====================================================================
            // W2 European-number resolution
            // ====================================================================

            auto checkW2 =
                [&](std::initializer_list<UnicodeBidiClass> originalTypeList,
                    std::initializer_list<UnicodeBidiClass> workingTypeList,
                    std::initializer_list<UnicodeBidiLevel> levelList,
                    UnicodeBidiLevel paragraphLevel,
                    std::initializer_list<UnicodeBidiClass> expectedTypeList,
                    const char* description) -> bool
                {
                    ++w2Cases;


                    if (originalTypeList.size() != workingTypeList.size() ||
                        originalTypeList.size() != levelList.size() ||
                        originalTypeList.size() != expectedTypeList.size())
                    {
                        return fail(
                            "internal W2 test definition has mismatched lengths");
                    }


                    std::vector<UnicodeBidiClass> originalTypes(
                        originalTypeList.begin(),
                        originalTypeList.end());

                    std::vector<UnicodeBidiClass> types(
                        workingTypeList.begin(),
                        workingTypeList.end());

                    std::vector<UnicodeBidiLevel> levels(
                        levelList.begin(),
                        levelList.end());

                    std::vector<UnicodeBidiClass> expectedTypes(
                        expectedTypeList.begin(),
                        expectedTypeList.end());


                    std::vector<uint32_t> bidiIndices;

                    for (uint32_t i = 0;
                        i < static_cast<uint32_t>(types.size());
                        ++i)
                    {
                        bidiIndices.push_back(i);
                    }


                    std::vector<BidiLevelRun> runs;
                    std::vector<uint32_t> runForPosition;
                    std::vector<uint32_t> matches;
                    std::vector<uint32_t> sequenceRunIndices;
                    std::vector<BidiIsolatingRunSequence> sequences;


                    if (!buildBidiLevelRuns(
                        bidiIndices.empty() ? nullptr : bidiIndices.data(),
                        static_cast<uint32_t>(bidiIndices.size()),
                        levels.empty() ? nullptr : levels.data(),
                        static_cast<uint32_t>(types.size()),
                        runs,
                        runForPosition))
                    {
                        return fail("W2 level-run construction failed");
                    }


                    if (!buildBidiIsolateMatches(
                        bidiIndices.empty() ? nullptr : bidiIndices.data(),
                        static_cast<uint32_t>(bidiIndices.size()),
                        originalTypes.empty() ? nullptr : originalTypes.data(),
                        static_cast<uint32_t>(types.size()),
                        matches))
                    {
                        return fail("W2 isolate matching failed");
                    }


                    if (!buildBidiIsolatingRunSequences(
                        bidiIndices.empty() ? nullptr : bidiIndices.data(),
                        static_cast<uint32_t>(bidiIndices.size()),
                        originalTypes.empty() ? nullptr : originalTypes.data(),
                        levels.empty() ? nullptr : levels.data(),
                        static_cast<uint32_t>(types.size()),
                        paragraphLevel,
                        runs,
                        runForPosition,
                        matches,
                        sequenceRunIndices,
                        sequences))
                    {
                        return fail("W2 isolating run sequence construction failed");
                    }


                    // W2 runs after W1.

                    if (!resolveBidiWeakTypesW1(
                        bidiIndices.empty() ? nullptr : bidiIndices.data(),
                        static_cast<uint32_t>(bidiIndices.size()),
                        originalTypes.empty() ? nullptr : originalTypes.data(),
                        types.empty() ? nullptr : types.data(),
                        static_cast<uint32_t>(types.size()),
                        runs,
                        sequenceRunIndices,
                        sequences))
                    {
                        return fail("W2 prerequisite W1 resolution failed");
                    }


                    if (!resolveBidiWeakTypesW2(
                        bidiIndices.empty() ? nullptr : bidiIndices.data(),
                        static_cast<uint32_t>(bidiIndices.size()),
                        types.empty() ? nullptr : types.data(),
                        static_cast<uint32_t>(types.size()),
                        runs,
                        sequenceRunIndices,
                        sequences))
                    {
                        return fail("W2 resolution failed");
                    }


                    for (uint32_t i = 0;
                        i < static_cast<uint32_t>(types.size());
                        ++i)
                    {
                        if (types[i] == expectedTypes[i])
                            continue;


                        std::printf(
                            "Unicode bidi analysis: FAIL: %s\n"
                            "  Index:         %u\n"
                            "  Expected type: %u\n"
                            "  Actual type:   %u\n",
                            description,
                            i,
                            static_cast<unsigned>(expectedTypes[i]),
                            static_cast<unsigned>(types[i]));

                        return false;
                    }


                    ++w2Passed;
                    return true;
                };


            // --------------------------------------------------------------------
            // W2 - direct AL EN.
            //
            //      AL EN -> AL AN
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                1, 1
            },
                1,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicNumber
                },
                "W2 direct AL before EN"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W2 - intervening non-strong types do not stop the search.
            //
            //      AL ON WS EN -> AL ON WS AN
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::WhiteSpace,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::WhiteSpace,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                1, 1, 1, 1
            },
                1,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::WhiteSpace,
                    UnicodeBidiClass::ArabicNumber
                },
                "W2 scans through non-strong types"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W2 - L stops the search and leaves EN unchanged.
            //
            //      AL ON L ON EN -> AL ON L ON EN
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                1, 1, 1, 1, 1
            },
                1,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::EuropeanNumber
                },
                "W2 L stops AL influence"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W2 - R stops the search and leaves EN unchanged.
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                1, 1, 1
            },
                1,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::EuropeanNumber
                },
                "W2 R stops AL influence"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W2 - sos is the boundary strong type.
            //
            // No AL precedes the EN, so it remains EN.
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::OtherNeutral,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                1, 1
            },
                0,
                {
                    UnicodeBidiClass::OtherNeutral,
                    UnicodeBidiClass::EuropeanNumber
                },
                "W2 sos leaves EN unchanged"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W1 feeds W2.
            //
            //      AL NSM EN
            //
            // W1:
            //
            //      AL AL EN
            //
            // W2:
            //
            //      AL AL AN
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::NonspacingMark,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::NonspacingMark,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                1, 1, 1
            },
                1,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicNumber
                },
                "W2 consumes W1 resolved type"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W2 crosses constituent level-run boundaries in one isolating run
            // sequence.
            //
            //      AL RLI | R | PDI EN
            //      0   0    1    0   0
            //
            // Outer isolating run sequence:
            //
            //      AL RLI PDI EN
            //
            // The previous strong type for EN is therefore still AL.
            // --------------------------------------------------------------------

            if (!checkW2(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::RightToLeftIsolate,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::PopDirectionalIsolate,
                    UnicodeBidiClass::EuropeanNumber
                },
            {
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::RightToLeftIsolate,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::PopDirectionalIsolate,
                UnicodeBidiClass::EuropeanNumber
            },
            {
                0, 0, 1, 0, 0
            },
                0,
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::RightToLeftIsolate,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::PopDirectionalIsolate,
                    UnicodeBidiClass::ArabicNumber
                },
                "W2 across isolating run sequence boundary"))
            {
                return false;
            }


            // ====================================================================
            // W3 Arabic-letter resolution
            // ====================================================================

            auto checkW3 =
                [&](std::initializer_list<UnicodeBidiClass> inputTypeList,
                    std::initializer_list<uint32_t> bidiIndexList,
                    std::initializer_list<UnicodeBidiClass> expectedTypeList,
                    const char* description) -> bool
                {
                    ++w3Cases;


                    if (inputTypeList.size() != expectedTypeList.size())
                    {
                        return fail(
                            "internal W3 test definition has mismatched lengths");
                    }


                    std::vector<UnicodeBidiClass> types(
                        inputTypeList.begin(),
                        inputTypeList.end());

                    std::vector<uint32_t> bidiIndices(
                        bidiIndexList.begin(),
                        bidiIndexList.end());

                    std::vector<UnicodeBidiClass> expectedTypes(
                        expectedTypeList.begin(),
                        expectedTypeList.end());


                    if (!resolveBidiWeakTypesW3(
                        bidiIndices.empty() ? nullptr : bidiIndices.data(),
                        static_cast<uint32_t>(bidiIndices.size()),
                        types.empty() ? nullptr : types.data(),
                        static_cast<uint32_t>(types.size())))
                    {
                        std::printf(
                            "Unicode bidi analysis: FAIL: %s\n"
                            "  resolveBidiWeakTypesW3 returned false\n",
                            description);

                        return false;
                    }


                    for (uint32_t i = 0;
                        i < static_cast<uint32_t>(types.size());
                        ++i)
                    {
                        if (types[i] == expectedTypes[i])
                            continue;


                        std::printf(
                            "Unicode bidi analysis: FAIL: %s\n"
                            "  Index:         %u\n"
                            "  Expected type: %u\n"
                            "  Actual type:   %u\n",
                            description,
                            i,
                            static_cast<unsigned>(expectedTypes[i]),
                            static_cast<unsigned>(types[i]));

                        return false;
                    }


                    ++w3Passed;
                    return true;
                };


            // --------------------------------------------------------------------
            // W3 - basic AL conversion.
            //
            //      AL -> R
            // --------------------------------------------------------------------

            if (!checkW3(
                {
                    UnicodeBidiClass::ArabicLetter
                },
            {
                0
            },
            {
                UnicodeBidiClass::RightToLeft
            },
                "W3 direct AL to R"))
            {
                return false;
            }


            // --------------------------------------------------------------------
            // W3 - every remaining AL is converted independently.
            // --------------------------------------------------------------------

            if (!checkW3(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter
                },
            {
                0, 1, 2
            },
            {
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::RightToLeft
            },
                "W3 multiple AL conversion"))
            {
                return false;
            }

            // --------------------------------------------------------------------
// W3 - all non-AL types remain unchanged.
// --------------------------------------------------------------------

            if (!checkW3(
                {
                    UnicodeBidiClass::LeftToRight,
                    UnicodeBidiClass::RightToLeft,
                    UnicodeBidiClass::EuropeanNumber,
                    UnicodeBidiClass::ArabicNumber,
                    UnicodeBidiClass::OtherNeutral
                },
            {
                0, 1, 2, 3, 4
            },
            {
                UnicodeBidiClass::LeftToRight,
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::EuropeanNumber,
                UnicodeBidiClass::ArabicNumber,
                UnicodeBidiClass::OtherNeutral
            },
                "W3 preserves non-AL types"))
            {
                return false;
            }

            // --------------------------------------------------------------------
// W3 follows W2.
//
// W2 has already produced:
//
//      AL AN
//
// W3 must produce:
//
//      R AN
// --------------------------------------------------------------------

            if (!checkW3(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicNumber
                },
            {
                0, 1
            },
            {
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::ArabicNumber
            },
                "W3 preserves W2 EN to AN result"))
            {
                return false;
            }


            // --------------------------------------------------------------------
// W3 only processes post-X9 positions.
//
// Scalar 1 is deliberately absent from bidiIndices.
//
//      raw:         AL AL AL
//      bidiIndices:  0     2
//
// Result:
//
//      R AL R
// --------------------------------------------------------------------

            if (!checkW3(
                {
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter,
                    UnicodeBidiClass::ArabicLetter
                },
            {
                0, 2
            },
            {
                UnicodeBidiClass::RightToLeft,
                UnicodeBidiClass::ArabicLetter,
                UnicodeBidiClass::RightToLeft
            },
                "W3 respects post-X9 index sequence"))
            {
                return false;
            }





















        // ====================================================================
        // Stream case 1
        //
        // Latin paragraph.
        //
        // Also verifies:
        //
        //      borrowed -> owned promotion
        //      multi-scalar cluster
        //      cluster scalar offsets
        //      normalized provenance
        //      source provenance
        //      Script metadata preservation
        //      explicit scalar levels
        //      final EOF state
        // ====================================================================

        {
            ++streamCases;


            const UnicodeScriptSet latinCandidates =
                database.scriptExtensions(0x0061);

            if (!latinCandidates.isSingleton())
                return fail("Latin Script_Extensions is not singleton");

            const UnicodeScriptIndex latinScript =
                latinCandidates.first();


            BidiTestScriptSource source;

            source.add(
                { 0x0061, 0x0301 },
                20,
                SourceRange{ 100, 103 },
                latinCandidates,
                latinScript);

            source.add(
                { 0x0062 },
                22,
                SourceRange{ 103, 104 },
                latinCandidates,
                latinScript);


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("Latin paragraph was not emitted");


            if (paragraph.scalarCount != 3)
                return fail("Latin paragraph scalar count");

            if (paragraph.clusterCount != 2)
                return fail("Latin paragraph cluster count");

            if (paragraph.paragraphLevel != 0)
                return fail("Latin paragraph level");

            if (!paragraph.leftToRight() ||
                paragraph.rightToLeft())
            {
                return fail("Latin paragraph direction helpers");
            }


            // ---------------------------------------------------------------
            // The synthetic source has reused its scalar storage between
            // graphemes. All three values must still survive in the paragraph.
            // ---------------------------------------------------------------

            if (paragraph.scalars[0].value != 0x0061 ||
                paragraph.scalars[1].value != 0x0301 ||
                paragraph.scalars[2].value != 0x0062)
            {
                return fail("paragraph did not own borrowed scalar data");
            }


            // ---------------------------------------------------------------
            // With no explicit directional formatting, X1-X8 leaves these
            // scalars at the paragraph level.
            // ---------------------------------------------------------------

            for (uint32_t i = 0; i < paragraph.scalarCount; ++i)
            {
                if (paragraph.levels[i] != 0)
                    return fail("Latin provisional scalar level");
            }


            // ---------------------------------------------------------------
            // Cluster geometry.
            // ---------------------------------------------------------------

            if (paragraph.clusters[0].scalarOffset != 0 ||
                paragraph.clusters[0].scalarCount != 2 ||
                paragraph.clusters[0].normalizedBegin != 20)
            {
                return fail("first shaping cluster metadata");
            }


            if (paragraph.clusters[1].scalarOffset != 2 ||
                paragraph.clusters[1].scalarCount != 1 ||
                paragraph.clusters[1].normalizedBegin != 22)
            {
                return fail("second shaping cluster metadata");
            }


            if (paragraph.clusters[0].source.begin != 100 ||
                paragraph.clusters[0].source.end != 103 ||
                paragraph.clusters[1].source.begin != 103 ||
                paragraph.clusters[1].source.end != 104)
            {
                return fail("cluster source provenance");
            }


            // ---------------------------------------------------------------
            // Paragraph provenance envelope.
            // ---------------------------------------------------------------

            if (paragraph.normalizedBegin != 20)
                return fail("paragraph normalizedBegin");

            if (paragraph.source.begin != 100 ||
                paragraph.source.end != 104)
            {
                return fail("paragraph source provenance");
            }


            // ---------------------------------------------------------------
            // Scalar provenance survived the ownership transition.
            // ---------------------------------------------------------------

            if (paragraph.scalars[0].source.begin != 100 ||
                paragraph.scalars[0].source.end != 103 ||
                paragraph.scalars[2].source.begin != 103 ||
                paragraph.scalars[2].source.end != 104)
            {
                return fail("scalar source provenance");
            }


            // ---------------------------------------------------------------
            // Temporary Script metadata survived as the parallel array.
            // ---------------------------------------------------------------

            if (paragraph.scripts[0].script != latinScript ||
                paragraph.scripts[1].script != latinScript)
            {
                return fail("Script index metadata");
            }


            if (!paragraph.scripts[0].candidates.contains(latinScript) ||
                !paragraph.scripts[1].candidates.contains(latinScript))
            {
                return fail("Script candidate metadata");
            }


            // ---------------------------------------------------------------
            // Clean EOF becomes visible on the next pull.
            // ---------------------------------------------------------------

            BidiParagraphView next{};

            if (stream(next))
                return fail("Latin stream emitted unexpected second paragraph");

            if (stream.status() != TextStreamStatus::End)
                return fail("Latin stream did not enter End state");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 2
        //
        // Arabic -> paragraph level 1.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0627 },
                0,
                SourceRange{ 0, 2 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("Arabic paragraph was not emitted");

            if (paragraph.paragraphLevel != 1)
                return fail("Arabic paragraph level");

            if (!paragraph.rightToLeft() ||
                paragraph.leftToRight())
            {
                return fail("Arabic paragraph direction helpers");
            }

            if (paragraph.levels[0] != 1)
                return fail("Arabic explicit scalar level");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 3
        //
        // Leading whitespace must not force LTR.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0020 },
                0,
                SourceRange{ 0, 1 });

            source.add(
                { 0x0627 },
                1,
                SourceRange{ 1, 3 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("leading-whitespace paragraph was not emitted");

            if (paragraph.paragraphLevel != 1)
                return fail("leading whitespace before Arabic paragraph level");

            if (paragraph.scalarCount != 2)
                return fail("leading-whitespace scalar count");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 4
        //
        // Neutral-only paragraph defaults to level zero.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0020 },
                0,
                SourceRange{ 0, 1 });

            source.add(
                { 0x0021 },
                1,
                SourceRange{ 1, 2 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("neutral paragraph was not emitted");

            if (paragraph.paragraphLevel != 0)
                return fail("neutral-only paragraph level");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 5
        //
        // P2 isolate skipping using actual Unicode Bidi_Class lookups:
        //
        //      SPACE
        //      RLI
        //          LATIN A
        //      PDI
        //      ARABIC ALEF
        //
        // Latin A is inside the isolate. Arabic ALEF determines the outer
        // paragraph level.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0020 },
                0,
                SourceRange{ 0, 1 });

            source.add(
                { 0x2067 },
                1,
                SourceRange{ 1, 4 });

            source.add(
                { 0x0041 },
                2,
                SourceRange{ 4, 5 });

            source.add(
                { 0x2069 },
                3,
                SourceRange{ 5, 8 });

            source.add(
                { 0x0627 },
                4,
                SourceRange{ 8, 10 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("isolate paragraph was not emitted");

            if (paragraph.paragraphLevel != 1)
                return fail("stream P2 isolate skipping");

            if (paragraph.scalarCount != 5)
                return fail("isolate paragraph scalar count");


            // Paragraph level is 1 because the outer Arabic ALEF determines
            // P2. The RLI therefore opens the next odd level, which is 3.
            //
            //      SPACE        1
            //      RLI          1
            //          LATIN A  3
            //      PDI          1
            //      ARABIC ALEF  1

            if (paragraph.levels[0] != 1 ||
                paragraph.levels[1] != 1 ||
                paragraph.levels[2] != 3 ||
                paragraph.levels[3] != 1 ||
                paragraph.levels[4] != 1)
            {
                return fail("stream RLI explicit levels");
            }


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 6
        //
        // P1 paragraph collection.
        //
        // U+2029 has Bidi_Class B and belongs to the paragraph it terminates.
        //
        //      A U+2029
        //      ARABIC ALEF
        //
        // must produce two paragraphs.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0041 },
                0,
                SourceRange{ 0, 1 });

            source.add(
                { 0x2029 },
                1,
                SourceRange{ 1, 4 });

            source.add(
                { 0x0627 },
                2,
                SourceRange{ 4, 6 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView first{};

            if (!stream(first))
                return fail("first P1 paragraph was not emitted");

            if (first.scalarCount != 2)
                return fail("first P1 paragraph scalar count");

            if (first.scalars[0].value != 0x0041 ||
                first.scalars[1].value != 0x2029)
            {
                return fail("paragraph separator did not remain in first paragraph");
            }

            if (first.paragraphLevel != 0)
                return fail("first P1 paragraph level");

            if (first.levels[0] != 0 ||
                first.levels[1] != 0)
            {
                return fail("X8 paragraph separator level");
            }


            BidiParagraphView second{};

            if (!stream(second))
                return fail("second P1 paragraph was not emitted");

            if (second.scalarCount != 1 ||
                second.scalars[0].value != 0x0627)
            {
                return fail("second P1 paragraph contents");
            }

            if (second.paragraphLevel != 1)
                return fail("second P1 paragraph level");


            BidiParagraphView end{};

            if (stream(end))
                return fail("P1 stream emitted unexpected third paragraph");

            if (stream.status() != TextStreamStatus::End)
                return fail("P1 stream did not end cleanly");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 7
        //
        // Explicit higher-level LTR overrides Arabic P2 result.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0627 },
                0,
                SourceRange{ 0, 2 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database,
                UnicodeBidiParagraphDirection::LeftToRight);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("explicit LTR paragraph was not emitted");

            if (paragraph.paragraphLevel != 0)
                return fail("explicit LTR stream paragraph level");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 8
        //
        // Explicit higher-level RTL overrides Latin P2 result.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0041 },
                0,
                SourceRange{ 0, 1 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database,
                UnicodeBidiParagraphDirection::RightToLeft);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("explicit RTL paragraph was not emitted");

            if (paragraph.paragraphLevel != 1)
                return fail("explicit RTL stream paragraph level");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 9
        //
        // Invalid upstream input must not flush a partial paragraph.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x0041 },
                0,
                SourceRange{ 0, 1 });

            source.failAtEnd();


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (stream(paragraph))
                return fail("partial paragraph flushed after invalid input");

            if (stream.status() != TextStreamStatus::InvalidInput)
                return fail("invalid upstream status was not propagated");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 10
        //
        // Empty input.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (stream(paragraph))
                return fail("empty stream emitted a paragraph");

            if (stream.status() != TextStreamStatus::End)
                return fail("empty stream did not enter End state");


            ++streamPassed;
        }




        // ====================================================================
        // Stream case 11
        //
        // Actual Unicode RLE/PDF characters.
        //
        //      RLE
        //          LATIN A
        //      PDF
        //
        // P2 sees the embedded Latin A, so paragraph level is zero.
        // X2 places A at level one.
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x202B },
                0,
                SourceRange{ 0, 3 });

            source.add(
                { 0x0041 },
                1,
                SourceRange{ 3, 4 });

            source.add(
                { 0x202C },
                2,
                SourceRange{ 4, 7 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("RLE paragraph was not emitted");

            if (paragraph.paragraphLevel != 0)
                return fail("RLE paragraph level");

            if (paragraph.scalarCount != 3)
                return fail("RLE paragraph scalar count");

            if (paragraph.levels[1] != 1)
                return fail("RLE enclosed scalar level");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 12
        //
        // Actual Unicode LRE/PDF characters from an RTL paragraph.
        //
        // Higher-level paragraph direction is explicitly RTL:
        //
        //      paragraph level 1
        //          LRE
        //              LATIN A -> level 2
        //          PDF
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x202A },
                0,
                SourceRange{ 0, 3 });

            source.add(
                { 0x0041 },
                1,
                SourceRange{ 3, 4 });

            source.add(
                { 0x202C },
                2,
                SourceRange{ 4, 7 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database,
                UnicodeBidiParagraphDirection::RightToLeft);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("LTR embedding paragraph was not emitted");

            if (paragraph.paragraphLevel != 1)
                return fail("LTR embedding paragraph level");

            if (paragraph.scalarCount != 3)
                return fail("LTR embedding scalar count");

            if (paragraph.levels[1] != 2)
                return fail("LTR embedding enclosed scalar level");


            ++streamPassed;
        }


        // ====================================================================
        // Stream case 13
        //
        // Actual Unicode FSI/PDI characters.
        //
        //      FSI
        //          ARABIC ALEF
        //      PDI
        //      LATIN A
        //
        // P2 skips the isolate contents and sees the trailing Latin A, so the
        // paragraph level is zero.
        //
        // X5c examines the isolate contents, sees Arabic, and treats the FSI
        // as RLI:
        //
        //      FSI          0
        //      ARABIC ALEF  1
        //      PDI          0
        //      LATIN A      0
        // ====================================================================

        {
            ++streamCases;

            BidiTestScriptSource source;

            source.add(
                { 0x2068 },
                0,
                SourceRange{ 0, 3 });

            source.add(
                { 0x0627 },
                1,
                SourceRange{ 3, 5 });

            source.add(
                { 0x2069 },
                2,
                SourceRange{ 5, 8 });

            source.add(
                { 0x0041 },
                3,
                SourceRange{ 8, 9 });


            UnicodeBidiStream<BidiTestScriptSource> stream(
                source,
                database);


            BidiParagraphView paragraph{};

            if (!stream(paragraph))
                return fail("FSI paragraph was not emitted");

            if (paragraph.paragraphLevel != 0)
                return fail("FSI paragraph level");

            if (paragraph.scalarCount != 4)
                return fail("FSI paragraph scalar count");

            if (paragraph.levels[0] != 0 ||
                paragraph.levels[1] != 1 ||
                paragraph.levels[2] != 0 ||
                paragraph.levels[3] != 0)
            {
                return fail("FSI explicit levels");
            }


            ++streamPassed;
        }


        // ====================================================================
        // Summary
        // ====================================================================
        
        std::printf(
            "Unicode bidi analysis: PASS\n"
            "  P2/P3 helper cases:  %u\n"
            "  P2/P3 helper passed: %u\n"
            "  X1-X8 cases:         %u\n"
            "  X1-X8 passed:        %u\n"
            "  X9 cases:            %u\n"
            "  X9 passed:           %u\n"
            "  X10 cases:           %u\n"
            "  X10 passed:          %u\n"
            "  W1 cases:            %u\n"
            "  W1 passed:           %u\n"
            "  W2 cases:            %u\n"
            "  W2 passed:           %u\n"
            "  W3 cases:            %u\n"
            "  W3 passed:           %u\n"
            "  Stream cases:        %u\n"
            "  Stream passed:       %u\n",
            helperCases,
            helperPassed,
            explicitCases,
            explicitPassed,
            x9Cases,
            x9Passed,
            x10Cases,
            x10Passed,
            w1Cases,
            w1Passed,
            w2Cases,
            w2Passed,
            w3Cases,
            w3Passed,
            streamCases,
            streamPassed);


        return
            helperPassed == helperCases &&
            explicitPassed == explicitCases &&
            x9Passed == x9Cases &&
            x10Passed == x10Cases &&
            w1Passed == w1Cases &&
            w2Passed == w2Cases &&
            w3Passed == w3Cases &&
            streamPassed == streamCases;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static bool testUnicodeBidiAnalysis(const char* databaseFilename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(databaseFilename, fileData))
        {
            std::printf(
                "Unicode bidi analysis: FAIL: unable to read database\n"
                "  File: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan databaseData(
            fileData.data(),
            fileData.size());


        return testUnicodeBidiAnalysis(databaseData);
    }

} // namespace waavs